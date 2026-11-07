// Host-side test of stick_test/sketch.ino (the 10 LED stick test with a separate 5 V supply).
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/stick_test_harness tests/stick_test_harness.cpp && /tmp/stick_test_harness
// All expected values are written out here on purpose (they are not taken from the sketch constants),
// so a changed constant in the sketch is detected.
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
std::vector<Adafruit_NeoPixel*> g_strips;
#include "../stick_test/sketch.ino"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)

static unsigned maxBrightnessSeen = 0;
static void run(unsigned long ms) {
  for (unsigned long k = 0; k < ms; k++) {
    g_millis++;
    loop();
    for (auto *s : g_strips) if (s->shownBrightness_ > maxBrightnessSeen) maxBrightnessSeen = s->shownBrightness_;
  }
}

static const uint32_t R = 0xFF0000, G = 0x00FF00, B = 0x0000FF, W = 0xFFFFFF;

// Number of lit LEDs on stick i (as last shown) and whether they all have colour c.
static int lit(int i) { int n = 0; for (auto c : g_strips[i]->shown_) if (c) n++; return n; }
static bool stickAll(int i, uint32_t c) { for (auto v : g_strips[i]->shown_) if (v != c) return false; return true; }
static int totalLit() { int n = 0; for (int i = 0; i < 10; i++) n += lit(i); return n; }
static bool allSticksAll(uint32_t c) { for (int i = 0; i < 10; i++) if (!stickAll(i, c)) return false; return true; }

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  setup();

  // --- the set-up: 10 sticks, 10 LEDs each, the planned data pins, GRB at 800 kHz
  CHECK(g_strips.size() == 10, "ten stick objects exist");
  const int pins[10] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2};
  bool setupOk = g_strips.size() == 10;
  for (int i = 0; setupOk && i < 10; i++) {
    if (g_strips[i]->pin_ != pins[i] || g_strips[i]->numPixels() != 10 || g_strips[i]->type_ != (NEO_GRB | NEO_KHZ800)) setupOk = false;
  }
  CHECK(setupOk, "stick n is on the n-th data pin (2,3,4,5,6,7,8,A0,A1,A2), 10 LEDs, GRB 800 kHz");
  CHECK(PIN_BUTTON == 22 && PIN_STATUS_LED == 13, "button on pin 22, status LED on pin 13");

  // --- start: a dark pause of 1.5 s
  run(1000);
  CHECK(phase == PH_START && totalLit() == 0, "power-up: dark for the first 1.5 s");

  // --- phase 1 IDENTIFY: stick k alone, white, brightness 40, 1.2 s each
  run(520);                                          // t = 1520
  CHECK(phase == PH_IDENTIFY, "phase 1 (identify) starts at 1.5 s");
  for (int k = 0; k < 10; k++) {
    // jump to the middle of the 1.2 s slot of stick k (slots start at 1500 + 1200*k)
    unsigned long target = 1500UL + 1200UL * k + 700UL;
    run(target - g_millis);
    bool only = stickAll(k, W) && g_strips[k]->shownBrightness_ == 40;
    for (int j = 0; j < 10; j++) if (j != k && lit(j) != 0) only = false;
    CHECK(only, "identify: exactly stick k lit, all 10 LEDs white, brightness 40");
  }

  // --- phase 2 COLOURS at 13.5 s: red, green, blue, 2.5 s each, all sticks, brightness 60
  run(13500UL + 500UL - g_millis);
  CHECK(phase == PH_COLOURS, "phase 2 (colours) starts at 13.5 s");
  CHECK(allSticksAll(R) && g_strips[0]->shownBrightness_ == 60, "colours: all LEDs of all sticks red, brightness 60");
  run(2500); CHECK(allSticksAll(G), "colours: then all green");
  run(2500); CHECK(allSticksAll(B), "colours: then all blue");

  // --- phase 3 WALK at 21 s: one white LED, one step per 60 ms, through stick 1 ... stick 10, two passes
  run(21000UL + 30UL - g_millis);
  CHECK(phase == PH_WALK, "phase 3 (walk) starts at 21 s");
  int prev = -1; bool walkOk = true; int steps = 0;
  for (int k = 0; k < 198; k++) {
    int where = -1, count = 0;
    for (int i = 0; i < 10; i++) for (int j = 0; j < 10; j++) if (g_strips[i]->shown_[j]) { count++; where = i * 10 + j; if (g_strips[i]->shown_[j] != W) walkOk = false; }
    if (count != 1) walkOk = false;
    if (prev >= 0 && where != (prev + 1) % 100) walkOk = false;
    if (prev >= 0) steps++;
    prev = where;
    run(60);
  }
  CHECK(walkOk && steps == 197, "walk: exactly one white LED, one LED further every 60 ms, in order stick 1..10, wrapping");

  // --- phase 4 RAMP at 33 s: all 100 LEDs white; brightness 16, 32, 64, 96, 128 (5 s each); never above 128
  run(33000UL + 500UL - g_millis);
  CHECK(phase == PH_RAMP, "phase 4 (ramp) starts at 33 s");
  const int expectB[5] = {16, 32, 64, 96, 128};
  for (int s = 0; s < 5; s++) {
    unsigned long target = 33000UL + 5000UL * s + 2500UL;
    run(target - g_millis);
    CHECK(allSticksAll(W) && totalLit() == 100, "ramp: all 100 LEDs white");
    bool b = true; for (int i = 0; i < 10; i++) if (g_strips[i]->shownBrightness_ != expectB[s]) b = false;
    CHECK(b, "ramp: brightness steps 16, 32, 64, 96, 128");
    CHECK(g_pinOut[13] == HIGH, "status LED on during the ramp");
  }

  // --- phase 5 SOAK at 58 s: 30 s all white at 128
  run(58000UL + 5000UL - g_millis);
  CHECK(phase == PH_SOAK, "phase 5 (soak) starts at 58 s");
  CHECK(allSticksAll(W) && g_strips[9]->shownBrightness_ == 128, "soak: all LEDs white at brightness 128");
  CHECK(g_pinOut[13] == HIGH, "status LED on during the soak");
  run(20000);
  CHECK(phase == PH_SOAK && totalLit() == 100, "soak lasts the full 30 s");

  // --- phase 6 REST at 88 s: dark for 3 s, then the next cycle starts at the identify phase
  run(88000UL + 1000UL - g_millis);
  CHECK(phase == PH_REST && totalLit() == 0 && g_pinOut[13] == LOW, "rest: dark, status LED off");
  run(2500);
  CHECK(phase == PH_IDENTIFY && cycleCount == 1, "after the rest the test starts again from phase 1 (cycle 1 finished)");
  run(100);
  CHECK(stickAll(0, W) && lit(1) == 0, "second cycle: stick 1 lit first");

  // --- brightness limit: never above MAX_BRIGHTNESS (128) anywhere in the whole run
  CHECK(maxBrightnessSeen == 128, "the highest brightness ever shown is exactly 128 (MAX_BRIGHTNESS)");

  // --- button: a press skips to the next phase once (bounce ignored), holding does not repeat
  g_millis = 500000; startPhase(PH_IDENTIFY, g_millis); run(300);
  g_pinLevel[22] = LOW; run(10); g_pinLevel[22] = HIGH; run(10); g_pinLevel[22] = LOW; run(200);
  CHECK(phase == PH_COLOURS, "a (bouncing) button press skips to the next phase exactly once");
  run(1000);
  CHECK(phase == PH_COLOURS, "holding the button does not repeat");
  g_pinLevel[22] = HIGH; run(200);
  g_pinLevel[22] = LOW; run(100); g_pinLevel[22] = HIGH; run(100);
  CHECK(phase == PH_WALK, "the next press skips again");

  // --- no pixel written outside a stick
  bool noOverflow = true; for (auto *s : g_strips) if (s->overflow) noOverflow = false;
  CHECK(noOverflow, "no LED written beyond the end of a stick");

  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
