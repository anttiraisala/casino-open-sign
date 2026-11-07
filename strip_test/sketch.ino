/*
 * LED strip test - find out what kind of addressable strip you have
 * (made for the 12 V WS2811 strip LED-N-RGB-W2812X150-5M from partco.fi, but it works with any
 * NeoPixel-type strip: WS2811, WS2812, WS2812B, ...).
 *
 * Cut a short piece of the strip (3 to 6 pieces of 10 cm are plenty), connect it as described in
 * README.txt (data on pin 6, strip power from a 12 V supply, common GND) and run this sketch.
 * It steps through 12 settings: 6 colour orders x 2 bit speeds (800 kHz and 400 kHz). Each
 * setting runs for PHASE_MS, then the next one starts (or press the button on pin 3 to go on at once).
 *
 * What every setting shows:
 *   1. First 3 seconds, COLOUR TEST: pixel 1 should be RED, pixel 2 GREEN, pixel 3 BLUE.
 *   2. Then WALKING PIXEL: one white pixel moves along the strip, one step at a time.
 * The built-in LED on pin 13 blinks the number of the setting (1..12) at the start of every setting,
 * and the Serial Monitor (115200 baud) prints the same with the names.
 *
 * Tell what you see:
 *   - Which setting numbers show red, green, blue in the right order (that is the colour order and
 *     the speed to use). A wrong speed usually shows nothing, random colours or flicker.
 *   - When the white pixel walks: how many LEDs light up at the same time (1 or 3)? Three LEDs per
 *     step means one control chip drives three LEDs, so the pixels are 10 cm long.
 *   - Whether the last pixels of the piece work, and whether anything flickers.
 *
 * Settings are at the top. Only built-in types are used in function signatures (Arduino's
 * automatic prototypes, see the main sketch).
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t PIN_STRIP_DATA = 6;       // data line (through a 330 ohm resistor) to DIN of the strip
constexpr uint8_t PIN_BUTTON = 3;           // optional push button to GND: next setting
constexpr uint8_t PIN_STATUS_LED = 13;      // blinks the number of the setting
constexpr uint16_t TEST_PIXELS = 12;        // how many pixels to drive (more than the piece has is harmless)
constexpr uint8_t BRIGHTNESS = 60;          // 0..255, keep it low for a bench test
constexpr uint32_t PHASE_MS = 12000;        // time one setting is shown
constexpr bool AUTO_ADVANCE = true;         // false: change the setting only with the button
constexpr uint32_t COLOUR_TEST_MS = 3000;   // the first part of a setting: red, green, blue
constexpr uint16_t WALK_STEP_MS = 500;      // time the walking pixel stays on one pixel
constexpr uint16_t DEBOUNCE_MS = 40;

constexpr uint8_t SETTING_COUNT = 12;
// ===========================================================================

Adafruit_NeoPixel strip(TEST_PIXELS, PIN_STRIP_DATA, NEO_GRB + NEO_KHZ800);

uint8_t setting = 0;                 // 0..SETTING_COUNT-1
unsigned long phaseStart = 0;
uint8_t buttonStable = HIGH;
uint8_t buttonRaw = HIGH;
unsigned long buttonChangedAt = 0;

// The wire colour order of a setting (the first six settings use 800 kHz, the next six 400 kHz).
uint16_t orderFlags(uint8_t s) {
  switch (s % 6) {
    case 0:  return NEO_GRB;
    case 1:  return NEO_RGB;
    case 2:  return NEO_BRG;
    case 3:  return NEO_RBG;
    case 4:  return NEO_GBR;
    default: return NEO_BGR;
  }
}

uint16_t speedFlags(uint8_t s) {
  return s < 6 ? NEO_KHZ800 : NEO_KHZ400;
}

void printSettingName(uint8_t s) {
  switch (s % 6) {
    case 0:  Serial.print(F("GRB")); break;
    case 1:  Serial.print(F("RGB")); break;
    case 2:  Serial.print(F("BRG")); break;
    case 3:  Serial.print(F("RBG")); break;
    case 4:  Serial.print(F("GBR")); break;
    default: Serial.print(F("BGR")); break;
  }
  Serial.print(s < 6 ? F(", 800 kHz") : F(", 400 kHz"));
}

void startSetting(uint8_t s, unsigned long now) {
  setting = s % SETTING_COUNT;
  phaseStart = now;
  strip.updateType(orderFlags(setting) + speedFlags(setting));
  strip.clear();
  strip.show();
  Serial.print(F("Setting "));
  Serial.print(setting + 1);
  Serial.print(F("/"));
  Serial.print(SETTING_COUNT);
  Serial.print(F(": colour order "));
  printSettingName(setting);
  Serial.println(F("  (pixels 1-3 should be red, green, blue)"));
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

// Blinks the status LED (setting number) during the first part of every setting.
void updateStatusLed(unsigned long t) {
  const unsigned long blinkTime = (unsigned long)(setting + 1) * 400UL;
  const bool on = t < blinkTime && ((t / 200UL) % 2UL) == 0;
  digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);
}

void drawSetting(unsigned long t) {
  strip.clear();
  if (t < COLOUR_TEST_MS) {
    strip.setPixelColor(0, strip.Color(255, 0, 0));    // pixel 1: red
    strip.setPixelColor(1, strip.Color(0, 255, 0));    // pixel 2: green
    strip.setPixelColor(2, strip.Color(0, 0, 255));    // pixel 3: blue
  } else {
    const uint16_t pos = (uint16_t)(((t - COLOUR_TEST_MS) / WALK_STEP_MS) % TEST_PIXELS);
    strip.setPixelColor(pos, strip.Color(255, 255, 255));  // one white pixel walks along the strip
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  Serial.println(F("LED strip test: watch the strip and tell what you see (see the top of sketch.ino)."));
  startSetting(0, millis());
}

void loop() {
  const unsigned long now = millis();
  if (buttonPressed(now)) {
    startSetting(setting + 1, now);
  } else if (AUTO_ADVANCE && now - phaseStart >= PHASE_MS) {
    startSetting(setting + 1, now);
  }
  const unsigned long t = now - phaseStart;
  updateStatusLed(t);
  drawSetting(t);
  delay(20);
}
