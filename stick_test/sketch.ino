/*
 * Stick test - 10 Grove RGB LED sticks (10 x WS2813 mini LEDs each) on a SEPARATE 5 V supply
 *
 * Purpose: check that all 10 sticks (100 LEDs) work together, that the colour order is right and that
 * the separate 5 V supply and its wiring are strong enough. The Arduino only sends the data signal
 * (one data pin per stick) and shares GND; it does NOT power the sticks. Power the sticks BEFORE
 * the Arduino (see README.txt), and keep MAX_BRIGHTNESS low until the supply has been checked.
 *
 * The test runs these phases and then starts over (the push button on pin 22 skips to the next phase):
 *   1. IDENTIFY  one stick at a time lights up white (stick 1, 2, ... 10, see layout-diagram.pdf).
 *                Checks that every stick is on its own data pin and that all 10 LEDs of it work.
 *   2. COLOURS   all sticks red, then green, then blue. Wrong colour order shows other colours.
 *   3. WALK      one white LED walks through all 100 LEDs (stick 1 to stick 10).
 *   4. RAMP      all LEDs white, brightness goes up in steps (up to MAX_BRIGHTNESS). The Serial Monitor
 *                (115200 baud) prints the step and a rough current estimate. Watch for: flicker,
 *                white turning yellow/orange/red at the far end (voltage drop), the Arduino
 *                restarting (bad ground), a supply that clicks or shuts down.
 *   5. SOAK      everything stays white at MAX_BRIGHTNESS for SOAK_MS. Touch the wires and the
 *                supply: nothing should get hot. Measure the voltage at the last stick (4.5 V or more).
 *   6. REST      everything dark for a moment, then the test starts again from phase 1.
 *
 * Settings are at the top. Only built-in types are used in function signatures (Arduino's
 * automatic prototypes, see the main sketch).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t STICK_COUNT = 10;          // sticks the sketch knows (do not change)
constexpr uint8_t ACTIVE_STICKS = 10;        // sticks really connected, 1..STICK_COUNT, in the pin order below
constexpr uint8_t LEDS_PER_STICK = 10;
// Data pins = the yellow SIG wire of the sticks. These are the signal pins of the Grove connectors
// D2..D8 and A0..A2 of the Grove Base Shield (stick n is plugged into the n-th connector in this list).
constexpr uint8_t STICK_PINS[STICK_COUNT] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2};
constexpr uint8_t PIN_BUTTON = 22;           // optional push button to GND (pin 22 on the Mega's long header)
constexpr uint8_t PIN_STATUS_LED = 13;       // on during the high-load phases RAMP and SOAK
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // WS2813: green-red-blue order, 800 kHz

constexpr uint8_t MAX_BRIGHTNESS = 128;      // 0..255. 255 = all 100 LEDs full white (about 5-6 A). Raise it
                                             // only when the 5 V supply is rated 8 A or more and the wiring is checked.
constexpr uint8_t IDENTIFY_BRIGHTNESS = 40;
constexpr uint8_t COLOUR_BRIGHTNESS = 60;
constexpr uint8_t WALK_BRIGHTNESS = 80;

constexpr uint32_t BOOT_MS = 1500;           // dark pause at power-up (lets the sticks start)
constexpr uint32_t IDENTIFY_STICK_MS = 1200; // time one stick is lit in phase 1
constexpr uint32_t COLOUR_MS = 2500;         // time one colour is shown in phase 2
constexpr uint32_t WALK_STEP_MS = 60;        // time the walking LED stays on one LED
constexpr uint8_t WALK_PASSES = 2;           // how many times it walks through all LEDs
constexpr uint32_t RAMP_STEP_MS = 5000;      // time one brightness step is held in phase 4
constexpr uint32_t SOAK_MS = 30000;          // phase 5 length
constexpr uint32_t REST_MS = 3000;
constexpr uint32_t SOAK_REPORT_MS = 5000;    // Serial message interval during SOAK
constexpr uint8_t ESTIMATE_MA_PER_CHANNEL = 20;   // guess for the Serial estimate: mA per colour channel at full brightness
constexpr uint32_t FRAME_MS = 20;            // data refresh interval
constexpr uint16_t DEBOUNCE_MS = 40;

constexpr uint8_t RAMP_STEP_COUNT = 9;
constexpr uint8_t RAMP_STEPS[RAMP_STEP_COUNT] = {16, 32, 64, 96, 128, 160, 192, 224, 255};
// ===========================================================================

constexpr uint8_t PH_START = 0;
constexpr uint8_t PH_IDENTIFY = 1;
constexpr uint8_t PH_COLOURS = 2;
constexpr uint8_t PH_WALK = 3;
constexpr uint8_t PH_RAMP = 4;
constexpr uint8_t PH_SOAK = 5;
constexpr uint8_t PH_REST = 6;
constexpr uint8_t PH_COUNT = 7;

// One object per stick (the objects are never copied; the pointer array below walks through them).
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
Adafruit_NeoPixel *const sticks[STICK_COUNT] = {&stick1, &stick2, &stick3, &stick4, &stick5,
                                                &stick6, &stick7, &stick8, &stick9, &stick10};

uint8_t phase = PH_START;
unsigned long phaseStart = 0;
unsigned long lastFrame = 0;
int announced = -1;               // last Serial announcement inside the current phase (step, stick, ...)
unsigned int cycleCount = 0;      // finished test cycles
uint8_t buttonStable = HIGH;
uint8_t buttonRaw = HIGH;
unsigned long buttonChangedAt = 0;

uint16_t totalLeds() {
  return (uint16_t)ACTIVE_STICKS * LEDS_PER_STICK;
}

// The brightness used by ramp step i (the steps stop at MAX_BRIGHTNESS).
uint8_t rampBrightness(uint8_t step) {
  const uint8_t b = RAMP_STEPS[step];
  return b > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : b;
}

// Number of ramp steps: up to and including the first step that reaches MAX_BRIGHTNESS.
uint8_t rampLength() {
  uint8_t n = 0;
  while (n < RAMP_STEP_COUNT) {
    n++;
    if (RAMP_STEPS[n - 1] >= MAX_BRIGHTNESS) break;
  }
  return n;
}

unsigned long phaseDuration(uint8_t ph) {
  switch (ph) {
    case PH_START:    return BOOT_MS;
    case PH_IDENTIFY: return (unsigned long)ACTIVE_STICKS * IDENTIFY_STICK_MS;
    case PH_COLOURS:  return 3UL * COLOUR_MS;
    case PH_WALK:     return (unsigned long)WALK_PASSES * totalLeds() * WALK_STEP_MS;
    case PH_RAMP:     return (unsigned long)rampLength() * RAMP_STEP_MS;
    case PH_SOAK:     return SOAK_MS;
    default:          return REST_MS;
  }
}

// Rough current of all active LEDs lit white at this brightness, in mA (a guess, not a measurement).
unsigned long estimateMilliamps(uint8_t brightness) {
  return (unsigned long)totalLeds() * 3UL * ESTIMATE_MA_PER_CHANNEL * brightness / 255UL;
}

void printAmps(unsigned long milliamps) {
  Serial.print((int)(milliamps / 1000UL));
  Serial.print(F("."));
  Serial.print((int)((milliamps % 1000UL) / 100UL));
  Serial.print(F(" A"));
}

void setBrightnessAll(uint8_t brightness) {
  for (uint8_t i = 0; i < STICK_COUNT; i++) sticks[i]->setBrightness(brightness);
}

void clearAll() {
  for (uint8_t i = 0; i < STICK_COUNT; i++) sticks[i]->clear();
}

void fillAll(uint32_t color) {
  for (uint8_t i = 0; i < ACTIVE_STICKS; i++) {
    for (uint8_t k = 0; k < LEDS_PER_STICK; k++) sticks[i]->setPixelColor(k, color);
  }
}

void showAll() {
  for (uint8_t i = 0; i < STICK_COUNT; i++) sticks[i]->show();
}

void printPhaseName(uint8_t ph) {
  switch (ph) {
    case PH_START:    Serial.print(F("START (dark pause)")); break;
    case PH_IDENTIFY: Serial.print(F("1 IDENTIFY (one stick at a time)")); break;
    case PH_COLOURS:  Serial.print(F("2 COLOURS (red, green, blue)")); break;
    case PH_WALK:     Serial.print(F("3 WALK (one white LED through all LEDs)")); break;
    case PH_RAMP:     Serial.print(F("4 RAMP (all white, brightness steps up)")); break;
    case PH_SOAK:     Serial.print(F("5 SOAK (all white at MAX_BRIGHTNESS)")); break;
    default:          Serial.print(F("6 REST (dark)")); break;
  }
}

void startPhase(uint8_t next, unsigned long now) {
  if (next >= PH_COUNT) {
    cycleCount++;
    Serial.print(F("Test cycle "));
    Serial.print((int)cycleCount);
    Serial.println(F(" finished. Starting again."));
    next = PH_IDENTIFY;
  }
  phase = next;
  phaseStart = now;
  announced = -1;
  clearAll();
  showAll();
  digitalWrite(PIN_STATUS_LED, (phase == PH_RAMP || phase == PH_SOAK) ? HIGH : LOW);
  Serial.print(F("Phase "));
  printPhaseName(phase);
  Serial.println();
}

// True when the button was just pressed (debounced, active low).
bool buttonPressed(unsigned long now) {
  const uint8_t raw = digitalRead(PIN_BUTTON);
  if (raw != buttonRaw) {
    buttonRaw = raw;
    buttonChangedAt = now;
  }
  if (raw != buttonStable && now - buttonChangedAt >= DEBOUNCE_MS) {
    buttonStable = raw;
    return raw == LOW;
  }
  return false;
}

void drawIdentify(unsigned long t) {
  const uint8_t lit = (uint8_t)(t / IDENTIFY_STICK_MS);
  setBrightnessAll(IDENTIFY_BRIGHTNESS);
  clearAll();
  if (lit < ACTIVE_STICKS) {
    for (uint8_t k = 0; k < LEDS_PER_STICK; k++) sticks[lit]->setPixelColor(k, Adafruit_NeoPixel::Color(255, 255, 255));
    if (announced != lit) {
      announced = lit;
      Serial.print(F("  stick "));
      Serial.print(lit + 1);
      Serial.print(F(" (data pin "));
      Serial.print((int)STICK_PINS[lit]);
      Serial.println(F(") should be lit"));
    }
  }
}

void drawColours(unsigned long t) {
  const uint8_t c = (uint8_t)(t / COLOUR_MS);
  setBrightnessAll(COLOUR_BRIGHTNESS);
  clearAll();
  if (c == 0) fillAll(Adafruit_NeoPixel::Color(255, 0, 0));
  else if (c == 1) fillAll(Adafruit_NeoPixel::Color(0, 255, 0));
  else fillAll(Adafruit_NeoPixel::Color(0, 0, 255));
  if (announced != c) {
    announced = c;
    Serial.println(c == 0 ? F("  all sticks should be RED") : (c == 1 ? F("  all sticks should be GREEN") : F("  all sticks should be BLUE")));
  }
}

void drawWalk(unsigned long t) {
  const uint16_t pos = (uint16_t)((t / WALK_STEP_MS) % totalLeds());
  setBrightnessAll(WALK_BRIGHTNESS);
  clearAll();
  sticks[pos / LEDS_PER_STICK]->setPixelColor(pos % LEDS_PER_STICK, Adafruit_NeoPixel::Color(255, 255, 255));
}

void drawRamp(unsigned long t) {
  uint8_t step = (uint8_t)(t / RAMP_STEP_MS);
  if (step >= rampLength()) step = rampLength() - 1;
  const uint8_t brightness = rampBrightness(step);
  setBrightnessAll(brightness);
  clearAll();
  fillAll(Adafruit_NeoPixel::Color(255, 255, 255));
  if (announced != step) {
    announced = step;
    Serial.print(F("  step "));
    Serial.print(step + 1);
    Serial.print(F("/"));
    Serial.print((int)rampLength());
    Serial.print(F(": brightness "));
    Serial.print((int)brightness);
    Serial.print(F(" of 255, estimated current about "));
    printAmps(estimateMilliamps(brightness));
    Serial.println();
  }
}

void drawSoak(unsigned long t) {
  setBrightnessAll(MAX_BRIGHTNESS);
  clearAll();
  fillAll(Adafruit_NeoPixel::Color(255, 255, 255));
  const int report = (int)(t / SOAK_REPORT_MS);
  if (announced != report) {
    announced = report;
    Serial.print(F("  soak "));
    Serial.print((int)(t / 1000UL));
    Serial.print(F(" s, estimated current about "));
    printAmps(estimateMilliamps(MAX_BRIGHTNESS));
    Serial.println(F(" (measure the voltage at the last stick now)"));
  }
}

void drawPhase(unsigned long t) {
  switch (phase) {
    case PH_IDENTIFY: drawIdentify(t); break;
    case PH_COLOURS:  drawColours(t); break;
    case PH_WALK:     drawWalk(t); break;
    case PH_RAMP:     drawRamp(t); break;
    case PH_SOAK:     drawSoak(t); break;
    default:          clearAll(); break;
  }
  showAll();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (uint8_t i = 0; i < STICK_COUNT; i++) sticks[i]->begin();
  setBrightnessAll(IDENTIFY_BRIGHTNESS);
  Serial.println(F("Stick test: 10 LED sticks on a separate 5 V supply (see the top of sketch.ino)."));
  Serial.print(F("Sticks: "));
  Serial.print((int)ACTIVE_STICKS);
  Serial.print(F(", LEDs: "));
  Serial.print((int)totalLeds());
  Serial.print(F(", MAX_BRIGHTNESS "));
  Serial.print((int)MAX_BRIGHTNESS);
  Serial.print(F(", estimated current at MAX_BRIGHTNESS about "));
  printAmps(estimateMilliamps(MAX_BRIGHTNESS));
  Serial.println();
  startPhase(PH_START, millis());
}

void loop() {
  const unsigned long now = millis();
  if (buttonPressed(now)) {
    startPhase(phase + 1, now);
  } else if (now - phaseStart >= phaseDuration(phase)) {
    startPhase(phase == PH_START ? PH_IDENTIFY : phase + 1, now);
  }
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    drawPhase(now - phaseStart);
  }
}
