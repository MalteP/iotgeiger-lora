#include <SPI.h>
#include <Adafruit_FRAM_SPI.h>
#include "fram.h"

// Internal function
uint8_t fram_crc8(const uint8_t *data, size_t len);

// Global variables
Adafruit_FRAM_SPI *fram = NULL;
bool fram_found = false;

// Initialize FRAM
bool fram_setup(void) {
  uint8_t statusRegister;

  // Check SPI bus for any known FRAM chip
  fram = new Adafruit_FRAM_SPI(FRAM_CS);
  fram_found = fram->begin();
  if(fram_found == false) {
    return false;
  }

  // Check status register for active write protection
  statusRegister = fram->getStatusRegister();
  if(statusRegister & ((1 << FRAM_WPEN) | (1 << FRAM_BP1) | (1 << FRAM_BP0))) {
    statusRegister &= ~((1 << FRAM_WPEN) | (1 << FRAM_BP1) | (1 << FRAM_BP0));
    fram->writeEnable(true);
    fram->setStatusRegister(statusRegister);
    fram->writeEnable(false);
    Serial.println(F("FRAM write protection has been disabled"));
  }

  return true;
}

// Read FRAM data and validate CRC8 checksum
bool fram_read(void *data, size_t length) {
  uint8_t crc, crc2;

  // FRAM initialized?
  if(fram_found == false) {
    return false;
  }

  // Read CRC from first byte, then read data
  fram->read(0, (uint8_t *)&crc, sizeof(uint8_t));
  fram->read(1, (uint8_t *)data, length);

  // Calculate and validate checksum
  crc2 = fram_crc8((uint8_t *)data, length);
  if(crc != crc2) {
    Serial.println(F("FRAM checksum mismatch"));
    memset(data, 0, length);
    return false;
  }

  return true;
}

// Write CRC and data to FRAM
bool fram_write(void *data, size_t length) {
  uint8_t crc;

  // FRAM initialized?
  if(fram_found == false) {
    return false;
  }

  // Calculate checksum and write data
  crc = fram_crc8((uint8_t *)data, length);
  fram->writeEnable(true);
  fram->write(0, (uint8_t *)&crc, sizeof(uint8_t));
  fram->writeEnable(false);
  fram->writeEnable(true);
  fram->write(1, (uint8_t *)data, length);
  fram->writeEnable(false);

  return true;
}

// Format FRAM
bool fram_clear(size_t length) {
  // FRAM initialized?
  if(fram_found == false) {
    return false;
  }

  fram->writeEnable(true);
  for(size_t pos=0; pos<length; pos++) {
    fram->write8(--length, 0x00);
  }
  fram->writeEnable(false);

  return true;
}

// CRC8 checksum calculation
uint8_t fram_crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;        // Initial value 0x00
  const uint8_t poly = 0x07; // Polynominal x^8+x^2+x+1

  if(len > 0) {
    do {
      crc ^= *data++;
      for(uint8_t i = 0; i < 8; i++) {
        if((crc & 0x80) != 0) {
          crc = (uint8_t)((crc << 1) ^ poly);
        } else {
          crc <<= 1;
        }
      }
    } while(--len);
  }
  return crc;
}