/*
 * Hex rotary switch test - a free-running 16-position hex rotary switch (0-F) changes a number A
 *
 * Purpose: find out how the hex rotary switch (partco "hex rotary switch 0-F", legs 1, 2, 4, 8 and two C) behaves
 * before it is used as the control of the planned settings menu (see ../RDD-REQUIREMENTS.md). Nothing here has
 * anything to do with the LED strips or sticks of the casino sign.
 *
 * WHAT IT DOES: the program keeps one number A in memory. A runs from A_MIN (2) to A_MAX (51), it does NOT wrap
 * (it stops at the ends) and one switch step changes it by A_STEP (1). Turning the switch clockwise makes A grow,
 * counter-clockwise makes it smaller. The switch is an absolute 4-bit code (not an incremental encoder): the
 * sketch reads all four legs and works out the direction and the number of steps from the CHANGE of the position:
 *     delta = (new position - old position) mod 16
 *     1..7  = that many steps forward,   9..15 = (16 - delta) steps backward,   8 = ambiguous, ignored.
 * So a fast turn that skips positions (up to 7 at a time) is still counted correctly.
 * The LCD (2x16, I2C 0x27) shows A all the time:
 *     row 0   "A=27  (2..51)"
 *     row 1   "Pos:7 CW  n:123"     raw position 0-F, last direction (CW, CCW or -- before the first move), steps so far
 *
 * SETTINGS you may need to change (all at the top):
 *   reverseDirection   one bit in memory: false = a rising position code is clockwise, true = turn the direction round.
 *                      If A goes the wrong way when you turn the switch, change false to true.
 *   HEX_BIT_IS_LOW     true = a leg connected to C reads LOW and counts as a set bit (normal). If position 0 shows
 *                      as F on the LCD, the switch is complement-coded: set this to false.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, four times as
 * fast while the switch is being turned a lot; it is shortly inverted at every position change). Once per second
 * (DEBUG_MS) the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 *     DBG up=<s> pos=<0-F> A=<n> dir=<CW|CCW|--> steps=<n> chg=<n> raw=<bits> beats=<n> lcd=<0|1>
 * (uptime, switch position, A, last direction, steps, position changes, raw leg bits, LED toggles, LCD state).
 *
 * LCD: the display is checked all the time. If it stops answering, or is not there at start-up, the sketch tries
 * to start it again every LCD_RETRY_MS. Every LCD_REINIT_MS the display is also started from scratch and redrawn
 * (a brief flicker): this recovers a controller that was reset by a voltage dip while the backlight stayed on.
 * Set LCD_REINIT_MS to 0 to turn that off.
 *
 * Only built-in types are used in function signatures (Arduino's automatic prototypes).
 */

#include <Wire.h>

// ============================== CONFIGURATION ==============================
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (also printed at start-up)
constexpr uint8_t PIN_STATUS_LED = 13;       // onboard LED, sign of life

// Hex rotary switch: legs 1, 2, 4, 8 (bit weights) on these pins, the common leg C to GND.
constexpr uint8_t HEX_PINS[4] = {34, 35, 36, 37};   // leg 1, leg 2, leg 4, leg 8
constexpr bool HEX_BIT_IS_LOW = true;        // a leg connected to C (pin LOW) is a set bit
constexpr unsigned long HEX_DEBOUNCE_MS = 10;       // a new position must stay unchanged this long to count

bool reverseDirection = false;               // one bit in memory: true = swap clockwise and counter-clockwise

// The number A.
constexpr int A_MIN = 2;
constexpr int A_MAX = 51;
constexpr int A_STEP = 1;                    // change of A per switch step
constexpr int A_START = 26;                  // A at power-up (in the middle, so both directions can be tried)

// LCD: 2x16 HD44780 behind a PCF8574 I2C backpack (P0 RS, P1 RW, P2 E, P3 backlight, P4..P7 D4..D7).
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;    // if the scan lists another address (often 0x3F), set it here
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint8_t LCD_CHARS_PER_LOOP = 6;    // characters sent per loop() pass (max 7: the Wire buffer holds 32 bytes)
constexpr unsigned long LCD_BUILD_MS = 50;   // how often the text is rebuilt
constexpr unsigned long LCD_RETRY_MS = 1000; // retry interval while the display does not answer
constexpr unsigned long LCD_REINIT_MS = 5000;  // full re-initialisation interval (0 = never)

// Sign of life.
constexpr unsigned long HEARTBEAT_MS = 500;
constexpr unsigned long HEARTBEAT_FAST_MS = 125;
constexpr unsigned long INVERT_MS = 80;
constexpr unsigned long DEBUG_MS = 1000;
constexpr uint8_t BUSY_CHANGES = 3;          // position changes within about a second that make the LED blink faster

constexpr uint8_t LCD_BIT_RS = 0x01;
constexpr uint8_t LCD_BIT_E = 0x04;
constexpr uint8_t LCD_BIT_BACKLIGHT = 0x08;

// ============================== STATE ======================================
int valueA = A_START;                        // the number A
uint8_t stablePos = 0;                       // the accepted switch position 0..15
uint8_t candidatePos = 0;                    // the position being debounced
unsigned long candidateSince = 0;
int8_t lastDir = 0;                          // +1 = A grew (CW), -1 = A shrank (CCW), 0 = no move yet
unsigned long stepCount = 0;                 // switch steps counted since power-up
unsigned long changeCount = 0;               // position changes since power-up
unsigned long ambiguousCount = 0;            // changes of exactly 8 positions (direction unknown, ignored)

bool beatOn = false;
unsigned long beatCount = 0;
unsigned long lastBeat = 0;
unsigned long lastDebug = 0;
unsigned long lastChangeAt = 0;
bool anyChange = false;
uint8_t changesNow = 0;
uint8_t changesPrev = 0;
unsigned long changeWindowStart = 0;

bool lcdOk = false;                          // the display answered during the last start or update
char lcdWanted[LCD_ROWS][LCD_COLS + 1];      // the text that should be on screen
char lcdShown[LCD_ROWS][LCD_COLS];           // what the display shows now
unsigned long lcdLastBuild = 0;
unsigned long lcdLastTry = 0;                // last (re-)initialisation attempt
unsigned long lcdLastInit = 0;               // last successful initialisation

// ============================== HEX SWITCH =================================

// Reads the four legs: bit k of the result is set when leg k is connected to C (see HEX_BIT_IS_LOW).
uint8_t readRawPosition() {
  uint8_t value = 0;
  for (uint8_t k = 0; k < 4; k++) {
    const bool low = (digitalRead(HEX_PINS[k]) == LOW);
    if (low == HEX_BIT_IS_LOW) value |= (uint8_t)(1 << k);
  }
  return value;
}

// Applies a move of the switch from position `from` to position `to` to the number A.
void applyMove(uint8_t from, uint8_t to) {
  changeCount++;
  lastChangeAt = millis();
  anyChange = true;
  changesNow++;
  const uint8_t delta = (uint8_t)((to - from) & 15);
  if (delta == 8) {                          // exactly half a turn: forward or backward cannot be told
    ambiguousCount++;
    return;
  }
  int steps = delta;
  int dir = 1;                               // +1 = a rising position code
  if (delta > 8) {
    steps = 16 - delta;
    dir = -1;
  }
  if (reverseDirection) dir = -dir;
  stepCount += (unsigned long)steps;
  lastDir = (int8_t)dir;
  int next = valueA + dir * steps * A_STEP;
  if (next < A_MIN) next = A_MIN;
  if (next > A_MAX) next = A_MAX;
  valueA = next;
}

// Reads the switch and, when a new position has been stable long enough, applies the move.
void pollSwitch(unsigned long now) {
  const uint8_t raw = readRawPosition();
  if (raw != candidatePos) {                 // still changing (or bouncing): start timing again
    candidatePos = raw;
    candidateSince = now;
    return;
  }
  if (candidatePos == stablePos) return;
  if (now - candidateSince < HEX_DEBOUNCE_MS) return;
  applyMove(stablePos, candidatePos);
  stablePos = candidatePos;
}

// ============================== LCD DISPLAY ================================

// Adds one byte (two nibbles, each with an E pulse) to the open Wire transmission.
void lcdQueue(uint8_t value, uint8_t rs) {
  const uint8_t hi = (uint8_t)((value & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  const uint8_t lo = (uint8_t)(((value << 4) & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  Wire.write((uint8_t)(hi | LCD_BIT_E));
  Wire.write(hi);
  Wire.write((uint8_t)(lo | LCD_BIT_E));
  Wire.write(lo);
}

// Sends a single nibble (only used while the display is being switched to 4-bit mode).
bool lcdNibble(uint8_t nibble) {
  const uint8_t b = (uint8_t)((nibble << 4) | LCD_BIT_BACKLIGHT);
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  Wire.write((uint8_t)(b | LCD_BIT_E));
  Wire.write(b);
  return Wire.endTransmission() == 0;
}

bool lcdCommand(uint8_t command) {
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  lcdQueue(command, 0);
  return Wire.endTransmission() == 0;
}

// The start-up sequence of the HD44780 (4-bit mode). Returns false when nobody answers.
bool lcdInit() {
  lcdOk = false;
  delay(50);
  if (!lcdNibble(0x3)) return false;
  delay(5);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x2);               // switch to 4-bit mode
  delay(1);
  lcdCommand(0x28);             // 4-bit, two lines, 5x8 font
  lcdCommand(0x0C);             // display on, no cursor
  lcdCommand(0x06);             // cursor moves right
  lcdCommand(0x01);             // clear
  delay(3);
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) lcdShown[row][col] = ' ';
  }
  lcdOk = true;
  return true;
}

// Boot-time I2C scan: prints every device that acknowledges its address and tells whether the LCD answered.
uint8_t scanI2cBus() {
  Serial.println(F("I2C scan (0x08-0x77):"));
  uint8_t found = 0;
  bool lcdFound = false;
  for (uint8_t address = 0x08; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    found++;
    if (address == LCD_I2C_ADDRESS) lcdFound = true;
    Serial.print(F("  found 0x"));
    if (address < 16) Serial.print(F("0"));
    Serial.print((int)address, HEX);
    Serial.println(address == LCD_I2C_ADDRESS ? F("  (LCD address in this sketch)") : F(""));
  }
  Serial.print(F("I2C scan done: "));
  Serial.print((int)found);
  Serial.println(F(" device(s)"));
  if (!lcdFound) {
    Serial.print(F("No device on the LCD address 0x"));
    Serial.print((int)LCD_I2C_ADDRESS, HEX);
    Serial.println(F(": check SDA D20 / SCL D21 and the 5 V supply; if the scan lists another address, set LCD_I2C_ADDRESS to it."));
  }
  return found;
}

// Copies text into a row of lcdWanted and pads it with spaces.
void lcdPut(uint8_t row, const char *text) {
  uint8_t col = 0;
  while (col < LCD_COLS && text[col] != 0) {
    lcdWanted[row][col] = text[col];
    col++;
  }
  while (col < LCD_COLS) lcdWanted[row][col++] = ' ';
  lcdWanted[row][LCD_COLS] = 0;
}

const char *directionName() {
  if (lastDir > 0) return "CW";
  if (lastDir < 0) return "CCW";
  return "--";
}

void buildLcdText() {
  char line[24];
  snprintf(line, sizeof(line), "A=%-4d(%d..%d)", valueA, A_MIN, A_MAX);
  lcdPut(0, line);
  unsigned int shownSteps = (unsigned int)(stepCount > 9999UL ? 9999UL : stepCount);
  snprintf(line, sizeof(line), "Pos:%X %-3s n:%u", (unsigned int)stablePos, directionName(), shownSteps);
  lcdPut(1, line);
}

// Sends the next few characters that differ from what the display shows. A failed transmission marks the display as lost.
void lcdSendChanges() {
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) {
      if (lcdWanted[row][col] == lcdShown[row][col]) continue;
      Wire.beginTransmission(LCD_I2C_ADDRESS);
      lcdQueue((uint8_t)(0x80 | (row ? 0x40 : 0x00) | col), 0);  // set the cursor
      uint8_t sent = 0;
      uint8_t startCol = col;
      while (col < LCD_COLS && sent < LCD_CHARS_PER_LOOP) {
        lcdQueue((uint8_t)lcdWanted[row][col], LCD_BIT_RS);
        col++;
        sent++;
      }
      if (Wire.endTransmission() != 0) {     // nobody answered: start again later
        lcdOk = false;
        return;
      }
      for (uint8_t c = startCol; c < col; c++) lcdShown[row][c] = lcdWanted[row][c];
      return;  // the rest follows on the next pass
    }
  }
}

void updateLcd(unsigned long now) {
  if (!lcdOk) {                              // not started yet or lost: try again now and then
    if (now - lcdLastTry >= LCD_RETRY_MS) {
      lcdLastTry = now;
      if (lcdInit()) lcdLastInit = now;
    }
    return;
  }
  if (LCD_REINIT_MS > 0 && now - lcdLastInit >= LCD_REINIT_MS) {  // start from scratch now and then
    lcdLastTry = now;
    lcdLastInit = now;
    lcdInit();
    return;
  }
  if (now - lcdLastBuild >= LCD_BUILD_MS) {
    lcdLastBuild = now;
    buildLcdText();
  }
  lcdSendChanges();
}

// ============================== SIGN OF LIFE ===============================

// Prints the once-per-second debug line (uptime and beats change every time).
void printDebug(unsigned long now) {
  Serial.print(F("DBG up="));
  Serial.print(now / 1000UL);
  Serial.print(F(" pos="));
  Serial.print((int)stablePos, HEX);
  Serial.print(F(" A="));
  Serial.print(valueA);
  Serial.print(F(" dir="));
  Serial.print(directionName());
  Serial.print(F(" steps="));
  Serial.print(stepCount);
  Serial.print(F(" chg="));
  Serial.print(changeCount);
  Serial.print(F(" raw="));
  Serial.print((int)readRawPosition(), HEX);
  Serial.print(F(" beats="));
  Serial.print(beatCount);
  Serial.print(F(" lcd="));
  Serial.println(lcdOk ? 1 : 0);
}

void signOfLife(unsigned long now) {
  if (now - changeWindowStart >= 1000) {     // sliding one-second window of position changes
    changesPrev = changesNow;
    changesNow = 0;
    changeWindowStart = now;
  }
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebug(now);
  }
  const bool busy = (changesNow + changesPrev >= BUSY_CHANGES);
  const unsigned long beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
  if (now - lastBeat >= beatMs) {            // the LED blinks all the time, faster when the switch is turned a lot
    lastBeat = now;
    beatOn = !beatOn;
    beatCount++;
  }
  const bool inverted = anyChange && (now - lastChangeAt < INVERT_MS);   // shortly inverted after a position change
  digitalWrite(PIN_STATUS_LED, (beatOn != inverted) ? HIGH : LOW);
}

// ============================== SETUP / LOOP ===============================

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (uint8_t k = 0; k < 4; k++) pinMode(HEX_PINS[k], INPUT_PULLUP);
  stablePos = readRawPosition();
  candidatePos = stablePos;

  Serial.println(F("Hex rotary switch test: turn the switch, A changes (see the top of sketch.ino)."));
  Serial.print(F("Serial speed "));
  Serial.print(SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));

  Wire.begin();
  Wire.setClock(100000UL);
  scanI2cBus();
  if (lcdInit()) {
    lcdLastInit = millis();
  } else {
    Serial.println(F("LCD did not answer at start-up; trying again every second."));
  }

  const unsigned long now = millis();
  candidateSince = now;
  lastBeat = now;
  lastDebug = now;
  changeWindowStart = now;
  lcdLastTry = now;
  buildLcdText();
}

void loop() {
  const unsigned long now = millis();
  pollSwitch(now);
  signOfLife(now);
  updateLcd(now);
}
