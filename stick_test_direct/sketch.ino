/*
 * Direct stick test - 11 Grove RGB LED sticks plugged UNMODIFIED into the Grove Base Shield
 *
 * Purpose: a proof of concept that the LED sticks work and that the Arduino Mega + Grove Base Shield,
 * fed from an external 5 V supply, can run all of them at the same time (see README.txt: the supply goes
 * to the Mega's 5V and GND pins, the shield 3V3/5V switch must be on 5 V). Nothing here has anything to do
 * with the casino sign itself.
 *
 * WHAT IT DOES: every stick shows a moving pattern ALL THE TIME, starting immediately at power-up
 * (no dark pause, no test phases). The Grove Base Shield for the Mega has 11 Grove connectors with a
 * data pin (D2..D8 and A0..A3), so up to 11 sticks are supported; set ACTIVE_STICKS to the number you have
 * (the sticks go in the connectors in the pin order below). Unused connectors do no harm.
 *   - Each stick runs one of 11 patterns (scanner / "Knight Rider", chase, comet, rainbow, theater chase,
 *     two bouncing dots, fill and empty, odd/even blink, sparkle, police lights, reverse runner).
 *   - SPOTLIGHT: one stick at a time (changing every SPOT_MS) lights ALL its 10 LEDs white. All other sticks
 *     show only a few LEDs, so the total load stays moderate. The spotlight walks from stick to stick.
 *   - PATTERN SHIFT: every SET_MS the patterns move one stick further, so every stick shows every pattern.
 *   - LOAD TEST: hold the push button on pin 22 (to GND): all LEDs of all sticks light white at
 *     MAX_BRIGHTNESS (the worst case; about 3.3 A at 128 with 11 sticks) until you let go.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as
 * fast while many state changes happen or the load test is on; it is shortly inverted when the spotlight
 * or the pattern set changes). Once per second (DEBUG_MS) the Serial Monitor (115200 baud, SERIAL_BAUD)
 * gets a line "DBG up=.. fps=.. spot=.. set=.. events=.. beats=.. load=..": uptime, frames per second,
 * spotlight stick, pattern set, state changes in the last second, LED toggles and load-test flag.
 * The values change from line to line. (With the USB cable unplugged, as the supply is on, there is no
 * Serial Monitor; the pin 13 LED is then the only sign of life.)
 *
 * Settings are at the top. Only built-in types are used in function signatures (Arduino's
 * automatic prototypes, see the main sketch).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t STICK_COUNT = 11;          // sticks the sketch knows (do not change)
constexpr uint8_t ACTIVE_STICKS = 11;        // sticks really connected, 1..STICK_COUNT, in the pin order below
constexpr uint8_t LEDS_PER_STICK = 10;
// Data pins = the yellow SIG wire of the sticks. These are the signal pins of the Grove connectors
// D2..D8 and A0..A3 of the Grove Base Shield (stick n is plugged into the n-th connector in this list).
constexpr uint8_t STICK_PINS[STICK_COUNT] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3};
constexpr uint8_t PIN_BUTTON = 22;           // optional push button to GND (pin 22 on the Mega's long header)
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a state change
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated at the top of this file and printed at start)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" debug line
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // WS2813: green-red-blue order, 800 kHz

constexpr uint8_t MAX_BRIGHTNESS = 128;      // 0..255. 255 = all LEDs full white (about 0.5 A per stick).
constexpr uint32_t FRAME_MS = 20;            // data refresh interval
constexpr uint32_t SPOT_MS = 1200;           // time the spotlight stays on one stick
constexpr uint32_t SET_MS = 9600;            // time until the patterns move one stick further (a multiple of SPOT_MS)
constexpr uint8_t PATTERN_COUNT = 11;
constexpr uint8_t ESTIMATE_MA_PER_CHANNEL = 20;   // guess for the Serial estimate: mA per colour channel at full brightness
// ===========================================================================

// One named object per stick (NeoPixel objects must not be copied) and a pointer array to loop over them.
Adafruit_NeoPixel stick1(LEDS_PER_STICK, STICK_PINS[0], COLOR_ORDER);
Adafruit_NeoPixel stick2(LEDS_PER_STICK, STICK_PINS[1], COLOR_ORDER);
Adafruit_NeoPixel stick3(LEDS_PER_STICK, STICK_PINS[2], COLOR_ORDER);
Adafruit_NeoPixel stick4(LEDS_PER_STICK, STICK_PINS[3], COLOR_ORDER);
Adafruit_NeoPixel stick5(LEDS_PER_STICK, STICK_PINS[4], COLOR_ORDER);
Adafruit_NeoPixel stick6(LEDS_PER_STICK, STICK_PINS[5], COLOR_ORDER);
Adafruit_NeoPixel stick7(LEDS_PER_STICK, STICK_PINS[6], COLOR_ORDER);
Adafruit_NeoPixel stick8(LEDS_PER_STICK, STICK_PINS[7], COLOR_ORDER);
Adafruit_NeoPixel stick9(LEDS_PER_STICK, STICK_PINS[8], COLOR_ORDER);
Adafruit_NeoPixel stick10(LEDS_PER_STICK, STICK_PINS[9], COLOR_ORDER);
Adafruit_NeoPixel stick11(LEDS_PER_STICK, STICK_PINS[10], COLOR_ORDER);
Adafruit_NeoPixel *const sticks[STICK_COUNT] = {&stick1, &stick2, &stick3, &stick4, &stick5, &stick6, &stick7, &stick8, &stick9, &stick10, &stick11};

unsigned long lastFrame = 0;
unsigned long lastBeat = 0;
bool beatOn = false;
unsigned long beatCount = 0;      // LED toggles since power-up (shown in the debug line)
unsigned long lastDebug = 0;
unsigned long frameCount = 0;     // frames drawn since the last debug line
unsigned long lastEventAt = 0;    // when the last state change happened
bool anyEvent = false;
unsigned int eventsNow = 0;       // state changes in the current second
unsigned int eventsPrev = 0;      // state changes in the second before
unsigned long eventWindowStart = 0;
unsigned long eventsShown = 0;    // total state changes (spotlight moves, pattern shifts, load test on/off)
uint8_t lastSpot = 255;
unsigned long lastSet = 0xFFFFFFFFUL;
bool lastLoad = false;
bool loadMode = false;

uint32_t estimateMilliamps(uint8_t brightness) {
  return (uint32_t)ACTIVE_STICKS * LEDS_PER_STICK * 3UL * ESTIMATE_MA_PER_CHANNEL * brightness / 255UL;
}

// Colour wheel: 0 = red, 85 = green, 170 = blue, back to red at 255.
uint32_t wheel(uint8_t pos) {
  if (pos < 85) return Adafruit_NeoPixel::Color(255 - pos * 3, pos * 3, 0);
  if (pos < 170) { pos -= 85; return Adafruit_NeoPixel::Color(0, 255 - pos * 3, pos * 3); }
  pos -= 170;
  return Adafruit_NeoPixel::Color(pos * 3, 0, 255 - pos * 3);
}

// Cheap deterministic "random" number for the sparkle pattern.
uint16_t scramble(unsigned long a, uint8_t b, uint8_t c) {
  uint32_t h = (uint32_t)a * 2654435761UL + b * 40503UL + c * 9973UL + 12345UL;
  h ^= h >> 15;
  h *= 2246822519UL;
  h ^= h >> 13;
  return (uint16_t)(h & 0xFFFF);
}

// Position of a dot that bounces 0,1,...,9,8,...,1,0,1... one step every stepMs.
uint8_t bouncePos(unsigned long t, uint16_t stepMs) {
  uint8_t s = (uint8_t)((t / stepMs) % 18);
  return s < 10 ? s : 18 - s;
}

void px(uint8_t stick, int pos, uint32_t colour) {   // sets one LED; positions outside the stick are ignored
  if (pos >= 0 && pos < LEDS_PER_STICK) sticks[stick]->setPixelColor(pos, colour);
}

void dim(uint8_t stick, int pos, uint8_t r, uint8_t g, uint8_t b) {
  px(stick, pos, Adafruit_NeoPixel::Color(r, g, b));
}

// The 11 patterns. Every pattern lights between 1 and 6 LEDs at any moment, and is a pure function of the time t.
void drawPattern(uint8_t stick, uint8_t pattern, unsigned long t) {
  switch (pattern) {
    case 0: {                                          // scanner ("Knight Rider"): red dot with a dim glow
      int p = bouncePos(t, 90);
      dim(stick, p - 1, 50, 0, 0);
      dim(stick, p + 1, 50, 0, 0);
      dim(stick, p, 255, 0, 0);
      break;
    }
    case 1: {                                          // chase: green runners, every 5th LED
      uint8_t s = (uint8_t)((t / 70) % 5);
      for (uint8_t i = 0; i < LEDS_PER_STICK; i++) if (i % 5 == s) dim(stick, i, 0, 255, 0);
      break;
    }
    case 2: {                                          // comet: white head with a fading tail, runs off the end
      int head = (int)((t / 60) % 12);
      dim(stick, head, 255, 255, 255);
      dim(stick, head - 1, 120, 120, 160);
      dim(stick, head - 2, 40, 40, 80);
      dim(stick, head - 3, 10, 10, 30);
      break;
    }
    case 3: {                                          // rainbow comet: 4 LEDs with a rolling colour
      uint8_t pos = (uint8_t)((t / 110) % LEDS_PER_STICK);
      for (uint8_t k = 0; k < 4; k++) px(stick, (pos + k) % LEDS_PER_STICK, wheel((uint8_t)(k * 50 + t / 12)));
      break;
    }
    case 4: {                                          // theater chase: every 3rd LED, orange
      uint8_t s = (uint8_t)((t / 150) % 3);
      for (uint8_t i = 0; i < LEDS_PER_STICK; i++) if (i % 3 == s) dim(stick, i, 255, 90, 0);
      break;
    }
    case 5: {                                          // two dots bouncing towards each other
      int p = bouncePos(t, 100);
      dim(stick, p, 0, 0, 255);
      dim(stick, 9 - p, 255, 0, 200);
      break;
    }
    case 6: {                                          // fill up to 6 LEDs and empty again, cyan
      uint8_t s = (uint8_t)((t / 120) % 11);
      uint8_t n = s < 6 ? s + 1 : 11 - s;
      for (uint8_t i = 0; i < n; i++) dim(stick, i, 0, 220, 220);
      break;
    }
    case 7: {                                          // odd / even LEDs blink in turn, yellow
      uint8_t s = (uint8_t)((t / 200) % 2);
      for (uint8_t i = 0; i < LEDS_PER_STICK; i++) if (i % 2 == s) dim(stick, i, 255, 200, 0);
      break;
    }
    case 8: {                                          // sparkle: 3 pseudo-random LEDs, new ones every 80 ms
      unsigned long step = t / 80;
      for (uint8_t k = 0; k < 3; k++) {
        uint16_t h = scramble(step, stick, k);
        px(stick, h % LEDS_PER_STICK, wheel((uint8_t)(h >> 8)));
      }
      break;
    }
    case 9: {                                          // police lights: 3 red LEDs at one end, 3 blue at the other, alternating
      if ((t / 150) % 2 == 0) { for (uint8_t i = 0; i < 3; i++) dim(stick, i, 255, 0, 0); }
      else { for (uint8_t i = 7; i < 10; i++) dim(stick, i, 0, 0, 255); }
      break;
    }
    default: {                                         // reverse runner: two purple LEDs running backwards
      int p = 9 - (int)((t / 80) % 10);
      dim(stick, p, 180, 0, 255);
      dim(stick, (p + 5) % LEDS_PER_STICK, 180, 0, 255);
      break;
    }
  }
}

uint8_t spotStick(unsigned long t) { return (uint8_t)((t / SPOT_MS) % ACTIVE_STICKS); }
unsigned long patternSet(unsigned long t) { return t / SET_MS; }
uint8_t patternFor(uint8_t stick, unsigned long t) { return (uint8_t)((stick + patternSet(t)) % PATTERN_COUNT); }

void noteEvent(unsigned long now) {
  lastEventAt = now;
  anyEvent = true;
  eventsNow++;
  eventsShown++;
}

// Draws and shows one frame (the whole picture is a function of the time t).
void drawFrame(unsigned long t) {
  const uint8_t spot = spotStick(t);
  const unsigned long set = patternSet(t);
  if (spot != lastSpot) { if (lastSpot != 255) noteEvent(t); lastSpot = spot; }
  if (set != lastSet) { if (lastSet != 0xFFFFFFFFUL) noteEvent(t); lastSet = set; }
  if (loadMode != lastLoad) { noteEvent(t); lastLoad = loadMode; }
  for (uint8_t s = 0; s < ACTIVE_STICKS; s++) {
    sticks[s]->clear();
    if (loadMode || s == spot) {
      for (uint8_t i = 0; i < LEDS_PER_STICK; i++) sticks[s]->setPixelColor(i, Adafruit_NeoPixel::Color(255, 255, 255));
    } else {
      drawPattern(s, patternFor(s, t), t);
    }
  }
  for (uint8_t s = 0; s < ACTIVE_STICKS; s++) sticks[s]->show();
  frameCount++;
}

// Prints the once-per-second debug line (uptime, fps and beats change every time).
void printDebug(unsigned long now) {
  Serial.print(F("DBG up="));
  Serial.print(now / 1000UL);
  Serial.print(F("s fps="));
  Serial.print(frameCount);
  Serial.print(F(" spot="));
  Serial.print((int)(spotStick(now) + 1));
  Serial.print(F(" set="));
  Serial.print(patternSet(now));
  Serial.print(F(" events="));
  Serial.print(eventsShown);
  Serial.print(F(" beats="));
  Serial.print(beatCount);
  Serial.print(F(" load="));
  Serial.println(loadMode ? 1 : 0);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (uint8_t s = 0; s < STICK_COUNT; s++) {
    sticks[s]->begin();
    sticks[s]->setBrightness(MAX_BRIGHTNESS);
    sticks[s]->clear();
    sticks[s]->show();
  }
  Serial.println(F("Direct stick test: LED sticks in the Grove shield, moving patterns (see the top of sketch.ino)."));
  Serial.print(F("Serial speed "));
  Serial.print(SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));
  Serial.print(F("Load test (button on pin 22 held): estimated "));
  Serial.print(estimateMilliamps(MAX_BRIGHTNESS));
  Serial.println(F(" mA (a guess, not a measurement)."));
  const unsigned long now = millis();
  lastFrame = now;
  lastBeat = now;
  lastDebug = now;
  eventWindowStart = now;
  drawFrame(now);                            // the patterns start at once, no dark pause
}

void loop() {
  const unsigned long now = millis();
  loadMode = (digitalRead(PIN_BUTTON) == LOW);
  if (now - eventWindowStart >= 1000) {      // sliding one-second window of state changes
    eventsPrev = eventsNow;
    eventsNow = 0;
    eventWindowStart = now;
  }
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebug(now);
    frameCount = 0;
  }
  const bool busy = loadMode || (eventsNow + eventsPrev >= 2);
  const unsigned long beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
  if (now - lastBeat >= beatMs) {            // sign of life: the LED blinks all the time, faster when busy
    lastBeat = now;
    beatOn = !beatOn;
    beatCount++;
  }
  const bool inverted = anyEvent && (now - lastEventAt < INVERT_MS);   // shortly inverted after a state change
  digitalWrite(PIN_STATUS_LED, (beatOn != inverted) ? HIGH : LOW);
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    drawFrame(now);
  }
}
