#include "login_uart.h"

#include <Preferences.h>

#include "app_config.h"

namespace login_uart {
namespace {
constexpr uint32_t kSupportedBauds[] = {
    115200,
    230400,
    460800,
    921600,
};

HardwareSerial loginSerial(1);
uint32_t activeBaud = LOGIN_UART_DEFAULT_BAUD;
uint32_t persistedBaud = 0;
bool started = false;
bool pending = false;
uint32_t pendingBaud = 0;
uint32_t pendingDueMs = 0;
PersistenceAction pendingPersistence = PersistenceAction::Keep;

bool writePersistedBaud(uint32_t baud)
{
    Preferences preferences;
    if (!preferences.begin(LOGIN_UART_NVS_NAMESPACE, false)) {
        return false;
    }

    const bool ok =
        preferences.putUInt(LOGIN_UART_NVS_BAUD_KEY, baud) == sizeof(uint32_t);
    preferences.end();
    return ok;
}

bool clearPersistedBaud()
{
    Preferences preferences;
    if (!preferences.begin(LOGIN_UART_NVS_NAMESPACE, false)) {
        return false;
    }

    const bool ok =
        !preferences.isKey(LOGIN_UART_NVS_BAUD_KEY) ||
        preferences.remove(LOGIN_UART_NVS_BAUD_KEY);
    preferences.end();
    return ok;
}

uint32_t readPersistedBaud()
{
    Preferences preferences;
    if (!preferences.begin(LOGIN_UART_NVS_NAMESPACE, false)) {
        return 0;
    }

    const uint32_t baud =
        preferences.getUInt(LOGIN_UART_NVS_BAUD_KEY, 0);
    preferences.end();
    return baud;
}
} // namespace

bool begin()
{
    persistedBaud = readPersistedBaud();
    if (persistedBaud != 0 && !isSupportedBaud(persistedBaud)) {
        clearPersistedBaud();
        persistedBaud = 0;
    }

    activeBaud =
        persistedBaud == 0 ? LOGIN_UART_DEFAULT_BAUD : persistedBaud;
    loginSerial.setRxBufferSize(LOGIN_UART_RX_BUFFER_SIZE);
    loginSerial.begin(
        activeBaud,
        SERIAL_8N1,
        LOGIN_UART_RX_PIN,
        LOGIN_UART_TX_PIN);
    started = true;
    return true;
}

HardwareSerial& serial()
{
    return loginSerial;
}

bool isSupportedBaud(uint32_t baud)
{
    for (const uint32_t supported : kSupportedBauds) {
        if (baud == supported) {
            return true;
        }
    }
    return false;
}

uint32_t defaultBaud()
{
    return LOGIN_UART_DEFAULT_BAUD;
}

State state()
{
    uint32_t delay = 0;
    if (pending) {
        const int32_t remaining =
            static_cast<int32_t>(pendingDueMs - millis());
        delay = remaining > 0 ? static_cast<uint32_t>(remaining) : 0;
    }

    return {
        .active_baud = activeBaud,
        .persisted_baud = persistedBaud,
        .pending = pending,
        .pending_baud = pendingBaud,
        .pending_delay_ms = delay,
    };
}

bool scheduleBaud(
    uint32_t baud,
    uint32_t delay_ms,
    PersistenceAction persistence)
{
    if (!started || pending || !isSupportedBaud(baud)) {
        return false;
    }

    pending = true;
    pendingBaud = baud;
    pendingDueMs = millis() + delay_ms;
    pendingPersistence = persistence;
    return true;
}

ApplyResult update()
{
    if (!pending ||
        static_cast<int32_t>(millis() - pendingDueMs) < 0) {
        return ApplyResult::None;
    }

    loginSerial.flush();
    loginSerial.updateBaudRate(pendingBaud);
    activeBaud = pendingBaud;

    bool persistenceOk = true;
    if (pendingPersistence == PersistenceAction::Save) {
        persistenceOk = writePersistedBaud(activeBaud);
        if (persistenceOk) {
            persistedBaud = activeBaud;
        }
    } else if (pendingPersistence == PersistenceAction::Clear) {
        persistenceOk = clearPersistedBaud();
        if (persistenceOk) {
            persistedBaud = 0;
        }
    }

    pending = false;
    pendingBaud = 0;
    pendingDueMs = 0;
    pendingPersistence = PersistenceAction::Keep;
    return persistenceOk
        ? ApplyResult::Applied
        : ApplyResult::AppliedPersistenceFailed;
}

} // namespace login_uart
