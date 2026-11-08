// Host-side test of sticks_11_test/sketch.ino. Expected values are hard-coded.
//   g++ -std=c++17 -O1 -w -Itests -o /tmp/s11 tests/sticks_11_test_harness.cpp && /tmp/s11
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
#include "../sticks_11_test/sketch.ino"
#endif

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { if (fails < 25) printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)
static void run(unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } }
static uint32_t grey(int v) { return ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v; }
// expected brightness of LED `led` of arrow stick `s` (0-based) at frame `tick`: triangle wave, phase = led*28 + s*37 - tick*6 (mod 256), scaled to 80
static int expectArrow(int s, int led, long tick) {
  int phase = (int)((((long)led * 28 + (long)s * 37 - tick * 6) % 256 + 256) % 256);
  int tri = phase < 128 ? phase * 2 : (255 - phase) * 2;
  return tri * 80 / 255;
}

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();
  Serial.capture = false;
  CHECK(g_strips.size() == 11, "eleven sticks");
  const int pins[11] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3};
  for (int s = 0; s < 11; s++) CHECK(g_strips[s]->pin_ == pins[s] && g_strips[s]->numPixels() == 10, "stick on the expected connector, 10 LEDs");
  CHECK(A3 == 57 && g_strips[10]->pin_ == 57, "the 11th stick (CLOSED light) is on A3");
  CHECK(g_strips[0]->type_ == (NEO_GRB | NEO_KHZ800), "GRB, 800 kHz");
  CHECK(Serial.baud == 115200 && Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "baud rate 115200 stated and printed");
  CHECK(Serial.captured.find("Estimated current now:") != std::string::npos, "start-up current estimate printed");
  CHECK(g_strips[10]->shown_[0] == grey(80), "lit at once at power-up (no dark pause)");

  // FLOW mode, 4 s sampled every 20 ms (frame number = t / 20): CLOSED stick steady white 80, arrow sticks a flowing wave
  long maxLit = 0, minLit = 1000;
  for (unsigned long t = 20; t <= 4000; t += 20) {
    run(20);
    const long tickNow = t / 20;
    long lit = 0;
    for (int s = 0; s < 11; s++) for (int i = 0; i < 10; i++) {
      const uint32_t v = g_strips[s]->shown_[i];
      if (v) lit++;
      const uint32_t want = grey(s == 10 ? 80 : expectArrow(s, i, tickNow));
      if (v != want) { CHECK(false, "pixel colour matches the expected wave / steady CLOSED light"); s = 11; break; }
    }
    maxLit = lit > maxLit ? lit : maxLit; minLit = lit < minLit ? lit : minLit;
  }
  CHECK(minLit >= 10 && maxLit <= 110 && maxLit - minLit < 50, "CLOSED light always lit (10 LEDs) and about half of the arrow LEDs");
  { long sum = 0, mx = 0, mn = 255; for (int s = 0; s < 10; s++) for (int i = 0; i < 10; i++) { long v = g_strips[s]->shown_[i] & 255; sum += v; if (v > mx) mx = v; if (v < mn) mn = v; }
    CHECK(sum >= 100 * 25 && sum <= 100 * 55 && mx - mn >= 30, "the wave: average arrow LED brightness about half of 80, with bright and dim LEDs"); }

  // button: a press (100 ms) switches to ALL ON: all 110 LEDs white 80; another press goes back to FLOW
  g_pinLevel[33] = LOW; run(100);
  CHECK(g_strips[0]->shown_[0] == grey(80) && g_strips[5]->shown_[3] == grey(80), "the mode changes while the button is pressed (not on release)");
  g_pinLevel[33] = HIGH; run(40);
  for (int s = 0; s < 11; s++) for (int i = 0; i < 10; i++) if (g_strips[s]->shown_[i] != grey(80)) { CHECK(false, "ALL ON: every LED white 80"); s = 11; break; }
  CHECK((g_pinOut[13] == HIGH) != beatOn, "LED 13 is inverted right after the mode change");
  run(300);
  CHECK((g_pinOut[13] == HIGH) == beatOn, "LED 13 is back to normal after the inversion");
  g_pinLevel[33] = LOW; run(100); g_pinLevel[33] = HIGH; run(40);
  { long lit = 0; for (int s = 0; s < 10; s++) for (int i = 0; i < 10; i++) if (g_strips[s]->shown_[i]) lit++; CHECK(lit < 100, "second press: back to FLOW (not every arrow LED lit)"); }
  CHECK(g_strips[10]->shown_[5] == grey(80), "CLOSED light still steady white after the mode changes");
  // a short bounce (5 ms) is not a press
  { const long lit0 = 0; (void)lit0; g_pinLevel[33] = LOW; run(5); g_pinLevel[33] = HIGH; run(100);
    long lit = 0; for (int s = 0; s < 10; s++) for (int i = 0; i < 10; i++) if (g_strips[s]->shown_[i]) lit++;
    CHECK(lit < 100, "a 5 ms blip of the button does not switch the mode"); }

  // sign of life: LED 13 toggles all the time, DBG line once per second with changing values, mode shown
  Serial.capture = true; Serial.captured.clear();
  int toggles = 0, heartbeats = 0; uint8_t prev = g_pinOut[13]; bool prevState = beatOn;
  for (int k = 0; k < 20000; k++) { run(1); if (g_pinOut[13] != prev) toggles++; prev = g_pinOut[13]; if (beatOn != prevState) heartbeats++; prevState = beatOn; }
  Serial.capture = false;
  CHECK(toggles > 60, "status LED 13 blinks (at least 3 toggles per second)");
  CHECK(heartbeats >= 75 && heartbeats <= 85, "the heartbeat toggles every 250 ms: about 80 toggles in 20 s");
  size_t pos = 0; int dbg = 0; while ((pos = Serial.captured.find("DBG up=", pos)) != std::string::npos) { dbg++; pos++; }
  CHECK(dbg == 20, "20 DBG lines in 20 s");
  CHECK(Serial.captured.find(" mode=flow") != std::string::npos && Serial.captured.find(" fps=50") != std::string::npos && Serial.captured.find(" lit=") != std::string::npos && Serial.captured.find(" ma=") != std::string::npos && Serial.captured.find(" events=") != std::string::npos && Serial.captured.find(" beats=") != std::string::npos, "DBG line has mode, fps=50, lit, ma, events and beats");
  {
    const size_t at = Serial.captured.rfind(" ma=");
    const long ma = at == std::string::npos ? -1 : atol(Serial.captured.c_str() + at + 4);
    // CLOSED stick alone: 10 LEDs x 3 channels x 80 x 20 mA / 255 = 188 mA; the wave adds up to 100 LEDs x the same 18.8 mA each
    CHECK(ma >= 188 && ma <= 2070, "estimated current between the CLOSED light alone and everything on");
    const size_t b = Serial.captured.rfind("beats=");
    const long beatsSeen = b == std::string::npos ? -1 : atol(Serial.captured.c_str() + b + 6);
    CHECK(beatsSeen >= 90 && beatsSeen <= 110, "heartbeat toggles counted (about 4 per second, 25 s so far)");
  }
  // ALL ON estimate: 110 LEDs x 3 x 80 x 20 / 255 = 2070 mA
  g_pinLevel[33] = LOW; run(100); g_pinLevel[33] = HIGH; run(1100);
  Serial.capture = true; Serial.captured.clear(); run(1000); Serial.capture = false;
  CHECK(Serial.captured.find(" mode=all") != std::string::npos && Serial.captured.find(" lit=110") != std::string::npos && Serial.captured.find(" ma=2070") != std::string::npos, "ALL ON: 110 LEDs lit, 2070 mA estimated");
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no pixel written beyond the stick");
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
