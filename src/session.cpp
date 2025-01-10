#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include "config.h"
#include "session.h"
#include "fram.h"

// LMIC callbacks for reading OTAA keys
#ifdef USE_OTAA
void os_getArtEui(u1_t *buf) {
  memcpy_P(buf, APPEUI, 8);
}
void os_getDevEui(u1_t *buf) {
  memcpy_P(buf, DEVEUI, 8);
}
void os_getDevKey(u1_t *buf) {
  memcpy_P(buf, APPKEY, 16);
}
#else
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}
#endif

// Counter for session data changes
static uint8_t fram_session_changed = 0;

// Internal functions
void session_save(void);
void session_restore(void);
void session_defaults(void);
void session_defaults_abp_channels(void);
void session_resetduty(void);

// Initialize session parameters
void session_setup(void) {
  // Initialize FRAM
  fram_setup();

#ifdef CONFIG_H_CHECKSUM
  // Set FRAM magic number according to last four hex digits of SHA256 config hash
  // to invalidate the current session if OTAA keys are changed (see scripts/auto_config_h_checksum.py)
  fram_setmagic(CONFIG_H_CHECKSUM);
#endif

  // Restore frame counter and session parameters
  session_restore();

#ifdef USE_OTAA
  // Join network if no session is set
  if(LMIC.devaddr == 0) {
    LMIC_startJoining();
  }
#endif
}

// Called if session data is changed, may be saved directly
// or after SESSION_SAVE_CHANGES times (to reduce write cylces)
void session_changed(bool save_now) {
  if(!save_now) {
    ++fram_session_changed;
  } else {
    fram_session_changed = SESSION_SAVE_CHANGES;
  }
}

// Called from main loop
void session_loop(void) {
  if(fram_session_changed >= SESSION_SAVE_CHANGES) {
    // Skip if there are LMIC operations in progress
    if(!LMIC_queryTxReady()) {
      return;
    }
    fram_session_changed = 0;
    session_save();
  }
}

// Save LoRa session parameters
void session_save(void) {
  bool fram_status = fram_write(&LMIC, sizeof(lmic_t));
  if(fram_status) {
    Serial.println(F("Session stored"));
  } else {
    Serial.println(F("Session store failed"));
  }
}

// Restore LoRa session parameters
void session_restore(void) {
  // Fetch OTAA activation & frame counters from FRAM memory
  bool fram_status = fram_read(&LMIC, sizeof(lmic_t));
  if(fram_status) {
    session_resetduty();
    Serial.println(F("Session restored"));
  } else {
    session_defaults();
    Serial.println(F("Session restore failed"));
  }
}

// Setup default parameters for a new session
void session_defaults(void) {
  // Reset the MAC state again, data structure may have been overwritten by fram_read()
  LMIC_reset();
  // Set static ABP session parameters
#ifdef USE_ABP
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
  // ABP mode uses a fixed channel config
  session_defaults_abp_channels();

  // Disable link check validation & ADR
  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(false);

  // Set a few link parameters for The Things Network
  LMIC.rxDelay = 5;                 // Rx1 delay
  LMIC.rx1DrOffset = 0;             // Rx1 datarate offset
  LMIC.dn2Dr = DR_SF9;              // Downlink band
  LMIC_setDrTxpow(TX_DATARATE, 14); // Uplink datarate
#endif

  // Compensate for clock skew
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
}

// Setup ABP default channels
void session_defaults_abp_channels(void) {
#if defined(CFG_eu868)
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band
#elif defined(CFG_us915)
  LMIC_selectSubBand(1);
#endif
}

// The LMIC structure contains timestamps for duty cycle calculation.
// These should not contain a future timestamp after reset
// See: https://jackgruber.github.io/2020-04-13-ESP32-DeepSleep-and-LoraWAN-OTAA-join/
void session_resetduty(void) {
#if defined(CFG_LMIC_EU_like)
  for(int i = 0; i < MAX_BANDS; i++) {
    LMIC.bands[i].avail = os_getTime();
  }
  LMIC.globalDutyAvail = os_getTime();
#endif
}