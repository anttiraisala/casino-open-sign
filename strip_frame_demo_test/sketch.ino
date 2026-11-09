/*
 * Strip frame demo test - the LED strip of the sign frame (16 pixels) and a single pixel
 *
 * Uses the same 12 V WS2811 strip as strip_knight_rider_test: one control chip ("pixel") per 3 LEDs (10 cm).
 * The frame strip is mounted so that pixel 1 is at the lower-left corner of the sign and the strip runs
 * counter-clockwise: along the bottom edge to the right, up the right side, along the top edge to the left and
 * down the left side to pixel 16 (FRAME_PIXELS). The direction matters only for how you read the result.
 *
 *   Pin D9 (frame strip, 16 pixels): three Knight Rider lights on the SAME strip, each bouncing pixel 1 -> 16 -> 1
 *     RED   light: one step every 500 ms  (2 pixels per second, base speed)
 *     GREEN light: 1.3 x faster           (one step every 385 ms)
 *     BLUE  light: 1.5 x faster           (one step every 333 ms)
 *   The lights add their colours where they overlap (red + green = yellow, all three = white), and drift out of
 *   phase, so the strip shows how the colours mix. The end pixels are shown once per turn, so a cycle is 30 steps.
 *
 *   Pin D10 (single pixel, 1 chip = 3 LEDs): a pulse dark -> bright -> dark every PULSE_MS (3 s). The colour changes
 *     at every pulse: red, orange, yellow, green, cyan, blue, violet, white, then again.
 *
 * Peak brightness is TEST_LEVEL = 80 (of 255) for every colour channel that is fully on, same as the other tests.
 * The strips are powered from a 12 V supply and the Arduino from USB (see README.txt).
 * If the colours are wrong (red looks green), change COLOR_ORDER (see README.txt).
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as fast
 * while many state changes happen; it is shortly inverted when a frame light turns around at an end or the pulse
 * colour changes). Once per second (DEBUG_MS) the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 * "DBG up=.. red=.. green=.. blue=.. d10=<colour>:<level> events=.. beats=..": uptime in seconds, the pixel
 * (1..16) of each frame light, colour number and brightness of the single pixel, state changes in the last second
 * and LED toggles. The values change from line to line.
 *
 * All timing uses millis(); the sketch never calls delay(). Function signatures use only built-in types
 * (Arduino's automatic prototypes).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t PIN_FRAME = 9;             // frame strip, 16 pixels
constexpr uint8_t PIN_SINGLE = 10;           // single pixel (one 3-LED cluster)
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint16_t FRAME_PIXELS = 16;        // pixels of the frame strip (16 chips = 48 LEDs = 1.6 m)
constexpr uint16_t SINGLE_PIXELS = 1;        // pixels on D10
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // try NEO_RGB + NEO_KHZ800 if the colours are wrong
constexpr uint8_t TEST_LEVEL = 80;           // 0..255, peak brightness of a fully-on colour channel
constexpr uint32_t STEP_RED_MS = 500;        // red: 2 pixels per second
constexpr uint32_t STEP_GREEN_MS = 385;      // green: 1.3 x faster (500 / 1.3 = 384.6)
constexpr uint32_t STEP_BLUE_MS = 333;       // blue: 1.5 x faster (500 / 1.5 = 333.3)
constexpr uint32_t PULSE_UPDATE_MS = 20;     // D10: brightness update interval
constexpr uint16_t PULSE_TICKS = 150;        // updates per pulse: 150 x 20 ms = 3 s per pulse (dark -> bright -> dark)
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a state change
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated at the top of this file and printed at start)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" debug line
constexpr uint8_t FAST_EVENTS = 3;           // this many state changes in the last second = "many"
// ===========================================================================

// Colours of the D10 pulse, full-scale (the brightness scales them so that the largest value peaks at TEST_LEVEL).
constexpr uint8_t PALETTE_SIZE = 8;
constexpr uint8_t PALETTE[PALETTE_SIZE][3] = {
  {255, 0, 0},      // 0 red
  {255, 100, 0},    // 1 orange
  {255, 255, 0},    // 2 yellow
  {0, 255, 0},      // 3 green
  {0, 255, 255},    // 4 cyan
  {0, 0, 255},      // 5 blue
  {160, 0, 255},    // 6 violet
  {255, 255, 255},  // 7 white
};

// One named object per strip (NeoPixel objects must not be copied).
Adafruit_NeoPixel frame(FRAME_PIXELS, PIN_FRAME, COLOR_ORDER);
Adafruit_NeoPixel single(SINGLE_PIXELS, PIN_SINGLE, COLOR_ORDER);

// A cycle has 2 * (FRAME_PIXELS - 1) steps: 0 .. 15 going up, then 16 .. 29 going down (pixel 30 - step).
constexpr uint16_t CYCLE_STEPS = 2 * (FRAME_PIXELS - 1);
constexpr uint16_t PULSE_HALF = PULSE_TICKS / 2;

uint16_t stepRed = 0, stepGreen = 0, stepBlue = 0;     // position in the cycle of each light
uint32_t nextRed = 0, nextGreen = 0, nextBlue = 0;
uint16_t pulseTick = 0;                                // 0 .. PULSE_TICKS - 1
uint8_t colourIndex = 0;                               // palette entry of the current pulse
uint8_t singleLevel = 0;                               // brightness of the current pulse step
uint32_t nextPulse = 0;
uint32_t nextBeat = 0, invertUntil = 0, nextDebug = 0;
bool ledState = false;
uint8_t eventsNow = 0, eventsPrev = 0, eventsShown = 0;
uint32_t beats = 0;

// The lit pixel of a step in the cycle.
uint16_t pixelOfStep(uint16_t step) {
  return step < FRAME_PIXELS ? step : CYCLE_STEPS - step;
}

// A state change: the status LED is shortly inverted and the event is counted.
void noteEvent(uint32_t now) {
  invertUntil = now + INVERT_MS;
  eventsNow++;
  eventsShown++;
}

// Advances one light by one step; returns true when it arrived at an end of the strip.
bool advance(uint16_t &step) {
  step = (step + 1) % CYCLE_STEPS;
  const uint16_t p = pixelOfStep(step);
  return p == 0 || p == FRAME_PIXELS - 1;
}

// Brightness of a pulse step: 0 at the start, TEST_LEVEL at the middle, back towards 0 at the end.
uint8_t pulseLevel(uint16_t tick) {
  return tick < PULSE_HALF ? (uint8_t)((uint32_t)tick * TEST_LEVEL / PULSE_HALF)
                           : (uint8_t)((uint32_t)(PULSE_TICKS - tick) * TEST_LEVEL / PULSE_HALF);
}

uint8_t addSaturated(uint8_t a, uint8_t b) {
  const uint16_t s = (uint16_t)a + b;
  return s > 255 ? 255 : (uint8_t)s;
}

// Frame strip: the three lights add their colours (red, green, blue channel each).
void drawFrame() {
  uint8_t r[FRAME_PIXELS], g[FRAME_PIXELS], b[FRAME_PIXELS];
  for (uint16_t i = 0; i < FRAME_PIXELS; i++) { r[i] = 0; g[i] = 0; b[i] = 0; }
  const uint16_t pr = pixelOfStep(stepRed), pg = pixelOfStep(stepGreen), pb = pixelOfStep(stepBlue);
  r[pr] = addSaturated(r[pr], TEST_LEVEL);
  g[pg] = addSaturated(g[pg], TEST_LEVEL);
  b[pb] = addSaturated(b[pb], TEST_LEVEL);
  for (uint16_t i = 0; i < FRAME_PIXELS; i++) frame.setPixelColor(i, Adafruit_NeoPixel::Color(r[i], g[i], b[i]));
  frame.show();
}

// Single pixel: the palette colour scaled by the pulse brightness (largest channel = singleLevel).
void drawSingle() {
  const uint8_t *c = PALETTE[colourIndex];
  single.setPixelColor(0, Adafruit_NeoPixel::Color((uint16_t)c[0] * singleLevel / 255,
                                                   (uint16_t)c[1] * singleLevel / 255,
                                                   (uint16_t)c[2] * singleLevel / 255));
  single.show();
}

void printDebug(uint32_t now) {
  Serial.print(F("DBG up="));
  Serial.print((unsigned long)(now / 1000));
  Serial.print(F(" red="));
  Serial.print(pixelOfStep(stepRed) + 1);
  Serial.print(F(" green="));
  Serial.print(pixelOfStep(stepGreen) + 1);
  Serial.print(F(" blue="));
  Serial.print(pixelOfStep(stepBlue) + 1);
  Serial.print(F(" d10="));
  Serial.print(colourIndex);
  Serial.print(F(":"));
  Serial.print(singleLevel);
  Serial.print(F(" events="));
  Serial.print(eventsShown);
  Serial.print(F(" beats="));
  Serial.println((unsigned long)beats);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  frame.begin();
  single.begin();
  drawFrame();
  drawSingle();
  const uint32_t now = millis();
  nextRed = now + STEP_RED_MS;
  nextGreen = now + STEP_GREEN_MS;
  nextBlue = now + STEP_BLUE_MS;
  nextPulse = now + PULSE_UPDATE_MS;
  nextBeat = now + HEARTBEAT_MS;
  nextDebug = now + DEBUG_MS;
  Serial.println(F("Strip frame demo: D9 16 pixels, red 2/s, green 1.3x, blue 1.5x; D10 single pixel pulse"));
  Serial.print(F("Serial speed "));
  Serial.print((unsigned long)SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));
}

void loop() {
  const uint32_t now = millis();

  bool frameChanged = false;
  if ((int32_t)(now - nextRed) >= 0) {
    nextRed += STEP_RED_MS;
    if (advance(stepRed)) noteEvent(now);
    frameChanged = true;
  }
  if ((int32_t)(now - nextGreen) >= 0) {
    nextGreen += STEP_GREEN_MS;
    if (advance(stepGreen)) noteEvent(now);
    frameChanged = true;
  }
  if ((int32_t)(now - nextBlue) >= 0) {
    nextBlue += STEP_BLUE_MS;
    if (advance(stepBlue)) noteEvent(now);
    frameChanged = true;
  }
  if (frameChanged) drawFrame();

  if ((int32_t)(now - nextPulse) >= 0) {
    nextPulse += PULSE_UPDATE_MS;
    pulseTick = (pulseTick + 1) % PULSE_TICKS;
    if (pulseTick == 0) {                       // a new pulse starts: next colour
      colourIndex = (colourIndex + 1) % PALETTE_SIZE;
      noteEvent(now);
    }
    singleLevel = pulseLevel(pulseTick);
    drawSingle();
  }

  // Sign of life: LED 13 blinks, twice as fast while many state changes happen, shortly inverted on a state change
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
