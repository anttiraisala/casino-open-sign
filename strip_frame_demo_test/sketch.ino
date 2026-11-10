/*
 * Strip frame demo test - the LED strip of the sign frame (18 pixels) and a single pixel
 *
 * Uses the same 12 V WS2811 strip as strip_knight_rider_test: one control chip ("pixel") per 3 LEDs (10 cm).
 * One strip of FRAME_PIXELS = 18 pixels runs around the whole sign (60 x 30 cm): it starts at the lower-left corner
 * and runs counter-clockwise, 6 pixels on the bottom, 3 on the right, 6 on the top, 3 on the left:
 *   pixels  1.. 6  bottom edge, left to right      pixels 10..15  top edge, right to left
 *   pixels  7.. 9  right edge, going up            pixels 16..18  left edge, going down (18 is right above 1)
 *
 *   Pin D9 (frame strip, 18 pixels): three Knight Rider lights on the SAME strip. The light route starts at the
 *     upper-left corner (pixel 15) and runs CLOCKWISE: top edge to the right (15..10), right edge down (9..7),
 *     bottom edge to the left (6..1), left edge up (18..16). At pixel 16, the last one, a light turns around and runs
 *     the same route backwards to pixel 15, and so on.
 *     RED   light: one step every 500 ms  (2 pixels per second, base speed)
 *     GREEN light: 1.3 x faster           (one step every 385 ms)
 *     BLUE  light: 1.5 x faster           (one step every 333 ms)
 *   The lights add their colours where they overlap (red + green = yellow, all three = white), and drift out of
 *   phase, so the strip shows how the colours mix. The end pixels are shown once per turn, so a cycle is 34 steps.
 *
 *   Pin D10 (single pixel, 1 chip = 3 LEDs): a pulse dark -> bright -> dark every PULSE_MS (3 s). The colour changes
 *     at every pulse: red, orange, yellow, green, cyan, blue, violet, white, then again.
 *
 *   Pin D11 (colour bar, 6 pixels = 60 cm): a colour sequence red, green, blue, cyan, violet, white shows one colour
 *     per pixel and moves on by one pixel every BAR_STEP_MS (1 s) in the direction of the pixel numbers (1 -> 6); the
 *     colour that leaves pixel 6 comes back at pixel 1, so the bar goes round and round.
 *
 * Peak brightness is TEST_LEVEL = 80 (of 255) for every colour channel that is fully on, same as the other tests.
 * The strips are powered from a 12 V supply and the Arduino from USB (see README.txt).
 * If the colours are wrong (red looks green), change COLOR_ORDER (see README.txt).
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as fast
 * while many state changes happen; it is shortly inverted when a frame light turns around at an end or the pulse
 * colour changes). Once per second (DEBUG_MS) the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 * "DBG up=.. red=.. green=.. blue=.. d10=<colour>:<level> d11=<shift> events=.. beats=..": uptime in seconds, the strip
 * pixel (1..18) of each frame light, colour number and brightness of the single pixel, the shift of the colour bar, state changes in the last second
 * and LED toggles. The values change from line to line.
 *
 * All timing uses millis(); the sketch never calls delay(). Function signatures use only built-in types
 * (Arduino's automatic prototypes).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t PIN_FRAME = 9;             // frame strip, 18 pixels
constexpr uint8_t PIN_SINGLE = 10;           // single pixel (one 3-LED cluster)
constexpr uint8_t PIN_BAR = 11;              // colour bar strip, 6 pixels
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint16_t FRAME_PIXELS = 18;        // pixels of the frame strip (6 + 3 + 6 + 3 chips = 54 LEDs = 1.8 m)
constexpr uint16_t ROUTE_START_PIXEL = 15;   // the light route starts here (upper-left corner), strip pixel 1..FRAME_PIXELS
constexpr uint16_t SINGLE_PIXELS = 1;        // pixels on D10
constexpr uint16_t BAR_PIXELS = 6;           // pixels on D11 (60 cm)
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // try NEO_RGB + NEO_KHZ800 if the colours are wrong
constexpr uint8_t TEST_LEVEL = 80;           // 0..255, peak brightness of a fully-on colour channel
constexpr uint32_t STEP_RED_MS = 500;        // red: 2 pixels per second
constexpr uint32_t STEP_GREEN_MS = 385;      // green: 1.3 x faster (500 / 1.3 = 384.6)
constexpr uint32_t STEP_BLUE_MS = 333;       // blue: 1.5 x faster (500 / 1.5 = 333.3)
constexpr uint32_t BAR_STEP_MS = 1000;        // D11: the colour sequence moves on by one pixel
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

// D11 colour sequence: red, green, blue, cyan, violet, white (indices into PALETTE).
constexpr uint8_t BAR_COLOURS[BAR_PIXELS] = {0, 3, 5, 4, 6, 7};

// One named object per strip (NeoPixel objects must not be copied).
Adafruit_NeoPixel frame(FRAME_PIXELS, PIN_FRAME, COLOR_ORDER);
Adafruit_NeoPixel single(SINGLE_PIXELS, PIN_SINGLE, COLOR_ORDER);
Adafruit_NeoPixel bar(BAR_PIXELS, PIN_BAR, COLOR_ORDER);

// The light route: position 0 is the upper-left corner (strip pixel 15), the route runs clockwise:
// top edge (strip pixels 15..10), right edge (9..7), bottom edge (6..1), left edge (18..16). Position 17 is strip pixel 16.
// A cycle has 2 * (FRAME_PIXELS - 1) steps: positions 0 .. 17 going out, then 16 .. 1 coming back.
constexpr uint16_t CYCLE_STEPS = 2 * (FRAME_PIXELS - 1);
constexpr uint16_t PULSE_HALF = PULSE_TICKS / 2;

uint16_t stepRed = 0, stepGreen = 0, stepBlue = 0;     // position in the cycle of each light
uint32_t nextRed = 0, nextGreen = 0, nextBlue = 0;
uint16_t pulseTick = 0;                                // 0 .. PULSE_TICKS - 1
uint8_t colourIndex = 0;                               // palette entry of the current pulse
uint8_t singleLevel = 0;                               // brightness of the current pulse step
uint32_t nextPulse = 0;
uint8_t barShift = 0;                                  // 0 .. BAR_PIXELS - 1: how far the colour sequence has moved
uint32_t nextBar = 0;
uint32_t nextBeat = 0, invertUntil = 0, nextDebug = 0;
bool ledState = false;
uint8_t eventsNow = 0, eventsPrev = 0, eventsShown = 0;
uint32_t beats = 0;

// Position on the route (0 .. FRAME_PIXELS - 1) of a step in the cycle.
uint16_t routeOfStep(uint16_t step) {
  return step < FRAME_PIXELS ? step : CYCLE_STEPS - step;
}

// Strip pixel (0-based, 0 = strip pixel 1 at the lower-left corner) of a route position: the route runs from
// the upper-left corner (0-based pixel ROUTE_START_PIXEL - 1) against the numbering of the strip, wrapping around.
uint16_t pixelOfRoute(uint16_t route) {
  return (ROUTE_START_PIXEL - 1 + FRAME_PIXELS - route) % FRAME_PIXELS;
}

// The lit strip pixel of a step in the cycle.
uint16_t pixelOfStep(uint16_t step) {
  return pixelOfRoute(routeOfStep(step));
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
  const uint16_t r = routeOfStep(step);
  return r == 0 || r == FRAME_PIXELS - 1;
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

// Colour bar: pixel i shows sequence entry (i - barShift) modulo BAR_PIXELS, scaled so that the largest channel is TEST_LEVEL.
void drawBar() {
  for (uint16_t i = 0; i < BAR_PIXELS; i++) {
    const uint8_t *c = PALETTE[BAR_COLOURS[(i + BAR_PIXELS - barShift) % BAR_PIXELS]];
    bar.setPixelColor(i, Adafruit_NeoPixel::Color((uint16_t)c[0] * TEST_LEVEL / 255, (uint16_t)c[1] * TEST_LEVEL / 255,
                                                  (uint16_t)c[2] * TEST_LEVEL / 255));
  }
  bar.show();
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
  Serial.print(F(" d11="));
  Serial.print(barShift);
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
  bar.begin();
  drawFrame();
  drawSingle();
  drawBar();
  const uint32_t now = millis();
  nextRed = now + STEP_RED_MS;
  nextGreen = now + STEP_GREEN_MS;
  nextBlue = now + STEP_BLUE_MS;
  nextPulse = now + PULSE_UPDATE_MS;
  nextBar = now + BAR_STEP_MS;
  nextBeat = now + HEARTBEAT_MS;
  nextDebug = now + DEBUG_MS;
  Serial.println(F("Strip frame demo: D9 18 pixels, light route clockwise from the upper-left corner, red 2/s, green 1.3x, blue 1.5x; D10 single pixel pulse, D11 colour bar"));
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

  if ((int32_t)(now - nextBar) >= 0) {
    nextBar += BAR_STEP_MS;
    barShift = (barShift + 1) % BAR_PIXELS;   // a steady rhythm, not counted as a state change event
    drawBar();
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
