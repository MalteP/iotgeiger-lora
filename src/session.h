#ifndef SESSION_H
#define SESSION_H

#define SESSION_FRAM_LMIC_OFFSET 0
#define SESSION_FRAM_USER_OFFSET 1024

void session_setup(void);
void session_changed(bool save_now);
void session_loop(void);

// Userdata stored in FRAM
struct session_userdata_t {
  uint8_t interval;
};

#endif