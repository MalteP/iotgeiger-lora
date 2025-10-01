#include <Arduino.h>
#include <lmic.h>
#include "config.h"
#include "settings.h"

// Interval for data aggregation
static uint8_t settings_interval = 0;

void settings_init(void) {
  settings_set_interval(NO_VALUES);
}

void settings_set_interval(uint8_t interval) {
  if(interval < SETTINGS_INTERVAL_MIN) {
    interval = SETTINGS_INTERVAL_MIN;
  } else {
    if(interval > SETTINGS_INTERVAL_MAX)
      interval = SETTINGS_INTERVAL_MAX;
  }
  settings_interval = interval;
}

uint8_t settings_get_interval(void) {
  return settings_interval;
}