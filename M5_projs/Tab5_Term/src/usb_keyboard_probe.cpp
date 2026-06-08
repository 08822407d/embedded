#include "usb_keyboard_probe.h"

#include "app_config.h"

#if ENABLE_USB_KEYBOARD_PROBE

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_host.h"

namespace {
constexpr char kTag[] = "usb_kbd";

constexpr int kClientEventQueueSize = 8;
constexpr uint8_t kHidSubClassBootInterface = 0x01;
constexpr uint8_t kHidProtocolKeyboard = 0x01;
constexpr uint8_t kHidRequestSetIdle = 0x0A;
constexpr uint8_t kHidRequestSetProtocol = 0x0B;
constexpr uint16_t kHidProtocolBoot = 0;
constexpr uint8_t kBootKeyboardReportBytes = 8;
constexpr uint8_t kBootKeyboardKeyCount = 6;
constexpr TickType_t kControlTransferTimeout = pdMS_TO_TICKS(1000);
constexpr TickType_t kClientPollInterval = pdMS_TO_TICKS(50);

constexpr uint8_t kModLeftCtrl = 0x01;
constexpr uint8_t kModLeftShift = 0x02;
constexpr uint8_t kModLeftAlt = 0x04;
constexpr uint8_t kModRightCtrl = 0x10;
constexpr uint8_t kModRightShift = 0x20;
constexpr uint8_t kModRightAlt = 0x40;

constexpr uint8_t kHidKeyA = 0x04;
constexpr uint8_t kHidKeyZ = 0x1D;
constexpr uint8_t kHidKeyEnter = 0x28;
constexpr uint8_t kHidKeyEsc = 0x29;
constexpr uint8_t kHidKeyBackspace = 0x2A;
constexpr uint8_t kHidKeyTab = 0x2B;
constexpr uint8_t kHidKeySlash = 0x38;
constexpr uint8_t kHidKeyRight = 0x4F;
constexpr uint8_t kHidKeyLeft = 0x50;
constexpr uint8_t kHidKeyDown = 0x51;
constexpr uint8_t kHidKeyUp = 0x52;

struct KeyboardEndpoint {
    uint8_t interface_number = 0;
    uint8_t alternate_setting = 0;
    uint8_t endpoint_address = 0;
    uint16_t max_packet_size = kBootKeyboardReportBytes;
};

struct KeyboardDevice {
    usb_host_client_handle_t client = nullptr;
    usb_device_handle_t device = nullptr;
    usb_transfer_t *report_transfer = nullptr;
    KeyboardEndpoint endpoint;
    uint8_t previous_keys[kBootKeyboardKeyCount] = {};
    bool interface_claimed = false;
    bool report_in_flight = false;
    bool connected = false;
};

struct ControlTransferContext {
    SemaphoreHandle_t done = nullptr;
    usb_transfer_status_t status = USB_TRANSFER_STATUS_ERROR;
};

QueueHandle_t key_queue = nullptr;
SemaphoreHandle_t host_ready = nullptr;
TaskHandle_t host_task = nullptr;
TaskHandle_t client_task = nullptr;
KeyboardDevice keyboard;

bool host_installed = false;
bool probe_enabled = false;
bool open_requested = false;
bool close_requested = false;
uint8_t pending_device_address = 0;

const uint8_t keycode_to_ascii[kHidKeySlash + 1][2] = {
    {0, 0},       // 0x00
    {0, 0},       // 0x01
    {0, 0},       // 0x02
    {0, 0},       // 0x03
    {'a', 'A'},   // 0x04
    {'b', 'B'},   // 0x05
    {'c', 'C'},   // 0x06
    {'d', 'D'},   // 0x07
    {'e', 'E'},   // 0x08
    {'f', 'F'},   // 0x09
    {'g', 'G'},   // 0x0A
    {'h', 'H'},   // 0x0B
    {'i', 'I'},   // 0x0C
    {'j', 'J'},   // 0x0D
    {'k', 'K'},   // 0x0E
    {'l', 'L'},   // 0x0F
    {'m', 'M'},   // 0x10
    {'n', 'N'},   // 0x11
    {'o', 'O'},   // 0x12
    {'p', 'P'},   // 0x13
    {'q', 'Q'},   // 0x14
    {'r', 'R'},   // 0x15
    {'s', 'S'},   // 0x16
    {'t', 'T'},   // 0x17
    {'u', 'U'},   // 0x18
    {'v', 'V'},   // 0x19
    {'w', 'W'},   // 0x1A
    {'x', 'X'},   // 0x1B
    {'y', 'Y'},   // 0x1C
    {'z', 'Z'},   // 0x1D
    {'1', '!'},   // 0x1E
    {'2', '@'},   // 0x1F
    {'3', '#'},   // 0x20
    {'4', '$'},   // 0x21
    {'5', '%'},   // 0x22
    {'6', '^'},   // 0x23
    {'7', '&'},   // 0x24
    {'8', '*'},   // 0x25
    {'9', '('},   // 0x26
    {'0', ')'},   // 0x27
    {'\r', '\r'}, // 0x28
    {0x1B, 0x1B}, // 0x29
    {0x7F, 0x7F}, // 0x2A
    {'\t', 0},    // 0x2B
    {' ', ' '},   // 0x2C
    {'-', '_'},   // 0x2D
    {'=', '+'},   // 0x2E
    {'[', '{'},   // 0x2F
    {']', '}'},   // 0x30
    {'\\', '|'},  // 0x31
    {'\\', '|'},  // 0x32
    {';', ':'},   // 0x33
    {'\'', '"'},  // 0x34
    {'`', '~'},   // 0x35
    {',', '<'},   // 0x36
    {'.', '>'},   // 0x37
    {'/', '?'}    // 0x38
};

bool isShift(uint8_t modifier)
{
    return (modifier & (kModLeftShift | kModRightShift)) != 0;
}

bool isCtrl(uint8_t modifier)
{
    return (modifier & (kModLeftCtrl | kModRightCtrl)) != 0;
}

bool isAlt(uint8_t modifier)
{
    return (modifier & (kModLeftAlt | kModRightAlt)) != 0;
}

void enqueueByte(uint8_t byte)
{
    if (key_queue == nullptr) {
        return;
    }
    if (xQueueSend(key_queue, &byte, 0) != pdTRUE) {
        ESP_LOGW(kTag, "keyboard input queue full");
    }
}

void enqueueBytes(const char *bytes)
{
    while (*bytes != '\0') {
        enqueueByte(static_cast<uint8_t>(*bytes));
        ++bytes;
    }
}

bool keyFound(const uint8_t *keys, uint8_t key)
{
    for (uint8_t i = 0; i < kBootKeyboardKeyCount; ++i) {
        if (keys[i] == key) {
            return true;
        }
    }
    return false;
}

bool keyToByte(uint8_t modifier, uint8_t key_code, uint8_t *byte)
{
    if (isCtrl(modifier)) {
        if (key_code >= kHidKeyA && key_code <= kHidKeyZ) {
            *byte = static_cast<uint8_t>(key_code - kHidKeyA + 1);
            return true;
        }
        switch (key_code) {
        case 0x2F:
            *byte = 0x1B; // Ctrl+[
            return true;
        case 0x30:
            *byte = 0x1D; // Ctrl+]
            return true;
        case 0x31:
            *byte = 0x1C; // Ctrl+backslash
            return true;
        default:
            break;
        }
    }

    if (key_code <= kHidKeySlash) {
        const uint8_t shifted = isShift(modifier) ? 1 : 0;
        const uint8_t value = keycode_to_ascii[key_code][shifted];
        if (value != 0) {
            *byte = value;
            return true;
        }
    }

    return false;
}

void handlePressedKey(uint8_t modifier, uint8_t key_code)
{
    if (key_code <= 0x03) {
        return;
    }

    if (key_code == kHidKeyTab && isShift(modifier)) {
        enqueueBytes("\x1B[Z");
        return;
    }

    uint8_t byte = 0;
    if (keyToByte(modifier, key_code, &byte)) {
        if (isAlt(modifier)) {
            enqueueByte(0x1B);
        }
        enqueueByte(byte);
        return;
    }

    switch (key_code) {
    case kHidKeyUp:
        enqueueBytes("\x1B[A");
        break;
    case kHidKeyDown:
        enqueueBytes("\x1B[B");
        break;
    case kHidKeyRight:
        enqueueBytes("\x1B[C");
        break;
    case kHidKeyLeft:
        enqueueBytes("\x1B[D");
        break;
    case 0x49:
        enqueueBytes("\x1B[2~"); // Insert
        break;
    case 0x4A:
        enqueueBytes("\x1B[H"); // Home
        break;
    case 0x4B:
        enqueueBytes("\x1B[5~"); // Page Up
        break;
    case 0x4C:
        enqueueBytes("\x1B[3~"); // Delete
        break;
    case 0x4D:
        enqueueBytes("\x1B[F"); // End
        break;
    case 0x4E:
        enqueueBytes("\x1B[6~"); // Page Down
        break;
    default:
        break;
    }
}

void processKeyboardReport(const uint8_t *data, int length)
{
    if (length < kBootKeyboardReportBytes) {
        return;
    }

    const uint8_t modifier = data[0];
    const uint8_t *keys = &data[2];

    for (uint8_t i = 0; i < kBootKeyboardKeyCount; ++i) {
        const uint8_t key_code = keys[i];
        if (key_code > 0x03 && !keyFound(keyboard.previous_keys, key_code)) {
            handlePressedKey(modifier, key_code);
        }
    }

    memcpy(keyboard.previous_keys, keys, kBootKeyboardKeyCount);
}

void reportTransferCallback(usb_transfer_t *transfer);

bool submitKeyboardReportTransfer()
{
    if (!keyboard.connected || keyboard.report_transfer == nullptr) {
        return false;
    }

    usb_transfer_t *transfer = keyboard.report_transfer;
    memset(transfer->data_buffer, 0, transfer->data_buffer_size);
    transfer->num_bytes = static_cast<int>(keyboard.endpoint.max_packet_size);
    transfer->device_handle = keyboard.device;
    transfer->bEndpointAddress = keyboard.endpoint.endpoint_address;
    transfer->callback = reportTransferCallback;
    transfer->context = &keyboard;

    const esp_err_t err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "submit keyboard report failed: %s", esp_err_to_name(err));
        keyboard.report_in_flight = false;
        return false;
    }

    keyboard.report_in_flight = true;
    return true;
}

void reportTransferCallback(usb_transfer_t *transfer)
{
    keyboard.report_in_flight = false;

    if (transfer->status == USB_TRANSFER_STATUS_COMPLETED) {
        processKeyboardReport(transfer->data_buffer, transfer->actual_num_bytes);
    } else if (transfer->status == USB_TRANSFER_STATUS_NO_DEVICE) {
        close_requested = true;
        return;
    } else {
        ESP_LOGW(kTag, "keyboard report status=%d", static_cast<int>(transfer->status));
    }

    if (keyboard.connected) {
        submitKeyboardReportTransfer();
    }
}

void controlTransferCallback(usb_transfer_t *transfer)
{
    auto *context = static_cast<ControlTransferContext *>(transfer->context);
    context->status = transfer->status;
    xSemaphoreGive(context->done);
}

esp_err_t sendHidClassOutRequest(
    usb_host_client_handle_t client,
    usb_device_handle_t device,
    uint8_t interface_number,
    uint8_t request,
    uint16_t value)
{
    usb_transfer_t *transfer = nullptr;
    esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE, 0, &transfer);
    if (err != ESP_OK) {
        return err;
    }

    auto *context = static_cast<ControlTransferContext *>(calloc(1, sizeof(ControlTransferContext)));
    if (context == nullptr) {
        usb_host_transfer_free(transfer);
        return ESP_ERR_NO_MEM;
    }

    context->done = xSemaphoreCreateBinary();
    if (context->done == nullptr) {
        free(context);
        usb_host_transfer_free(transfer);
        return ESP_ERR_NO_MEM;
    }

    auto *setup = reinterpret_cast<usb_setup_packet_t *>(transfer->data_buffer);
    setup->bmRequestType = USB_BM_REQUEST_TYPE_DIR_OUT
        | USB_BM_REQUEST_TYPE_TYPE_CLASS
        | USB_BM_REQUEST_TYPE_RECIP_INTERFACE;
    setup->bRequest = request;
    setup->wValue = value;
    setup->wIndex = interface_number;
    setup->wLength = 0;

    transfer->num_bytes = USB_SETUP_PACKET_SIZE;
    transfer->device_handle = device;
    transfer->bEndpointAddress = 0;
    transfer->callback = controlTransferCallback;
    transfer->context = context;

    err = usb_host_transfer_submit_control(client, transfer);
    if (err != ESP_OK) {
        vSemaphoreDelete(context->done);
        free(context);
        usb_host_transfer_free(transfer);
        return err;
    }

    const TickType_t start = xTaskGetTickCount();
    while (xSemaphoreTake(context->done, 0) != pdTRUE) {
        if ((xTaskGetTickCount() - start) >= kControlTransferTimeout) {
            ESP_LOGW(kTag, "control request 0x%02x timed out", request);
            return ESP_ERR_TIMEOUT;
        }
        const esp_err_t event_err = usb_host_client_handle_events(client, pdMS_TO_TICKS(20));
        if (event_err != ESP_OK && event_err != ESP_ERR_TIMEOUT) {
            ESP_LOGW(kTag, "client event during control request: %s", esp_err_to_name(event_err));
        }
    }

    err = (context->status == USB_TRANSFER_STATUS_COMPLETED) ? ESP_OK : ESP_FAIL;
    vSemaphoreDelete(context->done);
    free(context);
    usb_host_transfer_free(transfer);
    return err;
}

bool findBootKeyboardEndpoint(const usb_config_desc_t *config_desc, KeyboardEndpoint *endpoint)
{
    const uint8_t *ptr = config_desc->val;
    int remaining = config_desc->wTotalLength;
    const usb_intf_desc_t *current_interface = nullptr;
    bool current_interface_matches = false;

    while (remaining >= USB_STANDARD_DESC_SIZE) {
        const auto *standard = reinterpret_cast<const usb_standard_desc_t *>(ptr);
        if (standard->bLength < USB_STANDARD_DESC_SIZE || standard->bLength > remaining) {
            break;
        }

        if (standard->bDescriptorType == USB_B_DESCRIPTOR_TYPE_INTERFACE
            && standard->bLength >= USB_INTF_DESC_SIZE) {
            current_interface = reinterpret_cast<const usb_intf_desc_t *>(ptr);
            current_interface_matches =
                current_interface->bInterfaceClass == USB_CLASS_HID
                && current_interface->bInterfaceSubClass == kHidSubClassBootInterface
                && current_interface->bInterfaceProtocol == kHidProtocolKeyboard;
        } else if (standard->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT
            && standard->bLength >= USB_EP_DESC_SIZE
            && current_interface != nullptr
            && current_interface_matches) {
            const auto *ep = reinterpret_cast<const usb_ep_desc_t *>(ptr);
            const bool is_interrupt_in =
                USB_EP_DESC_GET_EP_DIR(ep) != 0
                && USB_EP_DESC_GET_XFERTYPE(ep) == USB_TRANSFER_TYPE_INTR;

            if (is_interrupt_in) {
                endpoint->interface_number = current_interface->bInterfaceNumber;
                endpoint->alternate_setting = current_interface->bAlternateSetting;
                endpoint->endpoint_address = ep->bEndpointAddress;
                endpoint->max_packet_size = USB_EP_DESC_GET_MPS(ep);
                if (endpoint->max_packet_size < kBootKeyboardReportBytes) {
                    endpoint->max_packet_size = kBootKeyboardReportBytes;
                }
                if (endpoint->max_packet_size > 64) {
                    endpoint->max_packet_size = 64;
                }
                return true;
            }
        }

        ptr += standard->bLength;
        remaining -= standard->bLength;
    }

    return false;
}

void resetKeyboardStateButKeepClient()
{
    usb_host_client_handle_t client = keyboard.client;
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.client = client;
}

void closeKeyboardDevice()
{
    if (keyboard.report_in_flight) {
        return;
    }

    keyboard.connected = false;

    if (keyboard.report_transfer != nullptr) {
        usb_host_transfer_free(keyboard.report_transfer);
        keyboard.report_transfer = nullptr;
    }

    if (keyboard.device != nullptr && keyboard.interface_claimed) {
        const esp_err_t err = usb_host_interface_release(
            keyboard.client,
            keyboard.device,
            keyboard.endpoint.interface_number);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "release interface failed: %s", esp_err_to_name(err));
        }
        keyboard.interface_claimed = false;
    }

    if (keyboard.device != nullptr) {
        const esp_err_t err = usb_host_device_close(keyboard.client, keyboard.device);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "close device failed: %s", esp_err_to_name(err));
        }
    }

    ESP_LOGI(kTag, "keyboard disconnected");
    resetKeyboardStateButKeepClient();
}

void openDevice(uint8_t address)
{
    if (keyboard.device != nullptr) {
        ESP_LOGW(kTag, "ignoring device %u while keyboard is active", address);
        return;
    }

    resetKeyboardStateButKeepClient();

    esp_err_t err = usb_host_device_open(keyboard.client, address, &keyboard.device);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "open device %u failed: %s", address, esp_err_to_name(err));
        resetKeyboardStateButKeepClient();
        return;
    }

    const usb_device_desc_t *device_desc = nullptr;
    if (usb_host_get_device_descriptor(keyboard.device, &device_desc) == ESP_OK) {
        ESP_LOGI(
            kTag,
            "USB device %u VID=0x%04x PID=0x%04x",
            address,
            device_desc->idVendor,
            device_desc->idProduct);
    }

    const usb_config_desc_t *config_desc = nullptr;
    err = usb_host_get_active_config_descriptor(keyboard.device, &config_desc);
    if (err != ESP_OK || !findBootKeyboardEndpoint(config_desc, &keyboard.endpoint)) {
        ESP_LOGI(kTag, "device %u is not a HID boot keyboard", address);
        closeKeyboardDevice();
        return;
    }

    err = usb_host_interface_claim(
        keyboard.client,
        keyboard.device,
        keyboard.endpoint.interface_number,
        keyboard.endpoint.alternate_setting);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "claim keyboard interface failed: %s", esp_err_to_name(err));
        closeKeyboardDevice();
        return;
    }
    keyboard.interface_claimed = true;

    err = sendHidClassOutRequest(
        keyboard.client,
        keyboard.device,
        keyboard.endpoint.interface_number,
        kHidRequestSetProtocol,
        kHidProtocolBoot);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "SET_PROTOCOL boot failed: %s", esp_err_to_name(err));
    }

    err = sendHidClassOutRequest(
        keyboard.client,
        keyboard.device,
        keyboard.endpoint.interface_number,
        kHidRequestSetIdle,
        0);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "SET_IDLE failed: %s", esp_err_to_name(err));
    }

    err = usb_host_transfer_alloc(keyboard.endpoint.max_packet_size, 0, &keyboard.report_transfer);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "alloc keyboard report transfer failed: %s", esp_err_to_name(err));
        closeKeyboardDevice();
        return;
    }

    keyboard.connected = true;
    memset(keyboard.previous_keys, 0, sizeof(keyboard.previous_keys));
    ESP_LOGI(
        kTag,
        "keyboard connected interface=%u ep=0x%02x mps=%u",
        keyboard.endpoint.interface_number,
        keyboard.endpoint.endpoint_address,
        keyboard.endpoint.max_packet_size);
    submitKeyboardReportTransfer();
}

void clientEventCallback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    (void)arg;

    switch (event_msg->event) {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
        pending_device_address = event_msg->new_dev.address;
        open_requested = true;
        break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
        if (keyboard.device == event_msg->dev_gone.dev_hdl) {
            close_requested = true;
            keyboard.connected = false;
        }
        break;
    default:
        break;
    }
}

void usbHostTask(void *arg)
{
    (void)arg;

    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .root_port_unpowered = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
        .enum_filter_cb = nullptr,
    };

    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "usb_host_install failed: %s", esp_err_to_name(err));
        if (host_ready != nullptr) {
            xSemaphoreGive(host_ready);
        }
        vTaskDelete(nullptr);
        return;
    }

    host_installed = true;
    if (host_ready != nullptr) {
        xSemaphoreGive(host_ready);
    }

    while (true) {
        uint32_t event_flags = 0;
        err = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "host event error: %s", esp_err_to_name(err));
            continue;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

void usbKeyboardClientTask(void *arg)
{
    (void)arg;

    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = kClientEventQueueSize,
        .async = {
            .client_event_callback = clientEventCallback,
            .callback_arg = nullptr,
        },
    };

    esp_err_t err = usb_host_client_register(&client_config, &keyboard.client);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "usb_host_client_register failed: %s", esp_err_to_name(err));
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(kTag, "waiting for USB HID boot keyboard");

    while (true) {
        if (open_requested) {
            const uint8_t address = pending_device_address;
            open_requested = false;
            openDevice(address);
        }

        if (close_requested && !keyboard.report_in_flight) {
            close_requested = false;
            closeKeyboardDevice();
        }

        err = usb_host_client_handle_events(keyboard.client, kClientPollInterval);
        if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
            ESP_LOGW(kTag, "client event error: %s", esp_err_to_name(err));
        }
    }
}
} // namespace

void usbKeyboardProbeBegin()
{
    if (probe_enabled) {
        return;
    }

    key_queue = xQueueCreate(USB_KEYBOARD_INPUT_QUEUE_LENGTH, sizeof(uint8_t));
    if (key_queue == nullptr) {
        ESP_LOGE(kTag, "keyboard input queue allocation failed");
        return;
    }

    host_ready = xSemaphoreCreateBinary();
    if (host_ready == nullptr) {
        ESP_LOGE(kTag, "host ready semaphore allocation failed");
        vQueueDelete(key_queue);
        key_queue = nullptr;
        return;
    }

    BaseType_t created = xTaskCreate(
        usbHostTask,
        "usb_host",
        USB_KEYBOARD_HOST_TASK_STACK_SIZE,
        nullptr,
        USB_KEYBOARD_HOST_TASK_PRIORITY,
        &host_task);
    if (created != pdTRUE) {
        ESP_LOGE(kTag, "USB host task creation failed");
        vSemaphoreDelete(host_ready);
        host_ready = nullptr;
        vQueueDelete(key_queue);
        key_queue = nullptr;
        return;
    }

    if (xSemaphoreTake(host_ready, pdMS_TO_TICKS(1000)) != pdTRUE || !host_installed) {
        ESP_LOGE(kTag, "USB host did not start");
        return;
    }

    created = xTaskCreate(
        usbKeyboardClientTask,
        "usb_kbd",
        USB_KEYBOARD_CLIENT_TASK_STACK_SIZE,
        nullptr,
        USB_KEYBOARD_CLIENT_TASK_PRIORITY,
        &client_task);
    if (created != pdTRUE) {
        ESP_LOGE(kTag, "USB keyboard client task creation failed");
        return;
    }

    probe_enabled = true;
    ESP_LOGI(kTag, "USB keyboard probe enabled");
}

bool usbKeyboardProbeReadByte(uint8_t *byte)
{
    if (byte == nullptr || key_queue == nullptr) {
        return false;
    }
    return xQueueReceive(key_queue, byte, 0) == pdTRUE;
}

bool usbKeyboardProbeIsEnabled()
{
    return probe_enabled;
}

#else

void usbKeyboardProbeBegin() {}

bool usbKeyboardProbeReadByte(uint8_t *byte)
{
    (void)byte;
    return false;
}

bool usbKeyboardProbeIsEnabled()
{
    return false;
}

#endif
