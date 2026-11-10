// Host-side test of hex_rotary_test/sketch.ino (hex rotary switch changes the number A, shown on an I2C LCD).
//   g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH="\"$PWD/hex_rotary_test/sketch.ino\"" -o /tmp/hex_rotary_harness tests/hex_rotary_test_harness.cpp && /tmp/hex_rotary_harness
// All expected values are written out here on purpose (they are not taken from the sketch constants),
// so a changed constant or rule in the sketch is detected. tests/mutate_hex_rotary.py checks that.
#include "Arduino.h"
#include "Wire.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
TwoWire Wire;
#include SKETCH_PATH

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)

static void run(unsigned long ms) {
  for (unsigned long k = 0; k < ms; k++) {
    g_millis++;
    loop();
  }
}

// Counts the toggles of the LED pin during the next ms milliseconds.
static int togglesIn(unsigned long ms) {
  int toggles = 0;
  uint8_t prev = g_pinOut[13];
  for (unsigned long k = 0; k < ms; k++) {
    run(1);
    if (g_pinOut[13] != prev) { toggles++; prev = g_pinOut[13]; }
  }
  return toggles;
}

// Puts the switch into position p: a leg connected to C reads LOW (bit set), the others read HIGH (pull-up).
static void setPos(int p) {
  const int pins[4] = {34, 35, 36, 37};
  for (int k = 0; k < 4; k++) g_pinLevel[pins[k]] = ((p >> k) & 1) ? LOW : HIGH;
}

// Turns the switch to position p, lets the debounce time pass and the display catch up.
static void turn(int p) {
  setPos(p);
  run(12);
}

static std::string pad16(std::string s) { s.resize(16, ' '); return s; }

// Checks both LCD rows against the expected A, switch position, direction text and step count.
static bool screenIs(int a, int pos, const char *dir, int n) {
  char r0[40], r1[40];
  snprintf(r0, sizeof r0, "A=%-4d(2..51)", a);
  snprintf(r1, sizeof r1, "Pos:%X %-3s n:%d", pos, dir, n);
  return Wire.row(0) == pad16(r0) && Wire.row(1) == pad16(r1);
}

// The last "DBG ..." line the sketch printed.
static std::string lastDbg() {
  const std::string &c = Serial.captured;
  size_t at = c.rfind("DBG up=");
  if (at == std::string::npos) return "";
  size_t end = c.find('\n', at);
  return c.substr(at, end == std::string::npos ? std::string::npos : end - at);
}

// One move of the switch: new position, expected A, expected total steps, expected direction text.
struct Move { int to, a, n; const char *dir; const char *what; };

int main() {
  memset(g_pinLevel, HIGH, sizeof g_pinLevel);
  Serial.capture = true;
  setup();

  // --- start-up: serial speed, I2C scan, LCD found, wire speed
  CHECK(Serial.baud == 115200, "serial speed is 115200");
  CHECK(Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "the start text states the 115200 baud speed");
  CHECK(Serial.captured.find("I2C scan (0x08-0x77):") != std::string::npos, "the I2C scan starts at boot");
  CHECK(Serial.captured.find("  found 0x27  (LCD address in this sketch)") != std::string::npos, "the scan lists the LCD at 0x27");
  CHECK(Serial.captured.find("I2C scan done: 1 device(s)") != std::string::npos, "the scan counts one device");
  CHECK(Serial.captured.find("No device") == std::string::npos, "no warning when the LCD answers");
  CHECK(Wire.begun && Wire.clock == 100000, "I2C runs at 100 kHz");
  CHECK(Wire.fourBit, "the LCD was switched to 4-bit mode");

  // --- each leg has its own bit weight: leg 1 = D34, leg 2 = D35, leg 4 = D36, leg 8 = D37
  bool legsOk = true;
  for (int p = 0; p < 16; p++) { setPos(p); if (readRawPosition() != p) legsOk = false; }
  CHECK(legsOk, "positions 0..15 read back through pins D34 (1), D35 (2), D36 (4), D37 (8), LOW = set");
  setPos(0);
  CHECK(readRawPosition() == 0, "all legs HIGH = position 0");

  // --- the display shows A at once
  run(100);
  CHECK(screenIs(26, 0, "--", 0), "start screen: A=26, Pos:0, no direction yet, 0 steps");
  CHECK(Wire.row(0) == "A=26  (2..51)   ", "row 0 reads exactly A=26  (2..51)");
  CHECK(Wire.row(1) == "Pos:0 --  n:0   ", "row 1 reads exactly Pos:0 --  n:0");

  // --- sign of life from t = 0: the LED toggles every 500 ms, one DBG line per second with changing values
  // (the run above already passed t = 100; continue with the exact moments)
  CHECK(g_pinOut[13] == LOW, "t=100: LED is in its first (off) half period");
  run(399);
  CHECK(g_millis == 499 && g_pinOut[13] == LOW, "t=499: LED still off");
  run(1);
  CHECK(g_pinOut[13] == HIGH, "t=500: LED toggles on");
  run(499);
  CHECK(lastDbg() == "", "t=999: no debug line yet");
  run(1);
  CHECK(lastDbg() == "DBG up=1 pos=0 A=26 dir=-- steps=0 chg=0 raw=0 beats=1 lcd=1", "t=1000: first debug line");
  CHECK(g_pinOut[13] == LOW, "t=1000: LED toggles off again");
  run(1000);
  CHECK(lastDbg() == "DBG up=2 pos=0 A=26 dir=-- steps=0 chg=0 raw=0 beats=3 lcd=1", "t=2000: second debug line, uptime and beats changed");

  // --- three quick position changes: the LED is shortly inverted and then blinks faster
  turn(1);
  CHECK(g_pinOut[13] != (beatOn ? HIGH : LOW), "right after a position change the LED is inverted");
  turn(2);
  turn(3);
  run(50);
  CHECK(g_pinOut[13] != (beatOn ? HIGH : LOW), "50 ms after a position change the LED is still inverted");
  run(50);
  CHECK(g_pinOut[13] == (beatOn ? HIGH : LOW), "100 ms later the inversion is over");
  {
    int toggles = 0;
    uint8_t prev = g_pinOut[13];
    for (int k = 0; k < 400; k++) {
      run(1);
      if (g_pinOut[13] != prev) { toggles++; prev = g_pinOut[13]; }
    }
    CHECK(toggles >= 3, "while the switch is turned a lot the LED blinks faster (125 ms)");
  }
  CHECK(screenIs(29, 3, "CW", 3), "after 1, 2, 3 (three single steps forward): A=29, Pos:3, CW, n:3");

  // --- moves from position 3 (A=29, n=3): position, expected A, total steps, direction
  const Move moves[] = {
    {2, 28, 4, "CCW", "3->2 = one step back"},
    {0, 26, 6, "CCW", "2->0 = two steps back"},
    {5, 31, 11, "CW", "0->5 = five steps forward"},
    {12, 38, 18, "CW", "5->12 = seven steps forward (the most that is counted forward)"},
    {3, 45, 25, "CW", "12->3 = seven steps forward over the wrap (delta 7)"},
    {10, 51, 32, "CW", "3->10 = seven steps forward, A stops at 51 (52 would be too much)"},
    {11, 51, 33, "CW", "10->11 = one more step forward, A stays at 51"},
    {4, 44, 40, "CCW", "11->4 = delta 9 = seven steps BACK"},
    {15, 39, 45, "CCW", "4->15 = delta 11 = five steps back"},
    {0, 40, 46, "CW", "15->0 = delta 1 = one step forward over the wrap"},
    {15, 39, 47, "CCW", "0->15 = delta 15 = one step back over the wrap"},
    {7, 39, 47, "CCW", "15->7 = delta 8: ambiguous, ignored (A, steps and direction unchanged)"},
    {0, 32, 54, "CCW", "7->0 = delta 9 = seven steps back"},
    {9, 25, 61, "CCW", "0->9 = delta 9 = seven steps back"},
    {2, 18, 68, "CCW", "9->2 = seven steps back"},
    {11, 11, 75, "CCW", "2->11 = seven steps back"},
    {4, 4, 82, "CCW", "11->4 = seven steps back"},
    {13, 2, 89, "CCW", "4->13 = seven steps back, A stops at 2 (-3 would be too little)"},
  };
  int pos = 3;
  for (const Move &m : moves) {
    turn(m.to);
    run(100);
    pos = m.to;
    if (!screenIs(m.a, pos, m.dir, m.n)) {
      printf("FAIL: %s: expected A=%d Pos:%X %s n:%d, LCD shows [%s] [%s] (t=%lu)\n", m.what, m.a, pos, m.dir, m.n,
             Wire.row(0).c_str(), Wire.row(1).c_str(), g_millis);
      fails++;
    }
  }
  CHECK(Wire.row(0) == "A=2   (2..51)   ", "at the lower limit row 0 reads A=2   (2..51)");
  {
    const long w0 = Wire.dataWrites;
    run(200);
    CHECK(Wire.dataWrites == w0, "an unchanged screen sends no characters (only changed characters are sent)");
  }

  // --- debounce: a 5 ms glitch to another position does not count; a new position counts after exactly 10 ms
  setPos(3);
  run(5);
  buildLcdText();
  CHECK(std::string(lcdWanted[1]).substr(0, 5) == "Pos:D", "while a new position is still being debounced the text shows the accepted position (D), not the raw one");
  setPos(13);
  run(40);
  CHECK(screenIs(2, 13, "CCW", 89), "a 5 ms glitch to position 3 and back changes nothing");
  setPos(14);
  run(10);
  CHECK(valueA == 2 && stablePos == 13, "a new position is not yet accepted after 10 loop passes");
  run(1);
  CHECK(valueA == 3 && stablePos == 14, "a new position is accepted when it has been stable for 10 ms");
  run(100);
  CHECK(screenIs(3, 14, "CW", 90), "14 after 13 = one step forward: A=3");

  // --- the direction bit in memory: turning it round makes a forward turn shrink A
  turn(5);                                   // 14 -> 5 = seven steps forward: A=10
  run(100);
  CHECK(screenIs(10, 5, "CW", 97), "14->5 = seven steps forward: A=10");
  reverseDirection = true;
  turn(6);                                   // one step forward in the code, reversed: A shrinks
  run(100);
  CHECK(screenIs(9, 6, "CCW", 98), "reversed: one step forward in the code makes A smaller (CCW)");
  turn(5);                                   // one step back in the code, reversed: A grows
  run(100);
  CHECK(screenIs(10, 5, "CW", 99), "reversed: one step back in the code makes A bigger (CW)");
  reverseDirection = false;

  // --- the debug line shows the position changes counted so far (3 + 18 + 1 + 3 = 25)
  run(1000);
  CHECK(lastDbg().find(" pos=5 A=10 dir=CW steps=99 chg=25 raw=5 ") != std::string::npos, "debug line: pos, A, direction, steps, changes, raw bits");

  // --- the ambiguous half-turn was counted but did not move A (checked through the sketch counter)
  CHECK(ambiguousCount == 1, "exactly one ambiguous half-turn was seen");

  // --- the LCD is lost for a while (voltage dip or loose wire): the sketch notices and starts it again
  Wire.present = false;
  turn(7);                                   // 5 -> 7 = two steps forward: A=12, the text changes, nobody answers
  run(100);
  CHECK(!lcdOk, "a failed LCD transmission marks the display as lost");
  CHECK(valueA == 12, "A keeps working while the display is lost");
  {
    const long t0 = Wire.transactions;
    run(3000);
    CHECK(Wire.transactions - t0 == 3, "while the display is lost the sketch tries to start it once per second (3 tries in 3 s)");
  }
  run(1000);
  CHECK(lastDbg().find(" lcd=0") != std::string::npos, "the debug line shows lcd=0 while the display is lost");
  Wire.present = true;
  const long clearsBefore = Wire.clears;
  run(1100);
  CHECK(lcdOk && Wire.clears == clearsBefore + 1, "the display is started again within a second of coming back");
  CHECK(g_millis - lcdLastInit <= 1100, "the time of the restart is remembered (the 5 s restart timer begins then)");
  run(100);
  CHECK(screenIs(12, 7, "CW", 101), "after the restart the whole text is back: A=12, Pos:7, n:101");
  run(1000);
  CHECK(lastDbg().find(" lcd=1") != std::string::npos, "the debug line shows lcd=1 again");

  // --- the display is started from scratch every 5 s, not earlier (recovers a reset controller)
  run(50);                                   // let any pending restart settle
  {
    unsigned long sinceInit = g_millis - lcdLastInit;
    CHECK(sinceInit < 5000, "the last full start is less than 5 s old");
    const long c0 = Wire.clears;
    run(4900 - sinceInit);                   // 4900 ms after the last full start
    CHECK(Wire.clears == c0, "no full restart before 5 s");
    run(200);
    CHECK(Wire.clears == c0 + 1, "one full restart after 5 s");
    run(100);
    CHECK(screenIs(12, 7, "CW", 101), "the screen is complete again after the restart");
  }

  // --- the step counter on the display stops at 9999
  stepCount = 12345;
  run(150);
  CHECK(Wire.row(1) == "Pos:7 CW  n:9999", "the displayed step count is capped at 9999");
  stepCount = 101;
  run(150);
  CHECK(screenIs(12, 7, "CW", 101), "the real step count is shown again below the cap");

  // --- fast blinking needs three position changes in the sliding window; the previous window counts too
  run(2100);                                 // quiet: both windows are empty
  turn(8);
  turn(9);                                   // only two changes
  run(100);
  CHECK(togglesIn(400) <= 1, "two position changes do not make the LED blink faster");
  run(2100);
  {
    const unsigned long s0 = changeWindowStart;
    while (changeWindowStart == s0) run(1);  // a window has just started
    turn(10);
    turn(11);
    turn(12);                                // three changes in the new window
    const unsigned long s = changeWindowStart;
    run(s + 1100 - g_millis);                // the window has rolled over: the three changes are now the previous window
    CHECK(togglesIn(400) >= 3, "three changes in the previous window still make the LED blink faster");
  }

  // --- a switch change during a restart still counts, and nothing is ever sent as an oversize transaction
  CHECK(!Wire.oversize, "no I2C transmission above the 32-byte Wire buffer");
  CHECK(Wire.backlight, "the backlight bit is on");

  // --- no display at start-up: the scan warns, the sketch carries on and retries every second
  {
    const int addrs[] = {0x08, 0x0A, 0x77, 0x07, 0x78};
    const char *want[] = {"  found 0x08\n", "  found 0x0A\n", "  found 0x77\n", "", ""};
    for (int k = 0; k < 5; k++) {
      Wire.address = (uint8_t)addrs[k];
      Serial.captured.clear();
      const uint8_t n = scanI2cBus();
      const bool inRange = addrs[k] >= 0x08 && addrs[k] <= 0x77;
      CHECK(n == (inRange ? 1 : 0), "the scan covers exactly 0x08..0x77");
      CHECK(!inRange || Serial.captured.find(want[k]) != std::string::npos, "the scan prints the address with two hex digits");
    }
  }
  Serial.captured.clear();
  Wire.address = 0x3F;                       // the display answers on the other usual address
  setPos(9);                                 // the switch is not at 0 when the program starts
  const int aBefore = valueA;
  setup();
  CHECK(Serial.captured.find("  found 0x3F\n") != std::string::npos, "the scan lists the device at 0x3F (not marked as the LCD address)");
  CHECK(Serial.captured.find("No device on the LCD address 0x27") != std::string::npos, "the scan warns that nothing is on 0x27");
  CHECK(Serial.captured.find("LCD did not answer at start-up; trying again every second.") != std::string::npos, "start-up says the LCD did not answer");
  CHECK(!lcdOk, "no LCD at start-up: lcdOk is false");
  run(2000);
  CHECK(!lcdOk, "still no LCD after two seconds (wrong address)");
  CHECK(valueA == aBefore && stablePos == 9, "a switch that is not at 0 at start-up is taken as the start position, A does not move");
  turn(8);                                   // the switch still works without a display
  Wire.address = 0x27;
  run(1100);
  CHECK(lcdOk, "when the display answers at 0x27 it is started on the next retry");
  run(100);
  CHECK(Wire.row(0).substr(0, 2) == "A=" && Wire.row(1).substr(0, 5) == "Pos:8", "the text appears after the late start (row 1 shows Pos:8)");

  // --- a successful start-up remembers when the display was started
  setup();
  CHECK(lcdOk && lcdLastInit == g_millis, "start-up with a working display stores the start time");

  printf(fails ? "RESULT: %d failures\n" : "RESULT: all checks passed\n", fails);
  return fails ? 1 : 0;
}
