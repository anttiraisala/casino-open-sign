#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "Wire.h"
#include "EEPROM.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
MockSerial Serial;
uint8_t g_pinOut[64];
TwoWire Wire;
EEPROMClass EEPROM;
std::vector<Adafruit_NeoPixel*> g_strips;
#include SKETCH_PATH
#include <cstring>
// Expected value of STICK_BALL_AT_DIN_END in the sketch under test (hard-coded here; tests/mutate.py builds a
// variant with the bit false and defines EXPECT_BALL_AT_DIN=0). The logical LED number counts from the ball.
#ifndef EXPECT_BALL_AT_DIN
#define EXPECT_BALL_AT_DIN 1
#endif
// Expected value of ARROW_POINTS_LEFT_WHEN_LOW (hard-coded; mutate.py builds a variant with false and defines
// EXPECT_LOW_IS_LEFT=0). SW_LEFT / SW_RIGHT are the pin levels that must make the arrow point left / right.
#ifndef EXPECT_LOW_IS_LEFT
#define EXPECT_LOW_IS_LEFT 1
#endif
#define SW_LEFT (EXPECT_LOW_IS_LEFT ? LOW : HIGH)
#define SW_RIGHT (EXPECT_LOW_IS_LEFT ? HIGH : LOW)
#define PHYS(ld) (EXPECT_BALL_AT_DIN ? (ld) : 9 - (ld))
#include <string>

// Names of the strips in the order they are created in the sketch (= order in g_strips).
static const char* kStripNames[] = {"frame", "letters", "closedLight", "L-CAP", "L-WING-UP", "L-WING-DN",
                                    "RAIL-UP-L", "RAIL-UP-R", "RAIL-DN-L", "RAIL-DN-R", "R-CAP", "R-WING-UP", "R-WING-DN"};
static const char* nameOf(Adafruit_NeoPixel *st) {
  for (size_t k = 0; k < g_strips.size(); k++) if (g_strips[k] == st) return kStripNames[k];
  return "?";
}
static void dump(FILE *f) {
  fprintf(f, "{\"t\":%lu,\"mode\":%d", g_millis, (int)letterMode);
  for (auto *s : g_strips) {
    fprintf(f, ",\"%s\":[", nameOf(s));
    for (size_t i = 0; i < s->shown_.size(); i++) fprintf(f, "%s\"%06x\"", i ? "," : "", s->shown_[i]);
    fprintf(f, "]");
  }
  fprintf(f, "}\n");
}
static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { if (fails < 25) printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)

// Level of letter l read back from the strip (the red channel carries the level).
static uint8_t shownLevel(Adafruit_NeoPixel *let, uint8_t l) {
  // V2: one pixel per letter. Hard-coded strip pixels (0-based) of the letters C A S I N O, O P E N: the OPEN letters
  // are wired REVERSED on the strip (strip order C A S I N O N E P O).
  static const int PIXEL_OF_LETTER[10] = {0, 1, 2, 3, 4, 5, 9, 8, 7, 6};
  return (let->shown_[PIXEL_OF_LETTER[l]] >> 16) & 0xFF;
}

static void setProgramPins(int v) {
  for (int b = 0; b < 4; b++) g_pinLevel[PIN_PROGRAM[b]] = ((v >> b) & 1) ? LOW : HIGH;   // a set bit reads LOW
}

int main(int argc, char **argv) {
  FILE *out = argc > 1 ? fopen(argv[1], "w") : nullptr;
  Serial.capture = true;                                // everything printed is kept for the sign-of-life checks
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);          // pull-ups: open switch/button read HIGH
  g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT;             // slide switch handle right -> arrow points right
  if (getenv("V2_BOOT")) {   // power-up with a saved menu image in EEPROM (the mutation test runs this mode, too)
    for (int k = 0; k < 4; k++) EEPROM.mem[k] = (uint8_t)(MENU_LAYOUT_TAG >> (8 * k));
    for (uint8_t pr = 0; pr < MENU_PROP_COUNT; pr++) { const uint8_t sl = menuRd8(PP_SLOT, pr); if (sl != MENU_NO_SLOT) EEPROM.mem[4 + sl] = (uint8_t)menuRd16(PP_DEFPOS, pr); }
    auto saveSlot = [&](uint8_t pr, int pos) { EEPROM.mem[4 + menuRd8(PP_SLOT, pr)] = (uint8_t)pos; };
    saveSlot(P_speedFrame, 20); saveSlot(P_speedArrow, 4); saveSlot(P_speedLetters, 16); saveSlot(P_arrowReverse, 1); saveSlot(P_stripMax, 40);
    saveSlot(P_lettersMax, 60); saveSlot(P_stickMax, 100); saveSlot(P_randomDelay, 5);
    const std::string bootMode = getenv("V2_BOOT");
    if (bootMode == "tag") EEPROM.mem[1] ^= 0x55;            // a saved image of another menu layout
    if (bootMode == "range") saveSlot(P_speedFrame, 255);    // one stored value outside its property's range
    setup();
    finishStartup(g_millis, true);
    for (int k = 0; k < 300; k++) { g_millis++; loop(); }
    if (bootMode == "tag" || bootMode == "range") {
      CHECK(!menuEepromValid && Serial.captured.find("EEPROM: no saved values for this menu layout, defaults used.") != std::string::npos, "boot: a wrong layout tag / a value out of range gives the defaults and says so");
      CHECK(menuPos(P_speedFrame) == 18 && menuPos(P_speedArrow) == 18 && menuPos(P_speedLetters) == 18 && menuPos(P_arrowReverse) == 0 && menuPos(P_randomDelay) == 2, "boot: after the rejected image all properties have their defaults (also the valid-looking ones)");
      if (fails) printf("RESULT: %d failures\n", fails); else printf("RESULT: all checks passed (boot mode %s)\n", bootMode.c_str());
      return fails ? 1 : 0;
    }
    CHECK(menuEepromValid && Serial.captured.find("EEPROM: saved menu values loaded.") != std::string::npos, "boot: the saved menu image is loaded");
    CHECK(menuPos(P_speedFrame) == 20 && menuPos(P_speedArrow) == 4 && menuPos(P_speedLetters) == 16, "boot: the saved speeds are loaded");
    CHECK(fabs((double)frameIntervalMs - 200.0 / (0.125 * pow(32.0, 20 / 30.0))) <= 1.0, "boot: the saved frame speed is applied");
    CHECK(fabs((double)arrowIntervalMs - 35.0 / (0.125 * pow(32.0, 4 / 30.0))) <= 2.0, "boot: the saved arrow speed is applied");
    CHECK(letterMult256 == (uint16_t)(0.125 * pow(32.0, 16 / 30.0) * 256.0 + 0.5), "boot: the saved letters speed is applied");
    CHECK(!arrowPointsRight && menuPos(P_arrow) == 1, "boot: the saved Arrow reverse flag turns the arrow (switch right -> arrow left)");
    CHECK(brightnessLut[LED_TYPE_STRIP][255] == 40 && brightnessLut[LED_TYPE_LETTERS][255] == 60 && brightnessLut[LED_TYPE_STICK][255] == 100, "boot: the saved curves are built for the three LED types");
    setProgramPins(15); for (int k = 0; k < 300; k++) { g_millis++; loop(); }
    CHECK(autoDelayMs == 300000UL, "boot: the saved Change delay (5 min) is used by random auto");
    CHECK(menuMode == MM_STATUS && menuLastInput >= 0, "boot: the status page is shown");
    printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed (boot mode)\n", fails);
    return fails ? 1 : 0;
  }
  setup();
  CHECK(startupTestActive, "the power-on LED test starts at power-up");
  // The older tests below check the colours the sketch computes, so they run with an identity curve (min 0,
  // max 255, exponent 1, mix 0). The default curves are tested separately at the end.
  CHECK(brightnessLut[0][128] == 13 && brightnessLut[1][128] == 13 && brightnessLut[2][128] == 13 && brightnessLut[0][255] == 80 && (brightnessLut[1][192] == 37 || brightnessLut[1][192] == 38) && brightnessLut[2][255] == 80, "setup() builds the default tables of the three LED types (frame, sticks, letters)");
  for (auto *s : g_strips) CHECK(s->brightness_ == 255, "no global library brightness: the curve maximum is the limit");
  CHECK(LED_TYPE_COUNT == 3 && CURVE_DEFAULT_MIN[0] == 0 && CURVE_DEFAULT_MIN[1] == 0 && CURVE_DEFAULT_MIN[2] == 0 &&
        CURVE_DEFAULT_MAX[0] == 80 && CURVE_DEFAULT_MAX[1] == 80 && CURVE_DEFAULT_MAX[2] == 80 &&
        CURVE_DEFAULT_EXPONENT[0] == 2.67f && CURVE_DEFAULT_EXPONENT[1] == 2.67f && CURVE_DEFAULT_EXPONENT[2] == 2.67f &&
        CURVE_DEFAULT_MIX[0] == 1.0f && CURVE_DEFAULT_MIX[1] == 1.0f && CURVE_DEFAULT_MIX[2] == 1.0f &&
        CURVE_DEFAULT_KEEP_ZERO[0] && CURVE_DEFAULT_KEEP_ZERO[1] && CURVE_DEFAULT_KEEP_ZERO[2], "default curve values: min 0, max 80, exponent 2.67, mix 1, keep zero dark");
  const uint8_t IDENT_TYPES[3] = {0, 1, 2};
  for (uint8_t ty : IDENT_TYPES) setBrightnessCurve(ty, 0, 255, 1.0f, 0.0f, true);
  finishStartup(g_millis, true);   // the older tests below start from the normal operation
  // Pins hard-coded here (not taken from the sketch): frame D11, letters D12, CLOSED light = 11th stick on A0 (54),
  // sticks in the order L-CAP D2, L-WING-UP D6, L-WING-DN D5, RAIL-UP-L D7, RAIL-UP-R D8, RAIL-DN-L D3, RAIL-DN-R D4,
  // R-CAP A3 (57), R-WING-UP A1 (55), R-WING-DN A2 (56) (A0 = 54 on the Mega), arrow switch D32, mode button D33.
  CHECK(FRAME_SPEED_MS == 200 && FRAME_BAR_LEDS == 2 && STARTUP_TEST_WINDOW_LEDS == 2, "frame step 200 ms, bar of 2 pixels, LED test window of 2 pixels");
  CHECK(PIN_FRAME == 11 && PIN_LETTERS == 12 && PIN_CLOSED_LIGHT == 54 && PIN_STATUS_LED == 13, "strip pins (D13 stays the status LED)");
  for (auto *s : g_strips) CHECK(s->pin_ != 31, "pin 31 is free: nothing is on D31 any more");
  CHECK(PIN_ARROW_SWITCH == 32 && PIN_SKIP_BUTTON == 33 && PIN_CASINO_CLOSED_SWITCH == 30, "input pins");
  const int wantStickPins[10] = {2, 6, 5, 7, 8, 3, 4, 57, 55, 56};
  Adafruit_NeoPixel *fr=nullptr,*let=nullptr,*clo=nullptr;
  Adafruit_NeoPixel *st[10] = {};          // the sticks in the order L-CAP, L-WING-UP, L-WING-DN, RAIL-UP-L, RAIL-UP-R,
                                           // RAIL-DN-L, RAIL-DN-R, R-CAP, R-WING-UP, R-WING-DN
  for (auto *s : g_strips) {
    if (s->pin_ == 11) fr = s; else if (s->pin_ == 12) let = s; else if (s->pin_ == 54) clo = s;
    for (int k = 0; k < 10; k++) if (s->pin_ == wantStickPins[k]) st[k] = s;
  }
  { bool allThere = fr && let && clo; for (int k = 0; k < 10; k++) allThere = allThere && st[k];
    if (!allThere) { printf("RESULT: pins do not match, stopping (1 failures)\n"); return 1; } }
  CHECK(g_strips.size() == 13 && STRIP_COUNT == 13, "13 strips: frame, letters, CLOSED light stick (A0), 10 arrow sticks (2 strips + 11 sticks)");
  CHECK(fr && let && clo, "frame, letters and CLOSED strips exist on the expected pins");
  for (int k = 0; k < 10; k++) CHECK(st[k] != nullptr && st[k]->numPixels() == 10, "every stick exists on its pin and has 10 LEDs");
  CHECK(clo && clo->numPixels() == 10 && CLOSED_LIGHT_LEDS == 10, "closed light is a 10-LED Grove stick");
  CHECK(fr->numPixels() == 18 && FRAME_PIXELS == 18, "the frame is one strip of 18 pixels");
  CHECK(let->numPixels() == 10 && LETTER_COUNT == 10, "the letters are one strip of 10 pixels (one per letter)");
  CHECK(arrowPointsRight, "arrow starts pointing right with the switch released (HIGH)");
  CHECK(g_analogPin == 69, "the random seed is read from the free pin A15 (A0 carries the CLOSED stick)");

  // ---- Frame patterns (one per program): expected levels are hard-coded (made by an independent reference
  // implementation, /tmp-free: see tests/frame_pattern_reference.py), 2 hex digits per pixel, pixels 0..17 ----
  {
    const int NSTEPS = 24;
    struct FrameExpect { int mode; bool right; const char *rows[24]; };
    static const int STEPS[24] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 12, 13, 14, 15, 17, 20, 25, 29, 30, 33, 35, 36, 38, 39};
    static const FrameExpect EXPECT[] = {
{0, true, {"ffff0000ffffff0000ffff0000ffffff0000", "00ffff0000ff0000ffff0000ffff00ffff00", "0000ffff000000ffff0000ffff000000ffff", "ff0000ffff00ffff0000ffff0000ff0000ff", "ffff0000ffffff0000ffff0000ffffff0000", "00ffff0000ff0000ffff0000ffff00ffff00", "0000ffff000000ffff0000ffff000000ffff", "ff0000ffff00ffff0000ffff0000ff0000ff", "00ffff0000ff0000ffff0000ffff00ffff00", "0000ffff000000ffff0000ffff000000ffff", "ffff0000ffffff0000ffff0000ffffff0000", "00ffff0000ff0000ffff0000ffff00ffff00", "0000ffff000000ffff0000ffff000000ffff", "ff0000ffff00ffff0000ffff0000ff0000ff", "00ffff0000ff0000ffff0000ffff00ffff00", "ffff0000ffffff0000ffff0000ffffff0000", "00ffff0000ff0000ffff0000ffff00ffff00", "00ffff0000ff0000ffff0000ffff00ffff00", "0000ffff000000ffff0000ffff000000ffff", "00ffff0000ff0000ffff0000ffff00ffff00", "ff0000ffff00ffff0000ffff0000ff0000ff", "ffff0000ffffff0000ffff0000ffffff0000", "0000ffff000000ffff0000ffff000000ffff", "ff0000ffff00ffff0000ffff0000ff0000ff"}},
  {0, false, {"ffff0000ffff0000ffffff0000ffff0000ff", "ff0000ffff0000ffff00ffff0000ffff0000", "0000ffff0000ffff000000ffff0000ffff00", "00ffff0000ffff0000ff0000ffff0000ffff", "ffff0000ffff0000ffffff0000ffff0000ff", "ff0000ffff0000ffff00ffff0000ffff0000", "0000ffff0000ffff000000ffff0000ffff00", "00ffff0000ffff0000ff0000ffff0000ffff", "ff0000ffff0000ffff00ffff0000ffff0000", "0000ffff0000ffff000000ffff0000ffff00", "ffff0000ffff0000ffffff0000ffff0000ff", "ff0000ffff0000ffff00ffff0000ffff0000", "0000ffff0000ffff000000ffff0000ffff00", "00ffff0000ffff0000ff0000ffff0000ffff", "ff0000ffff0000ffff00ffff0000ffff0000", "ffff0000ffff0000ffffff0000ffff0000ff", "ff0000ffff0000ffff00ffff0000ffff0000", "ff0000ffff0000ffff00ffff0000ffff0000", "0000ffff0000ffff000000ffff0000ffff00", "ff0000ffff0000ffff00ffff0000ffff0000", "00ffff0000ffff0000ff0000ffff0000ffff", "ffff0000ffff0000ffffff0000ffff0000ff", "0000ffff0000ffff000000ffff0000ffff00", "00ffff0000ffff0000ff0000ffff0000ffff"}},
  {1, true, {"3700000000000000000000000000ffcd9b69", "00000000000000000000000000ffcd9b6937", "000000000000000000000000ffcd9b693700", "0000000000000000000000ffcd9b69370000", "00000000000000000000ffcd9b6937000000", "000000000000000000ffcd9b693700000000", "0000000000000000ffcd9b69370000000000", "00000000000000ffcd9b6937000000000000", "0000000000ffcd9b69370000000000000000", "00000000ffcd9b6937000000000000000000", "0000ffcd9b69370000000000000000000000", "00ffcd9b6937000000000000000000000000", "ffcd9b693700000000000000000000000000", "cd9b693700000000000000000000000000ff", "693700000000000000000000000000ffcd9b", "000000000000000000000000ffcd9b693700", "00000000000000ffcd9b6937000000000000", "000000ffcd9b693700000000000000000000", "0000ffcd9b69370000000000000000000000", "cd9b693700000000000000000000000000ff", "693700000000000000000000000000ffcd9b", "3700000000000000000000000000ffcd9b69", "000000000000000000000000ffcd9b693700", "0000000000000000000000ffcd9b69370000"}},
  {1, false, {"000000000037699bcdff0000000000000000", "00000000000037699bcdff00000000000000", "0000000000000037699bcdff000000000000", "000000000000000037699bcdff0000000000", "00000000000000000037699bcdff00000000", "0000000000000000000037699bcdff000000", "000000000000000000000037699bcdff0000", "00000000000000000000000037699bcdff00", "ff0000000000000000000000000037699bcd", "cdff0000000000000000000000000037699b", "699bcdff0000000000000000000000000037", "37699bcdff00000000000000000000000000", "0037699bcdff000000000000000000000000", "000037699bcdff0000000000000000000000", "0000000037699bcdff000000000000000000", "0000000000000037699bcdff000000000000", "00000000000000000000000037699bcdff00", "9bcdff000000000000000000000000003769", "699bcdff0000000000000000000000000037", "000037699bcdff0000000000000000000000", "0000000037699bcdff000000000000000000", "000000000037699bcdff0000000000000000", "0000000000000037699bcdff000000000000", "000000000000000037699bcdff0000000000"}},
  {2, true, {"0000000000000000000000000000ff000000", "00000000000000000000000000ffffff0000", "000000000000000000000000ffffffffff00", "0000000000000000000000ffffffffffffff", "ff000000000000000000ffffffffffffffff", "ffff00000000000000ffffffffffffffffff", "ffffff0000000000ffffffffffffffffffff", "ffffffff000000ffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "00000000000000000000000000ffffff0000", "ff000000000000000000ffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "00000000000000000000000000ffffff0000", "0000000000000000000000ffffffffffffff", "ff000000000000000000ffffffffffffffff", "ffffff0000000000ffffffffffffffffffff", "ffffffff000000ffffffffffffffffffffff"}},
  {2, false, {"000000000000000000ff0000000000000000", "0000000000000000ffffff00000000000000", "00000000000000ffffffffff000000000000", "000000000000ffffffffffffff0000000000", "0000000000ffffffffffffffffff00000000", "00000000ffffffffffffffffffffff000000", "000000ffffffffffffffffffffffffff0000", "0000ffffffffffffffffffffffffffffff00", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "0000000000000000ffffff00000000000000", "0000000000ffffffffffffffffff00000000", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "0000000000000000ffffff00000000000000", "000000000000ffffffffffffff0000000000", "0000000000ffffffffffffffffff00000000", "000000ffffffffffffffffffffffffff0000", "0000ffffffffffffffffffffffffffffff00"}},
  {4, true, {"c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414"}},
  {4, false, {"c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8ffffffc8c8c8c8c8c8ffffff", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414", "c8c8c8c8c8c8141414c8c8c8c8c8c8141414"}},
  {5, true, {"00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00"}},
  {5, false, {"00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "00ff00ff00ff00ff00ff00ff00ff00ff00ff", "ff00ff00ff00ff00ff00ff00ff00ff00ff00", "ff00ff00ff00ff00ff00ff00ff00ff00ff00"}},
  {6, true, {"0000000000000000000000000000ff000000", "00000000000000000000000000ffb9000000", "000000000000000000000000ffb973000000", "0000000000000000000000ffb9732d000000", "00000000000000000000ffb9732d00000000", "000000000000000000ffb9732d0000000000", "0000000000000000ffb9732d000000000000", "00000000000000ffb9732d00000000000000", "0000000000ffb9732d000000000000000000", "00000000ffb9732d00000000000000000000", "0000ffb9732d000000000000000000000000", "00ffb9732d00000000000000000000000000", "ffb9732d0000000000000000000000000000", "b9732d0000000000000000000000000000ff", "2d0000000000000000000000000000ffb973", "00000000000000000000000000000073b9ff", "002d73b9ff00000000000000000000000000", "00000000002d73b9ff000000000000000000", "0000000000002d73b9ff0000000000000000", "0000000000000000002d73b9ff0000000000", "00000000000000000000002d73b9ff000000", "0000000000000000000000000000ff000000", "000000000000000000000000ffb973000000", "0000000000000000000000ffb9732d000000"}},
  {6, false, {"000000000000000000ff0000000000000000", "000000000000000000b9ff00000000000000", "00000000000000000073b9ff000000000000", "0000000000000000002d73b9ff0000000000", "000000000000000000002d73b9ff00000000", "00000000000000000000002d73b9ff000000", "0000000000000000000000002d73b9ff0000", "000000000000000000000000002d73b9ff00", "ff00000000000000000000000000002d73b9", "b9ff00000000000000000000000000002d73", "2d73b9ff0000000000000000000000000000", "002d73b9ff00000000000000000000000000", "00002d73b9ff000000000000000000000000", "0000002d73b9ff0000000000000000000000", "00000000002d73b9ff000000000000000000", "000000000000ffb973000000000000000000", "00ffb9732d00000000000000000000000000", "2d0000000000000000000000000000ffb973", "0000000000000000000000000000ffb9732d", "0000000000000000000000ffb9732d000000", "000000000000000000ffb9732d0000000000", "000000000000000000ff0000000000000000", "00000000000000000073b9ff000000000000", "0000000000000000002d73b9ff0000000000"}},
  {8, true, {"0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff"}},
  {8, false, {"ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "ff0000ff0000ff0000ff0000ff0000ff0000", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "0000ff0000ff0000ff0000ff0000ff0000ff", "00ff0000ff0000ff0000ff0000ff0000ff00", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000", "0000ff0000ff0000ff0000ff0000ff0000ff", "ff0000ff0000ff0000ff0000ff0000ff0000"}},
  {9, true, {"ffffffffffffffffffffffffffffffffffff", "787878787878787878787878787878787878", "c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414"}},
  {9, false, {"ffffffffffffffffffffffffffffffffffff", "787878787878787878787878787878787878", "c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "ffffffffffffffffffffffffffffffffffff", "3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414", "141414141414141414141414141414141414"}},
  {10, true, {"ff00000000ff000000ff00000000ff000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000"}},
  {10, false, {"ff00000000ff000000ff00000000ff000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "ffff0000ffffff00ffffff0000ffffff00ff", "ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffff", "000000000000000000000000000000000000", "000000000000000000000000000000000000"}},
  {11, true, {"ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff"}},
  {11, false, {"ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "ffffffffffff000000ffffffffffff000000", "ffffffffffff000000ffffffffffff000000", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff", "000000000000ffffff000000000000ffffff"}},
  {13, true, {"0000000000000000000000005fafffaf5f00", "00000000000000000000000000ffafff0000", "000000000000000000000000ffaf5fafff00", "0000000000000000000000ffaf5f005fafff", "ff000000000000000000ffaf5f0000005faf", "afff00000000000000ffaf5f00000000005f", "5fafff0000000000ffaf5f00000000000000", "005fafff000000ffaf5f0000000000000000", "0000005fafffaf5f00000000000000000000", "00000000ffafff0000000000000000000000", "0000ffaf5f005fafff000000000000000000", "00ffaf5f0000005fafff0000000000000000", "ffaf5f00000000005fafff00000000000000", "af5f000000000000005fafff0000000000ff", "00000000000000000000005fafff00ffaf5f", "000000000000000000000000ffaf5fafff00", "005fafff000000ffaf5f0000000000000000", "000000ffaf5fafff00000000000000000000", "0000ffaf5f005fafff000000000000000000", "af5f000000000000005fafff0000000000ff", "00000000000000000000005fafff00ffaf5f", "0000000000000000000000005fafffaf5f00", "000000000000000000000000ffaf5fafff00", "0000000000000000000000ffaf5f005fafff"}},
  {13, false, {"000000000000005fafffaf5f000000000000", "0000000000000000ffafff00000000000000", "00000000000000ffaf5fafff000000000000", "000000000000ffaf5f005fafff0000000000", "0000000000ffaf5f0000005fafff00000000", "00000000ffaf5f00000000005fafff000000", "000000ffaf5f000000000000005fafff0000", "0000ffaf5f0000000000000000005fafff00", "ffaf5f000000000000000000000000005faf", "afff000000000000000000000000000000ff", "005fafff0000000000000000000000ffaf5f", "00005fafff000000000000000000ffaf5f00", "0000005fafff00000000000000ffaf5f0000", "000000005fafff0000000000ffaf5f000000", "0000000000005fafff00ffaf5f0000000000", "00000000000000ffaf5fafff000000000000", "0000ffaf5f0000000000000000005fafff00", "5fafff00000000000000000000000000ffaf", "005fafff0000000000000000000000ffaf5f", "000000005fafff0000000000ffaf5f000000", "0000000000005fafff00ffaf5f0000000000", "000000000000005fafffaf5f000000000000", "00000000000000ffaf5fafff000000000000", "000000000000ffaf5f005fafff0000000000"}},
  {14, true, {"ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00"}},
  {14, false, {"00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "ffff00ffff00ffff00ffff00ffff00ffff00", "ff00ffff00ffff00ffff00ffff00ffff00ff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff", "ffff00ffff00ffff00ffff00ffff00ffff00", "00ffff00ffff00ffff00ffff00ffff00ffff"}},
    };
    for (const FrameExpect &e : EXPECT) {
      for (int k = 0; k < NSTEPS; k++) {
        std::string got;
        char b[4];
        for (uint16_t i = 0; i < 18; i++) { snprintf(b, sizeof b, "%02x", frameLevel(i, e.right, (uint8_t)e.mode, (uint16_t)STEPS[k])); got += b; }
        if (got != e.rows[k]) { printf("FAIL: frame program %d arrow %s step %d: got %s expected %s\n", e.mode, e.right ? "right" : "left", STEPS[k], got.c_str(), e.rows[k]); fails++; }
      }
    }
    // frame program 3: steady until step 29, dark from step 30
    for (int right = 0; right < 2; right++) for (uint16_t i = 0; i < 18; i++) {
      CHECK(frameLevel(i, right, 3, 29) == 230 && frameLevel(i, right, 3, 30) == 0, "frame program 3: steady until step 29, dark from step 30");
    }
    // frame program 12: pixel i locks at step 6 + i (spinning before, 255 from then on until step 36)
    { bool sawDarkSpin = false;
      for (uint16_t i = 0; i < 18; i++) { CHECK(frameLevel(i, true, 12, (uint16_t)(6 + i)) == 255, "frame program 12: pixel i is locked at step 6 + i");
        if (frameLevel(i, true, 12, (uint16_t)(5 + i)) == 0) sawDarkSpin = true; }
      CHECK(sawDarkSpin, "frame program 12: a pixel is still spinning at step 5 + i"); }
    // the step counter wraps at 3600 and restarts when the program changes
    { frameStep = 3599; updateFrame(); CHECK(frameStep == 0, "frame step wraps at 3600");
      frameStep = 7; updateFrame(); CHECK(frameStep == 8, "frame step counts the frame updates");
      frameStep = 7; setLetterMode(5, g_millis); CHECK(frameStep == 0, "the frame step restarts when the program changes");
      frameStep = 7; pickAutoMode(g_millis); CHECK(frameStep == 0, "the frame step restarts when random auto picks a program");
      setLetterMode(0, g_millis); }
    // random-looking programs: check what is certain
    for (int right = 0; right < 2; right++) {
      for (uint16_t i = 0; i < 18; i++) {
        CHECK(frameLevel(i, right, 3, 25) == 230 && frameLevel(i, right, 3, 33) == 0, "frame program 3: all steady at step 25, dark at step 33");
        CHECK(frameLevel(i, right, 12, 30) == 255 && frameLevel(i, right, 12, 38) == 0, "frame program 12: all locked at step 30, dark at step 38");
        for (int t = 0; t < 200; t++) { uint8_t v = frameLevel(i, right, 7, (uint16_t)t); CHECK(v == 255 || v == 30, "frame program 7: twinkle levels 255 or 30"); }
        for (int t = 0; t < 40; t++) { uint8_t v = frameLevel(i, right, 12, (uint16_t)t); CHECK(v == 255 || v == 0, "frame program 12: spinning pixels are on or off"); }
      }
    }
    // the programs that follow the arrow differ between the two directions; the others do not
    const int FOLLOW[] = {0, 1, 2, 6, 8, 13, 14};
    for (int m = 0; m < 15; m++) {
      bool differs = false;
      for (int t = 0; t < 36 && !differs; t++) for (uint16_t i = 0; i < 18; i++) if (m != 3 && m != 7 && m != 12 && frameLevel(i, true, m, t) != frameLevel(i, false, m, t)) differs = true;
      bool follows = false; for (int f : FOLLOW) if (f == m) follows = true;
      if (m != 3 && m != 7 && m != 12) CHECK(differs == follows, "frame programs 0, 1, 2, 6, 8, 13, 14 follow the arrow direction, the others do not");
    }
  }

  // ---- The five new letter programs (10-14): expected levels hard-coded from the descriptions ----
  {
    const uint8_t saved = activeMode;
    auto lv = [&](int mode, unsigned long t, int letter) { activeMode = (uint8_t)mode; return (int)baseLetterLevel((uint8_t)letter, t); };
    // 10 converge: step = t / 400 mod 8; each word lights from its ends: lit (230) when the distance from the nearer end <= step
    { const int e0[10] = {230, 25, 25, 25, 25, 230, 230, 25, 25, 230};            // step 0: CASINO ends C and O, OPEN ends O and N
      const int e1[10] = {230, 230, 25, 25, 230, 230, 230, 230, 230, 230};        // step 1
      for (int l = 0; l < 10; l++) { CHECK(lv(10, 0, l) == e0[l] && lv(10, 399, l) == e0[l], "converge step 0");
        CHECK(lv(10, 400, l) == e1[l], "converge step 1"); CHECK(lv(10, 800, l) == 230 && lv(10, 2000, l) == 230, "converge: all lit from step 2 to step 5");
        CHECK(lv(10, 2400, l) == 0 && lv(10, 2800, l) == 0 && lv(10, 3199, l) == 0 && lv(10, 3200, l) == e0[l], "converge: dark in steps 6 and 7"); } }
    // 11 words swap: the first word follows the sine table, the second word is half a cycle behind
    { for (int l = 0; l < 6; l++) { CHECK(lv(11, 0, l) == lv(11, 0, 0) && lv(11, 1200, l) == lv(11, 0, 6), "words swap: the first word at half a cycle equals the second word at 0"); }
      for (int l = 6; l < 10; l++) { CHECK(lv(11, 0, l) == lv(11, 0, 6) && lv(11, 1200, l) == lv(11, 0, 0), "words swap: the second word at half a cycle equals the first word at 0"); }
      CHECK(lv(11, 300, 0) != lv(11, 300, 6), "words swap: the words differ at the same moment");
      CHECK(lv(11, 0, 0) >= 40 && lv(11, 0, 0) <= 255 && lv(11, 600, 0) >= 40, "words swap: never darker than 40"); }
    // 12 slot machine: letter k locks at 1500 + 300 k ms; locked = 230 until 7200 ms, dark until 8000 ms, then again
    { for (int l = 0; l < 10; l++) { int v = lv(12, 0, l); CHECK(v == 255 || v == 30, "slot: spinning letters are 255 or 30");
        { int w = lv(12, 4199, 9); CHECK(w == 255 || w == 30, "slot: the last letter still spins at 4199 ms"); }
        CHECK(lv(12, 4200, l) == 230 && lv(12, 7199, l) == 230 && lv(12, 7200, l) == 0 && lv(12, 7999, l) == 0, "slot: all locked at 4200, dark from 7200");
        CHECK(lv(12, 8000 + 4200, l) == 230, "slot: the cycle repeats every 8000 ms"); }
      for (int l = 0; l < 4; l++) CHECK(lv(12, 2401, l) == 230, "slot: letters 0-3 are locked at 2401 ms");
      for (int l = 4; l < 10; l++) { int v = lv(12, 2401, l); CHECK(v == 255 || v == 30, "slot: letters 4-9 still spin at 2401 ms"); }
      bool sawOn = false, sawOff = false; for (int t = 0; t < 1500; t += 80) { int v = lv(12, t, 9); if (v == 255) sawOn = true; if (v == 30) sawOff = true; }
      CHECK(sawOn && sawOff, "slot: a spinning letter changes between 255 and 30"); }
    // 13 twin chase: dim (30) letters step and 9 - step, step = t / 250 mod 10
    { for (int step = 0; step < 10; step++) for (int l = 0; l < 10; l++) {
        const bool dim = (l == step || l == 9 - step);
        CHECK(lv(13, (unsigned long)step * 250 + 10, l) == (dim ? 30 : 230), "twin chase: the two dim letters run in opposite directions"); }
      CHECK(lv(13, 2500, 0) == 30 && lv(13, 2500, 9) == 30 && lv(13, 2500, 4) == 230, "twin chase: the cycle repeats after 10 steps"); }
    // 14 marquee: a letter is dark (40) when (letter + step) mod 4 == 0, step = t / 250 mod 4
    { for (int step = 0; step < 4; step++) for (int l = 0; l < 10; l++)
        CHECK(lv(14, (unsigned long)step * 250 + 5, l) == (((l + step) % 4 == 0) ? 40 : 230), "marquee: one letter in four is dark and the dark ones move");
      CHECK(lv(14, 1000, 0) == 40 && lv(14, 1000, 1) == 230, "marquee: repeats every 4 steps"); }
    activeMode = saved;
  }

  // Timeline: program k is active from MODE_MS*k. The program switch is turned to k (with contact bounce) just before.
  const unsigned long MODE_MS = 14000, END = MODE_MS * LETTER_MODE_COUNT + 2000;
  // Arrow switch: LOW (left) at 5 s, HIGH (right) at 25 s, short glitch at 45 s, bouncing change at 60 s.
  long lastFrameShows = fr->shows, lastArrowShows = 0; long arrowFlowChecks = 0;
  std::vector<int> prevPath; bool prevMirrored = false, sawMirrored = false, sawNormal = false;
  long modeChanges = 0; int lastMode = letterMode;
  bool minMax[LETTER_MODE_COUNT][4] = {};   // per mode: sawAllOff, sawAllLit, sawSpread, sawWordBlink
  unsigned long lastDump = 0; int modeSeenHigh2 = 0;
  uint8_t secondWordLo = 255, secondWordHi = 0, firstWordLo = 255; // for MODE_WORD_BLINK
  uint8_t breatheMin = 255; bool altDiffer = false, sawFlicker = false;
  bool glitchIgnored = true, bounceHandled = false;
  for (g_millis = 0; g_millis <= END; g_millis++) {
    // ---- stimulus ----
    for (int k = 1; k < LETTER_MODE_COUNT; k++) {
      unsigned long t0 = MODE_MS * k - 200;                // press at t0, with bounce, held 120 ms
      if (g_millis >= t0 && g_millis < t0 + 120) setProgramPins((g_millis < t0 + 8 && (g_millis & 1)) ? (k ^ 8) : k);   // bounce, then settle
    }
    if (g_millis == 5000) g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT;
    if (g_millis == 25000) g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT;
    if (g_millis == 45000) g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT;    // 10 ms glitch: must be ignored
    if (g_millis == 45010) g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT;
    if (g_millis >= 60000 && g_millis < 60012) g_pinLevel[PIN_ARROW_SWITCH] = (g_millis & 1) ? HIGH : LOW;  // bounce
    if (g_millis == 60012) g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT;     // settles LOW -> left

    loop();

    // ---- checks ----
    if (g_millis > 1 && letterMode != lastMode) { modeChanges++; lastMode = letterMode; }
    if (fr->shows != lastFrameShows) {
      lastFrameShows = fr->shows;
      // Rebuild the two light paths (start corner -> end corner) from the ONE frame strip. Strip pixels (0-based, seen from
      // the front): 0 = lower-left corner, 0-5 bottom going right, 5 = lower-right, 6-8 right edge going up, 9 = upper-right,
      // 9-14 top going left, 14 = upper-left, 15-17 left edge going down. All indices are hard-coded here.
      const bool mir = !arrowPointsRight;
      std::vector<int> A, B;
      if (activeMode == MODE_BREATHE) {   // the bar checks below are for program 0 only (the others are tested above)
      auto lit = [&](size_t i) { return (int)(fr->shown_[i] != 0); };
      for (auto c : fr->shown_) CHECK(c == 0 || c == 0xffffff, "frame colours black/white only");
      static const int RIGHT_CW[10] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5};     // upper-left, along the top, down the right edge
      static const int RIGHT_CCW[10] = {14, 15, 16, 17, 0, 1, 2, 3, 4, 5};     // upper-left, down the left edge, along the bottom
      static const int LEFT_CW[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};           // upper-right, down the right edge, along the bottom
      static const int LEFT_CCW[10] = {9, 10, 11, 12, 13, 14, 15, 16, 17, 0};  // upper-right, along the top, down the left edge
      for (int k = 0; k < 10; k++) {
        A.push_back(lit(mir ? LEFT_CW[k] : RIGHT_CW[k]));
        B.push_back(lit(mir ? LEFT_CCW[k] : RIGHT_CCW[k]));
      }
      CHECK(A == B, "both frame light paths (clockwise and counter-clockwise) show the same pattern");
      size_t i = 0;
      while (i < A.size()) { size_t j = i; while (j < A.size() && A[j] == A[i]) j++;
        if (i > 0 && j < A.size()) CHECK(j - i == 2, "frame run length is 2 pixels");
        i = j; }
      if (!prevPath.empty() && prevMirrored == mir) {
        bool shifted = true;
        for (size_t k = 1; k < A.size(); k++) if (A[k] != prevPath[k-1]) shifted = false;
        CHECK(shifted, "frame pattern advances one pixel per step toward the end corner");
      }
      prevPath = A; prevMirrored = mir;
      sawMirrored = sawMirrored || mir; sawNormal = sawNormal || !mir;
      } else prevPath.clear();
    }
    // Letters: one pixel per letter, green channel follows the deep-red colour rule (green = red / 12).
    {
      for (size_t p = 0; p < let->numPixels(); p++) {
        const uint32_t c = let->shown_[p];
        CHECK(((c >> 8) & 0xFF) == ((c >> 16) & 0xFF) / 12 && (c & 0xFF) == 0, "letter pixel colour: red with a little green, no blue");
      }
      if (flickerLetter != NO_LETTER) sawFlicker = true;
    }
    if (g_millis > 1) {
      // Per-mode behaviour (skip the flicker letter, which is deliberately overridden).
      const unsigned long tm = g_millis - letterModeStart;
      if (tm > 300) {
        const int m = letterMode;
        int lit = 0, off = 0; uint8_t lo = 255, hi = 0;
        for (uint8_t l = 0; l < LETTER_COUNT; l++) { if (l == flickerLetter) continue;
          uint8_t v = shownLevel(let, l); if (v > 150) lit++; if (v < 8) off++; if (v < lo) lo = v; if (v > hi) hi = v; }
        if (off >= LETTER_COUNT - 1) minMax[m][0] = true;
        if (lit >= LETTER_COUNT - 1) minMax[m][1] = true;
        if (hi - lo > 150) minMax[m][2] = true;
        if (m == MODE_WORD_BLINK) {
          uint8_t a = shownLevel(let, FIRST_WORD_LETTERS), b = shownLevel(let, 0);
          if (flickerLetter != FIRST_WORD_LETTERS) { if (a < secondWordLo) secondWordLo = a; if (a > secondWordHi) secondWordHi = a; }
          if (flickerLetter != 0 && b < firstWordLo) firstWordLo = b;
        }
        if (m == MODE_BREATHE) for (uint8_t l = 0; l < LETTER_COUNT; l++) if (l != flickerLetter && shownLevel(let,l) < breatheMin) breatheMin = shownLevel(let,l);
        if (m == MODE_ALTERNATE && flickerLetter != 0 && flickerLetter != 1 && abs((int)shownLevel(let,0) - (int)shownLevel(let,1)) > 100) altDiffer = true;
      }
    }
    // Casino open: the CLOSED light stays dark.
    for (auto c : clo->shown_) CHECK(c == 0, "CLOSED light is off while the casino is open");
    // Arrow: which LEDs are lit follows the switch (hard-coded lists; stick numbers: L-CAP 0, L-WING-UP 1, L-WING-DN 2,
    // RAIL-UP-L 3, RAIL-UP-R 4, RAIL-DN-L 5, RAIL-DN-R 6, R-CAP 7, R-WING-UP 8, R-WING-DN 9).
    //   points right: tail cap L-CAP lit on LEDs 2..7 only; head cap R-CAP lit on LEDs 0,1,2,7,8,9 only;
    //                 rails and R-WING-UP/DN fully lit; L-WING-UP/DN dark
    //   points left:  tail cap R-CAP lit on LEDs 2..7 only; head cap L-CAP lit on LEDs 0,1,2,7,8,9 only;
    //                 rails and L-WING-UP/DN fully lit; R-WING-UP/DN dark
    auto expectDir = [&](unsigned long from, unsigned long to, bool right, const char *msg) {
      if (g_millis > from && g_millis < to) {
        const bool tailLit[10] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 0}, headLit[10] = {1, 1, 1, 0, 0, 0, 0, 1, 1, 1};
        const int tailCap = right ? 0 : 7, headCap = right ? 7 : 0;
        for (int i = 0; i < 10; i++) {
          CHECK((st[tailCap]->shown_[i] != 0) == (tailLit[i] != 0), msg);
          CHECK((st[headCap]->shown_[i] != 0) == (headLit[i] != 0), msg);
        }
        const int rails[4] = {3, 4, 5, 6};
        for (int i = 0; i < 4; i++) for (auto c : st[rails[i]]->shown_) CHECK(c != 0, msg);
        for (int w : {right ? 8 : 1, right ? 9 : 2}) for (auto c : st[w]->shown_) CHECK(c != 0, msg);
        for (int w : {right ? 1 : 8, right ? 2 : 9}) for (auto c : st[w]->shown_) CHECK(c == 0, msg);
      } };
    expectDir(1000, 4900, true, "before the switch moves: right head, tail cap L-CAP middle 6, head cap R-CAP ends 3+3");
    expectDir(5200, 24900, false, "switch LOW: left head, tail cap R-CAP middle 6, head cap L-CAP ends 3+3");
    expectDir(25200, 44900, true, "switch HIGH: right head, tail cap L-CAP middle 6, head cap R-CAP ends 3+3");
    expectDir(45000, 59900, true, "10 ms glitch ignored: still the right head");   // 45000..: glitch
    expectDir(60200, END, false, "bouncing switch settles LOW: left head");

    // Flow check with the white pulse head: at every arrow update exactly one LED is pure white, the LED whose flow
    // coordinate equals the pulse position. The upper route's pulse position is tick % 72, the lower route's is
    // (tick + 36) % 72; a pulse head exists on a route only while its position is 0..35. The map flow coordinate ->
    // (stick, LED) is hard-coded here (36 LEDs per route):
    //   points right: upper s0-2 L-CAP LEDs 4,3,2; s3-12 RAIL-UP-L 0..9; s13-22 RAIL-UP-R 9..0; s23-25 R-CAP LEDs 2,1,0;
    //                 s26-35 R-WING-UP 9..0
    //                 lower s0-2 L-CAP LEDs 5,6,7; s3-12 RAIL-DN-L 0..9; s13-22 RAIL-DN-R 9..0; s23-25 R-CAP LEDs 7,8,9;
    //                 s26-35 R-WING-DN 9..0
    //   points left:  upper s0-2 R-CAP LEDs 4,3,2; s3-12 RAIL-UP-R 0..9; s13-22 RAIL-UP-L 9..0; s23-25 L-CAP LEDs 2,1,0;
    //                 s26-35 L-WING-UP 9..0
    //                 lower s0-2 R-CAP LEDs 5,6,7; s3-12 RAIL-DN-R 0..9; s13-22 RAIL-DN-L 9..0; s23-25 L-CAP LEDs 7,8,9;
    //                 s26-35 L-WING-DN 9..0
    CHECK(ARROW_POINTS_LEFT_WHEN_LOW == (EXPECT_LOW_IS_LEFT != 0), "ARROW_POINTS_LEFT_WHEN_LOW has the expected value");
    CHECK(STICK_BALL_AT_DIN_END == (EXPECT_BALL_AT_DIN != 0), "STICK_BALL_AT_DIN_END has the expected value");
    if (st[3]->shows != lastArrowShows && g_millis > 1000) {
      lastArrowShows = st[3]->shows;
      const bool right = arrowPointsRight;
      const int tickDrawn = (int)arrowTick - 1;
      auto where = [&](bool lower, int s, int &stick, int &led) {
        if (s < 3) { stick = right ? 0 : 7; led = lower ? 5 + s : 4 - s; }
        else if (s < 13) { stick = lower ? (right ? 5 : 6) : (right ? 3 : 4); led = s - 3; }
        else if (s < 23) { stick = lower ? (right ? 6 : 5) : (right ? 4 : 3); led = 22 - s; }
        else if (s < 26) { stick = right ? 7 : 0; led = lower ? s - 16 : 25 - s; }
        else { stick = lower ? (right ? 9 : 2) : (right ? 8 : 1); led = 35 - s; }
      };
      int want = 0; bool found[2] = {false, false};
      for (int lane = 0; lane < 2; lane++) {
        const int pulse = (tickDrawn + (lane ? 36 : 0)) % 72;
        if (pulse <= 35) { int sk, ld; where(lane == 1, pulse, sk, ld); want++;
          found[lane] = (st[sk]->shown_[PHYS(ld)] & 0xFFFFFF) == 0xFFFFFF; }
      }
      int whites = 0;
      for (int k = 0; k < 10; k++) for (auto c : st[k]->shown_) if ((c & 0xFFFFFF) == 0xFFFFFF) whites++;
      CHECK(want == 1 && whites == 1, "arrow: exactly one white pulse head on the arrow");
      for (int lane = 0; lane < 2; lane++) if ((tickDrawn + (lane ? 36 : 0)) % 72 <= 35) CHECK(found[lane], "arrow: the white pulse head is on the LED with the right flow coordinate");
      arrowFlowChecks++;
    }
    if (out && g_millis - lastDump >= 250) { dump(out); lastDump = g_millis; }
  }
  // ---- Speeds: menu properties Speed > Frame, Letters, Arrow (x0.125 .. x4 in 30 log steps, default x1 = position 18) ----
  auto run = [&](unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } };
  CHECK(frameIntervalMs == 200 && arrowIntervalMs == 35 && letterMult256 == 256, "default speeds: x1 for the frame, the arrow and the letters");
  CHECK(menuPos(P_speedFrame) == 18 && menuPos(P_speedArrow) == 18 && menuPos(P_speedLetters) == 18, "the speed properties start at position 18 (x1.000)");
  const int speedPos[][3] = {{0, 30, 18}, {30, 0, 4}, {8, 24, 30}, {18, 18, 0}};   // frame, letters, arrow positions
  for (auto &pr : speedPos) {
    menuSetPos(P_speedFrame, pr[0]); menuSetPos(P_speedLetters, pr[1]); menuSetPos(P_speedArrow, pr[2]);
    run(300);
    const double mFrame = 0.125 * pow(32.0, pr[0] / 30.0), mLet = 0.125 * pow(32.0, pr[1] / 30.0), mArrow = 0.125 * pow(32.0, pr[2] / 30.0);
    double wantFrame = 200.0 / mFrame, wantArrow = 35.0 / mArrow;
    if (wantFrame < 8) wantFrame = 8;
    if (wantArrow < 8) wantArrow = 8;
    CHECK(fabs((double)frameIntervalMs - wantFrame) <= 1.0 + 0.015 * wantFrame, "frame interval follows the frame speed property");
    CHECK(fabs((double)arrowIntervalMs - wantArrow) <= 1.0 + 0.015 * wantArrow, "arrow interval follows the arrow speed property (separate from the frame)");
    const long f0 = fr->shows, a0 = st[3]->shows, l0 = let->shows;
    const unsigned long clk0 = letterClockMs;
    run(24000);
    const double gotFrame = 24000.0 / (fr->shows - f0), gotArrow = 24000.0 / (st[3]->shows - a0);
    CHECK(fabs(gotFrame - wantFrame) <= 0.03 * wantFrame + 1.5, "measured frame update period matches");
    CHECK(fabs(gotArrow - wantArrow) <= 0.03 * wantArrow + 1.5, "measured arrow update period matches");
    const double clockRate = (double)(letterClockMs - clk0) / 24000.0;
    CHECK(fabs(clockRate - mLet) <= 0.02 * mLet + 0.005, "letter pattern clock runs at the letters speed multiplier");
    CHECK(let->shows - l0 > 150, "letters keep updating at their own rate");
    printf("  speeds at positions %d/%d/%d: frame %.0f ms, arrow %.1f ms, letters x%.2f\n", pr[0], pr[1], pr[2], gotFrame, gotArrow, clockRate);
  }
  menuSetPos(P_speedFrame, 18); menuSetPos(P_speedLetters, 18); menuSetPos(P_speedArrow, 18);
  // The program switch is a hex switch with a debounce: a position that is only passed through while turning is ignored.
  setProgramPins(0); run(200);
  setProgramPins(12); run(5);
  CHECK(hexStable[SLOT_PROGRAM] == 0, "a position held for 5 ms is not accepted");
  setProgramPins(3); run(5);
  setProgramPins(4); run(150);
  CHECK(hexStable[SLOT_PROGRAM] == 4 && letterMode == 4, "the final position is accepted after it settles and starts that program");
  setProgramPins(0); run(150);
  // Reading is consistent with the configured polarity.
  g_pinLevel[PIN_PROGRAM[0]] = HIGH; g_pinLevel[PIN_PROGRAM[1]] = LOW;
  g_pinLevel[PIN_PROGRAM[2]] = LOW; g_pinLevel[PIN_PROGRAM[3]] = HIGH;
  CHECK(readHexSwitch(PIN_PROGRAM) == (HEX_BIT_IS_LOW ? 6 : 9), "pin polarity: pins 2 and 4 LOW read as 6 (active low)");
  setProgramPins(0); run(150);

  // ---- Casino closed tests ----
  run(300);
  auto openLevel = [&](int k) { return shownLevel(let, FIRST_WORD_LETTERS + k); };
  auto pressMode = [&]() { const int to = (letterMode + 1) % 16; if (hexStable[SLOT_PROGRAM] == to) { setProgramPins(to ^ 8); run(60); } setProgramPins(to); run(180); };   // next program with the program switch
  while (letterMode != MODE_BREATHE) pressMode();
  // V2: the typewriter lights the letters in reading order C A S I N O, O P E N; on the strip that is the pixel order
  // 0 1 2 3 4 5, then 9 8 7 6 (OPEN is wired reversed). Pixel numbers are hard-coded here.
  {
    static const int WANT_ORDER[10] = {0, 1, 2, 3, 4, 5, 9, 8, 7, 6};
    while (letterMode != MODE_TYPEWRITER) pressMode();
    long firstLit[10]; for (int p = 0; p < 10; p++) firstLit[p] = -1;
    const unsigned long t0 = g_millis;
    for (int k = 0; k < 6000; k++) { run(1); for (int p = 0; p < 10; p++) if (firstLit[p] < 0 && ((let->shown_[p] >> 16) & 0xFF) > 100) firstLit[p] = (long)(g_millis - t0); }
    bool ordered = true;
    for (int i = 0; i < 10; i++) { if (firstLit[WANT_ORDER[i]] < 0) ordered = false; if (i > 0 && firstLit[WANT_ORDER[i]] <= firstLit[WANT_ORDER[i - 1]]) ordered = false; }
    CHECK(ordered, "typewriter runs in reading order: strip pixels 0-5, then 9, 8, 7, 6 (OPEN is reversed on the strip)");
    for (int l = 0; l < 10; l++) CHECK(letterPixel(l) == WANT_ORDER[l], "letterPixel() maps the letters to the strip pixels");
    while (letterMode != MODE_BREATHE) pressMode();
  }
  // a 10 ms glitch on the switch must be ignored
  g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(10); g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(200);
  CHECK(!casinoClosed, "10 ms glitch on the casino closed switch is ignored");
  // the OPEN letters are lit in breathe mode while open
  { int lit = 0; for (int k = 0; k < 600; k++) { run(1); for (int l = 0; l < LETTER_COUNT - FIRST_WORD_LETTERS; l++) if (openLevel(l) > 60) lit++; }
    CHECK(lit > 0, "OPEN letters are lit while the casino is open"); }
  g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(100);
  CHECK(casinoClosed, "casino closed after the switch goes LOW");
  int prevFlick = NO_LETTER; bool sawFull = false, sawDim = false, sawOffAfterStart = false, startupZero = false, startupFull = false;
  long openLit = 0, firstLit = 0, nonWhite = 0;
  for (int mode = 0; mode < LETTER_MODE_COUNT; mode++) {
    long casinoLit = 0;
    for (int k = 0; k < 6000; k++) {
      run(1);
      for (int l = 0; l < LETTER_COUNT - FIRST_WORD_LETTERS; l++) if (openLevel(l) != 0) openLit++;
      for (int l = 0; l < FIRST_WORD_LETTERS; l++) if (shownLevel(let, l) > 20) casinoLit++;
      if (flickerLetter != NO_LETTER && prevFlick == NO_LETTER) CHECK(flickerLetter < FIRST_WORD_LETTERS, "only CASINO letters flicker while closed");
      prevFlick = flickerLetter;
      // CLOSED light: always white (R = G = B) and all LEDs equal
      for (auto c : clo->shown_) { if (((c >> 16) & 0xFF) != ((c >> 8) & 0xFF) || ((c >> 8) & 0xFF) != (c & 0xFF) || c != clo->shown_[0]) nonWhite++; }
      const uint8_t v = clo->shown_[0] & 0xFF;
      if (g_millis % 7 == 0) { }
      if (closedState == CL_STEADY) { if (v == 255) sawFull = true; }
      if (closedState == CL_FLICKER) { if (v > 0 && v < 255) sawDim = true; if (v == 0) sawOffAfterStart = true; }
      if (closedState == CL_STARTUP) { if (v == 0) startupZero = true; if (v == 255) startupFull = true; }
      if (out && g_millis % 250 == 0) dump(out);
    }
    firstLit += casinoLit;
    CHECK(casinoLit > 0, "CASINO letters still follow the mode while closed");
    pressMode();
  }
  CHECK(openLit == 0, "OPEN letters are dark in every mode while the casino is closed");
  CHECK(nonWhite == 0, "CLOSED light is always white and uniform");
  CHECK(sawFull && sawDim && sawOffAfterStart, "CLOSED light has steady phases and flicker bursts with full, dim and dark levels");
  CHECK(startupZero && startupFull, "CLOSED light flashes while starting up");
  // speed: at x4 the flicker bursts come much more often than at x0.25
  auto bursts = [&](int pos, unsigned long ms) { menuSetPos(P_speedLetters, pos); run(300); int n = 0, prev = closedState;
    for (unsigned long k = 0; k < ms; k++) { run(1); if (closedState == CL_FLICKER && prev != CL_FLICKER) n++; prev = closedState; } return n; };
  const int fast = bursts(30, 60000), slow = bursts(0, 60000);
  CHECK(fast >= 5 && slow <= 2 && fast > slow, "CLOSED light timing follows the letters speed property");
  printf("  CLOSED flicker bursts in 60 s: x4.000 -> %d, x0.125 -> %d\n", fast, slow);
  menuSetPos(P_speedLetters, 18); run(300);
  // reopen: OPEN letters come back, CLOSED light goes dark
  g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(100);
  CHECK(!casinoClosed, "casino open again after the switch goes HIGH");
  while (letterMode != MODE_BREATHE) pressMode();
  { int lit = 0; bool lightOff = true; for (int k = 0; k < 1500; k++) { run(1); for (int l = 0; l < LETTER_COUNT - FIRST_WORD_LETTERS; l++) if (openLevel(l) > 60) lit++;
        if (k > 100) for (auto c : clo->shown_) if (c != 0) lightOff = false; }
    CHECK(lit > 0, "OPEN letters light up again after reopening"); CHECK(lightOff, "CLOSED light is off after reopening"); }
  printf("  closed phase: CASINO letters lit samples %ld\n", firstLit);

  // ---- New letter modes (scanner, sparkle, chase, pulse) and random auto ----
  menuSetPos(P_speedLetters, 18); run(300);
  auto gotoMode = [&](uint8_t m) { while (letterMode != m) pressMode(); run(400); };
  {
    // scanner: a bright head moves over all letters; letters behind it glow as a tail
    gotoMode(MODE_SCANNER);
    bool heads[LETTER_COUNT] = {}; int spreadMs = 0, tailMs = 0;
    for (int k = 0; k < 5000; k++) { run(1);
      if (flickerLetter != NO_LETTER) continue;
      int best = 0; uint8_t hi = 0, lo = 255;
      for (uint8_t l = 0; l < LETTER_COUNT; l++) { uint8_t v = shownLevel(let, l); if (v > hi) { hi = v; best = l; } if (v < lo) lo = v; }
      if (hi >= 180) heads[best] = true;
      if (hi - lo > 150) spreadMs++;
      int mid = 0; for (uint8_t l = 0; l < LETTER_COUNT; l++) { uint8_t v = shownLevel(let, l); if (v > 60 && v < 230) mid++; }
      if (mid >= 2) tailMs++;
    }
    int seen = 0; for (bool h : heads) seen += h;
    CHECK(seen >= LETTER_COUNT - 1, "scanner: the bright head visits (almost) every letter");
    CHECK(spreadMs > 3000, "scanner: most of the time only part of the text is bright");
    CHECK(tailMs > 2500, "scanner: letters around the head glow as a tail");
  }
  {
    // sparkle: random letters flash and fade; levels in between exist
    gotoMode(MODE_SPARKLE);
    long flashes[LETTER_COUNT] = {}; bool fading = false; long total = 0;
    for (int k = 0; k < 6000; k++) { run(1);
      if (flickerLetter != NO_LETTER) continue;
      total++;
      for (uint8_t l = 0; l < LETTER_COUNT; l++) { uint8_t v = shownLevel(let, l); if (v > 200) flashes[l]++; if (v > 80 && v < 180) fading = true; }
    }
    bool every = true; for (long f : flashes) if (f < total / 40 || f > total / 2) every = false;
    CHECK(every, "sparkle: every letter sparkles sometimes, none all the time");
    CHECK(fading, "sparkle: flashes fade out softly");
  }
  {
    // chase: exactly one dim letter per word, and it moves
    gotoMode(MODE_CHASE);
    bool dimSeen[LETTER_COUNT] = {}; bool exactlyTwo = true;
    for (int k = 0; k < 4000; k++) { run(1);
      if (flickerLetter != NO_LETTER) continue;
      int dimFirst = 0, dimSecond = 0;
      for (uint8_t l = 0; l < LETTER_COUNT; l++) if (shownLevel(let, l) < 60) { (l < FIRST_WORD_LETTERS ? dimFirst : dimSecond)++; dimSeen[l] = true; }
      if (dimFirst != 1 || dimSecond != 1) exactlyTwo = false;
    }
    int seen = 0; for (bool d : dimSeen) seen += d;
    CHECK(exactlyTwo, "chase: exactly one dim letter in each word at any time");
    CHECK(seen == LETTER_COUNT, "chase: the dim spot visits every letter");
  }
  {
    // pulse: all letters equal, strong beats and a dim pause
    gotoMode(MODE_PULSE);
    bool uniform = true; uint8_t lo = 255, hi = 0;
    for (int k = 0; k < 4000; k++) { run(1);
      if (flickerLetter != NO_LETTER) continue;
      uint8_t ref = shownLevel(let, 0); if (ref < lo) lo = ref; if (ref > hi) hi = ref;
      for (uint8_t l = 1; l < LETTER_COUNT; l++) if (shownLevel(let, l) != ref) uniform = false;
    }
    CHECK(uniform, "pulse: all letters have the same brightness at the same time");
    CHECK(lo <= 60 && hi >= 200, "pulse: strong beats and a dim pause");
  }
  {
    // random auto: picks only real patterns, never the same twice in a row, uses all of them,
    // and changes after the Change delay (default 2 minutes)
    gotoMode(MODE_RANDOM_AUTO);
    CHECK(letterMode == MODE_RANDOM_AUTO && activeMode < PATTERN_MODE_COUNT, "random auto: shows a real pattern");
    bool used[PATTERN_MODE_COUNT] = {}; bool noRepeat = true, inRange = true;
    uint8_t prev = activeMode;
    for (int k = 0; k < 3000; k++) { pickAutoMode(g_millis); if (activeMode == prev) noRepeat = false; if (activeMode >= PATTERN_MODE_COUNT) inRange = false; else used[activeMode] = true; prev = activeMode; }
    bool all = true; for (bool u : used) if (!u) all = false;
    CHECK(noRepeat, "random auto: never picks the same pattern twice in a row");
    CHECK(inRange && all, "random auto: picks every pattern 0..9 and nothing else");
    setLetterMode(MODE_RANDOM_AUTO, g_millis);
    int changes = 0; uint8_t last = activeMode; unsigned long firstChange = 0, start = g_millis;
    for (unsigned long k = 0; k < 3 * 120000UL + 5000UL; k++) { run(1);
      if (activeMode != last) { changes++; last = activeMode; if (!firstChange) firstChange = g_millis - start; } }
    CHECK(changes == 3, "random auto: a new pattern after each 2 minutes (default Change delay)");
    CHECK(firstChange >= 120000UL - 50 && firstChange <= 120000UL + 100, "random auto: first change after about two minutes");
    CHECK(letterMode == MODE_RANDOM_AUTO, "random auto: the selected mode stays random auto");
    // the Change delay property (Random program > Change delay): 1 minute, and 0 = a random delay of 1..30 minutes
    menuSetPos(P_randomDelay, 1); setLetterMode(MODE_RANDOM_AUTO, g_millis);
    CHECK(autoDelayMs == 60000UL, "random auto: Change delay 1 gives a 60 s delay");
    { unsigned long t0 = g_millis; uint8_t a0 = activeMode; int n = 0;
      while (g_millis - t0 < 70000UL) { run(1); if (activeMode != a0) { n++; a0 = activeMode; } }
      CHECK(n == 1, "random auto: with Change delay 1 the pattern changes after one minute"); }
    menuSetPos(P_randomDelay, 0);
    { bool multiple = true, inRange = true; int smallest = 99, largest = 0;
      for (int k = 0; k < 400; k++) { pickAutoMode(g_millis); if (autoDelayMs % 60000UL != 0) multiple = false;
        const int m = (int)(autoDelayMs / 60000UL); if (m < 1 || m > 30) inRange = false; if (m < smallest) smallest = m; if (m > largest) largest = m; }
      CHECK(multiple && inRange, "random auto: Change delay 0 draws whole minutes from 1 to 30");
      CHECK(smallest <= 3 && largest >= 28, "random auto: Change delay 0 uses the whole range 1..30 minutes"); }
    menuSetPos(P_randomDelay, 2);
    pressMode(); CHECK(letterMode == MODE_BREATHE, "the program switch wraps from F (random auto) back to 0");
  }
  // ---- LCD display (fake HD44780 + PCF8574 backpack on the mock Wire) and the settings menu on the real controls ----
  {
    auto rtrim = [](std::string r) { while (!r.empty() && r.back() == ' ') r.pop_back(); return r; };
    auto line0 = [&]() { return rtrim(Wire.row(0)); };
    auto line1 = [&]() { return rtrim(Wire.row(1)); };
    CHECK(Wire.begun && Wire.clock == 400000UL, "LCD: I2C started at 400 kHz");
    CHECK(Wire.fourBit && Wire.clears >= 1 && Wire.backlight, "LCD: initialised in 4-bit mode, cleared, backlight on");
    CHECK(lcdOk, "LCD: found at start-up");
    CHECK(!Wire.oversize, "LCD: no transmission longer than the 32 byte Wire buffer");
    auto printable = [&]() { for (int r = 0; r < 2; r++) for (char c : Wire.row(r)) if ((c < 32 || c > 126) && c != 0x7E && c != 0x7F) return false; return true; };
    // Hard-coded pins of the menu controls and a model of the menu switch (starts at position 0 = all pins HIGH).
    CHECK(PIN_MENU_SWITCH[0] == 42 && PIN_MENU_SWITCH[1] == 43 && PIN_MENU_SWITCH[2] == 44 && PIN_MENU_SWITCH[3] == 45, "menu switch pins D42-D45");
    CHECK(PIN_OK == 48 && PIN_BACK == 49 && PIN_SHIFT == 50, "OK D48, BACK D49, SHIFT D50");
    CHECK(PIN_PROGRAM[0] == 34 && PIN_PROGRAM[3] == 37, "program switch pins D34-D37");
    static int menuPinPos = 0;
    auto setMenuPins = [&](int v) { for (int b = 0; b < 4; b++) g_pinLevel[PIN_MENU_SWITCH[b]] = ((v >> b) & 1) ? LOW : HIGH; };
    auto turn = [&](int steps) { const int d = steps < 0 ? -1 : 1; for (int k = 0; k < (steps < 0 ? -steps : steps); k++) { menuPinPos = (menuPinPos + d) & 15; setMenuPins(menuPinPos); run(25); } };
    auto key = [&](int pin) { g_pinLevel[pin] = LOW; run(60); g_pinLevel[pin] = HIGH; run(60); };
    auto settle = [&]() { run(200); };

    // ---- status page (row 0 "#<program> Casino Open|Closed", row 1 arrow symbol and customer direction) ----
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT; run(200);
    while (letterMode != MODE_BREATHE) pressMode();
    run(300);
    CHECK(menuMode == MM_STATUS, "status page after start-up");
    const std::string ARR_L = std::string(1, (char)0x7F) + " Customer right", ARR_R = std::string(1, (char)0x7E) + " Customer left";   // HD44780 ROM arrows
    CHECK(line0() == "#0 Casino Open" && line1() == ARR_L, "status page: program 0, casino open, arrow points to the customer's right (left arrow symbol: seen from behind)");
    printf("  status page: '%s' / '%s'\n", line0().c_str(), line1().c_str());
    g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT; run(200);
    CHECK(line1() == ARR_R && !arrowPointsRight, "status page follows the arrow switch (customer left, right arrow symbol)");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(200);
    CHECK(line0() == "#0 Casino Closed" && casinoClosed, "status page shows Closed (and the sign follows)");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(200);
    CHECK(line0() == "#0 Casino Open" && !casinoClosed, "status page shows Open again");
    g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT; run(200);
    setProgramPins(10); run(200);
    CHECK(letterMode == 10 && line0() == "#A Casino Open", "the program number is shown in hex (A = converge)");
    setProgramPins(0); run(200);
    for (int m = 0; m < 16; m++) { setProgramPins(m); run(200); char want[24]; snprintf(want, sizeof want, "#%X Casino Open", m);
      CHECK(printable() && line0() == want && letterMode == m, "status page: every program position 0..F is shown"); }
    // OK acts on the press (not on the release); a half turn (exactly 8) of the menu switch is ignored
    { setProgramPins(0); run(200); CHECK(menuMode == MM_STATUS, "OK test starts at the status page");
      g_pinLevel[PIN_OK] = LOW; run(60);
      CHECK(menuMode == MM_MENU, "OK opens the menu while the button is still held (acts on the press)");
      g_pinLevel[PIN_OK] = HIGH; run(60);
      CHECK(menuMode == MM_MENU, "releasing OK does not do anything else");
      const std::string h0 = line0(), h1 = line1();
      menuPinPos = (menuPinPos + 8) & 15; setMenuPins(menuPinPos); run(300);
      CHECK(line0() == h0 && line1() == h1, "a half turn (code change of exactly 8) of the menu switch is ignored");
      key(PIN_BACK); run(200);
      CHECK(menuMode == MM_STATUS, "BACK from the menu root returns to the status page"); }
    // program F: row 1 alternates between the arrow text and "Change in M:SS"
    { setProgramPins(0); run(200); bool noCountdown = true;   // other programs never show the countdown
      for (int k = 0; k < 7000; k++) { run(1); if (line1().rfind("Change in ", 0) == 0) noCountdown = false; }
      CHECK(noCountdown && menuChangeInSec == -1, "program 0: row 1 never shows the countdown");
      setProgramPins(15); run(200); CHECK(line1() == ARR_L, "program F: row 1 starts with the arrow text, the countdown follows");
      bool sawArrow = false, sawCountdown = false;
      for (int k = 0; k < 7000; k++) { run(1); const std::string l1 = line1(); if (l1 == ARR_L) sawArrow = true; if (l1.rfind("Change in ", 0) == 0) sawCountdown = true; }
      CHECK(sawArrow && sawCountdown, "program F: row 1 alternates between the arrow text and the change countdown");
      CHECK(line0() == "#F Casino Open", "program F: row 0 shows #F");
      // the countdown shows the time left until the next random program: 2 minutes (default) minus the time passed
      bool okCount = false; int samples = 0;
      for (int k = 0; k < 7000 && samples < 3; k++) { run(1); const std::string l1 = line1();
        if (l1.rfind("Change in ", 0) == 0) { int m = 0, sec = 0; if (sscanf(l1.c_str() + 10, "%d:%d", &m, &sec) == 2) { samples++;
          const long left = 120L - (long)((g_millis - autoPickedAt) / 1000UL); if (labs((long)(m * 60 + sec) - left) <= 1 && menuChangeInSec == (long)((120000UL - (g_millis - autoPickedAt) + 999UL) / 1000UL)) okCount = true; else { okCount = false; break; } k += 400; } } }
      CHECK(okCount && samples >= 1, "program F: the countdown shows the time left of the 2 minutes (M:SS)"); }
    setProgramPins(0); run(200);

    // ---- the menu on the real controls ----
    key(PIN_OK);
    CHECK(menuMode == MM_MENU && menuDepth == 1, "OK opens the menu at the root");
    CHECK(line0().rfind("Menu", 0) == 0 && line1().rfind("Controls", 0) == 0, "root: title Menu, first item Controls");
    printf("  menu root: '%s' / '%s'\n", Wire.row(0).c_str(), Wire.row(1).c_str());
    turn(1); settle(); CHECK(line1().rfind("Speed", 0) == 0, "one step on the menu switch moves to Speed");
    turn(1); settle(); CHECK(line1().rfind("Random program", 0) == 0, "next: Random program");
    turn(-1); turn(-1); turn(-5); settle(); CHECK(line1().rfind("Controls", 0) == 0, "the selection stops at the top (no wrap)");
    turn(1); settle(); key(PIN_OK);                    // Speed
    CHECK(menuDepth == 2 && line1().rfind("Frame", 0) == 0, "OK enters Speed (first row Frame)");
    key(PIN_OK);                                        // edit Frame speed
    CHECK(menuMode == MM_EDIT, "OK on a property starts editing");
    turn(3); settle();
    CHECK(menuPos(P_speedFrame) == 21, "turning the menu switch three steps raises the frame speed by three positions");
    CHECK(fabs((double)frameIntervalMs - 200.0 / (0.125 * pow(32.0, 21 / 30.0))) <= 3.0, "the new frame speed is applied while editing");
    g_pinLevel[PIN_SHIFT] = LOW; run(60);
    turn(1); settle();
    CHECK(menuPos(P_speedFrame) == 30, "SHIFT held makes the step bigger (10 steps, clamped at the top)");
    g_pinLevel[PIN_SHIFT] = HIGH; run(60);
    key(PIN_BACK); settle();
    CHECK(menuMode == MM_MENU && menuPos(P_speedFrame) == 18 && frameIntervalMs == 200, "BACK cancels the edit: the old speed returns");
    key(PIN_OK); turn(-4); key(PIN_OK); settle();       // edit and accept
    CHECK(menuMode == MM_MENU && menuPos(P_speedFrame) == 14, "OK accepts the edited value");
    CHECK(EEPROM.mem[0] != 0xFF && EEPROM.mem[3] != 0xFF, "the layout tag was written to EEPROM at address 0");
    { unsigned char savedSlot = EEPROM.mem[4 + menuRd8(PP_SLOT, P_speedFrame)]; CHECK(savedSlot == 14, "EEPROM holds position 14 for the frame speed"); }
    // closing the menu with the program switch
    setProgramPins(5); run(200);
    CHECK(menuMode == MM_STATUS && letterMode == 5 && line0() == "#5 Casino Open", "turning the program switch closes the menu and starts the program");
    setProgramPins(0); run(200);
    // the 60 s timer runs only at the root while browsing
    key(PIN_OK); run(59000); CHECK(menuMode == MM_MENU, "root: still open after 59 s");
    run(1500); CHECK(menuMode == MM_STATUS, "root: back to the status page after 60 s without input");
    key(PIN_OK); turn(1); key(PIN_OK); run(65000); CHECK(menuMode == MM_MENU && menuDepth == 2, "inside a submenu there is no timeout");
    key(PIN_OK); run(65000); CHECK(menuMode == MM_EDIT, "while editing there is no timeout");
    key(PIN_BACK); key(PIN_BACK); key(PIN_BACK); settle();
    CHECK(menuMode == MM_STATUS, "BACK at the root returns to the status page");

    // ---- reverse flags: the slide switches follow the flags at once ----
    g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(200);
    CHECK(arrowPointsRight && !casinoClosed, "start: arrow right, casino open");
    menuSetPos(P_arrowReverse, 1); run(50);
    CHECK(!arrowPointsRight && menuPos(P_arrow) == 1, "Arrow reverse swaps the arrow direction without touching the switch");
    menuSetPos(P_casinoReverse, 1); run(50);
    CHECK(casinoClosed && menuPos(P_casino) == 0, "Casino reverse swaps open/closed");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(200);
    CHECK(!casinoClosed, "with Casino reverse on, the closed handle position means open");
    g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT; run(200);
    CHECK(arrowPointsRight, "with Arrow reverse on, the left handle position means right");
    menuSetPos(P_arrowReverse, 0); menuSetPos(P_casinoReverse, 0); run(50);
    CHECK(!arrowPointsRight && casinoClosed, "the flags off: the switch positions mean what they say again");
    g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(200);
    CHECK(arrowPointsRight && !casinoClosed, "switches back to right / open");

    // ---- brightness curves are menu properties: each LED type has its own curve ----
    rebuildAllCurves();                                  // the older tests ran with identity curves: back to the menu values
    menuSetPos(P_stripMax, 40); run(10);
    CHECK(brightnessLut[LED_TYPE_STRIP][255] == 40 && brightnessLut[LED_TYPE_LETTERS][255] == 80 && brightnessLut[LED_TYPE_STICK][255] == 80, "Strip Max changes only the frame curve");
    menuSetPos(P_lettersMax, 60); run(10);
    CHECK(brightnessLut[LED_TYPE_LETTERS][255] == 60 && brightnessLut[LED_TYPE_STRIP][255] == 40 && brightnessLut[LED_TYPE_STICK][255] == 80, "Letters Max changes only the letters curve");
    menuSetPos(P_stickMax, 100); run(10);
    CHECK(brightnessLut[LED_TYPE_STICK][255] == 100 && brightnessLut[LED_TYPE_LETTERS][255] == 60, "Stick Max changes only the stick curve");
    menuSetPos(P_stripMax, 80); menuSetPos(P_lettersMax, 80); menuSetPos(P_stickMax, 80); run(10);
    CHECK(brightnessLut[0][255] == 80 && brightnessLut[1][255] == 80 && brightnessLut[2][255] == 80, "the Max values back to 80");
    // Exponent and Mix reach the curves, too (property positions: exponent 0.1..20 log 50 steps, mix 0..1 linear 50 steps)
    menuSetPos(P_lettersMix, 0); run(10);
    CHECK(brightnessLut[LED_TYPE_LETTERS][128] == 40 && brightnessLut[LED_TYPE_STRIP][128] == 13, "Letters Mix 0 makes only the letters curve linear (128 -> 40)");
    menuSetPos(P_lettersMix, 50); run(10);
    CHECK(brightnessLut[LED_TYPE_LETTERS][128] == 13, "Letters Mix 1 again");
    menuSetPos(P_stickMin, 10); run(10);
    CHECK(brightnessLut[LED_TYPE_STICK][255] == 80 && brightnessLut[LED_TYPE_STICK][1] == 10 && brightnessLut[LED_TYPE_STICK][0] == 0 && brightnessLut[LED_TYPE_STRIP][1] == 0, "Stick Min 10 lifts only the stick curve (zero stays dark)");
    menuSetPos(P_stickMin, 0); run(10);

    // ---- Restore defaults through the menu: asks OK / BACK, sets every stored value to its default ----
    menuSetPos(P_speedArrow, 20); menuSetPos(P_randomDelay, 7); run(10);
    key(PIN_OK); turn(4); key(PIN_OK);                   // root item 4 = Restore defaults
    CHECK(menuMode == MM_CONFIRM, "Restore defaults asks for confirmation");
    key(PIN_BACK); CHECK(menuMode == MM_MENU && menuPos(P_speedArrow) == 20, "BACK at the question changes nothing");
    key(PIN_OK); key(PIN_OK); settle();
    CHECK(menuPos(P_speedArrow) == 18 && menuPos(P_speedFrame) == 18 && menuPos(P_randomDelay) == 2 && frameIntervalMs == 200 && arrowIntervalMs == 35,
          "OK at the question restores every default and applies it");
    key(PIN_BACK); key(PIN_BACK); settle();

    // ---- power cycle: the saved values come back from EEPROM ----
    menuSetPos(P_speedLetters, 20); menuSave(&P_speedLetters, 1);
    menuSetPos(P_speedLetters, 3);                       // a change that is not saved
    { const bool saved = menuBegin(); applySpeeds(); rebuildAllCurves();
      menuSet(P_program, hexStable[SLOT_PROGRAM]); menuSet(P_arrow, arrowPosition()); menuSet(P_casino, casinoClosed ? 0 : 1);   // the status values are set again by finishStartup() in the real start
      CHECK(saved && menuPos(P_speedLetters) == 20 && letterMult256 == (uint16_t)(0.125 * pow(32.0, 20 / 30.0) * 256.0 + 0.5), "after a restart the saved value is used, the unsaved one is gone"); }
    menuSetPos(P_speedLetters, 18); menuSave(&P_speedLetters, 1); run(10);

    for (uint8_t ty : IDENT_TYPES) setBrightnessCurve(ty, 0, 255, 1.0f, 0.0f, true);   // the older tests below run with identity curves

    // ---- LCD hardware behaviour ----
    run(1000);
    const long before = Wire.transactions; run(2500);
    CHECK(Wire.transactions - before <= 8, "LCD: the display is only written when the text changes");
    bool small = true; long worst = 0;
    for (int k = 0; k < 20000; k++) { const long t0 = Wire.transactions; run(1); if (Wire.transactions - t0 > 1) small = false; if ((long)Wire.lastBytes > worst) worst = Wire.lastBytes; }
    CHECK(small && worst <= 24 && !Wire.oversize, "LCD: at most one short I2C transmission per loop pass");
    printf("  LCD: longest I2C transmission %ld bytes\n", worst);

    // a display that is missing or at another address is left out without disturbing the sign, and found again later
    Wire.address = 0x3F; lcdOk = false; lcdInit();
    CHECK(!lcdOk, "LCD: a display at another address is not found");
    const long t1 = Wire.transactions; run(500);
    CHECK(Wire.transactions == t1 + 1 || Wire.transactions == t1, "LCD: while no display answers only the retry probe is sent");
    Wire.address = 0x27; run(1500);
    CHECK(lcdOk, "LCD: found again by the periodic retry when the display answers");
    run(400);
    CHECK(line0() == "#0 Casino Open" && printable(), "LCD: shows the status page again after re-initialisation");
    CHECK(Wire.present, "LCD: present");
    Wire.present = false; g_pinLevel[PIN_ARROW_SWITCH] = SW_LEFT; run(300);          // the text changes: sending it fails
    CHECK(!lcdOk, "LCD: a display that stops answering is noticed when the sketch writes to it");
    g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT;
    Wire.present = true; run(1500);
    CHECK(lcdOk, "LCD: and found again when it comes back");
    run(300);

    // boot-time I2C scan: the log lists the display and the scan hint appears only when the LCD address is silent
    {
      const std::string boot = Serial.captured;
      CHECK(boot.find("I2C scan (0x08-0x77):") != std::string::npos, "I2C scan: runs at boot and says so");
      CHECK(boot.find("  found 0x27  (LCD address in this sketch)\n") != std::string::npos, "I2C scan: boot log lists the display at 0x27");
      CHECK(boot.find("I2C scan done: 1 device(s)\n") != std::string::npos, "I2C scan: boot log counts one device");
      CHECK(boot.find("No device on the LCD address") == std::string::npos, "I2C scan: no warning when the display answers");
      CHECK(boot.find("I2C scan done") < boot.find("Serial speed 115200 baud"), "I2C scan: happens before the start-up lines");
      CHECK(boot.find("EEPROM: no saved values for this menu layout, defaults used.") != std::string::npos, "start-up: says that the EEPROM had no saved menu values");
      Serial.captured.clear();
      Wire.address = 0x3F;
      const uint8_t n3f = scanI2cBus();
      CHECK(n3f == 1 && Serial.captured.find("  found 0x3F\n") != std::string::npos, "I2C scan: finds a display at 0x3F (no LCD marker)");
      CHECK(Serial.captured.find("No device on the LCD address 0x27: check SDA D20 / SCL D21") != std::string::npos,
            "I2C scan: warns when nothing answers on the LCD address");
      Serial.captured.clear();
      Wire.present = false;
      const uint8_t n0 = scanI2cBus();
      CHECK(n0 == 0 && Serial.captured.find("I2C scan done: 0 device(s)") != std::string::npos, "I2C scan: reports no devices on an empty bus");
      Wire.present = true; Wire.address = 0x0A;
      Serial.captured.clear();
      CHECK(scanI2cBus() == 1 && Serial.captured.find("  found 0x0A\n") != std::string::npos, "I2C scan: addresses below 0x10 are printed with a leading zero");
      Wire.address = 0x27;
      Serial.captured.clear();
      CHECK(scanI2cBus() == 1 && Serial.captured.find("  found 0x08") == std::string::npos, "I2C scan: finds exactly the display (0x08 and 0x77 are the limits)");
      Serial.captured = boot;
    }
  }
  // ---- Power-on LED test ----
  {
    auto rtrimS = [](std::string r) { while (!r.empty() && r.back() == ' ') r.pop_back(); return r; };
    auto line0 = [&]() { return rtrimS(Wire.row(0)); };
    g_pinLevel[PIN_SKIP_BUTTON] = HIGH; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH;
    while (letterMode != MODE_BREATHE) pressMode();
    run(300);
    const unsigned long t0 = g_millis;
    beginStartupTest(g_millis);
    CHECK(STARTUP_TEST_MS == 10000 && STARTUP_TEST_PASSES == 2, "LED test: 10 seconds, 2 passes");
    const unsigned long passMs = 5000;
    // lit[pass][strip][led]
    std::vector<std::vector<std::vector<char>>> lit(STARTUP_TEST_PASSES);
    for (auto &p : lit) { p.resize(STRIP_COUNT); for (int k = 0; k < STRIP_COUNT; k++) p[k].assign(allStrips[k]->numPixels(), 0); }
    int maxLit = 0; bool wrongColour = false, onlyTestPattern = true;
    bool charsRow[2][256] = {};
    bool lcdExact = true; int lcdSamples = 0; std::string firstBad;
    const std::string marquee = "press D33 to skip   ";
    unsigned long endedAt = 0;
    for (unsigned long k = 0; k < STARTUP_TEST_MS + 200 && startupTestActive; k++) {
      if (k == 2000) g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW;      // changed during the test: takes effect afterwards
      run(1);
      if (!startupTestActive) { endedAt = g_millis - t0; break; }
      const unsigned long el = g_millis - t0;
      int pass = (int)(el / passMs); if (pass >= STARTUP_TEST_PASSES) pass = STARTUP_TEST_PASSES - 1;
      int total = 0;
      for (int s = 0; s < STRIP_COUNT; s++) for (uint16_t i = 0; i < allStrips[s]->numPixels(); i++) {
        const uint32_t c = allStrips[s]->shown_[i];
        if (!c) continue;
        total++; lit[pass][s][i] = 1;
        const uint32_t want = pass == 0 ? 0xFFFFFFu : (i % 3 == 0 ? 0xFF0000u : i % 3 == 1 ? 0x00FF00u : 0x0000FFu);
        if ((c & 0xFFFFFF) != want) wrongColour = true;
      }
      if (total > maxLit) maxLit = total;
      if (total > STRIP_COUNT * 2) onlyTestPattern = false;
      for (int s2 = 0; s2 < STRIP_COUNT; s2++) { int cnt = 0; for (auto c : allStrips[s2]->shown_) if (c) cnt++; if (cnt > 2) onlyTestPattern = false; }
      for (int r = 0; r < 2; r++) for (char ch : Wire.row(r)) charsRow[r][(unsigned char)ch] = true;
      // LCD: compare with an independent calculation at moments when nothing is about to change
      if (el >= 180 && (el - 180) % 250 == 0) {
        const unsigned long idx = el / 2500;
        const int counterRow = idx % 2 == 0 ? 1 : 0, skipRow = 1 - counterRow;
        std::string skip; const unsigned long sh = (el / 250) % marquee.size();
        for (int c = 0; c < 16; c++) skip += marquee[(sh + c) % marquee.size()];
        std::string label = std::to_string(el / 1000) + "/" + std::string("10") + " s";
        const int start = (16 - (int)label.size()) / 2, filled = (int)(el * 16 / STARTUP_TEST_MS);
        std::string bar;
        for (int c = 0; c < 16; c++) bar += (c >= start && c < start + (int)label.size()) ? label[c - start] : (c < filled ? '#' : '.');
        lcdSamples++;
        if (Wire.row(skipRow) != skip || Wire.row(counterRow) != bar) { lcdExact = false; if (firstBad.empty()) firstBad = "t=" + std::to_string(el) + " got '" + Wire.row(0) + "'/'" + Wire.row(1) + "' want '" + (skipRow == 0 ? skip : bar) + "'/'" + (skipRow == 0 ? bar : skip) + "'"; }
      }
    }
    CHECK(endedAt >= STARTUP_TEST_MS - 5 && endedAt <= STARTUP_TEST_MS + 40, "LED test: lasts 10 seconds");
    CHECK(!wrongColour, "LED test: white on pass 1, red-green-blue by LED number on pass 2");
    CHECK(onlyTestPattern, "LED test: at most 2 pixels per strip are lit at the same time (nothing else is drawn)");
    bool all = true;
    for (int pass = 0; pass < STARTUP_TEST_PASSES; pass++) for (int s = 0; s < STRIP_COUNT; s++) for (char c : lit[pass][s]) if (!c) all = false;
    CHECK(all, "LED test: every LED of every strip is lit in every pass");
    CHECK(maxLit >= 20, "LED test: all strips run at the same time");
    CHECK(lcdExact && lcdSamples >= 38, "LED test: the LCD shows the scrolling text and the counter/bar, rows swapping every 2.5 s");
    if (!lcdExact) printf("  LCD mismatch: %s\n", firstBad.c_str());
    bool charsOk = true; for (char ch : marquee) for (int r = 0; r < 2; r++) if (!charsRow[r][(unsigned char)ch]) charsOk = false;
    CHECK(charsOk, "LED test: every character of the skip text appears on both rows");
    printf("  LED test: %.1f s, at most %d LEDs lit at once\n", endedAt / 1000.0, maxLit);
    CHECK(!startupTestActive && letterMode == MODE_BREATHE, "after the test the sign starts in mode 0");
    CHECK(casinoClosed, "a switch changed during the test takes effect when the test ends");
    run(400);
    CHECK(line0() == "#0 Casino Closed", "after the test the LCD shows the status page (the casino switch was set to closed during the test)");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(100);
    CHECK(!casinoClosed, "casino open again");

    // the program switch position at power-up decides the first program
    setProgramPins(6); run(100); beginStartupTest(g_millis); run(500);
    g_pinLevel[PIN_SKIP_BUTTON] = LOW; run(100); g_pinLevel[PIN_SKIP_BUTTON] = HIGH; run(100);
    CHECK(!startupTestActive && letterMode == 6 && activeMode == 6, "after the LED test the sign starts in the program the switch shows");
    setProgramPins(0); run(200);
    // D33 skips the test and the same press does not change the mode
    run(300); beginStartupTest(g_millis); run(500);
    CHECK(startupTestActive, "LED test: still running after 0.5 s");
    g_pinLevel[PIN_SKIP_BUTTON] = LOW; run(100); g_pinLevel[PIN_SKIP_BUTTON] = HIGH; run(100);
    CHECK(!startupTestActive, "LED test: D33 skips the test");
    CHECK(letterMode == MODE_BREATHE, "LED test: the skipping press does not change the letter mode");
    run(400); CHECK(line0() == "#0 Casino Open", "LED test: after a skip the LCD shows the status page");

    // a button held down during power-up does not skip the test until it was released and pressed again
    g_pinLevel[PIN_SKIP_BUTTON] = LOW; run(100);          // normal operation: this changes the mode
    beginStartupTest(g_millis); run(2000);
    CHECK(startupTestActive, "LED test: a button held during power-up does not skip the test");
    g_pinLevel[PIN_SKIP_BUTTON] = HIGH; run(100);
    CHECK(startupTestActive, "LED test: releasing the button does not skip it");
    g_pinLevel[PIN_SKIP_BUTTON] = LOW; run(100);
    CHECK(!startupTestActive, "LED test: a new press skips it");
    g_pinLevel[PIN_SKIP_BUTTON] = HIGH; run(100);
    while (letterMode != MODE_BREATHE) pressMode();

    // a skipped test leaves no LED test pattern behind: the strips are cleared and the sign runs normally
    run(300);
    bool shows = false; for (auto *st : g_strips) for (auto c : st->shown_) if (c) shows = true;
    CHECK(shows, "after the test the normal animations are running");
    CHECK(rtrimS(Wire.row(0)).size() > 0, "LCD has text");
  }
  CHECK(PIN_PROGRAM[0] == 34 && PIN_PROGRAM[1] == 35 && PIN_PROGRAM[2] == 36 && PIN_PROGRAM[3] == 37, "program switch pins D34-D37");

  CHECK(sawFlicker, "a letter flickers at least once");
  CHECK(modeChanges == LETTER_MODE_COUNT - 1, "bouncing button presses change the mode exactly once each");
  CHECK(breatheMin >= 70, "breathe: never darker than 70");
  CHECK(minMax[MODE_WAVE][2], "wave: letters differ strongly at the same moment");
  CHECK(minMax[MODE_TYPEWRITER][0] && minMax[MODE_TYPEWRITER][1], "typewriter: sees all off and all lit");
  CHECK(minMax[MODE_IGNITE][0] && minMax[MODE_IGNITE][1], "ignite: sees all off and all lit");
  CHECK(sawMirrored && sawNormal, "frame was seen in both directions");
  CHECK(secondWordHi >= 250 && secondWordLo <= 30, "word blink: second word toggles bright/dim");
  CHECK(firstWordLo >= 200, "word blink: first word stays steady");
  CHECK(altDiffer, "alternate: odd and even letters differ");
  CHECK(arrowFlowChecks > 3000, "the arrow flow check ran on thousands of arrow updates");

  // ---- Sign of life: pin 13 LED and the once-per-second Serial line (115200 baud) ----
  {
    CHECK(SERIAL_BAUD == 115200 && Serial.baud == 115200, "serial speed is 115200 baud");
    CHECK(PIN_STATUS_LED == 13, "the debug LED is on pin 13");
    CHECK(Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "the baud rate is printed at start-up");
    g_pinLevel[PIN_SKIP_BUTTON] = HIGH; run(3000);                // calm: no state change for a while
    // calm: the heartbeat toggles every 250 ms (3 or 4 toggles in 800 ms)
    { const unsigned long b0 = beatCount; run(800); const unsigned long d = beatCount - b0;
      CHECK(d >= 3 && d <= 4, "calm: the LED toggles every 250 ms (3-4 times in 800 ms)"); }
    // one state change (arrow switch): the LED is shortly inverted, then back to the normal heartbeat
    const uint8_t sw = g_pinLevel[PIN_ARROW_SWITCH];
    g_pinLevel[PIN_ARROW_SWITCH] = sw == LOW ? HIGH : LOW; run(45);
    CHECK(g_pinOut[13] != (beatOn ? HIGH : LOW), "a state change inverts the LED for a moment");
    run(250);
    CHECK(g_pinOut[13] == (beatOn ? HIGH : LOW), "after 120 ms the LED follows the heartbeat again");
    // many state changes: the heartbeat gets twice as fast
    g_pinLevel[PIN_ARROW_SWITCH] = sw; run(100);
    { const unsigned long b0 = beatCount; run(500); const unsigned long d = beatCount - b0;
      CHECK(d >= 4 && d <= 5, "busy: the LED toggles every 125 ms (4-5 times in 500 ms; calm would give 2-3)"); }
    run(3500);
    { const unsigned long b0 = beatCount; run(800); const unsigned long d = beatCount - b0;
      CHECK(d >= 3 && d <= 4, "calm again: back to every 250 ms"); }
    // Serial Monitor: one DBG line per second, the values change
    const std::string &log = Serial.captured;
    int dbg = 0; size_t pos = 0;
    while ((pos = log.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
    CHECK(dbg == (int)(g_millis / 1000), "one DBG line per second since power-up");
    CHECK(log.find("DBG up=1s loops=1001 mode=0 arrow=R closed=0 menu=S events=0 beats=3 tick=") != std::string::npos,
          "DBG line at 1 s with all fields");
    CHECK(log.find("DBG up=2s loops=1000 ") != std::string::npos && log.find("DBG up=3s loops=1000 ") != std::string::npos,
          "the uptime counts up and the loop runs 1000 times a second here");
    CHECK(log.find(" arrow=L ") != std::string::npos && log.find(" arrow=R ") != std::string::npos, "the DBG line shows the arrow direction");
    CHECK(log.find(" closed=1 ") != std::string::npos, "the DBG line shows casino closed during the test");
    CHECK(log.find(" mode=10 ") != std::string::npos && log.find(" mode=5 ") != std::string::npos, "the DBG line shows the letter mode");
    CHECK(log.find(" menu=S ") != std::string::npos && log.find(" sw3=") == std::string::npos, "the DBG line shows the menu state (S = status page) and no speed switch positions any more");
    // the beat counter and the tick change from line to line
    { size_t a = log.find("DBG up=30s"), b = log.find("DBG up=31s");
      if (a == std::string::npos || b == std::string::npos) CHECK(false, "DBG lines at 30 s and 31 s exist");
      else {
        std::string l30 = log.substr(a, log.find('\n', a) - a), l31 = log.substr(b, log.find('\n', b) - b);
        auto field = [](const std::string &l, const char *key) { size_t k = l.find(key); return k == std::string::npos ? -1L : std::stol(l.substr(k + strlen(key))); };
        CHECK(field(l31, " beats=") > field(l30, " beats=") && field(l31, " tick=") != field(l30, " tick="), "beats and tick change every second");
      } }
    CHECK(log.find("DBG up=4s loops=1000 mode=0 arrow=R closed=0 menu=S events=0 ") != std::string::npos, "events=0 before the first change");
    CHECK(log.find("DBG up=6s loops=1000 mode=0 arrow=L closed=0 menu=S events=1 ") != std::string::npos,
          "after the arrow switch moved at 5 s: arrow=L, events=1");
  }
  // ---- Brightness curves (expected values hard-coded; see the formula in the sketch) ----
  {
    CHECK(LED_TYPE_COUNT == 3 && LED_TYPE_STRIP == 0 && LED_TYPE_STICK == 1 && LED_TYPE_LETTERS == 2, "three LED types: frame strip 0, sticks 1, letters strip 2");
    // defaults: min 0, max 255, exponent 1.97, mix 1, keep zero dark, for both types
    buildDefaultBrightnessCurves();
    const int in[10] = {0, 1, 2, 16, 32, 64, 128, 192, 254, 255}, want[10] = {0, 0, 0, 0, 0, 2, 13, 38, 79, 80};
    for (int ty = 0; ty < 3; ty++) for (int i = 0; i < 10; i++) CHECK(brightnessLut[ty][in[i]] == want[i], "default curve value");
    bool mono = true; for (int ty = 0; ty < 3; ty++) for (int i = 1; i < 256; i++) if (brightnessLut[ty][i] < brightnessLut[ty][i - 1]) mono = false;
    CHECK(mono, "default curve never decreases");
    // custom curve: min 20, max 200, exponent 1, mix 0 (linear), keep zero dark
    setBrightnessCurve(0, 20, 200, 1.0f, 0.0f, true);
    CHECK(brightnessLut[0][0] == 0 && brightnessLut[0][1] == 21 && brightnessLut[0][128] == 110 && brightnessLut[0][255] == 200, "min/max linear curve");
    setBrightnessCurve(0, 20, 200, 1.0f, 0.0f, false);
    CHECK(brightnessLut[0][0] == 20, "keep zero dark off: input 0 gives min");
    // mix 0 ignores the exponent; mix 0.5 is halfway between linear and the power curve
    setBrightnessCurve(0, 0, 255, 8.0f, 0.0f, true);
    CHECK(brightnessLut[0][128] == 128, "mix 0 is linear whatever the exponent");
    setBrightnessCurve(0, 0, 255, 2.0f, 0.5f, true);
    CHECK(brightnessLut[0][128] == 96, "mix 0.5 with exponent 2 at 128 (0.5*0.502+0.5*0.252)");
    // exponent below 1 brightens the dark end
    setBrightnessCurve(0, 0, 255, 0.5f, 1.0f, true);
    CHECK(brightnessLut[0][64] == 128 && brightnessLut[0][255] == 255, "exponent 0.5 at 64");
    // out-of-range values are clamped: exponent 100 -> 20, exponent 0 -> 0.1, mix 5 -> 1, min above max works downwards
    setBrightnessCurve(0, 0, 255, 100.0f, 1.0f, true);
    CHECK(brightnessLut[0][254] == 236 && brightnessLut[0][128] == 0 && brightnessLut[0][255] == 255, "exponent clamped to 20 (254 -> 236)");
    setBrightnessCurve(0, 0, 255, 0.0f, 1.0f, true);
    CHECK(brightnessLut[0][1] == 147, "exponent clamped to 0.1 (255 * (1/255)^0.1 = 146.5)");
    setBrightnessCurve(0, 0, 255, 2.0f, 5.0f, true);
    CHECK(brightnessLut[0][128] == 64, "mix clamped to 1");
    setBrightnessCurve(0, 200, 50, 1.0f, 0.0f, true);
    CHECK(brightnessLut[0][255] == 50 && brightnessLut[0][1] == 199, "min above max runs downwards");
    // a call for a type that does not exist changes nothing
    setBrightnessCurve(0, 0, 255, 1.0f, 0.0f, true); const uint8_t keep = brightnessLut[1][128]; setBrightnessCurve(7, 0, 10, 1.0f, 0.0f, true);
    CHECK(brightnessLut[1][128] == keep, "unknown LED type ignored");
    // shapeColor keeps the hue: max 100 curve, linear
    setBrightnessCurve(0, 0, 100, 1.0f, 0.0f, true);
    CHECK(shapeColor(0, 0) == 0, "black stays black");
    CHECK(shapeColor(0, 0xFFFFFF) == 0x646464, "white 255 -> 100");
    CHECK(shapeColor(0, 0xFF3300) == 0x641400, "orange keeps its ratio (255,51,0 -> 100,20,0)");
    setBrightnessCurve(1, 0, 255, 1.0f, 0.0f, true);
    CHECK(shapeColor(1, 0x123456) == 0x123456, "the stick table is separate (identity here)");
    // each LED type has its own table in the running sketch: frame, letters and sticks (arrow, CLOSED)
    setBrightnessCurve(0, 0, 50, 1.0f, 0.0f, true);     // frame strip: at most 50
    setBrightnessCurve(1, 0, 100, 1.0f, 0.0f, true);    // sticks: at most 100
    setBrightnessCurve(2, 0, 70, 1.0f, 0.0f, true);     // letters strip: at most 70
    run(3000);
    auto chmax = [](uint32_t c) { unsigned r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255; unsigned m = r > g ? r : g; return b > m ? b : m; };
    unsigned fmax = 0, lmax = 0, amax = 0;
    for (auto *s : {fr}) for (auto c : s->shown_) fmax = (chmax(c) > fmax ? chmax(c) : fmax);
    for (auto c : let->shown_) lmax = (chmax(c) > lmax ? chmax(c) : lmax);
    for (int k = 0; k < 10; k++) for (auto c : st[k]->shown_) amax = (chmax(c) > amax ? chmax(c) : amax);
    CHECK(fmax == 50, "frame LEDs reach exactly the strip maximum 50 (white 255 mapped)");
    CHECK(lmax <= 70 && lmax >= 40, "letters stay within the letters maximum 70 and use most of it (own curve, type 2)");
    CHECK(amax <= 100 && amax >= 90, "arrow sticks stay within the stick maximum 100 and use most of it");
    const uint8_t closedPin = g_pinLevel[PIN_CASINO_CLOSED_SWITCH];
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW;   // casino closed: the CLOSED light is on and flickers
    unsigned cmax = 0;
    for (int k = 0; k < 600; k++) { run(10); for (auto c : clo->shown_) cmax = (c & 255u) > cmax ? (c & 255u) : cmax; }
    CHECK(cmax == 100, "the CLOSED stick follows the stick table (its full white 255 shows as 100)");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = closedPin; run(500);
    // the lookup table is rebuilt at once when a value changes: a new maximum shows within one frame
    setBrightnessCurve(0, 0, 255, 1.0f, 0.0f, true); run(200);
    unsigned fmax2 = 0; for (auto *s : {fr}) for (auto c : s->shown_) fmax2 = (c & 255u) > fmax2 ? (c & 255u) : fmax2;
    CHECK(fmax2 == 255, "after setBrightnessCurve the frame shows full white again");
  }
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no setPixelColor beyond strip length");
  printf("mode changes: %ld; letters %u pixels; shows: frame %ld letters %ld rail %ld\n", modeChanges, (unsigned)let->numPixels(), fr->shows, let->shows, st[3]->shows);
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
