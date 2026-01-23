#include <M5Atom.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32-hal-timer.h"

// 可选：用于兼容判断（不同 Arduino-ESP32 core 版本 timer API 不一样）
#if __has_include("esp_arduino_version.h")
  #include "esp_arduino_version.h"
#endif
#ifndef ESP_ARDUINO_VERSION
  #define ESP_ARDUINO_VERSION ESP_ARDUINO_VERSION_VAL(0, 0, 0)
#endif

enum class LineMode : uint8_t { Columns, Rows };

// ====== 你要的两个静态全局变量（装板时改这里） ======
static bool     gUseRoll  = false;                 // true: roll；false: pitch
static LineMode gLineMode = LineMode::Columns;    // Columns: 只亮一列；Rows: 只亮一行

// ====== 其它可选配置 ======
static constexpr float kAngleSign = +1.0f;        // 左右反了就改成 -1
static constexpr bool  kFlipX = false;            // 列镜像
static constexpr bool  kFlipY = false;            // 行镜像

static constexpr float    kDeadbandDeg = 3.0f;
static constexpr float    kMidDeg      = 10.0f;
static constexpr uint32_t kColorWhite  = 0xFFFFFF;
static constexpr uint8_t  kBrightness  = 20;

// IMU 采样频率（由硬件定时器“打点”）
static constexpr uint32_t kSampleHz    = 200;     // 200Hz 足够显示姿态
static constexpr float    kFilterAlpha = 0.25f;   // 一阶低通

// ====== 共享数据（IMU 任务 -> loop）======
static portMUX_TYPE gAngleMux = portMUX_INITIALIZER_UNLOCKED;
static volatile float    gAngleFiltDegShared = 0.0f;
static volatile uint32_t gAngleSeq           = 0;

// ====== 定时器与任务句柄 ======
static hw_timer_t* gTimer = nullptr;
static TaskHandle_t gImuTaskHandle = nullptr;

#ifndef ARDUINO_ISR_ATTR
  #define ARDUINO_ISR_ATTR IRAM_ATTR
#endif

static inline uint8_t mapX(uint8_t x) { return kFlipX ? (4 - x) : x; }
static inline uint8_t mapY(uint8_t y) { return kFlipY ? (4 - y) : y; }

static int angleToIndex(float angleDegSigned) {
  float absA = fabsf(angleDegSigned);
  if (absA <= kDeadbandDeg) return 2;                           // 第3
  if (absA <= kMidDeg)      return (angleDegSigned > 0) ? 1 : 3; // 第2/4
  return (angleDegSigned > 0) ? 0 : 4;                          // 第1/5
}

static void drawLine(int idx0to4) {
  M5.dis.clear();
  if (gLineMode == LineMode::Columns) {
    uint8_t x = mapX((uint8_t)idx0to4);
    for (uint8_t y = 0; y < 5; ++y) M5.dis.drawpix(x, mapY(y), kColorWhite);
  } else {
    uint8_t y = mapY((uint8_t)idx0to4);
    for (uint8_t x = 0; x < 5; ++x) M5.dis.drawpix(mapX(x), y, kColorWhite);
  }
}

// ====== 硬件定时器 ISR：只做“唤醒 IMU 任务” ======
void ARDUINO_ISR_ATTR onTimer() {
  if (!gImuTaskHandle) return;
  BaseType_t hpTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(gImuTaskHandle, &hpTaskWoken);
  if (hpTaskWoken) portYIELD_FROM_ISR();
}

// ====== IMU 读取任务：被定时器稳定唤醒后读姿态（不在 ISR 里跑 I2C） ======
static void imuTask(void* arg) {
  (void)arg;
  float angleFilt = 0.0f;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    double pitch = 0.0, roll = 0.0;
    M5.IMU.getAttitude(&pitch, &roll);

    float angle = (float)(gUseRoll ? roll : pitch);
    angle *= kAngleSign;

    angleFilt += kFilterAlpha * (angle - angleFilt);

    portENTER_CRITICAL(&gAngleMux);
    gAngleFiltDegShared = angleFilt;
    gAngleSeq++;
    portEXIT_CRITICAL(&gAngleMux);
  }
}

static void startImuTimerAndTask() {
  // 1) 创建 IMU 任务（优先级稍高即可）
  xTaskCreatePinnedToCore(
    imuTask, "imuTask",
    4096, nullptr,
    4, &gImuTaskHandle,
    1  // pin 到 core1（一般 Arduino loop 也在 core1）
  );

  // 2) 配置硬件定时器
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  // Arduino-ESP32 3.x：timerBegin(frequency) + timerAlarm()
  // 官方文档示例就是 1MHz tick，然后 alarm_value 用“微秒数”来写。:contentReference[oaicite:0]{index=0}
  gTimer = timerBegin(1000000);               // 1MHz
  if (gTimer) {
    timerAttachInterrupt(gTimer, &onTimer);   // 3.x 只有两个参数 :contentReference[oaicite:1]{index=1}
    uint64_t alarmValue = 1000000ULL / kSampleHz; // us
    timerAlarm(gTimer, alarmValue, true, 0);  // autoreload 无限次 :contentReference[oaicite:2]{index=2}
  }
#else
  // Arduino-ESP32 2.x：老 API（timer num / divider / countUp）
  // divider=80 => 80MHz/80 = 1MHz tick
  gTimer = timerBegin(0, 80, true);
  if (gTimer) {
    timerAttachInterrupt(gTimer, &onTimer, true);
    uint64_t alarmValue = 1000000ULL / kSampleHz; // us
    timerAlarmWrite(gTimer, alarmValue, true);
    timerAlarmEnable(gTimer);
  }
#endif
}

void setup() {
  M5.begin(true, true, true);
  delay(50);

  M5.dis.setBrightness(kBrightness);
  M5.dis.clear();

  M5.IMU.Init();
  delay(50);

  startImuTimerAndTask();
}

void loop() {
  M5.update();

  static uint32_t lastSeq = 0;
  static int lastIdx = -1;

  float angleCopy = 0.0f;
  uint32_t seqCopy = 0;

  portENTER_CRITICAL(&gAngleMux);
  angleCopy = gAngleFiltDegShared;
  seqCopy   = gAngleSeq;
  portEXIT_CRITICAL(&gAngleMux);

  if (seqCopy != lastSeq) {
    int idx = angleToIndex(angleCopy);
    if (idx != lastIdx) {
      drawLine(idx);
      lastIdx = idx;
    }
    lastSeq = seqCopy;
  }

  // loop 不必固定周期，IMU 已被定时器稳定驱动
  delay(5);
}