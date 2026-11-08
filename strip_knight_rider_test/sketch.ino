/*
 * LED strip knight rider - three 12 V WS2811 strips, each with one light running back and forth
 *
 * Assumes a strip (or a piece cut from it) of STRIP_PIXELS = 50 pixels (one control chip per 3 LEDs, 5 m roll).
 * One pixel is lit at a time and it bounces between the two ends like the light of Knight Rider:
 * pixel 1, 2, ... 50, 49, ... 2, 1, 2, ... (the end pixels are shown once per turn, so a full cycle is 98 steps).
 *
 *   Pin D9:  RED   light, 1 pixel per second   (one cycle = 98 s)
 *   Pin D10: BLUE  light, 4 pixels per second  (one cycle = 24.5 s)
 *   Pin D11: GREEN light, 10 pixels per second (one cycle = 9.8 s)
 *
 * The strips are powered from a 12 V supply and the Arduino from USB (see README.txt). Nothing here has anything
 * to do with the casino sign itself. If your strip has a different number of pixels, change STRIP_PIXELS.
 * If the colours are wrong (red looks green), change COLOR_ORDER (see README.txt).
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as fast
 * while many state changes happen; it is shortly inverted when a light turns around at an end). Once per second
 * (DEBUG_MS) the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 * "DBG up=.. d9=../50 d10=../50 d11=../50 events=.. beats=..": uptime in seconds, the lit pixel of each strip,
 * turnarounds in the last second and LED toggles. The values change from line to line.
 *
 * All timing uses millis(); the sketch never calls delay(). Function signatures use only built-in types
 * (Arduino's automatic prototypes).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t PIN_STRIP_1 = 9;           // red light
constexpr uint8_t PIN_STRIP_2 = 10;          // blue light
constexpr uint8_t PIN_STRIP_3 = 11;          // green light
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint16_t STRIP_PIXELS = 50;        // pixels of each strip (a full 5 m roll: 3 LEDs per chip = 50 chips)
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // try NEO_RGB + NEO_KHZ800 if the colours are wrong
constexpr uint8_t TEST_LEVEL = 80;           // 0..255, brightness of the lit pixel
constexpr uint32_t STEP1_MS = 1000;          // D9: 1 pixel per second
constexpr uint32_t STEP2_MS = 250;           // D10: 4 pixels per second
constexpr uint32_t STEP3_MS = 100;           // D11: 10 pixels per second
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a turnaround
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated at the top of this file and printed at start)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" debug line
constexpr uint8_t FAST_EVENTS = 3;           // this many state changes in the last second = "many"
// ===========================================================================

// One named object per strip (NeoPixel objects must not be copied).
Adafruit_NeoPixel strip1(STRIP_PIXELS, PIN_STRIP_1, COLOR_ORDER);
Adafruit_NeoPixel strip2(STRIP_PIXELS, PIN_STRIP_2, COLOR_ORDER);
Adafruit_NeoPixel strip3(STRIP_PIXELS, PIN_STRIP_3, COLOR_ORDER);

// A cycle has 2 * (STRIP_PIXELS - 1) steps: 0 .. 49 going up, then 50 .. 97 going down (pixel 98 - step).
constexpr uint16_t CYCLE_STEPS = 2 * (STRIP_PIXELS - 1);

uint16_t step1 = 0, step2 = 0, step3 = 0;     // position in the cycle of each light
uint32_t nextStep1 = 0, nextStep2 = 0, nextStep3 = 0;
uint32_t nextBeat = 0, invertUntil = 0, nextDebug = 0;
bool ledState = false;
uint8_t eventsNow = 0, eventsPrev = 0, eventsShown = 0;
uint32_t beats = 0;

// The lit pixel of a step in the cycle.
uint16_t pixelOfStep(uint16_t step) {
  return step < STRIP_PIXELS ? step : CYCLE_STEPS - step;
}

// A turnaround: the status LED is shortly inverted and the event is counted.
void noteEvent(uint32_t now) {
  invertUntil = now + INVERT_MS;
  eventsNow++;
  eventsShown++;
}

// Advances one light by one step; returns true when it arrived at an end of the strip.
bool advance(uint16_t &step) {
  step = (step + 1) % CYCLE_STEPS;
  const uint16_t p = pixelOfStep(step);
  return p == 0 || p == STRIP_PIXELS - 1;
}

void drawStrip(Adafruit_NeoPixel &strip, uint16_t step, uint32_t colour) {
  strip.clear();
  strip.setPixelColor(pixelOfStep(step), colour);
  strip.show();
}

void drawRed() { drawStrip(strip1, step1, Adafruit_NeoPixel::Color(TEST_LEVEL, 0, 0)); }
void drawBlue() { drawStrip(strip2, step2, Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL)); }
void drawGreen() { drawStrip(strip3, step3, Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0)); }

void printDebug(uint32_t now) {
  Serial.print(F("DBG up="));
  Serial.print((unsigned long)(now / 1000));
  Serial.print(F(" d9="));
  Serial.print(pixelOfStep(step1) + 1);
  Serial.print(F("/"));
  Serial.print(STRIP_PIXELS);
  Serial.print(F(" d10="));
  Serial.print(pixelOfStep(step2) + 1);
  Serial.print(F("/"));
  Serial.print(STRIP_PIXELS);
  Serial.print(F(" d11="));
  Serial.print(pixelOfStep(step3) + 1);
  Serial.print(F("/"));
  Serial.print(STRIP_PIXELS);
  Serial.print(F(" events="));
  Serial.print(eventsShown);
  Serial.print(F(" beats="));
  Serial.println((unsigned long)beats);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  strip1.begin();
  strip2.begin();
  strip3.begin();
  drawRed();
  drawBlue();
  drawGreen();
  const uint32_t now = millis();
  nextStep1 = now + STEP1_MS;
  nextStep2 = now + STEP2_MS;
  nextStep3 = now + STEP3_MS;
  nextBeat = now + HEARTBEAT_MS;
  nextDebug = now + DEBUG_MS;
  Serial.println(F("LED strip knight rider: D9 red 1/s, D10 blue 4/s, D11 green 10/s"));
  Serial.print(F("Serial speed "));
  Serial.print((unsigned long)SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));
}

void loop() {
  const uint32_t now = millis();

  if ((int32_t)(now - nextStep1) >= 0) {
    nextStep1 += STEP1_MS;
    if (advance(step1)) noteEvent(now);
    drawRed();
  }
  if ((int32_t)(now - nextStep2) >= 0) {
    nextStep2 += STEP2_MS;
    if (advance(step2)) noteEvent(now);
    drawBlue();
  }
  if ((int32_t)(now - nextStep3) >= 0) {
    nextStep3 += STEP3_MS;
    if (advance(step3)) noteEvent(now);
    drawGreen();
  }

  // Sign of life: LED 13 blinks, twice as fast while many state changes happen, shortly inverted on a turnaround
  const bool busy = (eventsNow + eventsPrev) >= FAST_EVENTS;
  if ((int32_t)(now - nextBeat) >= 0) {
    nextBeat += busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
    ledState = !ledState;
    beats++;
  }
  const bool inverted = (int32_t)(invertUntil - now) > 0;
  digitalWrite(PIN_STATUS_LED, (ledState != inverted) ? HIGH : LOW);

  // Serial debug line once per second
  if ((int32_t)(now - nextDebug) >= 0) {
    nextDebug += DEBUG_MS;
    printDebug(now);
    eventsPrev = eventsNow;
    eventsNow = 0;
    eventsShown = 0;
  }
}
