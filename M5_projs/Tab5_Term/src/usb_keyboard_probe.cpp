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
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_host.h"

#include "hid_keyboard_mapper.h"
#include "input_event_queue.h"
#include "input_mapper.h"

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
constexpr uint32_t kRepeatDelayMs = 500;
constexpr uint32_t kRepeatIntervalMs = 55;

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
    uint8_t previous_modifier = 0;
    uint8_t repeat_key = 0;
    uint8_t repeat_modifier = 0;
    uint32_t next_repeat_ms = 0;
    bool interface_claimed = false;
    bool report_in_flight = false;
    bool connected = false;
};

struct ControlTransferContext {
    SemaphoreHandle_t done = nullptr;
    usb_transfer_status_t status = USB_TRANSFER_STATUS_ERROR;
};

SemaphoreHandle_t host_ready = nullptr;
TaskHandle_t host_task = nullptr;
TaskHandle_t client_task = nullptr;
KeyboardDevice keyboard;

bool host_installed = false;
bool probe_enabled = false;
bool open_requested = false;
bool close_requested = false;
uint8_t pending_device_address = 0;

bool isValidBootKey(uint8_t key)
{
    return key > 0x03;
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

uint8_t firstActiveKey(const uint8_t *keys)
{
    for (uint8_t i = 0; i < kBootKeyboardKeyCount; ++i) {
        if (isValidBootKey(keys[i])) {
            return keys[i];
        }
    }
    return 0;
}

void clearRepeat()
{
    keyboard.repeat_key = 0;
    keyboard.repeat_modifier = 0;
    keyboard.next_repeat_ms = 0;
}

void armRepeat(uint8_t modifier, uint8_t key, uint32_t delay_ms)
{
    if (!isValidBootKey(key)) {
        clearRepeat();
        return;
    }

    keyboard.repeat_key = key;
    keyboard.repeat_modifier = modifier;
    keyboard.next_repeat_ms = millis() + delay_ms;
}

void submitKeyEvent(uint8_t hid_modifier, uint8_t hid_usage, input::KeyAction action)
{
    if (!isValidBootKey(hid_usage)) {
        return;
    }

    const input::KeyCode key = input::hidUsageToKeyCode(hid_usage);
    if (key == input::KeyCode::Unknown) {
        return;
    }

    const uint8_t modifiers = input::normalizeHidModifiers(hid_modifier);
    if (!input::submitKeyEvent({
            .key = key,
            .modifiers = modifiers,
            .action = action,
        })) {
        ESP_LOGW(kTag, "input event queue full");
    }
}

void maybeSubmitRepeat(uint8_t modifier, const uint8_t *keys)
{
    if (!isValidBootKey(keyboard.repeat_key) || !keyFound(keys, keyboard.repeat_key)) {
        return;
    }

    keyboard.repeat_modifier = modifier;
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - keyboard.next_repeat_ms) < 0) {
        return;
    }

    submitKeyEvent(keyboard.repeat_modifier, keyboard.repeat_key, input::KeyAction::Repeat);
    keyboard.next_repeat_ms = now + kRepeatIntervalMs;
}

void processKeyboardReport(const uint8_t *data, int length)
{
    if (length < kBootKeyboardReportBytes) {
        return;
    }

    const uint8_t modifier = data[0];
    const uint8_t *keys = &data[2];
    bool pressed_new_key = false;

    for (uint8_t i = 0; i < kBootKeyboardKeyCount; ++i) {
        const uint8_t key = keyboard.previous_keys[i];
        if (isValidBootKey(key) && !keyFound(keys, key)) {
            submitKeyEvent(keyboard.previous_modifier, key, input::KeyAction::Release);
            if (keyboard.repeat_key == key) {
                clearRepeat();
            }
        }
    }

    for (uint8_t i = 0; i < kBootKeyboardKeyCount; ++i) {
        const uint8_t key = keys[i];
        if (isValidBootKey(key) && !keyFound(keyboard.previous_keys, key)) {
            submitKeyEvent(modifier, key, input::KeyAction::Press);
            armRepeat(modifier, key, kRepeatDelayMs);
            pressed_new_key = true;
        }
    }

    if (!pressed_new_key) {
        if (isValidBootKey(keyboard.repeat_key) && keyFound(keys, keyboard.repeat_key)) {
            maybeSubmitRepeat(modifier, keys);
        } else {
            const uint8_t key = firstActiveKey(keys);
            if (isValidBootKey(key)) {
                armRepeat(modifier, key, kRepeatDelayMs);
            } else {
                clearRepeat();
            }
        }
    }

    keyboard.previous_modifier = modifier;
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
            // Keep the transfer/context allocated on timeout because the USB
            // callback may still arrive after this function returns.
            return ESP_ERR_TIMEOUT;
        }
        const esp_err_t event_err = usb_host_client_handle_events(client, pdMS_TO_TICKS(20));
        if (event_err != ESP_OK && event_err != ESP_ERR_TIMEOUT) {
            ESP_LOGW(kTag, "client event during control request: %s", esp_err_to_name(event_err));
        }
    }

    if (err == ESP_OK) {
        err = (context->status == USB_TRANSFER_STATUS_COMPLETED) ? ESP_OK : ESP_FAIL;
    }

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
    clearRepeat();
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

    if (!input::beginEventQueue()) {
        ESP_LOGE(kTag, "input event queue allocation failed");
        return;
    }

    host_ready = xSemaphoreCreateBinary();
    if (host_ready == nullptr) {
        ESP_LOGE(kTag, "host ready semaphore allocation failed");
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

bool usbKeyboardProbeIsEnabled()
{
    return probe_enabled;
}

#else

void usbKeyboardProbeBegin() {}

bool usbKeyboardProbeIsEnabled()
{
    return false;
}

#endif
