#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_system.h>

#include <cmath>
#include <cstring>

#include "AppEvents.h"
#include "Config.h"
#include "ControllerState.h"
#include "SettingsStore.h"
#include "src/app/EspressoStateMachine.h"
#include "src/drivers/E5ccModbus.h"
#include "src/drivers/PressureSensor.h"
#include "src/drivers/PumpDriver.h"
#include "src/services/PressureLoop.h"
#include "src/services/PumpArbiter.h"
#include "src/services/SafetyMonitor.h"
#include "src/web/WebController.h"

QueueHandle_t gAppEventQueue = nullptr;

ControllerState gState;
SettingsStore gSettingsStore;
PumpDriver gPump;
PressureSensor gPressureSensor;
E5ccModbus gE5cc;
PressureLoop gPressureLoop;
PumpArbiter gPumpArbiter;
SafetyMonitor gSafety;
EspressoStateMachine gStateMachine(gState);
WebController gWeb(gState, gSettingsStore);

volatile uint32_t gControlHeartbeatMs = 0;

bool postAppEvent(AppEventType type, float value) {
  if (gAppEventQueue == nullptr) {
    return false;
  }
  AppEvent event{type, value};
  return xQueueSend(gAppEventQueue, &event, 0) == pdTRUE;
}

namespace {

bool gStationConfigured = false;
uint32_t gLastWifiRetryMs = 0;
bool gLastStationConnected = false;

bool buttonPressed() {
  const bool level =
      digitalRead(user_config::PIN_BREW_BUTTON) == HIGH;
  return user_config::BUTTON_ACTIVE_LOW ? !level : level;
}

bool knobSwitchActive() {
  const bool level =
      digitalRead(user_config::PIN_KNOB_ACTIVE) == HIGH;
  return user_config::KNOB_ACTIVE_LEVEL ? level : !level;
}

void setStatusLed(bool on) {
  if (user_config::PIN_STATUS_LED < 0) {
    return;
  }
  digitalWrite(user_config::PIN_STATUS_LED,
               (user_config::STATUS_LED_ACTIVE_HIGH ? on : !on) ? HIGH
                                                                : LOW);
}

float readKnobVoltage() {
  uint32_t totalMillivolts = 0;
  constexpr uint8_t samples = 5;
  for (uint8_t index = 0; index < samples; ++index) {
    totalMillivolts +=
        analogReadMilliVolts(user_config::PIN_KNOB_ADC);
    delayMicroseconds(180);
  }
  const float pinVolts =
      static_cast<float>(totalMillivolts) / samples / 1000.0f;
  const float ratio =
      user_config::PUMP_MAX_VOLTS /
      config::MANUAL_KNOB_FULL_SCALE_VOLTS;
  return constrain(pinVolts * ratio, 0.0f, user_config::PUMP_MAX_VOLTS);
}

bool startNetwork() {
  WiFi.persistent(false);
  WiFi.setSleep(false);
  gStationConfigured = strlen(user_config::WIFI_SSID) > 0;

  // AP 与可选 STA 同时工作。即使家庭 Wi-Fi 未配置或暂时离线，
  // 手机也能在开机后直接连接设备热点访问网页。
  const bool modeOk =
      WiFi.mode(gStationConfigured ? WIFI_AP_STA : WIFI_AP);
  if (!modeOk) {
    Serial.println("Wi-Fi mode initialization failed");
    return false;
  }

  const bool apStarted =
      WiFi.softAP(user_config::AP_SSID, user_config::AP_PASSWORD);
  if (!apStarted) {
    Serial.println("CoffeePilot access point failed to start");
    return false;
  }
  Serial.print("AP SSID: ");
  Serial.println(user_config::AP_SSID);
  Serial.print("AP password: ");
  Serial.println(user_config::AP_PASSWORD);
  Serial.print("Web: http://");
  Serial.println(WiFi.softAPIP());

  if (gStationConfigured) {
    WiFi.setHostname("coffee-pilot");
    WiFi.begin(user_config::WIFI_SSID, user_config::WIFI_PASSWORD);
    gLastWifiRetryMs = millis();
    Serial.print("Joining Wi-Fi: ");
    Serial.println(user_config::WIFI_SSID);
  }

  if (MDNS.begin(user_config::MDNS_HOST)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS: http://");
    Serial.print(user_config::MDNS_HOST);
    Serial.println(".local");
  }
  return true;
}

void serviceNetwork(uint32_t nowMs) {
  if (!gStationConfigured) {
    return;
  }

  const wl_status_t status = WiFi.status();
  const bool connected = status == WL_CONNECTED;
  if (connected != gLastStationConnected) {
    gLastStationConnected = connected;
    if (connected) {
      Serial.print("STA connected: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("STA disconnected; access point remains available");
    }
  }

  if (connected || status == WL_IDLE_STATUS ||
      nowMs - gLastWifiRetryMs < config::WIFI_RETRY_INTERVAL_MS) {
    return;
  }

  Serial.println("Retrying station Wi-Fi");
  WiFi.begin(user_config::WIFI_SSID, user_config::WIFI_PASSWORD);
  gLastWifiRetryMs = nowMs;
}

void modbusTask(void*) {
  gE5cc.begin();
  bool setpointInitialized = false;
  float lastWrittenSetpoint = NAN;
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    float pvC = NAN;
    if (gE5cc.readPv(pvC)) {
      gState.updateBoilerPv(pvC);
      gState.setModbusOnline(true);

      const RuntimeSettings settings = gState.settings().settings;
      const bool changed =
          !setpointInitialized ||
          !isfinite(lastWrittenSetpoint) ||
          fabs(settings.boilerSetpointC - lastWrittenSetpoint) > 0.05f;
      if (changed && gE5cc.writeSp(settings.boilerSetpointC)) {
        lastWrittenSetpoint = settings.boilerSetpointC;
        setpointInitialized = true;
        gState.setModbusOnline(true);
        Serial.print("E5CC SP = ");
        Serial.println(settings.boilerSetpointC, 1);
      }
      if (changed && !setpointInitialized) {
        gState.setModbusOnline(false);
      }
    } else {
      gState.setModbusOnline(false);
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(config::MODBUS_POLL_MS));
  }
}

void controlTask(void*) {
  gPressureSensor.begin();
  gPressureLoop.reset();

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastPidMs = millis();
  BrewState previousState = BrewState::kBoot;
  float autoVoltage = 0.0f;

  for (;;) {
    const uint32_t nowMs = millis();
    const PressureSample pressure = gPressureSensor.read();
    gState.updatePressure(pressure.bar, pressure.valid);

    const ControllerSnapshot snapshot = gState.snapshot();
    const RuntimeSettings settings = gState.settings().settings;
    const uint32_t dtMs = nowMs - lastPidMs;
    const bool closedLoop =
        snapshot.brewState == BrewState::kPressureRamp ||
        snapshot.brewState == BrewState::kExtraction;

    if (snapshot.brewState != previousState) {
      if (snapshot.brewState == BrewState::kPressureRamp ||
          snapshot.brewState == BrewState::kPreInfusion) {
        gPressureLoop.reset();
      }
      if (!closedLoop) {
        autoVoltage = 0.0f;
      }
      previousState = snapshot.brewState;
    }

    if (snapshot.brewState == BrewState::kPreInfusion &&
        snapshot.pressureSensorValid) {
      autoVoltage = settings.profile.preinfusionVoltage;
    } else if (closedLoop && pressure.valid &&
               dtMs >= config::PID_PERIOD_MS) {
      autoVoltage = gPressureLoop.update(
          settings.profile.extractionPressureBar, pressure.bar,
          static_cast<float>(dtMs) / 1000.0f, settings.pid);
      lastPidMs = nowMs;
    } else if (!closedLoop) {
      gPressureLoop.reset();
      autoVoltage = 0.0f;
    }

    const PumpDecision decision = gPumpArbiter.decide(
        snapshot.faultMask, snapshot.knobActive, snapshot.manualVoltage,
        snapshot.brewState, autoVoltage);
    gPump.setVoltage(decision.voltage);
    gState.updatePump(decision.voltage, decision.source);
    gControlHeartbeatMs = millis();

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(config::CONTROL_PERIOD_MS));
  }
}

void safetyTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t announcedMask = kFaultNone;

  for (;;) {
    const uint32_t nowMs = millis();
    const ControllerSnapshot snapshot = gState.snapshot();
    const BrewProfile profile = gState.settings().settings.profile;
    bool newlyLatched = false;
    const uint32_t mask =
        gSafety.evaluate(snapshot, profile, nowMs, newlyLatched);
    gState.setFaultMask(mask);

    if (newlyLatched && mask != announcedMask) {
      announcedMask = mask;
      postAppEvent(AppEventType::kSafetyTrip,
                   static_cast<float>(mask));
      Serial.print("SAFETY LATCH 0x");
      Serial.println(mask, HEX);
    }

    if (nowMs > 3000 &&
        nowMs - gControlHeartbeatMs > 1500) {
      gPump.emergencyOff();
      Serial.println("Control task stalled: pump off, restarting");
      delay(30);
      esp_restart();
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(config::SAFETY_PERIOD_MS));
  }
}

void inputTask(void*) {
  if (user_config::PIN_KNOB_ADC >= 0) {
    analogReadResolution(12);
    analogSetPinAttenuation(user_config::PIN_KNOB_ADC, ADC_11db);
  }
  pinMode(user_config::PIN_KNOB_ACTIVE, INPUT_PULLUP);
  pinMode(user_config::PIN_BREW_BUTTON, INPUT_PULLUP);
  if (user_config::PIN_STATUS_LED >= 0) {
    pinMode(user_config::PIN_STATUS_LED, OUTPUT);
    setStatusLed(false);
  }

  bool stableKnob = false;
  bool rawKnobPrevious = knobSwitchActive();
  uint32_t rawKnobChangedMs = millis();
  bool stableButton = buttonPressed();
  bool rawButtonPrevious = stableButton;
  uint32_t rawButtonChangedMs = millis();
  uint32_t buttonPressedMs = 0;
  bool buttonLongFired = false;

  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    const uint32_t nowMs = millis();
    const bool rawKnob = knobSwitchActive();
    if (rawKnob != rawKnobPrevious) {
      rawKnobPrevious = rawKnob;
      rawKnobChangedMs = nowMs;
    } else if (rawKnob != stableKnob &&
               nowMs - rawKnobChangedMs >= config::INPUT_DEBOUNCE_MS) {
      stableKnob = rawKnob;
      if (stableKnob) {
        postAppEvent(AppEventType::kManualOverride);
      } else {
        postAppEvent(AppEventType::kManualReleased);
      }
      Serial.println(stableKnob ? "Knob override" : "Knob released");
    }
    gState.updateKnob(stableKnob, readKnobVoltage());

    const bool rawButton = buttonPressed();
    if (rawButton != rawButtonPrevious) {
      rawButtonPrevious = rawButton;
      rawButtonChangedMs = nowMs;
    } else if (rawButton != stableButton &&
               nowMs - rawButtonChangedMs >= config::INPUT_DEBOUNCE_MS) {
      stableButton = rawButton;
      if (stableButton) {
        buttonPressedMs = nowMs;
        buttonLongFired = false;
      } else if (!buttonLongFired) {
        postAppEvent(AppEventType::kToggleBrew);
      }
    }

    if (stableButton && !buttonLongFired && buttonPressedMs != 0 &&
        nowMs - buttonPressedMs >= config::BUTTON_LONG_PRESS_MS) {
      buttonLongFired = true;
      postAppEvent(AppEventType::kFaultReset);
      Serial.println("Long press: fault reset requested");
    }

    const ControllerSnapshot snapshot = gState.snapshot();
    const bool ledOn =
        snapshot.faultMask != kFaultNone
            ? ((nowMs / 140) & 1U) != 0
            : snapshot.brewState == BrewState::kPreInfusion ||
                  snapshot.brewState == BrewState::kPressureRamp ||
                  snapshot.brewState == BrewState::kExtraction;
    setStatusLed(ledOn);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(config::INPUT_PERIOD_MS));
  }
}

void appTask(void*) {
  gStateMachine.begin(millis());
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    AppEvent event;
    if (xQueueReceive(gAppEventQueue, &event,
                      pdMS_TO_TICKS(25)) == pdTRUE) {
      const uint32_t nowMs = millis();
      switch (event.type) {
        case AppEventType::kToggleBrew: {
          const BrewState state = gState.snapshot().brewState;
          const bool active = state == BrewState::kPreInfusion ||
                              state == BrewState::kPressureRamp ||
                              state == BrewState::kExtraction ||
                              state == BrewState::kStop;
          if (active) {
            gStateMachine.requestStop(nowMs);
          } else {
            gStateMachine.requestStart(nowMs);
          }
          break;
        }
        case AppEventType::kStartBrew:
          gStateMachine.requestStart(nowMs);
          break;
        case AppEventType::kStopBrew:
          gStateMachine.requestStop(nowMs);
          break;
        case AppEventType::kManualOverride:
          gStateMachine.manualOverride(nowMs);
          break;
        case AppEventType::kManualReleased:
          gStateMachine.manualReleased(nowMs);
          break;
        case AppEventType::kSafetyTrip:
          gStateMachine.tripFault(nowMs);
          break;
        case AppEventType::kFaultReset: {
          const ControllerSnapshot snapshot = gState.snapshot();
          if (gSafety.canReset(snapshot)) {
            gSafety.clearLatch();
            gState.setFaultMask(kFaultNone);
            gStateMachine.resetFault(nowMs);
            Serial.println("Safety latches cleared");
          } else {
            Serial.println("Fault reset rejected: conditions unsafe");
          }
          break;
        }
        case AppEventType::kSettingsChanged:
          break;
      }
    }

    gStateMachine.tick(millis());
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(25));
  }
}

void webTask(void*) {
  for (;;) {
    serviceNetwork(millis());
    gWeb.handle();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void createTasks() {
  xTaskCreatePinnedToCore(appTask, "app", 5120, nullptr, 5, nullptr, 0);
  xTaskCreatePinnedToCore(inputTask, "input", 3072, nullptr, 4, nullptr, 0);
  xTaskCreatePinnedToCore(safetyTask, "safety", 4096, nullptr, 8, nullptr, 0);
  xTaskCreatePinnedToCore(controlTask, "control", 4096, nullptr, 10, nullptr,
                          1);
  xTaskCreatePinnedToCore(modbusTask, "modbus", 4096, nullptr, 7, nullptr, 1);
  xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 2, nullptr, 0);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);

  Serial.println();
  Serial.println("CoffeePilot ESP32-S3");
  Serial.print("Firmware ");
  Serial.println(config::FIRMWARE_VERSION);

  gAppEventQueue = xQueueCreate(16, sizeof(AppEvent));
  if (gAppEventQueue == nullptr) {
    Serial.println("FATAL: event queue allocation failed");
    delay(1000);
    esp_restart();
  }

  const RuntimeSettings savedSettings = gSettingsStore.load();
  gState.applySettings(savedSettings);
  gState.setFaultMask(kFaultNone);

  // 启动网络期间泵保持关闭，避免网络等待造成意外输出。
  gPump.begin();
  gPump.setVoltage(0.0f);
  if (startNetwork()) {
    gWeb.begin();
  } else {
    Serial.println("FATAL: web server not started because networking failed");
  }
  gControlHeartbeatMs = millis();
  createTasks();
}

void loop() {
  // 所有周期任务已由 FreeRTOS 接管。
  vTaskDelay(portMAX_DELAY);
}
