#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <TinyBME280.h>
#include "config.h"
#include "led.h"
#include "session.h"
#include "counter.h"

// LMIC pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 6,
  .dio = {2, 3, 4},
};

// Union for payload data, we may only send the first two bytes (cpm) if no BMP280 is connected
union payload_data_t {
  struct {
    uint16_t avg_cpm;
    int32_t temperature;
    int32_t pressure;
    int32_t humidity;
  };
  unsigned char bytes[2];
  unsigned char bytes_bme[14];
};

// Structure for BME280 sensor
tiny::BME280 bme;
// Set true if sensor is detected
bool bme_status;

// LMIC event handler
void onEvent(ev_t ev) {
  switch(ev) {
  case EV_JOINED:
    Serial.println(F("Join succeeded"));
    // Disable link check validation
    LMIC_setLinkCheckMode(0);
    // Store session data & frame counter immediately
    session_changed(true);
    // Deactivate LED
    led_disable();
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("Packet sent"));
    // Store session data & frame counter from time to time
    session_changed(false);
    // Deactivate LED
    led_disable();
    break;
  case EV_JOIN_TXCOMPLETE:
    Serial.println(F("Join failed"));
    // Activate flashing LED to indicate error status
    led_enable(LED_BLINK);
    break;
  default:
    break;
  }
}

// Generate payload and send LoRa packet
void geiger_send(uint16_t avg_cpm) {
  payload_data_t payload;
  // Possible to send data?
  if(!(LMIC.opmode & OP_TXRXPEND)) {
    // Prepare payload
    payload.avg_cpm = avg_cpm;
    if(bme_status) {
      payload.temperature = bme.readFixedTempC();
      payload.humidity = bme.readFixedHumidity();
      payload.pressure = bme.readFixedPressure();
    }
    // Send data
    if(!bme_status) {
      LMIC_setTxData2(1, payload.bytes, sizeof(payload.bytes), 0);
    } else {
      LMIC_setTxData2(1, payload.bytes_bme, sizeof(payload.bytes_bme), 0);
    }
    Serial.println(F("Packet queued"));
    // Activate LED to indicate TX in progress
    led_enable(LED_ON);
  }
}

// Callback function for counter_loop(), will be called if new measurement data is available (every minute)
// As the LoRa duty cyle is limited, data is averaged and sent if NO_VALUES is reached
void geiger_callback(uint16_t value) {
  static volatile uint8_t value_cnt = 0;
  static volatile uint32_t cumulative = 0;
  uint16_t average;

  Serial.print(F("CPM: "));
  Serial.println(value);

  // Sum up geiger pulses
  cumulative += value;

  if(++value_cnt >= NO_VALUES) {
    value_cnt = 0;

    // Calculate average and clear cumulative count
    average = cumulative / NO_VALUES;
    cumulative = 0;

    Serial.print(F("AVG: "));
    Serial.println(average);

    // Transmit average cpm
    geiger_send(average);
  }
}

// Arduino setup function
void setup() {
  Serial.begin(115200);
  Serial.println(F("System starting"));

  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Initialize LED
  led_setup();

  // Initialize network specific parameters
  session_setup();

  // Setup geiger pulse counter routines
  counter_setup(&geiger_callback);

  // Detect BME280 sensor at I2C address 0x77 or 0x76
  bme_status = bme.begin();
  if(!bme_status) {
    bme_status = bme.beginI2C(0x76);
  }
  if(bme_status) {
    Serial.println(F("BME280 found"));
  } else {
    Serial.println(F("BME280 not found"));
  }

#ifdef USE_OTAA
  // Join network if no session is set
  if(LMIC.devaddr == 0) {
    LMIC_startJoining();
    led_enable(LED_ON);
  }
#endif
}

// Arduino main loop
void loop() {
  os_runloop_once();
  counter_loop();
  session_loop();
  led_loop();
}