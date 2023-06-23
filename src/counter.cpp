#include <Arduino.h>
#include "counter.h"

static volatile uint8_t counter_flag = 0;
static volatile uint16_t counter_value = 0;
static volatile counter_callback_t counter_callback = NULL;

// Initialize Timer
void counter_setup(counter_callback_t func_ptr) {
  // Configure Timer1 as pulse counter
  pinMode(COUNTER_PIN, INPUT_PULLUP); // Configure pulse input
  TCCR1A = 0;                         // No PWM output on this timer
  TCCR1B = (1 << CS12) | (1 << CS11); // External clock source: T1 on falling edge
  TIMSK1 = 0;                         // No interrupts on this timer
  TCNT1 = 0;                          // Reset counter register

  // Configure Timer2 as interrupt clock source
  TCCR2A = 0;                                       // No PWM output on this timer
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // Prescaler clk/1024
  TIMSK2 = (1 << TOIE2);                            // Enable timer overflow interrupt
  TCNT2 = 0;                                        // Reset counter register

  // Save callback pointer
  counter_callback = func_ptr;
}

// This function must be called in main loop and will execute the callback if new data is available
void counter_loop(void) {
  if(counter_flag != 0) {
    counter_flag = 0;
    if(counter_callback != NULL) {
      (*counter_callback)(counter_value);
    }
  }
}

// ISR routine for Timer2
ISR(TIMER2_OVF_vect) {
  static volatile uint16_t ovf_count = COUNTER_OVF_60S;
  if(ovf_count == 0) {
    counter_value = TCNT1;
    TCNT1 = 0;
    counter_flag = 1;
    ovf_count = COUNTER_OVF_60S;
  }
  --ovf_count;
}