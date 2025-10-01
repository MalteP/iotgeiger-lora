#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_INTERVAL_MIN 5
#define SETTINGS_INTERVAL_MAX 60

void settings_init(void);
void settings_set_interval(uint8_t interval);
uint8_t settings_get_interval(void);

#endif