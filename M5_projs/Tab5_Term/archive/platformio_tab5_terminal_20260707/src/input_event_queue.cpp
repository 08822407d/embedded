#include "input_event_queue.h"

#include "app_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace input {
namespace {
QueueHandle_t event_queue = nullptr;
} // namespace

bool beginEventQueue()
{
    if (event_queue != nullptr) {
        return true;
    }

    event_queue = xQueueCreate(INPUT_EVENT_QUEUE_LENGTH, sizeof(KeyEvent));
    return event_queue != nullptr;
}

bool submitKeyEvent(const KeyEvent& event)
{
    return event_queue != nullptr && xQueueSend(event_queue, &event, 0) == pdTRUE;
}

bool readKeyEvent(KeyEvent *event)
{
    return event != nullptr
        && event_queue != nullptr
        && xQueueReceive(event_queue, event, 0) == pdTRUE;
}

} // namespace input
