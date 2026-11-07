#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "Wire.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
MockSerial Serial;
TwoWire Wire;
std::vector<Adafruit_NeoPixel*> g_strips;
#include SKETCH_PATH
#include <cstring>
#include <string>

static const char* nameOf(int pin) {
  switch (pin) { case PIN_FRAME_CW: return "frameCw"; case PIN_FRAME_CCW: return "frameCcw";
    case PIN_LETTERS: return "letters"; case PIN_ARROW_BODY_TOP: return "arrowBodyTop";
    case PIN_ARROW_BODY_BOTTOM: return "arrowBodyBottom"; case PIN_ARROW_HEAD_LEFT: return "arrowHeadLeft";
    case PIN_ARROW_CAP_LEFT: return "arrowCapLeft"; case PIN_ARROW_CAP_RIGHT: return "arrowCapRight";
    case PIN_CLOSED_LIGHT: return "closedLight";
    default: return "arrowHeadRight"; }
}
static void dump(FILE *f) {
  fprintf(f, "{\"t\":%lu,\"mode\":%d", g_millis, (int)letterMode);
  for (auto *s : g_strips) {
    fprintf(f, ",\"%s\":[", nameOf(s->pin_));
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
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);          // pull-ups: open switch/button read HIGH
  g_pinLevel[PIN_ARROW_SWITCH] = HIGH;                  // slide switch handle right -> arrow points right
  auto setCode = [&](const uint8_t *pins, int code) {
    for (int b = 0; b < 4; b++) g_pinLevel[pins[b]] = (((code >> b) & 1) == 1) == SPEED_BIT_IS_LOW ? LOW : HIGH;
  };
  setCode(PIN_SPEED_FRAME_ARROW, 8);                    // both speed switches at position 8 (about x1.1)
  setCode(PIN_SPEED_LETTERS, 8);
  setup();
  CHECK(startupTestActive, "the power-on LED test starts at power-up");
  finishStartup(g_millis, true);   // the older tests below start from the normal operation
  Adafruit_NeoPixel *fcw=nullptr,*fccw=nullptr,*let=nullptr,*bt=nullptr,*bb=nullptr,*hl=nullptr,*hr=nullptr,*cl=nullptr,*cr=nullptr,*clo=nullptr;
  for (auto *s : g_strips) { switch (s->pin_) { case PIN_FRAME_CW: fcw=s; break; case PIN_FRAME_CCW: fccw=s; break;
    case PIN_LETTERS: let=s; break; case PIN_ARROW_BODY_TOP: bt=s; break; case PIN_ARROW_BODY_BOTTOM: bb=s; break;
    case PIN_ARROW_HEAD_LEFT: hl=s; break; case PIN_ARROW_CAP_LEFT: cl=s; break;
    case PIN_ARROW_CAP_RIGHT: cr=s; break; case PIN_CLOSED_LIGHT: clo=s; break; default: hr=s; } }
  CHECK(clo && clo->numPixels() == CLOSED_LIGHT_LEDS, "closed light strip length");
  CHECK(fcw->numPixels()==FRAME_HALF_LEDS && fccw->numPixels()==FRAME_HALF_LEDS, "frame strip lengths");
  CHECK(let->numPixels()==lettersTotalLeds(), "letters length");
  CHECK(bt->numPixels()==ARROW_BODY_LEDS && bb->numPixels()==ARROW_BODY_LEDS, "arrow body lengths");
  CHECK(hl->numPixels()==ARROW_HEAD_LEDS && hr->numPixels()==ARROW_HEAD_LEDS, "arrow head lengths");
  CHECK(cl->numPixels()==ARROW_CAP_LEDS && cr->numPixels()==ARROW_CAP_LEDS, "arrow cap lengths");
  CHECK(arrowPointsRight, "arrow starts pointing right with the switch released (HIGH)");

  // Timeline: mode k is active from MODE_MS*k. The button is pressed (with contact bounce) just before.
  const unsigned long MODE_MS = 14000, END = MODE_MS * LETTER_MODE_COUNT + 2000;
  // Arrow switch: LOW (left) at 5 s, HIGH (right) at 25 s, short glitch at 45 s, bouncing change at 60 s.
  long lastFrameShows = fcw->shows;
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
    if (g_millis == 5000) g_pinLevel[PIN_ARROW_SWITCH] = LOW;
    if (g_millis == 25000) g_pinLevel[PIN_ARROW_SWITCH] = HIGH;
    if (g_millis == 45000) g_pinLevel[PIN_ARROW_SWITCH] = LOW;     // 10 ms glitch: must be ignored
    if (g_millis == 45010) g_pinLevel[PIN_ARROW_SWITCH] = HIGH;
    if (g_millis >= 60000 && g_millis < 60012) g_pinLevel[PIN_ARROW_SWITCH] = (g_millis & 1) ? HIGH : LOW;  // bounce
    if (g_millis == 60012) g_pinLevel[PIN_ARROW_SWITCH] = LOW;      // settles LOW -> left

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
    // Arrow: shaft/caps lit; only the pointed head lit; direction follows the switch.
    bool leftLit=false, rightLit=false;
    for (auto c : hl->shown_) if (c) leftLit = true;
    for (auto c : hr->shown_) if (c) rightLit = true;
    if (g_millis > 1000) {
      for (auto c : bt->shown_) CHECK(c != 0, "top shaft always lit");
      for (auto c : bb->shown_) CHECK(c != 0, "bottom shaft always lit");
    }
    bool capLeftLit = false, capRightLit = false;
    for (auto c : cl->shown_) if (c) capLeftLit = true;
    for (auto c : cr->shown_) if (c) capRightLit = true;
    auto expectDir = [&](unsigned long from, unsigned long to, bool right, const char *msg) {
      if (g_millis > from && g_millis < to) {
        CHECK(right ? (rightLit && !leftLit) : (leftLit && !rightLit), msg);
        // Only the cap at the tail end is lit; the head-end cap is dark (the shaft opens into the head).
        CHECK(right ? (capLeftLit && !capRightLit) : (capRightLit && !capLeftLit), "only the tail-end cap is lit");
        // Every LED of the lit cap is on.
        for (auto c : (right ? cl : cr)->shown_) CHECK(c != 0, "tail cap LEDs all lit");
      } };
    expectDir(1000, 4900, true, "before the switch moves: right head only");
    expectDir(5200, 24900, false, "switch LOW: left head only");
    expectDir(25200, 44900, true, "switch HIGH: right head only");
    expectDir(45000, 59900, true, "10 ms glitch ignored: still right head only");   // 45000..: glitch
    expectDir(60200, END, false, "bouncing switch settles LOW: left head only");
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
    const long f0 = fcw->shows, a0 = bt->shows, l0 = let->shows;
    const unsigned long clk0 = letterClockMs;
    run(6000);
    const double gotFrame = 6000.0 / (fcw->shows - f0), gotArrow = 6000.0 / (bt->shows - a0);
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
    CHECK(line0().rfind("Casino", 0) == 0 && line1() == (nowLeft ? "Arrow: <- LEFT" : "Arrow: RIGHT ->"), "LCD: status page follows the arrow switch");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = LOW; run(120);
    CHECK(line0() == "Casino: CLOSED", "LCD: status page shows CLOSED");
    g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = HIGH; run(120);
    CHECK(line0() == "Casino: OPEN", "LCD: status page shows OPEN");
    g_pinLevel[PIN_ARROW_SWITCH] = arrowPin; g_pinLevel[PIN_CASINO_CLOSED_SWITCH] = closedPin; run(200);
    CHECK(line1() == (arrowPointsRight ? "Arrow: RIGHT ->" : "Arrow: <- LEFT"), "LCD: status page arrow text matches the arrow");

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
    const std::string marquee = "press D3 to skip   ";
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

    // D3 skips the test and the same press does not change the mode
    run(300); beginStartupTest(g_millis); run(500);
    CHECK(startupTestActive, "LED test: still running after 0.5 s");
    g_pinLevel[PIN_MODE_BUTTON] = LOW; run(100); g_pinLevel[PIN_MODE_BUTTON] = HIGH; run(100);
    CHECK(!startupTestActive, "LED test: D3 skips the test");
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
  for (auto *s : g_strips) CHECK(s->overflow == 0, "no setPixelColor beyond strip length");
  printf("mode changes: %ld; letters %u LEDs; shows: frame %ld letters %ld body %ld\n", modeChanges, (unsigned)let->numPixels(), fcw->shows, let->shows, bt->shows);
  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
