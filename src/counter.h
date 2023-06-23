#ifndef COUNTER_H
#define COUNTER_H

#define COUNTER_PIN 5                                          // D5 (PD5 / T1) as pulse input
#define COUNTER_OVF_60S (60 / ((float)1 / F_CPU) / 1024 / 256) // 8bit timer preload value for 60s interval, prescaler 1024

// Definition for callback
typedef void (*counter_callback_t)(uint16_t value);

// Functions
void counter_setup(counter_callback_t func_ptr);
void counter_loop(void);

#endif