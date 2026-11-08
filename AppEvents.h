#ifndef COFFEE_CONTROLLER_APP_EVENTS_H_
#define COFFEE_CONTROLLER_APP_EVENTS_H_

#include "AppTypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t gAppEventQueue;

bool postAppEvent(AppEventType type, float value = 0.0f);

#endif  // COFFEE_CONTROLLER_APP_EVENTS_H_
