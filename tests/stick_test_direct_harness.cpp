// Host-side test of stick_test_direct/sketch.ino (11 LED sticks with moving patterns, plugged into the Grove shield).
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/stick_test_direct_harness tests/stick_test_direct_harness.cpp && /tmp/stick_test_direct_harness
// All expected values are written out here on purpose (they are not taken from the sketch constants),
// so a changed constant or pattern in the sketch is detected.
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
std::vector<Adafruit_NeoPixel*> g_strips;
#include "../stick_test_direct/sketch.ino"

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

static int lit(int i) { int n = 0; for (auto c : g_strips[i]->shown_) if (c) n++; return n; }
// True when exactly the pixels in `pos` (-1 terminated) have colour c and all other pixels are dark.
static bool exactly(int i, uint32_t c, std::initializer_list<int> pos) {
  std::vector<uint32_t> want(10, 0);
  for (int p : pos) want[p] = c;
  return g_strips[i]->shown_ == want;
}
static bool allWhite(int i) { for (auto v : g_strips[i]->shown_) if (v != 0xFFFFFF) return false; return true; }

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();
  Serial.capture = false;

  // --- set-up: 11 sticks, 10 LEDs each, data pins D2..D8, A0..A3, GRB at 800 kHz
  CHECK(g_strips.size() == 11, "eleven stick objects exist");
  const int pins[11] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3};
  bool setupOk = g_strips.size() == 11;
  for (int i = 0; setupOk && i < 11; i++)
    if (g_strips[i]->pin_ != pins[i] || g_strips[i]->numPixels() != 10 || g_strips[i]->type_ != (NEO_GRB | NEO_KHZ800)) setupOk = false;
  CHECK(setupOk, "stick n is on the n-th data pin (2..8, A0..A3), 10 LEDs, GRB 800 kHz");
  CHECK(PIN_BUTTON == 22 && PIN_STATUS_LED == 13 && ACTIVE_STICKS == 11, "button on pin 22, status LED on pin 13, 11 active sticks");
  CHECK(SERIAL_BAUD == 115200 && Serial.baud == 115200, "serial speed is 115200");
  CHECK(Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "the start text states the 115200 baud speed");

  // --- the patterns start at once: the first frame is already on the sticks at t = 0 (spotlight on stick 1)
  CHECK(allWhite(0), "t=0: the spotlight stick 1 shows all 10 LEDs white");
  bool startOk = true;
  for (int i = 1; i < 11; i++) if (lit(i) < 1 || lit(i) > 6) startOk = false;
  CHECK(startOk, "t=0: every other stick already shows a pattern (1..6 LEDs lit)");

  // --- 120 s: every 20 ms frame is checked; special moments are checked exactly
  Serial.capture = true; Serial.captured.clear();
  bool framesOk = true, spotOk = true, changed = false, stuck = false;
  std::vector<uint32_t> prevPicture;
  int toggles = 0, togglesQuiet = 0; uint8_t lastPin = g_pinOut[13];
  for (unsigned long t = 1; t <= 120000; t++) {
    run(1);
    if (g_pinOut[13] != lastPin) { toggles++; lastPin = g_pinOut[13]; if (t >= 4000 && t < 4800) togglesQuiet++; }
    if (t % 20 != 0) continue;
    int spot = (int)((t / 1200) % 11);
    for (int i = 0; i < 11; i++) {
      if (i == spot) { if (!allWhite(i)) spotOk = false; }
      else if (lit(i) < 1 || lit(i) > 6) framesOk = false;
    }
    std::vector<uint32_t> pic;
    for (int i = 0; i < 11; i++) for (auto v : g_strips[i]->shown_) pic.push_back(v);
    if (t % 1000 == 0) { if (!prevPicture.empty() && prevPicture == pic) stuck = true; prevPicture = pic; }
    if (!prevPicture.empty()) changed = true;

    if (t == 1180) CHECK(allWhite(0) && !allWhite(1), "t=1180: the spotlight is still on stick 1");
    if (t == 1220) {
      CHECK(allWhite(1) && !allWhite(0), "t=1220: the spotlight has moved to stick 2");
      CHECK(g_pinOut[13] != (beatOn ? HIGH : LOW), "t=1220: the debug LED is inverted right after a state change");
    }
    if (t == 1400) CHECK(g_pinOut[13] == (beatOn ? HIGH : LOW), "t=1400: the debug LED is back to the normal heartbeat");
    if (t == 13200) CHECK(allWhite(0) && !allWhite(10), "t=13200: the spotlight is back on stick 1 after stick 11");
    if (t == 12000) CHECK(allWhite(10), "t=12000: the spotlight is on stick 11");

    if (t == 2420) {                    // pattern set 0: stick s runs pattern s; the spotlight is on stick 3
      CHECK(allWhite(2), "t=2420: spotlight on stick 3");
      CHECK(exactly(0, 0xFF0000, {8}) == false && g_strips[0]->shown_[8] == 0xFF0000 && g_strips[0]->shown_[7] == 0x320000 &&
            g_strips[0]->shown_[9] == 0x320000 && lit(0) == 3, "t=2420 stick 1 scanner: red head on LED 9 with dim neighbours");
      CHECK(exactly(1, 0x00FF00, {4, 9}), "t=2420 stick 2 chase: green on LEDs 5 and 10");
      CHECK(exactly(4, 0xFF5A00, {1, 4, 7}), "t=2420 stick 5 theater chase: orange on LEDs 2, 5, 8");
      CHECK(g_strips[5]->shown_[6] == 0x0000FF && g_strips[5]->shown_[3] == 0xFF00C8 && lit(5) == 2,
            "t=2420 stick 6 bouncing dots: blue on LED 7, magenta on LED 4");
      CHECK(exactly(6, 0x00DCDC, {0, 1}), "t=2420 stick 7 fill: cyan on LEDs 1 and 2");
      CHECK(exactly(7, 0xFFC800, {0, 2, 4, 6, 8}), "t=2420 stick 8 odd/even blink: yellow on LEDs 1,3,5,7,9");
      CHECK(exactly(9, 0xFF0000, {0, 1, 2}), "t=2420 stick 10 police lights: red on LEDs 1-3");
      CHECK(exactly(10, 0xB400FF, {4, 9}), "t=2420 stick 11 reverse runner: purple on LEDs 5 and 10");
    }
    if (t == 2580) CHECK(exactly(9, 0x0000FF, {7, 8, 9}), "t=2580 stick 10 police lights: blue on LEDs 8-10");
    if (t == 12020) {                   // pattern set 1: stick s runs pattern (s+1)%11; spotlight on stick 11
      CHECK(allWhite(10), "t=12020: spotlight on stick 11");
      CHECK(exactly(0, 0x00FF00, {1, 6}), "t=12020 set 1: stick 1 now runs the chase pattern (green on LEDs 2 and 7)");
      CHECK(exactly(9, 0xB400FF, {4, 9}), "t=12020 set 1: stick 10 now runs the reverse runner (purple on LEDs 5 and 10)");
    }
    if (t == 120000) CHECK(true, "end");
  }
  Serial.capture = false;
  CHECK(spotOk, "120 s: the spotlight (stick (t/1200)%11) always has all 10 LEDs white");
  CHECK(framesOk, "120 s: every other stick always shows 1..6 LEDs (a pattern is always moving, never dark or full)");
  CHECK(changed && !stuck, "120 s: the picture changes from second to second");
  CHECK(maxBrightnessSeen == 128, "the brightness never exceeds 128 (and is 128)");
  long overflow = 0; for (auto *st : g_strips) overflow += st->overflow;
  CHECK(overflow == 0, "no pixel written outside a stick");

  // --- sign of life: LED toggles all the time; calm stretch = slower
  CHECK(toggles > 600 && toggles < 1500, "the debug LED toggled between 600 and 1500 times in 120 s");
  CHECK(togglesQuiet >= 2 && togglesQuiet <= 4, "calm stretch (4.0-4.8 s): the LED toggles every 250 ms (2-4 times)");

  // --- serial debug: one DBG line per second, changing values
  const std::string &log = Serial.captured;
  int dbg = 0; size_t pos = 0;
  while ((pos = log.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
  CHECK(dbg == 120, "120 DBG lines in 120 s (one per second)");
  CHECK(log.find("DBG up=1s fps=50 spot=1 set=0 ") != std::string::npos, "DBG line at 1 s: fps=50, spot=1, set=0");
  CHECK(log.find("DBG up=60s fps=50 spot=7 set=6 ") != std::string::npos, "DBG line at 60 s: spot=7, set=6");
  CHECK(log.find("DBG up=2s") != std::string::npos && log.find("DBG up=3s") != std::string::npos, "the uptime counts up");
  {
    size_t a10 = log.find("DBG up=10s"), a20 = log.find("DBG up=20s");
    std::string l10 = log.substr(a10, log.find('\n', a10) - a10), l20 = log.substr(a20, log.find('\n', a20) - a20);
    CHECK(l10.find(" events=9 ") != std::string::npos, "10 s: 9 state changes so far (8 spotlight moves + 1 pattern shift)");
    CHECK(l20.find(" events=18 ") != std::string::npos, "20 s: 18 state changes (16 spotlight moves + 2 pattern shifts)");
  }
  CHECK(log.find(" events=") != std::string::npos && log.find(" beats=") != std::string::npos && log.find(" load=0") != std::string::npos,
        "DBG line has events, beats and load");

  // --- load test: holding the button lights everything white and speeds up the LED
  g_pinLevel[22] = LOW;
  int t0 = 0; lastPin = g_pinOut[13];
  for (int k = 0; k < 1000; k++) { run(1); if (g_pinOut[13] != lastPin) { t0++; lastPin = g_pinOut[13]; } }
  bool allOn = true; for (int i = 0; i < 11; i++) if (!allWhite(i)) allOn = false;
  CHECK(allOn, "button held: all 11 sticks all white");
  CHECK(t0 >= 8 && t0 <= 11, "button held: the LED blinks twice as fast (8-11 toggles in a second)");
  g_pinLevel[22] = HIGH; run(100);
  int whiteSticks = 0; for (int i = 0; i < 11; i++) if (allWhite(i)) whiteSticks++;
  CHECK(whiteSticks == 1, "button released: only the spotlight stick is white again");

  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
