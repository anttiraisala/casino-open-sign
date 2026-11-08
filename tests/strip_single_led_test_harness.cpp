// Host-side test of strip_single_led_test/sketch.ino. Expected values are hard-coded.
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/ssl_harness tests/strip_single_led_test_harness.cpp && /tmp/ssl_harness
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
std::vector<Adafruit_NeoPixel*> g_strips;
#ifdef SKETCH_PATH
#include SKETCH_PATH
#else
#include "../strip_single_led_test/sketch.ino"
#endif

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)
static void run(unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } }
static const uint32_t W = (80u << 16) | (80u << 8) | 80u, R = 80u << 16, G = 80u << 8, B = 80u;

static int litCount(Adafruit_NeoPixel *s, int *where) {
  int n = 0; for (size_t i = 0; i < s->shown_.size(); i++) if (s->shown_[i]) { n++; *where = (int)i; } return n;
}

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();
  Serial.capture = false;
  CHECK(g_strips.size() == 3, "three strips");
  Adafruit_NeoPixel *a = g_strips[0], *b = g_strips[1], *c = g_strips[2];
  CHECK(a->pin_ == 9 && b->pin_ == 10 && c->pin_ == 11, "tests 1, 2, 3 are on D9, D10, D11");
  CHECK(a->numPixels() == 150 && b->numPixels() == 150 && c->numPixels() == 150, "150 pixels each");
  CHECK(a->type_ == (NEO_GRB | NEO_KHZ800) && b->type_ == (NEO_GRB | NEO_KHZ800), "GRB, 800 kHz");
  CHECK(Serial.baud == 115200 && Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "baud rate 115200 stated and printed");

  // All three tests are sampled every 30 ms for 50 s (the sketch's timers start at t = 0):
  //   test 1: one white pixel, position (t / 150 ms) mod 150
  //   test 2: one pixel, position (t / 60 ms) mod 150, colour by pass = (t / 9000 ms) mod 4: red, green, blue, white
  //   test 3: ruler on while (t / 500 ms) is even: 15 pixels (every 10th blue, every 50th white), else dark
  {
    const uint32_t colours[4] = {R, G, B, W};
    int onSamples = 0, offSamples = 0;
    for (unsigned long t = 30; t <= 50000; t += 30) {
      run(30);
      int where = -1, n = litCount(a, &where);
      CHECK(n == 1 && where == (int)((t / 150) % 150) && a->shown_[where] == W, "test 1: one white pixel at the expected position");
      n = litCount(b, &where);
      CHECK(n == 1 && where == (int)((t / 60) % 150), "test 2: one pixel at the expected position");
      if (n == 1) CHECK(b->shown_[where] == colours[(t / 9000) % 4], "test 2: colour of the pass (red, green, blue, white, red)");
      int lit = 0, wrong = 0;
      for (int i = 0; i < 150; i++) if (c->shown_[i]) { lit++; const bool big = i % 50 == 0; if (!(i % 10 == 0 && c->shown_[i] == (big ? W : B))) wrong++; }
      const bool shouldBeOn = ((t / 500) % 2) == 0;
      CHECK(wrong == 0 && lit == (shouldBeOn ? 15 : 0), "test 3: ruler blinks every 500 ms with the right pixels");
      if (lit) onSamples++; else offSamples++;
    }
    CHECK(onSamples > 700 && offSamples > 700, "test 3: ruler is both on and off");
  }
  // sign of life: LED 13 toggles all the time, DBG line once per second with changing values
  Serial.capture = true; Serial.captured.clear();
  int toggles = 0, heartbeats = 0; uint8_t prev = g_pinOut[13]; bool prevState = ledState;
  for (int k = 0; k < 20000; k++) { run(1); if (g_pinOut[13] != prev) toggles++; prev = g_pinOut[13]; if (ledState != prevState) heartbeats++; prevState = ledState; }
  Serial.capture = false;
  CHECK(toggles > 60, "status LED 13 blinks (at least 3 toggles per second)");
  CHECK(heartbeats >= 75 && heartbeats <= 165, "the heartbeat toggles every 250 ms (125 ms while busy): 75-165 toggles in 20 s");
  size_t pos = 0; int dbg = 0; while ((pos = Serial.captured.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
  CHECK(dbg == 20, "20 DBG lines in 20 s");
  CHECK(Serial.captured.find(" t1=") != std::string::npos && Serial.captured.find(" events=") != std::string::npos && Serial.captured.find(" beats=") != std::string::npos, "DBG line has t1, events and beats");
  {
    std::string l1 = Serial.captured.substr(Serial.captured.find("DBG up="), 30);
    CHECK(Serial.captured.find("DBG up=", Serial.captured.find("DBG up=") + 5) != std::string::npos && l1.size() > 0, "more than one DBG line");
  }
  {
    // the heartbeat: 250 ms per toggle (125 ms while busy), so 70 s give 280 to 560 toggles; the last DBG line says how many
    const size_t at = Serial.captured.rfind("beats=");
    const long beatsSeen = at == std::string::npos ? -1 : atol(Serial.captured.c_str() + at + 6);
    CHECK(beatsSeen >= 250 && beatsSeen <= 560, "heartbeat toggles counted in the last DBG line (about 4 per second since the start, 70 s)");
    CHECK(Serial.captured.find("events=") != std::string::npos, "events are reported");
  }
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no pixel written beyond the strip");
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
