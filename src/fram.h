#ifndef FRAM_H
#define FRAM_H

#define FRAM_CS 8 // D8 (PB0) as FRAM chip select

// Macros for FRAM status register (write & block protection)
#define FRAM_WPEN 7
#define FRAM_BP1 3
#define FRAM_BP0 2

// Functions
bool fram_setup(void);
void fram_setmagic(uint16_t magic);
bool fram_read(void *data, size_t length);
bool fram_write(void *data, size_t length);
bool fram_clear(size_t length);

#endif