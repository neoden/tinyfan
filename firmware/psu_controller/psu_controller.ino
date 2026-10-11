// PSU fan controller firmware for the psu_controller board (ATtiny85, SOIC-8).
//
// Hardware (see ../../README.md):
//   PB3 / ADC3  NTC thermistor to GND, 10k pull-up (R2) to +5 V, 100 nF (C2)
//   PB1 / OC1A  1k (R3) into the base of Q1 (NPN); Q1 collector on the fan PWM pin
//   PB0         fan tach (unused by this firmware)
//
// Build target: ATTinyCore, ATtiny85 @ 8 MHz internal RC (see ../Makefile).

#include <avr/wdt.h>

// --- Thermistor ---
const float R_FIXED = 10000.0;              // R2 on the board (10k 1%); the breadboard prototype had 10.5k
const float R0 = 10000.0, T0 = 298.15, BETA = 3435.0;
const uint8_t NTC_PIN = A3;                 // ADC3 = PB3 (pin 2 of the SOIC-8)

// --- Controller ---
const float T_TARGET = 55.0;                // setpoint, degC
const float T_PANIC  = 70.0;                // above this: 100% immediately
const float KP = 3.0;                       // duty counts per degC of error
const float KI = 0.05;                      // duty counts per second per degC, accumulated

// --- PWM ---
// PB1 drives Q1 through 1k; Q1 pulls the fan's PWM line low when on.
// Transistor on = line low, so the register gets the inverted duty:
// fan duty 100% = OCR1A 0 (transistor always off, line released).
// Timer1 runs at F_CPU/prescaler; TOP = 81 gives 2 MHz / 82 = 24.4 kHz,
// inside the 21-28 kHz band of the 4-pin fan spec.
const uint8_t PWM_TOP  = 81;
const uint8_t DUTY_MIN = 16;                // ~20%
const uint8_t FAN_PIN  = PIN_PB1;

#if F_CPU == 16000000L || F_CPU == 16500000L
  #define T1_PRESCALE (_BV(CS12))               // /8
#elif F_CPU == 8000000L
  #define T1_PRESCALE (_BV(CS11) | _BV(CS10))   // /4
#else
  #error "Unsupported F_CPU: pick 8 MHz or 16 MHz so the PWM lands near 25 kHz"
#endif

float duty = PWM_TOP;                       // start at 100%
float integral = 0;

// After a watchdog reset the ATtiny85 keeps the watchdog enabled with the
// shortest (16 ms) timeout. Clear it before the Arduino core and setup() run,
// otherwise the chip can get stuck in a reset loop.
void wdt_init(void) __attribute__((naked, used, section(".init3")));
void wdt_init(void) {
  MCUSR = 0;
  wdt_disable();
}

// d is the fan duty in timer counts, 0..PWM_TOP.
void setFan(float d) {
  uint8_t counts = (uint8_t)(d + 0.5);
  if (counts >= PWM_TOP) {
    // Full speed: detach the compare output and hold PB1 low so Q1 stays off.
    // Avoids relying on OCR1A = 0 edge behaviour.
    TCCR1 &= ~(_BV(COM1A1) | _BV(COM1A0));
    digitalWrite(FAN_PIN, LOW);
  } else {
    OCR1A = PWM_TOP - counts;
    TCCR1 |= _BV(COM1A1);
  }
}

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);               // Q1 off -> fan at 100% until the loop takes over
  PLLCSR &= ~_BV(PCKE);                     // Timer1 from the system clock, not the PLL
  TCCR1 = _BV(PWM1A) | T1_PRESCALE;         // PWM on OC1A, TOP = OCR1C
  OCR1C = PWM_TOP;
  setFan(PWM_TOP);
  wdt_enable(WDTO_2S);
}

// Returns 999 when the sensor reads open or shorted, which forces 100%.
float readTemp() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(NTC_PIN);
  float adc = sum / 16.0;
  if (adc < 20 || adc > 1000) return 999;
  float r = R_FIXED * adc / (1023.0 - adc);
  return 1.0 / (1.0 / T0 + log(r / R0) / BETA) - 273.15;
}

void loop() {
  wdt_reset();
  float t = readTemp();
  float err = t - T_TARGET;
  const float maxI = (PWM_TOP - DUTY_MIN) / KI;

  if (t > T_PANIC) {
    duty = PWM_TOP;
    integral = maxI;
  } else {
    integral += err;                        // loop period is 1 s, so this is degC*s
    if (integral > maxI) integral = maxI;   // anti-windup
    if (integral < 0)    integral = 0;
    duty = DUTY_MIN + KP * err + KI * integral;
  }

  if (duty > PWM_TOP)  duty = PWM_TOP;
  if (duty < DUTY_MIN) duty = DUTY_MIN;
  setFan(duty);
  delay(1000);
}
