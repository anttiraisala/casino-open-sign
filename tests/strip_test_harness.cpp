// Host-side test of strip_test/sketch.ino (the strip identification sketch).
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/strip_test_harness tests/strip_test_harness.cpp && /tmp/strip_test_harness
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
std::vector<Adafruit_NeoPixel*> g_strips;
#include "../strip_test/sketch.ino"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)

static void run(unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } }

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  setup();
  Adafruit_NeoPixel *s = g_strips[0];
  const uint32_t R = Adafruit_NeoPixel::Color(255, 0, 0), G = Adafruit_NeoPixel::Color(0, 255, 0),
                 B = Adafruit_NeoPixel::Color(0, 0, 255), W = Adafruit_NeoPixel::Color(255, 255, 255);
  CHECK(s->numPixels() == TEST_PIXELS && s->pin_ == PIN_STRIP_DATA, "strip on the data pin with TEST_PIXELS pixels");
  CHECK(s->type_ == (NEO_GRB | NEO_KHZ800), "setting 1 is GRB at 800 kHz");

  // colour test: red, green, blue on pixels 1-3, the rest dark
  run(500);
  CHECK(s->shown_[0] == R && s->shown_[1] == G && s->shown_[2] == B, "colour test shows red, green, blue on pixels 1-3");
  bool restDark = true; for (uint16_t i = 3; i < TEST_PIXELS; i++) if (s->shown_[i]) restDark = false;
  CHECK(restDark, "colour test leaves the other pixels dark");

  // the walking white pixel: exactly one pixel lit, moving one step per WALK_STEP_MS, wrapping around
  run(COLOUR_TEST_MS);                                   // now in the walking part
  std::vector<int> seen;
  for (int k = 0; k < (int)TEST_PIXELS + 2; k++) {
    int lit = 0, where = -1;
    for (uint16_t i = 0; i < TEST_PIXELS; i++) if (s->shown_[i]) { lit++; where = i; CHECK(s->shown_[i] == W, "walking pixel is white"); }
    CHECK(lit == 1, "exactly one pixel lit while walking");
    seen.push_back(where);
    run(WALK_STEP_MS);
  }
  bool stepsOk = true; for (size_t i = 1; i < seen.size(); i++) if (seen[i] != (seen[i-1] + 1) % TEST_PIXELS) stepsOk = false;
  CHECK(stepsOk, "the white pixel moves one pixel per step and wraps around");

  // status LED: setting 1 -> one blink at the start
  // (restart the timeline so the blink can be counted from the beginning of a setting)
  g_millis = 100000; startSetting(0, g_millis);
  int rises = 0; uint8_t prev = 0;
  for (int k = 0; k < 6000; k++) { run(1); if (g_pinOut[PIN_STATUS_LED] && !prev) rises++; prev = g_pinOut[PIN_STATUS_LED]; }
  CHECK(rises == 1, "status LED blinks once in setting 1");

  // automatic advance through all 12 settings, in the right order, then wrap around
  startSetting(0, g_millis);
  const uint16_t order[6] = {NEO_GRB, NEO_RGB, NEO_BRG, NEO_RBG, NEO_GBR, NEO_BGR};
  for (int n = 0; n < SETTING_COUNT * 2; n++) {
    const int idx = n % SETTING_COUNT;
    const uint16_t want = order[idx % 6] | (idx < 6 ? NEO_KHZ800 : NEO_KHZ400);
    CHECK(s->type_ == want && setting == idx, "settings run in the planned order (6 colour orders at 800 kHz, then at 400 kHz)");
    int r = 0; uint8_t p = g_pinOut[PIN_STATUS_LED];
    for (unsigned long k = 0; k < PHASE_MS - 10; k++) { run(1); if (g_pinOut[PIN_STATUS_LED] && !p) r++; p = g_pinOut[PIN_STATUS_LED]; }
    if (r != idx + 1) printf("  setting %d: %d blinks\n", idx + 1, r);
    CHECK(r == idx + 1, "status LED blinks the number of the setting");
    run(20);                                              // the phase ends and the next setting starts
  }
  CHECK(setting == 0, "after setting 12 the test starts again from setting 1");

  // the button: a press moves to the next setting at once; bouncing counts once; holding does not repeat
  startSetting(0, g_millis); run(500);
  g_pinLevel[PIN_BUTTON] = LOW; run(10); g_pinLevel[PIN_BUTTON] = HIGH; run(10); g_pinLevel[PIN_BUTTON] = LOW; run(200);   // bounce
  CHECK(setting == 1, "a (bouncing) button press selects the next setting exactly once");
  run(3000);
  CHECK(setting == 1, "holding the button does not repeat");
  g_pinLevel[PIN_BUTTON] = HIGH; run(200);
  g_pinLevel[PIN_BUTTON] = LOW; run(100); g_pinLevel[PIN_BUTTON] = HIGH; run(100);
  CHECK(setting == 2, "the next press selects the following setting");
  CHECK(s->typeChanges > 20, "the colour order and speed are really changed with updateType()");
  CHECK(s->overflow == 0, "no pixel written beyond the strip length");

  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
