// Host-side test of strip_frame_demo_test/sketch.ino. Expected values are hard-coded.
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/sf_harness tests/strip_frame_demo_test_harness.cpp && /tmp/sf_harness
#include <algorithm>
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
#include "../strip_frame_demo_test/sketch.ino"
#endif

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { if (fails < 25) printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)
static void run(unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } }
static uint32_t rgb(int r, int g, int b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b; }

// light route (clockwise from the upper-left corner), as 0-based strip pixels: strip pixels 15..10, 9..7, 6..1, 18..16
static const int ROUTE[18] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 17, 16, 15};
// expected strip pixel (0-based) of a light that makes one step every stepMs: bounces over the route 0..17..0, 34 steps per cycle
static int expectPixel(unsigned long t, unsigned long stepMs) { int s = (int)((t / stepMs) % 34); return ROUTE[s < 18 ? s : 34 - s]; }
// D10: 150 updates of 20 ms per pulse, peak 80 after 75 updates, palette entries (full scale values)
static const int PAL[8][3] = {{255,0,0},{255,100,0},{255,255,0},{0,255,0},{0,255,255},{0,0,255},{160,0,255},{255,255,255}};
static int expectLevel(unsigned long t) { int k = (int)((t / 20) % 150); return k < 75 ? k * 80 / 75 : (150 - k) * 80 / 75; }
// D11: red, green, blue, cyan, violet, white at peak 80 (violet 160 * 80 / 255 = 50); the sequence moves one pixel per second
static const uint32_t BARC[6] = {(80u << 16), (80u << 8), 80u, (80u << 8) | 80u, (50u << 16) | 80u, (80u << 16) | (80u << 8) | 80u};
static uint32_t expectBar(unsigned long t, int pixel) { int shift = (int)((t / 1000) % 6); return BARC[(pixel - shift + 6) % 6]; }
static uint32_t expectSingle(unsigned long t) {
  const int *c = PAL[(t / 3000) % 8]; int lv = expectLevel(t);
  return rgb(c[0] * lv / 255, c[1] * lv / 255, c[2] * lv / 255);
}

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();
  Serial.capture = false;
  CHECK(g_strips.size() == 3, "three strips");
  Adafruit_NeoPixel *f = g_strips[0], *s = g_strips[1], *bar_ = g_strips[2];
  CHECK(f->pin_ == 9 && s->pin_ == 10 && bar_->pin_ == 11, "frame strip on D9, single pixel on D10, colour bar on D11");
  CHECK(f->numPixels() == 18 && s->numPixels() == 1 && bar_->numPixels() == 6, "18 pixels on D9, 1 pixel on D10, 6 pixels on D11");
  CHECK(f->type_ == (NEO_GRB | NEO_KHZ800) && s->type_ == (NEO_GRB | NEO_KHZ800) && bar_->type_ == (NEO_GRB | NEO_KHZ800), "GRB, 800 kHz");
  CHECK(Serial.baud == 115200 && Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "baud rate 115200 stated and printed");
  CHECK(f->shown_[14] == rgb(80, 80, 80), "at start all three lights are on strip pixel 15 (upper-left corner): white");
  for (int i = 0; i < 18; i++) if (i != 14) CHECK(f->shown_[i] == 0, "at start only strip pixel 15 is lit");
  CHECK(s->shown_[0] == 0, "D10 starts dark");
  for (int i = 0; i < 6; i++) CHECK(bar_->shown_[i] == BARC[i], "D11 starts with red, green, blue, cyan, violet, white on pixels 1..6");

  bool sawYellow = false, sawCyan = false, sawMagenta = false, sawWhite = false, farEnd = false, back = false;
  int maxChan = 0; bool colourSeen[8] = {false, false, false, false, false, false, false, false};
  // 600 s sampled every 20 ms
  for (unsigned long t = 20; t <= 600000; t += 20) {
    run(20);
    const int pr = expectPixel(t, 500), pg = expectPixel(t, 385), pb = expectPixel(t, 333);
    bool ok = true;
    for (int i = 0; i < 18; i++) {
      const uint32_t want = rgb(i == pr ? 80 : 0, i == pg ? 80 : 0, i == pb ? 80 : 0);
      if (f->shown_[i] != want) ok = false;
    }
    CHECK(ok, "D9: red 500 ms, green 385 ms, blue 333 ms per step, colours added where they overlap");
    if (pr == pg && pr != pb) sawYellow = true;
    if (pg == pb && pg != pr) sawCyan = true;
    if (pr == pb && pr != pg) sawMagenta = true;
    if (pr == pg && pg == pb && t > 100) sawWhite = true;
    if (pr == 15) farEnd = true;                 // strip pixel 16: the end of the route
    if (farEnd && pr == 16) back = true;         // strip pixel 17: one step back
    CHECK(s->shown_[0] == expectSingle(t), "D10: palette colour scaled by the pulse brightness, new colour every 3 s");
    { bool okb = true; for (int i = 0; i < 6; i++) if (bar_->shown_[i] != expectBar(t, i)) okb = false;
      CHECK(okb, "D11: the colour sequence moves one pixel per second towards pixel 6 and wraps round"); }
    const uint32_t v = s->shown_[0];
    const int mc = (int)std::max({(v >> 16) & 255, (v >> 8) & 255, v & 255});
    if (mc > maxChan) maxChan = mc;
    colourSeen[(t / 3000) % 8] = true;
    // the LED is inverted for 120 ms after red reaches the end of the route, strip pixel 16 (step 17 at 8500 ms); no other event is near
    if (t == 8520) CHECK((g_pinOut[13] == HIGH) != ledState, "LED 13 is inverted right after a turnaround");
    if (t == 8640) CHECK((g_pinOut[13] == HIGH) == ledState, "LED 13 is back to normal 140 ms later");
    // and after green is back at the start of the route (step 34 at 13090 ms); no other event is near
    if (t == 13120) CHECK((g_pinOut[13] == HIGH) != ledState, "LED 13 is inverted right after a light is back at the start of the route");
    if (t == 13240) CHECK((g_pinOut[13] == HIGH) == ledState, "LED 13 is back to normal after that");
    // and right after a new pulse colour starts (6000 ms)
    if (t == 6020) CHECK((g_pinOut[13] == HIGH) != ledState, "LED 13 is inverted right after the pulse colour changes");
    if (t == 6140) CHECK((g_pinOut[13] == HIGH) == ledState, "LED 13 is back to normal after the colour change");
  }
  CHECK(farEnd && back, "the red light reaches the end of the route (strip pixel 16) and comes back");
  CHECK(sawYellow && sawCyan && sawMagenta, "pairs of lights overlap: yellow, cyan and magenta are seen");
  CHECK(sawWhite, "all three lights meet on one pixel: white");
  CHECK(maxChan == 80, "peak brightness is 80");
  for (int i = 0; i < 8; i++) CHECK(colourSeen[i], "all eight palette colours are used");

  // sign of life: LED 13 toggles all the time, DBG line once per second with changing values
  Serial.capture = true; Serial.captured.clear();
  int toggles = 0, heartbeats = 0; uint8_t prev = g_pinOut[13]; bool prevState = ledState;
  for (int k = 0; k < 20000; k++) { run(1); if (g_pinOut[13] != prev) toggles++; prev = g_pinOut[13]; if (ledState != prevState) heartbeats++; prevState = ledState; }
  Serial.capture = false;
  CHECK(toggles > 60, "status LED 13 blinks (at least 3 toggles per second)");
  CHECK(heartbeats >= 75 && heartbeats <= 165, "the heartbeat toggles every 250 ms (or 125 ms when busy): 75-165 toggles in 20 s");
  size_t pos = 0; int dbg = 0; while ((pos = Serial.captured.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
  CHECK(dbg == 20, "20 DBG lines in 20 s");
  CHECK(Serial.captured.find(" red=") != std::string::npos && Serial.captured.find(" green=") != std::string::npos && Serial.captured.find(" blue=") != std::string::npos && Serial.captured.find(" d10=") != std::string::npos && Serial.captured.find(" events=") != std::string::npos, "DBG line has red, green, blue, d10 and events");
  {
    const size_t at = Serial.captured.rfind("beats=");
    const long beatsSeen = at == std::string::npos ? -1 : atol(Serial.captured.c_str() + at + 6);
    CHECK(beatsSeen >= 2400 && beatsSeen <= 4800, "heartbeat toggles counted in the last DBG line (620 s: 2480 at the slow rate)");
  }
  for (auto *p : g_strips) CHECK(p->overflow == 0, "no pixel written beyond the strip");
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
