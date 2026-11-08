/*
 * LED strip single LED - test of three 12 V WS2811 strips (3 LEDs per 10 cm, 5 m roll), one test per strip
 *
 * Purpose: find out how many LEDs / pixels the strip really has and how it is addressed, by lighting ONE
 * pixel at a time. Three strips (or the same strip moved from pin to pin) run three different tests at the
 * same time. Nothing here has anything to do with the casino sign itself; the strip is powered from the
 * 12 V supply and the Arduino from USB (see README.txt).
 *
 *   Test 1, pin D9:  ONE white pixel walks along the strip: pixel 1, pixel 2, pixel 3 ... up to
 *                    STRIP_PIXELS (150), then starts again. If one control chip drives 3 LEDs, the walking
 *                    light is 10 cm long and the strip answers only to the first 50 steps (then it is dark).
 *   Test 2, pin D10: ONE pixel walks faster, and its colour changes every pass: red, green, blue, white.
 *                    If the colours are wrong (red looks green), change COLOR_ORDER (see README.txt).
 *   Test 3, pin D11: a ruler: every 10th pixel is blue, every 50th white, blinking once a second, so you
 *                    can count the pixels and see where the strip ends.
 *
 * Brightness is limited to TEST_LEVEL (0..255) for all tests.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as
 * fast while many state changes happen; it is shortly inverted when a pass ends). Once per second (DEBUG_MS)
 * the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 * "DBG up=.. t1=../150 t2=../150 pass=.. t3=on|off events=.. beats=..": uptime in seconds, the position of
 * the walking pixel in tests 1 and 2, the colour pass of test 2, the state of the ruler, state changes in
 * the last second and LED toggles. The values change from line to line.
 *
 * All timing uses millis(); the sketch never calls delay(). Function signatures use only built-in types
 * (Arduino's automatic prototypes).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t PIN_STRIP_1 = 9;           // test 1: one white pixel walks
constexpr uint8_t PIN_STRIP_2 = 10;          // test 2: one pixel walks, colour changes every pass
constexpr uint8_t PIN_STRIP_3 = 11;          // test 3: ruler
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint16_t STRIP_PIXELS = 150;       // steps / pixels the sketch tries (3 LEDs per 10 cm x 5 m = 150 LEDs;
                                             // if one chip drives 3 LEDs there are only 50 pixels, see README.txt)
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // try NEO_RGB + NEO_KHZ800 if the colours of test 2 are wrong
constexpr uint8_t TEST_LEVEL = 80;           // 0..255, brightness of the lit pixels
constexpr uint32_t STEP1_MS = 150;           // test 1: time per step (150 steps = 22.5 s per pass)
constexpr uint32_t STEP2_MS = 60;            // test 2: time per step (150 steps = 9 s per pass)
constexpr uint32_t RULER_MS = 500;           // test 3: the ruler is on for this long, then off for this long
constexpr uint16_t RULER_STEP = 10;          // test 3: every RULER_STEP-th pixel is blue
constexpr uint16_t RULER_BIG_STEP = 50;      // test 3: every RULER_BIG_STEP-th pixel is white
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a pass ends
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated at the top of this file and printed at start)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" debug line
constexpr uint8_t FAST_EVENTS = 3;           // this many state changes in the last second = "many"
// ===========================================================================

// One named object per strip (NeoPixel objects must not be copied).
Adafruit_NeoPixel strip1(STRIP_PIXELS, PIN_STRIP_1, COLOR_ORDER);
Adafruit_NeoPixel strip2(STRIP_PIXELS, PIN_STRIP_2, COLOR_ORDER);
Adafruit_NeoPixel strip3(STRIP_PIXELS, PIN_STRIP_3, COLOR_ORDER);

uint16_t pos1 = 0;            // test 1: lit pixel
uint16_t pos2 = 0;            // test 2: lit pixel
uint8_t pass2 = 0;            // test 2: colour pass 0..3 = red, green, blue, white
bool rulerOn = true;          // test 3: ruler state
uint32_t nextStep1 = 0, nextStep2 = 0, nextRuler = 0;
uint32_t nextBeat = 0, invertUntil = 0, nextDebug = 0;
bool ledState = false;
uint8_t eventsNow = 0, eventsPrev = 0, eventsShown = 0;
uint32_t beats = 0;

// A state change: the status LED is shortly inverted and the event is counted.
void noteEvent(uint32_t now) {
  invertUntil = now + INVERT_MS;
  eventsNow++;
  eventsShown++;
}

uint32_t colourOfPass(uint8_t pass) {
  if (pass == 0) return Adafruit_NeoPixel::Color(TEST_LEVEL, 0, 0);
  if (pass == 1) return Adafruit_NeoPixel::Color(0, TEST_LEVEL, 0);
  if (pass == 2) return Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL);
  return Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, TEST_LEVEL);
}

void drawTest1() {
  strip1.clear();
  strip1.setPixelColor(pos1, Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, TEST_LEVEL));
  strip1.show();
}

void drawTest2() {
  strip2.clear();
  strip2.setPixelColor(pos2, colourOfPass(pass2));
  strip2.show();
}

void drawTest3() {
  strip3.clear();
  if (rulerOn) {
    for (uint16_t i = 0; i < STRIP_PIXELS; i += RULER_STEP) {
      const bool big = (i % RULER_BIG_STEP) == 0;
      strip3.setPixelColor(i, big ? Adafruit_NeoPixel::Color(TEST_LEVEL, TEST_LEVEL, TEST_LEVEL)
                                  : Adafruit_NeoPixel::Color(0, 0, TEST_LEVEL));
    }
  }
  strip3.show();
}

void printDebug(uint32_t now) {
  Serial.print(F("DBG up="));
  Serial.print((unsigned long)(now / 1000));
  Serial.print(F(" t1="));
  Serial.print(pos1 + 1);
  Serial.print(F("/"));
  Serial.print(STRIP_PIXELS);
  Serial.print(F(" t2="));
  Serial.print(pos2 + 1);
  Serial.print(F("/"));
  Serial.print(STRIP_PIXELS);
  Serial.print(F(" pass="));
  Serial.print(pass2 + 1);
  Serial.print(F(" t3="));
  Serial.print(rulerOn ? F("on") : F("off"));
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
  drawTest1();
  drawTest2();
  drawTest3();
  const uint32_t now = millis();
  nextStep1 = now + STEP1_MS;
  nextStep2 = now + STEP2_MS;
  nextRuler = now + RULER_MS;
  nextBeat = now + HEARTBEAT_MS;
  nextDebug = now + DEBUG_MS;
  Serial.println(F("LED strip single LED test: D9 white pixel walks, D10 coloured pixel walks, D11 ruler"));
  Serial.print(F("Serial speed "));
  Serial.print((unsigned long)SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));
}

void loop() {
  const uint32_t now = millis();

  // Test 1: one white pixel walks, one step every STEP1_MS
  if ((int32_t)(now - nextStep1) >= 0) {
    nextStep1 += STEP1_MS;
    pos1++;
    if (pos1 >= STRIP_PIXELS) {
      pos1 = 0;
      noteEvent(now);
    }
    drawTest1();
  }

  // Test 2: one pixel walks, the colour changes after every pass
  if ((int32_t)(now - nextStep2) >= 0) {
    nextStep2 += STEP2_MS;
    pos2++;
    if (pos2 >= STRIP_PIXELS) {
      pos2 = 0;
      pass2 = (pass2 + 1) % 4;
      noteEvent(now);
    }
    drawTest2();
  }

  // Test 3: the ruler blinks
  if ((int32_t)(now - nextRuler) >= 0) {
    nextRuler += RULER_MS;
    rulerOn = !rulerOn;
    noteEvent(now);
    drawTest3();
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
