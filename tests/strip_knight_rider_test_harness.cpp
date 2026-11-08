// Host-side test of strip_knight_rider_test/sketch.ino. Expected values are hard-coded.
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/kr_harness tests/strip_knight_rider_test_harness.cpp && /tmp/kr_harness
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
#include "../strip_knight_rider_test/sketch.ino"
#endif

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { if (fails < 25) printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)
static void run(unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } }
static const uint32_t R = 80u << 16, G = 80u << 8, B = 80u;

static int litCount(Adafruit_NeoPixel *s, int *where) {
  int n = 0; for (size_t i = 0; i < s->shown_.size(); i++) if (s->shown_[i]) { n++; *where = (int)i; } return n;
}
// expected pixel (0-based) of a light that makes one step every stepMs: bounces 0..49..0, 98 steps per cycle
static int expectPixel(unsigned long t, unsigned long stepMs) { int s = (int)((t / stepMs) % 98); return s < 50 ? s : 98 - s; }

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();
  Serial.capture = false;
  CHECK(g_strips.size() == 3, "three strips");
  Adafruit_NeoPixel *a = g_strips[0], *b = g_strips[1], *c = g_strips[2];
  CHECK(a->pin_ == 9 && b->pin_ == 10 && c->pin_ == 11, "strips on D9, D10, D11");
  CHECK(a->numPixels() == 50 && b->numPixels() == 50 && c->numPixels() == 50, "50 pixels each");
  CHECK(a->type_ == (NEO_GRB | NEO_KHZ800), "GRB, 800 kHz");
  CHECK(Serial.baud == 115200 && Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "baud rate 115200 stated and printed");
  { int w = -1; CHECK(litCount(a, &w) == 1 && w == 0 && a->shown_[0] == R, "D9 starts at pixel 1, red"); }

  // 205 s sampled every 50 ms: D9 red 1 pixel/s, D10 blue 4 pixels/s, D11 green 10 pixels/s, one lit pixel each
  bool d9Top = false, d9Back = false, d11Top = false;
  for (unsigned long t = 50; t <= 205000; t += 50) {
    run(50);
    int w = -1;
    CHECK(litCount(a, &w) == 1 && w == expectPixel(t, 1000) && a->shown_[w] == R, "D9: one red pixel, 1 per second, bouncing");
    CHECK(litCount(b, &w) == 1 && w == expectPixel(t, 250) && b->shown_[w] == B, "D10: one blue pixel, 4 per second, bouncing");
    CHECK(litCount(c, &w) == 1 && w == expectPixel(t, 100) && c->shown_[w] == G, "D11: one green pixel, 10 per second, bouncing");
    if (expectPixel(t, 1000) == 49) d9Top = true;
    if (d9Top && expectPixel(t, 1000) == 48) d9Back = true;
    if (expectPixel(t, 100) == 49) d11Top = true;
    // the LED is inverted for 120 ms after a turnaround of D11 (pixel 50 reached at 4900 ms)
    if (t == 4950) CHECK((g_pinOut[13] == HIGH) != ledState, "LED 13 is inverted right after a turnaround");
    // and after D11 is back at pixel 1 (step 98, 9800 ms)
    if (t == 9850) CHECK((g_pinOut[13] == HIGH) != ledState, "LED 13 is inverted right after the light is back at pixel 1");
    if (t == 9950) CHECK((g_pinOut[13] == HIGH) == ledState, "LED 13 is back to normal after that");
    if (t == 5050) CHECK((g_pinOut[13] == HIGH) == ledState, "LED 13 is back to normal 150 ms later");
  }
  CHECK(d9Top && d9Back && d11Top, "the lights reach the far end and come back");

  // sign of life: LED 13 toggles all the time, DBG line once per second with changing values
  Serial.capture = true; Serial.captured.clear();
  int toggles = 0, heartbeats = 0; uint8_t prev = g_pinOut[13]; bool prevState = ledState;
  for (int k = 0; k < 20000; k++) { run(1); if (g_pinOut[13] != prev) toggles++; prev = g_pinOut[13]; if (ledState != prevState) heartbeats++; prevState = ledState; }
  Serial.capture = false;
  CHECK(toggles > 60, "status LED 13 blinks (at least 3 toggles per second)");
  CHECK(heartbeats >= 75 && heartbeats <= 165, "the heartbeat toggles every 250 ms: 75-165 toggles in 20 s");
  size_t pos = 0; int dbg = 0; while ((pos = Serial.captured.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
  CHECK(dbg == 20, "20 DBG lines in 20 s");
  CHECK(Serial.captured.find(" d9=") != std::string::npos && Serial.captured.find(" d10=") != std::string::npos && Serial.captured.find(" d11=") != std::string::npos && Serial.captured.find(" events=") != std::string::npos, "DBG line has d9, d10, d11 and events");
  {
    // 225 s at 250 ms per toggle = 900 toggles (the lights turn around rarely, so the LED is never "busy")
    const size_t at = Serial.captured.rfind("beats=");
    const long beatsSeen = at == std::string::npos ? -1 : atol(Serial.captured.c_str() + at + 6);
    CHECK(beatsSeen >= 850 && beatsSeen <= 950, "heartbeat toggles counted in the last DBG line");
  }
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no pixel written beyond the strip");
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
