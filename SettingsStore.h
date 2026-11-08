#ifndef COFFEE_CONTROLLER_SETTINGS_STORE_H_
#define COFFEE_CONTROLLER_SETTINGS_STORE_H_

#include "AppTypes.h"

class SettingsStore {
 public:
  RuntimeSettings load();
  bool save(const RuntimeSettings& settings);
  static RuntimeSettings validated(RuntimeSettings settings);
};

#endif  // COFFEE_CONTROLLER_SETTINGS_STORE_H_
