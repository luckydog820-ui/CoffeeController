#ifndef COFFEE_CONTROLLER_WEB_CONTROLLER_H_
#define COFFEE_CONTROLLER_WEB_CONTROLLER_H_

#include <WebServer.h>

#include "ControllerState.h"
#include "SettingsStore.h"

class WebController {
 public:
  WebController(ControllerState& state, SettingsStore& settingsStore);

  void begin();
  void handle();

 private:
  void sendStatus();
  void sendSettings();
  void saveSettings();
  void commandBrew();
  void resetFault();
  bool readSettingsFromJson(const String& body, RuntimeSettings& settings);

  ControllerState& state_;
  SettingsStore& settingsStore_;
  WebServer server_;
};

#endif  // COFFEE_CONTROLLER_WEB_CONTROLLER_H_
