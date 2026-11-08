#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "Wire.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
MockSerial Serial;
uint8_t g_pinOut[64];
TwoWire Wire;
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
static const char* kStripNames[] = {"frameCw", "frameCcw", "letters", "closedLight", "L-CAP", "L-WING-UP", "L-WING-DN",
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
  uint16_t pos = 0; for (uint8_t i = 0; i < l; i++) pos += LETTER_LEDS[i];
  return (let->shown_[pos] >> 16) & 0xFF;
}

int main(int argc, char **argv) {
  FILE *out = argc > 1 ? fopen(argv[1], "w") : nullptr;
  Serial.capture = true;                                // everything printed is kept for the sign-of-life checks
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);          // pull-ups: open switch/button read HIGH
  g_pinLevel[PIN_ARROW_SWITCH] = SW_RIGHT;             // slide switch handle right -> arrow points right
  auto setCode = [&](const uint8_t *pins, int code) {
    for (int b = 0; b < 4; b++) g_pinLevel[pins[b]] = (((code >> b) & 1) == 1) == SPEED_BIT_IS_LOW ? LOW : HIGH;
  };
  setCode(PIN_SPEED_FRAME_ARROW, 8);                    // both speed switches at position 8 (about x1.1)
  setCode(PIN_SPEED_LETTERS, 8);
  setup();
  CHECK(startupTestActive, "the power-on LED test starts at power-up");
  // The older tests below check the colours the sketch computes, so they run with an identity curve (min 0,
  // max 255, exponent 1, mix 0). The default curves are tested separately at the end.
  CHECK(brightnessLut[0][128] == 13 && brightnessLut[1][128] == 13 && brightnessLut[0][255] == 80 && brightnessLut[1][192] == 38, "setup() builds the default tables");
  for (auto *s : g_strips) CHECK(s->brightness_ == 255, "no global library brightness: the curve maximum is the limit");
  CHECK(CURVE_DEFAULT_MIN[0] == 0 && CURVE_DEFAULT_MIN[1] == 0 && CURVE_DEFAULT_MAX[0] == 80 && CURVE_DEFAULT_MAX[1] == 80 &&
        CURVE_DEFAULT_EXPONENT[0] == 2.67f && CURVE_DEFAULT_EXPONENT[1] == 2.67f && CURVE_DEFAULT_MIX[0] == 1.0f && CURVE_DEFAULT_MIX[1] == 1.0f &&
        CURVE_DEFAULT_KEEP_ZERO[0] && CURVE_DEFAULT_KEEP_ZERO[1], "default curve values: min 0, max 80, exponent 2.67, mix 1, keep zero dark");
  const uint8_t IDENT_TYPES[2] = {0, 1};
  for (uint8_t ty : IDENT_TYPES) setBrightnessCurve(ty, 0, 255, 1.0f, 0.0f, true);
  finishStartup(g_millis, true);   // the older tests below start from the normal operation
  // Pins hard-coded here (not taken from the sketch): frame D9/D10, letters D11, CLOSED light = 11th stick on A3 (57),
  // sticks D2..D8 and A0..A2 (A0 = 54 on the Mega), arrow switch D32, mode button D33.
  CHECK(PIN_FRAME_CW == 9 && PIN_FRAME_CCW == 10 && PIN_LETTERS == 11 && PIN_CLOSED_LIGHT == 57, "strip pins");
  for (auto *s : g_strips) CHECK(s->pin_ != 31, "pin 31 is free: nothing is on D31 any more");
  CHECK(PIN_ARROW_SWITCH == 32 && PIN_MODE_BUTTON == 33 && PIN_CASINO_CLOSED_SWITCH == 30, "input pins");
  const int wantStickPins[10] = {2, 3, 4, 5, 6, 7, 8, 54, 55, 56};
  Adafruit_NeoPixel *fcw=nullptr,*fccw=nullptr,*let=nullptr,*clo=nullptr;
  Adafruit_NeoPixel *st[10] = {};          // the sticks in the order L-CAP, L-WING-UP, L-WING-DN, RAIL-UP-L, RAIL-UP-R,
                                           // RAIL-DN-L, RAIL-DN-R, R-CAP, R-WING-UP, R-WING-DN
  for (auto *s : g_strips) {
    if (s->pin_ == 9) fcw = s; else if (s->pin_ == 10) fccw = s; else if (s->pin_ == 11) let = s; else if (s->pin_ == 57) clo = s;
    for (int k = 0; k < 10; k++) if (s->pin_ == wantStickPins[k]) st[k] = s;
  }
  { bool allThere = fcw && fccw && let && clo; for (int k = 0; k < 10; k++) allThere = allThere && st[k];
    if (!allThere) { printf("RESULT: pins do not match, stopping (1 failures)\n"); return 1; } }
  CHECK(g_strips.size() == 14 && STRIP_COUNT == 14, "14 strips: frame x2, letters, CLOSED light stick (A3), 10 arrow sticks");
  CHECK(fcw && fccw && let && clo, "frame, letters and CLOSED strips exist on the expected pins");
  for (int k = 0; k < 10; k++) CHECK(st[k] != nullptr && st[k]->numPixels() == 10, "every stick exists on its pin and has 10 LEDs");
  CHECK(clo && clo->numPixels() == 10 && CLOSED_LIGHT_LEDS == 10, "closed light is a 10-LED Grove stick");
  CHECK(fcw->numPixels()==FRAME_HALF_LEDS && fccw->numPixels()==FRAME_HALF_LEDS, "frame strip lengths");
  CHECK(let->numPixels()==lettersTotalLeds(), "letters length");
  CHECK(arrowPointsRight, "arrow starts pointing right with the switch released (HIGH)");
  CHECK(g_analogPin == 69, "the random seed is read from the free pin A15 (A0 is a stick pin now)");

  // Timeline: mode k is active from MODE_MS*k. The button is pressed (with contact bounce) just before.
  const unsigned long MODE_MS = 14000, END = MODE_MS * LETTER_MODE_COUNT + 2000;
  // Arrow switch: LOW (left) at 5 s, HIGH (right) at 25 s, short glitch at 45 s, bouncing change at 60 s.
  long lastFrameShows = fcw->shows, lastArrowShows = 0; long arrowFlowChecks = 0;
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
      if (g_millis >= t0 && g_millis < t0 + 120) g_pinLevel[PIN_MODE_BUTTON] = (g_millis < t0 + 8 && (g_millis & 1)) ? HIGH : LOW;
      else if (g_millis == t0 + 120) g_pinLevel[PIN_MODE_BUTTON] = HIGH;
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
    if (fcw->shows != lastFrameShows) {
      lastFrameShows = fcw->shows;
      // Rebuild the two light paths (start corner -> end corner) from the strips.
      const bool mir = !arrowPointsRight;
      std::vector<int> A, B;
      auto lit = [&](Adafruit_NeoPixel *st, size_t i) { return (int)(st->shown_[i] != 0); };
      for (auto *st : {fcw, fccw}) for (auto c : st->shown_) CHECK(c == 0 || c == 0xffffff, "frame colours black/white only");
      if (!mir) {
        for (size_t i = 0; i < FRAME_HALF_LEDS; i++) { A.push_back(lit(fcw, i)); B.push_back(lit(fccw, i)); }
      } else {
        for (int i = FRAME_TOP_LEDS - 1; i >= 0; i--) A.push_back(lit(fcw, i));              // top edge, right to left
        for (size_t j = 0; j < FRAME_SIDE_LEDS; j++) A.push_back(lit(fccw, j));              // left edge, downward
        for (size_t k = 0; k < FRAME_SIDE_LEDS; k++) B.push_back(lit(fcw, FRAME_TOP_LEDS + k)); // right edge, downward
        for (int m = FRAME_TOP_LEDS - 1; m >= 0; m--) B.push_back(lit(fccw, FRAME_SIDE_LEDS + m)); // bottom edge, right to left
      }
      CHECK(A.size() == FRAME_HALF_LEDS && B.size() == FRAME_HALF_LEDS, "frame paths have HALF LEDs");
      CHECK(A == B, "both frame light paths show the same pattern");
      size_t i = 0;
      while (i < A.size()) { size_t j = i; while (j < A.size() && A[j] == A[i]) j++;
        if (i > 0 && j < A.size()) CHECK(j - i == FRAME_BAR_LEDS, "frame run length");
        i = j; }
      if (!prevPath.empty() && prevMirrored == mir) {
        bool shifted = true;
        for (size_t k = 1; k < A.size(); k++) if (A[k] != prevPath[k-1]) shifted = false;
        CHECK(shifted, "frame pattern advances one LED per step toward the end corner");
      }
      prevPath = A; prevMirrored = mir;
      sawMirrored = sawMirrored || mir; sawNormal = sawNormal || !mir;
    }
    // Letters: every letter section uniformly coloured; sections cover the whole strip.
    {
      uint16_t pos = 0;
      for (uint8_t l = 0; l < LETTER_COUNT; l++) {
        for (uint8_t k = 1; k < LETTER_LEDS[l]; k++) CHECK(let->shown_[pos+k] == let->shown_[pos], "letter section uniform");
        pos += LETTER_LEDS[l];
      }
      CHECK(pos == let->numPixels(), "letter sections cover the strip");
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
  // ---- Speed switch tests (continue running the same sketch) ----
  auto run = [&](unsigned long ms) { for (unsigned long k = 0; k < ms; k++) { g_millis++; loop(); } };
  const int pairs[][2] = {{0, 15}, {15, 0}, {5, 10}, {8, 8}};
  for (auto &pr : pairs) {
    setCode(PIN_SPEED_FRAME_ARROW, pr[0]);
    setCode(PIN_SPEED_LETTERS, pr[1]);
    run(300);
    CHECK(speedStable[SLOT_SPEED_MAIN] == pr[0] && speedStable[SLOT_SPEED_LETTERS] == pr[1], "speed switch positions are read");
    const double mMain = SPEED_MIN_MULTIPLIER * pow(SPEED_MAX_MULTIPLIER / SPEED_MIN_MULTIPLIER, pr[0] / 15.0);
    const double mLet = SPEED_MIN_MULTIPLIER * pow(SPEED_MAX_MULTIPLIER / SPEED_MIN_MULTIPLIER, pr[1] / 15.0);
    double wantFrame = FRAME_SPEED_MS / mMain, wantArrow = ARROW_SPEED_MS / mMain;
    if (wantFrame < MIN_UPDATE_MS) wantFrame = MIN_UPDATE_MS;
    if (wantArrow < MIN_UPDATE_MS) wantArrow = MIN_UPDATE_MS;
    CHECK(fabs((double)frameIntervalMs - wantFrame) <= 1.0, "frame interval follows the speed switch");
    CHECK(fabs((double)arrowIntervalMs - wantArrow) <= 1.0, "arrow interval follows the speed switch");
    const long f0 = fcw->shows, a0 = st[3]->shows, l0 = let->shows;
    const unsigned long clk0 = letterClockMs;
    run(6000);
    const double gotFrame = 6000.0 / (fcw->shows - f0), gotArrow = 6000.0 / (st[3]->shows - a0);
    CHECK(fabs(gotFrame - wantFrame) <= 0.03 * wantFrame + 1.5, "measured frame update period matches");
    CHECK(fabs(gotArrow - wantArrow) <= 0.03 * wantArrow + 1.5, "measured arrow update period matches");
    const double clockRate = (double)(letterClockMs - clk0) / 6000.0;
    CHECK(fabs(clockRate - mLet) <= 0.02 * mLet + 0.005, "letter pattern clock runs at the switch multiplier");
    CHECK(let->shows - l0 > 150, "letters keep updating at their own rate");
    printf("  switches %X/%X: frame %.0f ms, arrow %.1f ms, letters x%.2f\n", pr[0], pr[1], gotFrame, gotArrow, clockRate);
  }
  // A position that is only passed through while turning must be ignored.
  setCode(PIN_SPEED_LETTERS, 8); run(200);
  setCode(PIN_SPEED_LETTERS, 12); run(10);
  CHECK(speedStable[SLOT_SPEED_LETTERS] == 8, "a position held for 10 ms is not accepted");
  setCode(PIN_SPEED_LETTERS, 3); run(10);
  setCode(PIN_SPEED_LETTERS, 4); run(150);
  CHECK(speedStable[SLOT_SPEED_LETTERS] == 4, "the final position is accepted after it settles");
  // Reading is consistent with the configured polarity.
  g_pinLevel[PIN_SPEED_LETTERS[0]] = HIGH; g_pinLevel[PIN_SPEED_LETTERS[1]] = LOW;
  g_pinLevel[PIN_SPEED_LETTERS[2]] = LOW; g_pinLevel[PIN_SPEED_LETTERS[3]] = HIGH;
  CHECK(readSpeedSwitch(PIN_SPEED_LETTERS) == (SPEED_BIT_IS_LOW ? 6 : 9), "pin polarity: pins 2 and 4 LOW read as 6 (active low)");

  // ---- Casino closed tests ----
  setCode(PIN_SPEED_FRAME_ARROW, 8); setCode(PIN_SPEED_LETTERS, 8); run(300);
  auto openLevel = [&](int k) { return shownLevel(let, FIRST_WORD_LETTERS + k); };
  auto pressMode = [&]() { g_pinLevel[PIN_MODE_BUTTON] = LOW; run(120); g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(60); };
  while (letterMode != MODE_BREATHE) pressMode();
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
  auto bursts = [&](int code, unsigned long ms) { setCode(PIN_SPEED_LETTERS, code); run(300); int n = 0, prev = closedState;
    for (unsigned long k = 0; k < ms; k++) { run(1); if (closedState == CL_FLICKER && prev != CL_FLICKER) n++; prev = closedState; } return n; };
  const int fast = bursts(15, 60000), slow = bursts(0, 60000);
  CHECK(fast >= 5 && slow <= 2 && fast > slow, "CLOSED light timing follows the letter speed switch");
  printf("  CLOSED flicker bursts in 60 s: x4.00 -> %d, x0.25 -> %d\n", fast, slow);
  setCode(PIN_SPEED_LETTERS, 8); run(300);
  // reopen: OPEN letters come back, CLOSED light goes dark
  g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(100);
  CHECK(!casinoClosed, "casino open again after the switch goes HIGH");
  while (letterMode != MODE_BREATHE) pressMode();
  { int lit = 0; bool lightOff = true; for (int k = 0; k < 1500; k++) { run(1); for (int l = 0; l < LETTER_COUNT - FIRST_WORD_LETTERS; l++) if (openLevel(l) > 60) lit++;
        if (k > 100) for (auto c : clo->shown_) if (c != 0) lightOff = false; }
    CHECK(lit > 0, "OPEN letters light up again after reopening"); CHECK(lightOff, "CLOSED light is off after reopening"); }
  printf("  closed phase: CASINO letters lit samples %ld\n", firstLit);

  // ---- New letter modes (scanner, sparkle, chase, pulse) and random auto ----
  setCode(PIN_SPEED_LETTERS, 8); run(300);
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
    // and changes about every LETTER_RANDOM_AUTO_MS
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
    for (unsigned long k = 0; k < 3 * LETTER_RANDOM_AUTO_MS + 5000UL; k++) { run(1);
      if (activeMode != last) { changes++; last = activeMode; if (!firstChange) firstChange = g_millis - start; } }
    CHECK(changes == 3, "random auto: a new pattern after each LETTER_RANDOM_AUTO_MS");
    CHECK(firstChange >= LETTER_RANDOM_AUTO_MS - 50 && firstChange <= LETTER_RANDOM_AUTO_MS + 100, "random auto: first change after about two minutes");
    CHECK(letterMode == MODE_RANDOM_AUTO, "random auto: the selected mode stays random auto");
    pressMode(); CHECK(letterMode == MODE_BREATHE, "the button wraps from random auto back to mode 0");
  }
  // ---- LCD display (fake HD44780 + PCF8574 backpack on the mock Wire) ----
  {
    auto text = [&]() { return Wire.row(0) + "|" + Wire.row(1); };
    auto rtrim = [](std::string r) { while (!r.empty() && r.back() == ' ') r.pop_back(); return r; };
    auto line0 = [&]() { return rtrim(Wire.row(0)); };
    auto line1 = [&]() { return rtrim(Wire.row(1)); };
    CHECK(Wire.begun && Wire.clock == 400000UL, "LCD: I2C started at 400 kHz");
    CHECK(Wire.fourBit && Wire.clears >= 1 && Wire.backlight, "LCD: initialised in 4-bit mode, cleared, backlight on");
    CHECK(lcdOk, "LCD: found at start-up");
    CHECK(!Wire.oversize, "LCD: no transmission longer than the 32 byte Wire buffer");
    // Every row is exactly 16 characters of printable text.
    auto printable = [&]() { for (int r = 0; r < 2; r++) for (char c : Wire.row(r)) if (c < 32 || c > 126) return false; return true; };

    // mode page: appears at once (within 150 ms) when the button is pressed, and is held
    setCode(PIN_SPEED_FRAME_ARROW, 8); setCode(PIN_SPEED_LETTERS, 8);
    while (letterMode != MODE_BREATHE) pressMode();
    run(7000);                                           // hold over, pages rotating
    g_pinLevel[PIN_MODE_BUTTON] = LOW;
    int ms = 0; while (line0() != "Mode 1" && ms < 400) { run(1); ms++; }
    g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(60);
    CHECK(line0() == "Mode 1" && line1() == "wave", "LCD: the mode page shows the new mode");
    CHECK(ms <= 150, "LCD: the new mode appears within 150 ms of the button press");
    run(5800);
    CHECK(line0() == "Mode 1", "LCD: the mode page is held for about 6 seconds after the change");
    run(400);
    CHECK(line0().rfind("SW3", 0) == 0, "LCD: after the hold the pages rotate on to the next page (SW3)");
    // names and widths of every mode
    for (int m = 0; m < LETTER_MODE_COUNT; m++) {
      setLetterMode(m, g_millis); lcdShowPage(LCD_PAGE_MODE, g_millis, true); run(300);
      CHECK(printable(), "LCD: only printable characters");
      if (m == MODE_RANDOM_AUTO) {
        CHECK(line0() == "Mode 10 (auto)", "LCD: random auto title");
        CHECK(line1() == std::string("now: ") + letterModeName(activeMode), "LCD: random auto shows the pattern in use");
      } else {
        CHECK(line0() == "Mode " + std::to_string(m) && line1() == letterModeName(m), "LCD: mode number and name");
      }
    }
    while (letterMode != MODE_BREATHE) pressMode();

    // SW3: position, multiplier and the two intervals, shown at once
    setCode(PIN_SPEED_FRAME_ARROW, 5);
    ms = 0; while (line0().rfind("SW3:5", 0) != 0 && ms < 400) { run(1); ms++; }
    CHECK(ms <= 150, "LCD: SW3 page appears within 150 ms of the switch change");
    run(100);
    { char e1[24]; snprintf(e1, sizeof e1, "Fr%ums Ar%ums", (unsigned)frameIntervalMs, (unsigned)arrowIntervalMs);
      unsigned h = (unsigned)(((uint32_t)speedMult256[5] * 100UL + 128UL) / 256UL);
      char e0[24]; snprintf(e0, sizeof e0, "SW3:5  x%u.%02u", h / 100, h % 100);
      CHECK(line0() == e0 && line1() == e1, "LCD: SW3 page shows position, multiplier, frame and arrow intervals");
      printf("  LCD SW3 page: '%s' / '%s'\n", line0().c_str(), line1().c_str()); }
    // SW4 while SW3 is held: the new page replaces it at once
    run(2000);
    setCode(PIN_SPEED_LETTERS, 12); run(120);
    CHECK(line0().rfind("SW4:C", 0) == 0 && line1() == "Letter patterns", "LCD: SW4 page replaces a held page at once");
    { unsigned h = (unsigned)(((uint32_t)speedMult256[12] * 100UL + 128UL) / 256UL);
      char e0[24]; snprintf(e0, sizeof e0, "SW4:C  x%u.%02u", h / 100, h % 100);
      CHECK(line0() == e0, "LCD: SW4 page shows the multiplier"); printf("  LCD SW4 page: '%s' / '%s'\n", line0().c_str(), line1().c_str()); }
    run(5700);
    CHECK(line0().rfind("SW4:C", 0) == 0, "LCD: a change made 2 s into the previous hold restarts the 6 s hold");
    run(400);
    CHECK(line0().rfind("Casino", 0) == 0, "LCD: after SW4 the pages rotate on to the status page");

    // status page: arrow direction and casino state
    const uint8_t arrowPin = g_pinLevel[PIN_ARROW_SWITCH], closedPin = g_pinLevel[PIN_CASINO_CLOSED_SWITCH];
    run(3500);
    g_pinLevel[PIN_ARROW_SWITCH] = arrowPin == LOW ? HIGH : LOW; run(120);
    const bool nowLeft = !arrowPointsRight;
    CHECK(line0().rfind("Casino", 0) == 0 && line1() == (nowLeft ? "->Customer left" : "<-Customer right"), "LCD: status page follows the arrow switch");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(120);
    CHECK(line0() == "Casino: CLOSED", "LCD: status page shows CLOSED");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(120);
    CHECK(line0() == "Casino: OPEN", "LCD: status page shows OPEN");
    g_pinLevel[PIN_ARROW_SWITCH] = arrowPin; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = closedPin; run(200);
    CHECK(line1() == (arrowPointsRight ? "<-Customer right" : "->Customer left"), "LCD: status page arrow text matches the arrow");

    // rotation: all four pages appear, 3 seconds each
    run(7000);                                           // hold over
    bool seen[LCD_PAGE_COUNT] = {}; uint8_t last = lcdPage; int changesSeen = 0;
    for (int k = 0; k < 12100; k++) { run(1); seen[lcdPage] = true; if (lcdPage != last) { changesSeen++; last = lcdPage; } }
    bool all = true; for (bool x : seen) if (!x) all = false;
    CHECK(all && changesSeen == 4, "LCD: the pages rotate (4 pages in about 12 s)");

    // quiet: nothing is sent when the text does not change; updates are small
    setCode(PIN_SPEED_LETTERS, 8); run(300);
    const long before = Wire.transactions; run(2500);   // within one 3 s page, mid-page
    CHECK(Wire.transactions - before <= 8, "LCD: the display is only written when the text changes");
    bool small = true; long worst = 0;
    for (int k = 0; k < 20000; k++) { const long t0 = Wire.transactions; run(1); if (Wire.transactions - t0 > 1) small = false; if ((long)Wire.lastBytes > worst) worst = Wire.lastBytes; }
    CHECK(small && worst <= 24 && !Wire.oversize, "LCD: at most one short I2C transmission per loop pass");
    printf("  LCD: longest I2C transmission %ld bytes\n", worst);

    // random auto: the automatic change shows the mode page, without a hold
    setLetterMode(MODE_RANDOM_AUTO, g_millis); lcdShowPage(LCD_PAGE_STATUS, g_millis, false);
    uint8_t startPattern = activeMode; unsigned long guard = 0;
    while (activeMode == startPattern && guard < 130000UL) { run(1); guard++; }
    run(150);
    CHECK(activeMode != startPattern && lcdPage == LCD_PAGE_MODE && !lcdHolding, "LCD: random auto's change jumps to the mode page without a hold");
    CHECK(line1() == std::string("now: ") + letterModeName(activeMode), "LCD: the mode page names the new pattern");
    while (letterMode != MODE_BREATHE) pressMode();

    // a display that is missing or at another address is left out without disturbing the sign
    Wire.address = 0x3F; lcdOk = false; lcdInit();
    CHECK(!lcdOk, "LCD: a display at another address is not found");
    const long t1 = Wire.transactions; run(500);
    CHECK(Wire.transactions == t1, "LCD: nothing is sent when no display answered");
    Wire.address = 0x27; lcdInit();
    CHECK(lcdOk, "LCD: found again when the address matches");
    lcdShowPage(LCD_PAGE_MODE, g_millis, true); run(400);
    CHECK(line0() == "Mode 0" && printable(), "LCD: shows the mode page again after re-initialisation");
    CHECK(true || text().empty(), "");
  }
  // ---- Power-on LED test ----
  {
    auto rtrimS = [](std::string r) { while (!r.empty() && r.back() == ' ') r.pop_back(); return r; };
    auto line0 = [&]() { return rtrimS(Wire.row(0)); };
    g_pinLevel[PIN_MODE_BUTTON] = HIGH; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH;
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
      if (total > STRIP_COUNT * 5) onlyTestPattern = false;
      for (int s2 = 0; s2 < STRIP_COUNT; s2++) { int cnt = 0; for (auto c : allStrips[s2]->shown_) if (c) cnt++; if (cnt > 5) onlyTestPattern = false; }
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
    CHECK(onlyTestPattern, "LED test: at most 5 LEDs per strip are lit at the same time (nothing else is drawn)");
    bool all = true;
    for (int pass = 0; pass < STARTUP_TEST_PASSES; pass++) for (int s = 0; s < STRIP_COUNT; s++) for (char c : lit[pass][s]) if (!c) all = false;
    CHECK(all, "LED test: every LED of every strip is lit in every pass");
    CHECK(maxLit >= 30, "LED test: all strips run at the same time");
    CHECK(lcdExact && lcdSamples >= 38, "LED test: the LCD shows the scrolling text and the counter/bar, rows swapping every 2.5 s");
    if (!lcdExact) printf("  LCD mismatch: %s\n", firstBad.c_str());
    bool charsOk = true; for (char ch : marquee) for (int r = 0; r < 2; r++) if (!charsRow[r][(unsigned char)ch]) charsOk = false;
    CHECK(charsOk, "LED test: every character of the skip text appears on both rows");
    printf("  LED test: %.1f s, at most %d LEDs lit at once\n", endedAt / 1000.0, maxLit);
    CHECK(!startupTestActive && letterMode == MODE_BREATHE, "after the test the sign starts in mode 0");
    CHECK(casinoClosed, "a switch changed during the test takes effect when the test ends");
    run(400);
    CHECK(line0() == "Mode 0", "after the test the LCD shows the mode page");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(100);
    CHECK(!casinoClosed, "casino open again");

    // D33 skips the test and the same press does not change the mode
    run(300); beginStartupTest(g_millis); run(500);
    CHECK(startupTestActive, "LED test: still running after 0.5 s");
    g_pinLevel[PIN_MODE_BUTTON] = LOW; run(100); g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(100);
    CHECK(!startupTestActive, "LED test: D33 skips the test");
    CHECK(letterMode == MODE_BREATHE, "LED test: the skipping press does not change the letter mode");
    run(400); CHECK(line0() == "Mode 0", "LED test: after a skip the LCD shows the mode page");

    // a button held down during power-up does not skip the test until it was released and pressed again
    g_pinLevel[PIN_MODE_BUTTON] = LOW; run(100);          // normal operation: this changes the mode
    beginStartupTest(g_millis); run(2000);
    CHECK(startupTestActive, "LED test: a button held during power-up does not skip the test");
    g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(100);
    CHECK(startupTestActive, "LED test: releasing the button does not skip it");
    g_pinLevel[PIN_MODE_BUTTON] = LOW; run(100);
    CHECK(!startupTestActive, "LED test: a new press skips it");
    g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(100);
    while (letterMode != MODE_BREATHE) pressMode();

    // a skipped test leaves no LED test pattern behind: the strips are cleared and the sign runs normally
    run(300);
    bool shows = false; for (auto *st : g_strips) for (auto c : st->shown_) if (c) shows = true;
    CHECK(shows, "after the test the normal animations are running");
    CHECK(rtrimS(Wire.row(0)).size() > 0, "LCD has text");
  }
  CHECK(LETTER_START_MODE == 0, "the sign starts in mode 0");

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
    g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(3000);                // calm: no state change for a while
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
    CHECK(log.find("DBG up=1s loops=1001 mode=0 arrow=R closed=0 sw3=8 sw4=8 events=0 beats=3 tick=") != std::string::npos,
          "DBG line at 1 s with all fields");
    CHECK(log.find("DBG up=2s loops=1000 ") != std::string::npos && log.find("DBG up=3s loops=1000 ") != std::string::npos,
          "the uptime counts up and the loop runs 1000 times a second here");
    CHECK(log.find(" arrow=L ") != std::string::npos && log.find(" arrow=R ") != std::string::npos, "the DBG line shows the arrow direction");
    CHECK(log.find(" closed=1 ") != std::string::npos, "the DBG line shows casino closed during the test");
    CHECK(log.find(" mode=10 ") != std::string::npos && log.find(" mode=5 ") != std::string::npos, "the DBG line shows the letter mode");
    CHECK(log.find(" sw3=0 ") != std::string::npos && log.find(" sw4=F ") != std::string::npos, "the DBG line shows the speed switch positions");
    // the beat counter and the tick change from line to line
    { size_t a = log.find("DBG up=30s"), b = log.find("DBG up=31s");
      if (a == std::string::npos || b == std::string::npos) CHECK(false, "DBG lines at 30 s and 31 s exist");
      else {
        std::string l30 = log.substr(a, log.find('\n', a) - a), l31 = log.substr(b, log.find('\n', b) - b);
        auto field = [](const std::string &l, const char *key) { size_t k = l.find(key); return k == std::string::npos ? -1L : std::stol(l.substr(k + strlen(key))); };
        CHECK(field(l31, " beats=") > field(l30, " beats=") && field(l31, " tick=") != field(l30, " tick="), "beats and tick change every second");
      } }
    CHECK(log.find("DBG up=4s loops=1000 mode=0 arrow=R closed=0 sw3=8 sw4=8 events=0 ") != std::string::npos, "events=0 before the first change");
    CHECK(log.find("DBG up=6s loops=1000 mode=0 arrow=L closed=0 sw3=8 sw4=8 events=1 ") != std::string::npos,
          "after the arrow switch moved at 5 s: arrow=L, events=1");
  }
  // ---- Brightness curves (expected values hard-coded; see the formula in the sketch) ----
  {
    CHECK(LED_TYPE_COUNT == 2 && LED_TYPE_STRIP == 0 && LED_TYPE_STICK == 1, "two LED types: strip 0, stick 1");
    // defaults: min 0, max 255, exponent 1.97, mix 1, keep zero dark, for both types
    buildDefaultBrightnessCurves();
    const int in[10] = {0, 1, 2, 16, 32, 64, 128, 192, 254, 255}, want[10] = {0, 0, 0, 0, 0, 2, 13, 38, 79, 80};
    for (int ty = 0; ty < 2; ty++) for (int i = 0; i < 10; i++) CHECK(brightnessLut[ty][in[i]] == want[i], "default curve value");
    bool mono = true; for (int ty = 0; ty < 2; ty++) for (int i = 1; i < 256; i++) if (brightnessLut[ty][i] < brightnessLut[ty][i - 1]) mono = false;
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
    // each LED type has its own table in the running sketch: strips (frame, letters) vs sticks (arrow, CLOSED)
    setBrightnessCurve(0, 0, 50, 1.0f, 0.0f, true);     // strips: at most 50
    setBrightnessCurve(1, 0, 100, 1.0f, 0.0f, true);    // sticks: at most 100
    run(3000);
    auto chmax = [](uint32_t c) { unsigned r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255; unsigned m = r > g ? r : g; return b > m ? b : m; };
    unsigned fmax = 0, lmax = 0, amax = 0;
    for (auto *s : {fcw, fccw}) for (auto c : s->shown_) fmax = (chmax(c) > fmax ? chmax(c) : fmax);
    for (auto c : let->shown_) lmax = (chmax(c) > lmax ? chmax(c) : lmax);
    for (int k = 0; k < 10; k++) for (auto c : st[k]->shown_) amax = (chmax(c) > amax ? chmax(c) : amax);
    CHECK(fmax == 50, "frame LEDs reach exactly the strip maximum 50 (white 255 mapped)");
    CHECK(lmax <= 50 && lmax >= 30, "letters stay within the strip maximum 50 and use most of it");
    CHECK(amax <= 100 && amax >= 90, "arrow sticks stay within the stick maximum 100 and use most of it");
    const uint8_t closedPin = g_pinLevel[PIN_CASINO_CLOSED_SWITCH];
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW;   // casino closed: the CLOSED light is on and flickers
    unsigned cmax = 0;
    for (int k = 0; k < 600; k++) { run(10); for (auto c : clo->shown_) cmax = (c & 255u) > cmax ? (c & 255u) : cmax; }
    CHECK(cmax == 100, "the CLOSED stick follows the stick table (its full white 255 shows as 100)");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = closedPin; run(500);
    // the lookup table is rebuilt at once when a value changes: a new maximum shows within one frame
    setBrightnessCurve(0, 0, 255, 1.0f, 0.0f, true); run(200);
    unsigned fmax2 = 0; for (auto *s : {fcw, fccw}) for (auto c : s->shown_) fmax2 = (c & 255u) > fmax2 ? (c & 255u) : fmax2;
    CHECK(fmax2 == 255, "after setBrightnessCurve the frame shows full white again");
  }
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no setPixelColor beyond strip length");
  printf("mode changes: %ld; letters %u LEDs; shows: frame %ld letters %ld rail %ld\n", modeChanges, (unsigned)let->numPixels(), fcw->shows, let->shows, st[3]->shows);
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
