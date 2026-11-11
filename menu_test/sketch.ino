/*
 * Menu test - the settings menu of Casino Open Sign V2 on its real controls and the I2C LCD (no LEDs)
 *
 * Purpose: try the planned settings menu (see ../RDD-REQUIREMENTS.md REQ-001..017) on the real hardware before it goes
 * into the sign: hex rotary switches, the OK / BACK / SHIFT buttons, the two slide switches and the LCD. The menu itself
 * is described in ../tools/menu-editor/menu-v2.json (edit it with the Menu Editor); the tables below are generated from
 * it by tools/generate_menu_test.js, and the engine is tools/menu_engine/engine.inc. Nothing here touches LED strips or sticks.
 *
 * CONTROLS (all wired to GND, internal pull-ups):
 *   program hex switch   legs 1,2,4,8 on D34-D37, C to GND   absolute position 0..F = the program number shown on the
 *                        status page (read all the time; turning it while the menu is open closes the menu)
 *   menu hex switch      legs 1,2,4,8 on D42-D45, C to GND   free-running: turning it moves the selection or changes the value
 *   OK    D48   BACK D49   SHIFT D50      push buttons to GND; SHIFT held while turning makes the step ten times larger
 *   arrow slide switch   D32   handle on the GND side = arrow points left (the "Arrow reverse" menu property swaps it)
 *   casino slide switch  D30   handle on the GND side = casino closed (the "Casino reverse" menu property swaps it)
 *
 * WHAT YOU SEE: the status page (row 0 "#<program> Casino Open|Closed", row 1 arrow + "Customer right|left"; in program F
 * row 1 alternates with "Change in M:SS"). OK opens the menu, the menu switch moves, OK enters or edits, BACK goes back or
 * cancels an edit. A value changes while you turn (the callbacks are printed on the Serial Monitor); OK also saves it to EEPROM.
 * "Restore defaults" asks OK / BACK. The menu closes after 60 s without input at the top level (the seconds are shown).
 * The arrows on row 1 are the LCD's ROM characters 0x7E / 0x7F: if your display shows other symbols, tell the AI
 * (the menu JSON then gets "arrowAscii": true).
 *
 * SIGN OF LIFE: the LED on pin 13 blinks while the program runs (shortly inverted at every key or switch event, faster
 * while many happen). Once per second the Serial Monitor (115200 baud, SERIAL_BAUD) gets a line
 *     DBG up=<s> mode=<S|M|E|C> prog=<hex> arrow=<L|R> casino=<C|O> menu=<n steps> keys=<n> cb=<n> eeprom=<saved|defaults> lcd=<0|1>
 *
 * Only built-in types are used in function signatures (Arduino's automatic prototypes).
 */

#include <Wire.h>
#include <EEPROM.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

// ============================== CONFIGURATION ==============================
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint8_t PIN_STATUS_LED = 13;

constexpr uint8_t PROGRAM_PINS[4] = {34, 35, 36, 37};    // program hex switch: leg 1, 2, 4, 8
constexpr uint8_t MENU_PINS[4] = {42, 43, 44, 45};       // menu hex switch: leg 1, 2, 4, 8
constexpr bool HEX_BIT_IS_LOW = true;                    // a leg connected to C reads LOW and counts as a set bit
constexpr unsigned long HEX_DEBOUNCE_MS = 10;
constexpr bool MENU_SWITCH_REVERSED = false;             // true: turning the menu switch clockwise moves the other way

constexpr uint8_t PIN_OK = 48;
constexpr uint8_t PIN_BACK = 49;
constexpr uint8_t PIN_SHIFT = 50;
constexpr uint8_t PIN_ARROW_SWITCH = 32;
constexpr uint8_t PIN_CASINO_SWITCH = 30;
constexpr bool ARROW_POINTS_LEFT_WHEN_LOW = true;
constexpr bool CASINO_CLOSED_WHEN_LOW = true;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 30;

constexpr int MENU_EEPROM_BASE = 0;                      // the menu image starts at this EEPROM address

constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint8_t LCD_CHARS_PER_LOOP = 6;
constexpr unsigned long LCD_BUILD_MS = 50;
constexpr unsigned long LCD_RETRY_MS = 1000;
constexpr unsigned long LCD_REINIT_MS = 5000;

constexpr unsigned long HEARTBEAT_MS = 500;
constexpr unsigned long HEARTBEAT_FAST_MS = 125;
constexpr unsigned long INVERT_MS = 80;
constexpr unsigned long DEBUG_MS = 1000;
constexpr uint8_t BUSY_EVENTS = 4;

constexpr uint8_t LCD_BIT_RS = 0x01;
constexpr uint8_t LCD_BIT_E = 0x04;
constexpr uint8_t LCD_BIT_BACKLIGHT = 0x08;

// BEGIN GENERATED MENU TABLES (do not edit: made by tools/menu_engine/generate_menu_tables.js from menu-v2.json)
// 21 properties, 29 items, 8 menus, 1 presets; EEPROM layout tag a1802e48, 18 stored properties
constexpr uint8_t MENU_PROP_COUNT = 21;
constexpr uint8_t MENU_STORED_COUNT = 18;
constexpr uint32_t MENU_LAYOUT_TAG = 0xA1802E48UL;
constexpr uint16_t MENU_NO_STR16 = 0xFFFF;
constexpr uint8_t P_program = 0;
constexpr uint8_t P_arrow = 1;
constexpr uint8_t P_casino = 2;
constexpr uint8_t P_arrowReverse = 3;
constexpr uint8_t P_casinoReverse = 4;
constexpr uint8_t P_speedFrame = 5;
constexpr uint8_t P_speedArrow = 6;
constexpr uint8_t P_speedLetters = 7;
constexpr uint8_t P_randomDelay = 8;
constexpr uint8_t P_stripMin = 9;
constexpr uint8_t P_stripMax = 10;
constexpr uint8_t P_stripExp = 11;
constexpr uint8_t P_stickMin = 12;
constexpr uint8_t P_stickMax = 13;
constexpr uint8_t P_stickExp = 14;
constexpr uint8_t P_stripMix = 15;
constexpr uint8_t P_lettersMin = 16;
constexpr uint8_t P_lettersMax = 17;
constexpr uint8_t P_lettersExp = 18;
constexpr uint8_t P_lettersMix = 19;
constexpr uint8_t P_stickMix = 20;
constexpr uint8_t CB_applySwitches = 0;
constexpr uint8_t CB_applySpeeds = 1;
constexpr uint8_t CB_rebuildStripCurve = 2;
constexpr uint8_t CB_rebuildStickCurve = 3;
constexpr uint8_t CB_rebuildLettersCurve = 4;
constexpr uint8_t MENU_CB_COUNT = 5;
constexpr uint8_t MENU_STATUS_PROGRAM = P_program;
constexpr uint8_t MENU_STATUS_CASINO = P_casino;
constexpr uint8_t MENU_STATUS_ARROW = P_arrow;
constexpr uint8_t MENU_STATUS_RANDOM_VALUE = 15;
constexpr bool MENU_STATUS_ARROW_ASCII = false;
constexpr uint16_t MENU_ROOT_NAME_STR = 26;
// strings
const char MENU_STR[] PROGMEM =
  "Program\0"
  "Arrow\0"
  "Left\0"
  "Right\0"
  "Casino\0"
  "Closed\0"
  "Open\0"
  "Arrow reverse\0"
  "No\0"
  "Yes\0"
  "Casino reverse\0"
  "Frame\0"
  "x\0"
  "Letters\0"
  "Change delay\0"
  " min\0"
  "random\0"
  "Min\0"
  "Max\0"
  "Exponent\0"
  "Mix\0"
  "applySwitches\0"
  "applySpeeds\0"
  "rebuildStripCurve\0"
  "rebuildStickCurve\0"
  "rebuildLettersCurve\0"
  "Menu\0"
  "Controls\0"
  "Speed\0"
  "Random program\0"
  "Brightness\0"
  "Restore defaults\0"
  "Strip\0"
  "Stick\0"
  "Restore defaults?\0";
const uint16_t MENU_STR_OFF[] PROGMEM = {0, 8, 14, 19, 25, 32, 39, 44, 58, 61, 65, 80, 86, 88, 96, 109, 114, 121, 125, 129, 138, 142, 156, 168, 186, 204, 224, 229, 238, 244, 259, 270, 287, 293, 299};
// properties
const uint8_t PP_KIND[] PROGMEM = {0, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 2, 0, 0, 2, 1, 0, 0, 2, 1, 1};
const uint8_t PP_FLAGS[] PROGMEM = {9, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
const uint8_t PP_SHIFT[] PROGMEM = {10, 1, 1, 1, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
const uint8_t PP_DEC[] PROGMEM = {0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 2, 2, 2};
const uint16_t PP_COUNT[] PROGMEM = {16, 2, 2, 2, 2, 25, 25, 25, 31, 256, 256, 51, 256, 256, 51, 51, 256, 256, 51, 51, 51};
const uint16_t PP_DEFPOS[] PROGMEM = {0, 0, 0, 0, 0, 12, 12, 12, 2, 0, 80, 31, 0, 80, 31, 50, 0, 80, 31, 50, 50};
const float PP_MIN[] PROGMEM = {0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.250000000f, 0.250000000f, 0.250000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f};
const float PP_MAX[] PROGMEM = {15.0000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 4.00000000f, 4.00000000f, 4.00000000f, 30.0000000f, 255.000000f, 255.000000f, 20.0000000f, 255.000000f, 255.000000f, 20.0000000f, 1.00000000f, 255.000000f, 255.000000f, 20.0000000f, 1.00000000f, 1.00000000f};
const uint16_t PP_AUX[] PROGMEM = {1, 0, 0, 0, 0, 24, 24, 24, 1, 1, 1, 50, 1, 1, 50, 50, 1, 1, 50, 50, 50};
const uint16_t PP_NAME[] PROGMEM = {0, 1, 4, 7, 10, 11, 1, 13, 14, 17, 18, 19, 17, 18, 19, 20, 17, 18, 19, 20, 20};
const uint16_t PP_UNIT[] PROGMEM = {MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, 12, 12, 12, 15, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
const uint8_t PP_VT_FIRST[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t PP_VT_COUNT[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t PP_CH_FIRST[] PROGMEM = {0, 0, 2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
const uint8_t PP_CB_FIRST[] PROGMEM = {0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const uint8_t PP_CB_COUNT[] PROGMEM = {0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t PP_SLOT[] PROGMEM = {255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
const float VT_KEY[] PROGMEM = {0.00000000f};
const uint16_t VT_STR[] PROGMEM = {16};
const uint16_t CH_STR[] PROGMEM = {2, 3, 5, 6, 8, 9, 8, 9};
const uint8_t CB_LIST[] PROGMEM = {0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 2, 4, 4, 4, 4, 3};
const uint16_t CB_NAME_STR[] PROGMEM = {21, 22, 23, 24, 25};
// menus and items
const uint8_t MN_FIRST[] PROGMEM = {0, 5, 10, 13, 14, 17, 21, 25};
const uint8_t MN_COUNT[] PROGMEM = {5, 5, 3, 1, 3, 4, 4, 4};
const uint8_t IT_TYPE[] PROGMEM = {0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t IT_A[] PROGMEM = {1, 2, 3, 4, 0, 0, 1, 3, 2, 4, 5, 7, 6, 8, 5, 6, 7, 9, 10, 11, 15, 16, 17, 18, 19, 12, 13, 14, 20};
const uint16_t IT_NAME[] PROGMEM = {27, 28, 29, 30, 31, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, 32, 13, 33, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
const uint16_t IT_TEXT[] PROGMEM = {MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
// presets
const uint16_t PR_NAME[] PROGMEM = {31};
const uint16_t PR_TEXT[] PROGMEM = {MENU_NO_STR16};
const uint16_t PR_CONFIRM[] PROGMEM = {34};
const uint8_t PR_FLAGS[] PROGMEM = {3};
const uint8_t PR_FIRST[] PROGMEM = {0};
const uint8_t PR_COUNT[] PROGMEM = {0};
const uint8_t PV_PROP[] PROGMEM = {0};
const uint16_t PV_POS[] PROGMEM = {0};
// END GENERATED MENU TABLES

// ============================== MENU ENGINE ===============================
// Same rules as tools/menu-editor/menu-core.js (the reference); tests/menu_diff_test.py checks that both give identical
// display rows, callbacks and EEPROM writes for the same key events. The tables above are generated from the menu JSON.
// Time is passed in as `now` (millis()). All text rows are exactly MENU_COLS characters.

constexpr uint8_t MENU_COLS = 16;
constexpr unsigned long MENU_TIMEOUT_MS = 60000UL;        // at the root level without input: back to the status page
constexpr unsigned long MENU_MARQUEE_PAUSE_MS = 1000UL;   // a long text stays at its start this long
constexpr unsigned long MENU_MARQUEE_STEP_MS = 400UL;     // then moves one character per step
constexpr unsigned long MENU_STATUS_ALT_MS = 3000UL;      // status row 1 alternates with the countdown (program F)
constexpr uint8_t MENU_MAX_DEPTH = 8;
constexpr char MENU_LEFT_ARROW = 0x7F;                    // HD44780 ROM arrows
constexpr char MENU_RIGHT_ARROW = 0x7E;
constexpr uint8_t MENU_KEY_OK = 0;
constexpr uint8_t MENU_KEY_BACK = 1;
constexpr uint8_t MENU_NO_SLOT = 255;                    // property is not stored in EEPROM

constexpr uint8_t MM_STATUS = 0, MM_MENU = 1, MM_EDIT = 2, MM_CONFIRM = 3;
constexpr uint8_t MK_INT = 0, MK_FLOAT = 1, MK_LOG = 2, MK_CHOICE = 3;
constexpr uint8_t MF_READONLY = 1, MF_PERSIST = 2, MF_WRAP = 4, MF_HEX = 8;

uint16_t menuPosArr[MENU_PROP_COUNT];            // current position of every property
uint8_t menuMode = MM_STATUS;
uint8_t menuDepth = 0;                           // number of menu levels open (1 = root)
uint8_t menuLevelMenu[MENU_MAX_DEPTH];           // menu table index of each level
uint8_t menuLevelItem[MENU_MAX_DEPTH];           // selected item (0-based, inside the menu) of each level
uint8_t menuLevelOpener[MENU_MAX_DEPTH];         // global item index that opened the level (its name is the title)
uint8_t menuEditProp = 0;
uint16_t menuEditStart = 0;
uint8_t menuConfirmPreset = 0;
unsigned long menuLastInput = 0;
unsigned long menuSince = 0;
long menuChangeInSec = -1;                       // seconds until the random program changes (-1 = unknown)
bool menuEepromValid = false;                    // the stored image matches this layout

#ifdef MENU_TEST_LOG
struct MenuLogCb { uint8_t cb, prop; uint16_t pos; };
MenuLogCb menuLogCb[4000];
uint16_t menuLogCbCount = 0;
uint8_t menuLogBatch[600];                       // batches: count, then the property indexes
uint16_t menuLogBatchLen = 0;
#endif

void menuCallback(uint8_t cb, uint8_t prop);     // supplied by the sketch: apply the new value of `prop`

// ---- table access
uint8_t menuRd8(const uint8_t *table, uint16_t i) { return pgm_read_byte(&table[i]); }
uint16_t menuRd16(const uint16_t *table, uint16_t i) { return pgm_read_word(&table[i]); }
float menuRdF(const float *table, uint16_t i) { return pgm_read_float(&table[i]); }

// Copies string number `index` into out (at most max-1 characters).
void menuStr(uint16_t index, char *out, uint8_t max) {
  out[0] = 0;
  if (index == MENU_NO_STR16) return;
  const char *p = MENU_STR + menuRd16(MENU_STR_OFF, index);
  uint8_t n = 0;
  while (n + 1 < max) {
    const char c = (char)pgm_read_byte(p + n);
    if (c == 0) break;
    out[n++] = c;
  }
  out[n] = 0;
}

// ---- property maths
uint16_t menuPropCount(uint8_t prop) { return menuRd16(PP_COUNT, prop); }

float menuValueAt(uint8_t prop, uint16_t pos) {
  const uint8_t kind = menuRd8(PP_KIND, prop);
  const float minV = menuRdF(PP_MIN, prop), maxV = menuRdF(PP_MAX, prop);
  const uint16_t aux = menuRd16(PP_AUX, prop);
  if (kind == MK_INT) return minV + (float)pos * (float)aux;
  if (kind == MK_FLOAT) return minV + (float)pos * (maxV - minV) / (float)aux;
  if (kind == MK_LOG) return minV * powf(maxV / minV, (float)pos / (float)aux);
  return (float)pos;
}

float menuValue(uint8_t prop) { return menuValueAt(prop, menuPosArr[prop]); }
uint16_t menuPos(uint8_t prop) { return menuPosArr[prop]; }

uint16_t menuMovePos(uint8_t prop, uint16_t pos, long delta) {
  const long n = menuPropCount(prop);
  long q = (long)pos + delta;
  if (menuRd8(PP_FLAGS, prop) & MF_WRAP) return (uint16_t)(((q % n) + n) % n);
  if (q < 0) q = 0;
  if (q > n - 1) q = n - 1;
  return (uint16_t)q;
}

// Appends the decimal text of x with `dec` decimals (round half away from zero) to out at position n.
uint8_t menuFmtNum(char *out, uint8_t n, float x, uint8_t dec) {
  bool neg;
  unsigned long v;
  if (dec == 0) {                                // same as Math.round: a half rounds up
    const long r = (long)floorf(x + 0.5f);
    neg = r < 0;
    v = (unsigned long)(neg ? -r : r);
  } else {
    neg = x < 0;
    const float a = neg ? -x : x;
    unsigned long scale = 1;
    for (uint8_t k = 0; k < dec; k++) scale *= 10UL;
    v = (unsigned long)floorf(a * (float)scale + 0.5f);
  }
  char digits[12];
  uint8_t len = 0;
  do { digits[len++] = (char)('0' + (v % 10)); v /= 10; } while (v > 0 || len <= dec);
  if (neg) out[n++] = '-';
  while (len > 0) {
    len--;
    out[n++] = digits[len];
    if (len == dec && dec > 0) out[n++] = '.';
  }
  out[n] = 0;
  return n;
}

// The text of a property value, for example "1.12x", "random", "B", "Yes".
void menuValueText(uint8_t prop, uint16_t pos, char *out) {
  const uint8_t kind = menuRd8(PP_KIND, prop);
  out[0] = 0;
  if (kind == MK_CHOICE) {
    menuStr(menuRd16(CH_STR, menuRd16(PP_CH_FIRST, prop) + pos), out, 24);
    return;
  }
  const float v = menuValueAt(prop, pos);
  const uint8_t vtCount = menuRd8(PP_VT_COUNT, prop);
  for (uint8_t k = 0; k < vtCount; k++) {
    const uint8_t at = menuRd8(PP_VT_FIRST, prop) + k;
    if (fabsf(v - menuRdF(VT_KEY, at)) < 0.0000005f) { menuStr(menuRd16(VT_STR, at), out, 24); return; }
  }
  uint8_t n = 0;
  if (menuRd8(PP_FLAGS, prop) & MF_HEX) {
    long r = (long)floorf(v + 0.5f);
    const char hexDigits[] = "0123456789ABCDEF";
    char tmp[9];
    uint8_t t = 0;
    do { tmp[t++] = hexDigits[r & 15]; r >>= 4; } while (r > 0 && t < 8);
    while (t > 0) out[n++] = tmp[--t];
    out[n] = 0;
  } else {
    n = menuFmtNum(out, 0, v, menuRd8(PP_DEC, prop));
  }
  const uint16_t unit = menuRd16(PP_UNIT, prop);
  if (unit != MENU_NO_STR16) {
    char u[12];
    menuStr(unit, u, sizeof(u));
    for (uint8_t k = 0; u[k] != 0 && n + 1 < 24; k++) out[n++] = u[k];
    out[n] = 0;
  }
}

// ---- row building
// Writes into out (MENU_COLS characters + 0): left text, then a gap and the right text aligned to the right edge.
// A left text that does not fit scrolls (marquee) inside the free width.
void menuFit(char *out, const char *left, const char *right, unsigned long now, unsigned long since) {
  const uint8_t ll = (uint8_t)strlen(left), rl = (uint8_t)strlen(right);
  int avail = MENU_COLS - rl - (rl ? 1 : 0);
  if (avail < 0) avail = 0;
  uint8_t off = 0;
  if (ll > avail) {
    const unsigned long maxOff = (unsigned long)(ll - avail);
    unsigned long t = now >= since ? now - since : 0;
    const unsigned long total = MENU_MARQUEE_PAUSE_MS + maxOff * MENU_MARQUEE_STEP_MS + MENU_MARQUEE_PAUSE_MS;
    t = t % total;
    unsigned long o = t < MENU_MARQUEE_PAUSE_MS ? 0 : (t - MENU_MARQUEE_PAUSE_MS) / MENU_MARQUEE_STEP_MS;
    if (o > maxOff) o = maxOff;
    off = (uint8_t)o;
  }
  for (uint8_t k = 0; k < MENU_COLS; k++) out[k] = ' ';
  for (int k = 0; k < avail && k < MENU_COLS; k++) {
    const int src = off + k;
    if (src < ll) out[k] = left[src];
  }
  uint8_t p = (uint8_t)avail;
  if (rl) {
    if (p < MENU_COLS) out[p++] = ' ';
    for (uint8_t k = 0; k < rl && p < MENU_COLS; k++) out[p++] = right[k];
  }
  out[MENU_COLS] = 0;
}

uint8_t menuItemBase(uint8_t menuIndex) { return menuRd8(MN_FIRST, menuIndex); }

void menuTouch(unsigned long now, bool newView) {
  menuLastInput = now;
  if (newView) menuSince = now;
}

void menuToStatus(unsigned long now) {
  menuMode = MM_STATUS;
  menuDepth = 0;
  menuSince = now;
}

// Builds the two display rows for time `now`.
void menuRows(unsigned long now, char *row0, char *row1) {
  char a[48], b[48];
  if (menuMode == MM_STATUS) {
    const char hexDigits[] = "0123456789ABCDEF";
    char valueTextBuf[24];
    menuValueText(MENU_STATUS_CASINO, menuPosArr[MENU_STATUS_CASINO], valueTextBuf);
    snprintf(a, sizeof(a), "#%c Casino %s", hexDigits[(long)floorf(menuValue(MENU_STATUS_PROGRAM) + 0.5f) & 15], valueTextBuf);
    const bool left = menuPosArr[MENU_STATUS_ARROW] == 0;
    if (MENU_STATUS_ARROW_ASCII) snprintf(b, sizeof(b), "%s", left ? "<-Customer right" : "->Customer left");
    else snprintf(b, sizeof(b), "%c Customer %s", left ? MENU_LEFT_ARROW : MENU_RIGHT_ARROW, left ? "right" : "left");
    if (menuValue(MENU_STATUS_PROGRAM) == (float)MENU_STATUS_RANDOM_VALUE && menuChangeInSec >= 0) {
      const unsigned long since = now >= menuSince ? now - menuSince : 0;
      if ((since / MENU_STATUS_ALT_MS) % 2 == 1) snprintf(b, sizeof(b), "Change in %ld:%02ld", menuChangeInSec / 60, menuChangeInSec % 60);
    }
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, b, "", now, menuSince);
    return;
  }
  if (menuMode == MM_EDIT) {
    menuStr(menuRd16(PP_NAME, menuEditProp), a, sizeof(a));
    char v[24];
    menuValueText(menuEditProp, menuPosArr[menuEditProp], v);
    snprintf(b, sizeof(b), "> %s", v);
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, b, "", now, menuSince);
    return;
  }
  if (menuMode == MM_CONFIRM) {
    const uint8_t pr = menuConfirmPreset;
    const uint16_t confirm = menuRd16(PR_CONFIRM, pr);
    if (confirm != MENU_NO_STR16) menuStr(confirm, a, sizeof(a));
    else {
      const uint16_t text = menuRd16(PR_TEXT, pr);
      menuStr(text != MENU_NO_STR16 ? text : menuRd16(PR_NAME, pr), a, sizeof(a) - 1);
      const size_t l = strlen(a);
      a[l] = '?';
      a[l + 1] = 0;
    }
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, "OK=yes BACK=no", "", now, menuSince);
    return;
  }
  const uint8_t level = menuDepth - 1;
  const uint8_t menuIndex = menuLevelMenu[level];
  const uint8_t item = menuItemBase(menuIndex) + menuLevelItem[level];
  if (level == 0) {
    const long rem = (long)MENU_TIMEOUT_MS - (long)(now - menuLastInput);
    const long sec = rem <= 0 ? 0 : (rem + 999) / 1000;
    char s[20];
    snprintf(s, sizeof(s), "%lds", sec);
    menuStr(MENU_ROOT_NAME_STR, a, sizeof(a));
    menuFit(row0, a, s, now, menuSince);
  } else {
    menuStr(menuRd16(IT_NAME, menuLevelOpener[level]), a, sizeof(a));
    menuFit(row0, a, "", now, menuSince);
  }
  const uint8_t type = menuRd8(IT_TYPE, item);
  const uint16_t text = menuRd16(IT_TEXT, item);
  if (type == 0) {                                  // submenu
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(IT_NAME, item), a, sizeof(a));
    menuFit(row1, a, ">", now, menuSince);
  } else if (type == 1) {                           // property
    const uint8_t prop = menuRd8(IT_A, item);
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(PP_NAME, prop), a, sizeof(a));
    char v[24];
    menuValueText(prop, menuPosArr[prop], v);
    menuFit(row1, a, v, now, menuSince);
  } else {                                          // preset
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(IT_NAME, item), a, sizeof(a));
    menuFit(row1, a, "", now, menuSince);
  }
}

// ---- values, callbacks, EEPROM
void menuCallCallbacks(uint8_t prop) {
  const uint8_t n = menuRd8(PP_CB_COUNT, prop);
  for (uint8_t k = 0; k < n; k++) {
    const uint8_t cb = menuRd8(CB_LIST, menuRd8(PP_CB_FIRST, prop) + k);
#ifdef MENU_TEST_LOG
    if (menuLogCbCount < 4000) { menuLogCb[menuLogCbCount].cb = cb; menuLogCb[menuLogCbCount].prop = prop; menuLogCb[menuLogCbCount].pos = menuPosArr[prop]; menuLogCbCount++; }
#endif
    menuCallback(cb, prop);
  }
}

bool menuSetPos(uint8_t prop, uint16_t pos) {       // changes the value and calls the callbacks
  if (pos == menuPosArr[prop]) return false;
  menuPosArr[prop] = pos;
  menuCallCallbacks(prop);
  return true;
}

void menuEepromWriteTag() {
  for (uint8_t k = 0; k < 4; k++) EEPROM.update(MENU_EEPROM_BASE + k, (uint8_t)(MENU_LAYOUT_TAG >> (8 * k)));
}
void menuEepromWriteSlot(uint8_t prop) {
  const uint8_t slot = menuRd8(PP_SLOT, prop);
  if (slot != MENU_NO_SLOT) EEPROM.update(MENU_EEPROM_BASE + 4 + slot, (uint8_t)menuPosArr[prop]);
}

// Writes the given properties (only the stored ones) to EEPROM as one batch.
void menuSave(const uint8_t *props, uint8_t n) {
  uint8_t list[MENU_PROP_COUNT];
  uint8_t count = 0;
  for (uint8_t k = 0; k < n; k++) {
    const uint8_t f = menuRd8(PP_FLAGS, props[k]);
    if ((f & MF_PERSIST) && !(f & MF_READONLY)) list[count++] = props[k];
  }
  if (count == 0) return;
#ifdef MENU_TEST_LOG
  if (menuLogBatchLen + count + 1 < 600) { menuLogBatch[menuLogBatchLen++] = count; for (uint8_t k = 0; k < count; k++) menuLogBatch[menuLogBatchLen++] = list[k]; }
#endif
  if (!menuEepromValid) {                           // first save after a layout change: write the whole image
    menuEepromWriteTag();
    for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) menuEepromWriteSlot(p);
    menuEepromValid = true;
    return;
  }
  for (uint8_t k = 0; k < count; k++) menuEepromWriteSlot(list[k]);
}

// Start-up: values from EEPROM when the stored layout matches and every slot is valid, otherwise all defaults.
bool menuBegin() {
  bool ok = true;
  for (uint8_t k = 0; k < 4; k++) if (EEPROM.read(MENU_EEPROM_BASE + k) != (uint8_t)(MENU_LAYOUT_TAG >> (8 * k))) ok = false;
  for (uint8_t p = 0; p < MENU_PROP_COUNT && ok; p++) {
    const uint8_t slot = menuRd8(PP_SLOT, p);
    if (slot != MENU_NO_SLOT && EEPROM.read(MENU_EEPROM_BASE + 4 + slot) >= menuPropCount(p)) ok = false;
  }
  for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) {
    const uint8_t slot = menuRd8(PP_SLOT, p);
    menuPosArr[p] = (ok && slot != MENU_NO_SLOT) ? (uint16_t)EEPROM.read(MENU_EEPROM_BASE + 4 + slot) : menuRd16(PP_DEFPOS, p);
  }
  menuEepromValid = ok;
  menuMode = MM_STATUS;
  menuDepth = 0;
  return ok;
}

// ---- events
void menuSet(uint8_t prop, uint16_t pos) { menuSetPos(prop, pos); }   // a value changed outside the menu (no EEPROM write)

bool menuIsStatusProp(uint8_t prop) {
  return prop == MENU_STATUS_PROGRAM || prop == MENU_STATUS_CASINO || prop == MENU_STATUS_ARROW;
}

// A physical control changed: the value follows; an open menu closes for the status properties.
void menuPhysical(uint8_t prop, uint16_t pos, unsigned long now) {
  const bool isStatus = menuIsStatusProp(prop);
  if (isStatus && menuMode == MM_EDIT) menuSetPos(menuEditProp, menuEditStart);
  menuSetPos(prop, pos);
  if (isStatus && menuMode != MM_STATUS) menuToStatus(now);
}

void menuTurn(int steps, bool shift, unsigned long now) {
  if (steps == 0) return;
  if (menuMode == MM_MENU) {
    const uint8_t level = menuDepth - 1;
    const long n = menuRd8(MN_COUNT, menuLevelMenu[level]);
    long k = (long)menuLevelItem[level] + steps;
    if (k < 0) k = 0;
    if (k > n - 1) k = n - 1;
    if (k != menuLevelItem[level]) { menuLevelItem[level] = (uint8_t)k; menuTouch(now, true); }
    else menuTouch(now, false);
  } else if (menuMode == MM_EDIT) {
    const long factor = shift ? menuRd8(PP_SHIFT, menuEditProp) : 1;
    menuSetPos(menuEditProp, menuMovePos(menuEditProp, menuPosArr[menuEditProp], (long)steps * factor));
    menuTouch(now, false);
  }
}

// Applies a preset: all values first, then the callbacks once per changed property, then one EEPROM batch.
void menuApplyPreset(uint8_t preset) {
  uint8_t order[MENU_PROP_COUNT];
  uint16_t want[MENU_PROP_COUNT];
  uint8_t n = 0;
  const uint8_t flags = menuRd8(PR_FLAGS, preset);
  if (flags & 2) {                                  // setAll: every property that is not read-only gets its default
    for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) {
      if (menuRd8(PP_FLAGS, p) & MF_READONLY) continue;
      order[n] = p;
      want[p] = menuRd16(PP_DEFPOS, p);
      n++;
    }
  }
  const uint8_t first = menuRd8(PR_FIRST, preset), count = menuRd8(PR_COUNT, preset);
  for (uint8_t k = 0; k < count; k++) {
    const uint8_t p = menuRd8(PV_PROP, first + k);
    bool present = false;
    for (uint8_t j = 0; j < n; j++) if (order[j] == p) present = true;
    if (!present) order[n++] = p;
    want[p] = menuRd16(PV_POS, first + k);
  }
  uint8_t changed[MENU_PROP_COUNT];
  uint8_t changedCount = 0;
  for (uint8_t j = 0; j < n; j++) {
    const uint8_t p = order[j];
    if (menuPosArr[p] != want[p]) { menuPosArr[p] = want[p]; changed[changedCount++] = p; }
  }
  for (uint8_t j = 0; j < changedCount; j++) menuCallCallbacks(changed[j]);
  menuSave(changed, changedCount);
}

void menuKey(uint8_t key, unsigned long now) {
  if (menuMode == MM_STATUS) {
    if (key == MENU_KEY_OK) {
      menuMode = MM_MENU;
      menuDepth = 1;
      menuLevelMenu[0] = 0;
      menuLevelItem[0] = 0;
      menuTouch(now, true);
    }
    return;
  }
  if (menuMode == MM_MENU) {
    if (key == MENU_KEY_BACK) {
      if (menuDepth > 1) { menuDepth--; menuTouch(now, true); }
      else menuToStatus(now);
      return;
    }
    const uint8_t level = menuDepth - 1;
    const uint8_t item = menuItemBase(menuLevelMenu[level]) + menuLevelItem[level];
    const uint8_t type = menuRd8(IT_TYPE, item);
    const uint8_t a = menuRd8(IT_A, item);
    if (type == 0) {
      if (menuDepth < MENU_MAX_DEPTH) {
        menuLevelMenu[menuDepth] = a;
        menuLevelItem[menuDepth] = 0;
        menuLevelOpener[menuDepth] = item;
        menuDepth++;
        menuTouch(now, true);
      }
    } else if (type == 1) {
      if (!(menuRd8(PP_FLAGS, a) & MF_READONLY)) {
        menuMode = MM_EDIT;
        menuEditProp = a;
        menuEditStart = menuPosArr[a];
        menuTouch(now, true);
      }
    } else if (menuRd8(PR_FLAGS, a) & 1) {          // preset with confirmation
      menuMode = MM_CONFIRM;
      menuConfirmPreset = a;
      menuTouch(now, true);
    } else {
      menuApplyPreset(a);
      menuTouch(now, false);
    }
    return;
  }
  if (menuMode == MM_EDIT) {
    if (key == MENU_KEY_OK) menuSave(&menuEditProp, 1);
    else menuSetPos(menuEditProp, menuEditStart);
    menuMode = MM_MENU;
    menuTouch(now, true);
    return;
  }
  if (key == MENU_KEY_OK) menuApplyPreset(menuConfirmPreset);   // confirm page
  menuMode = MM_MENU;
  menuTouch(now, true);
}

void menuTick(unsigned long now) {
  if (menuMode == MM_MENU && menuDepth == 1 && now - menuLastInput >= MENU_TIMEOUT_MS) menuToStatus(now);
}

// ============================== INPUTS =====================================
struct HexSwitch {
  const uint8_t *pins;
  uint8_t stable;
  uint8_t candidate;
  unsigned long since;
};
HexSwitch programSwitch = {PROGRAM_PINS, 0, 0, 0};
HexSwitch menuSwitch = {MENU_PINS, 0, 0, 0};

struct Button {
  uint8_t pin;
  bool stable;              // true = pressed
  bool candidate;
  unsigned long since;
};
Button buttonOk = {PIN_OK, false, false, 0};
Button buttonBack = {PIN_BACK, false, false, 0};
Button buttonShift = {PIN_SHIFT, false, false, 0};
Button switchArrow = {PIN_ARROW_SWITCH, false, false, 0};     // true = pin LOW
Button switchCasino = {PIN_CASINO_SWITCH, false, false, 0};

// Functions below take an index, not a struct: the Arduino IDE builds the function prototypes before the struct
// definitions, so a user-defined type in a signature does not compile.
constexpr uint8_t HEX_PROGRAM = 0, HEX_MENU = 1;
HexSwitch *const hexSwitches[2] = {&programSwitch, &menuSwitch};
constexpr uint8_t BTN_OK = 0, BTN_BACK = 1, BTN_SHIFT = 2, BTN_ARROW = 3, BTN_CASINO = 4;
Button *const buttons[5] = {&buttonOk, &buttonBack, &buttonShift, &switchArrow, &switchCasino};

unsigned long eventCount = 0;          // key and switch events (sign of life)
unsigned long keyCount = 0;
unsigned long menuStepCount = 0;
unsigned long callbackCount = 0;
unsigned long lastEventAt = 0;
bool anyEvent = false;
uint8_t eventsNow = 0, eventsPrev = 0;
unsigned long eventWindowStart = 0;
bool beatOn = false;
unsigned long lastBeat = 0, lastDebug = 0;
bool eepromWasValid = false;
long randomDelaySec = 0;               // test only: seconds the random program (F) would last
unsigned long randomStartedAt = 0;

bool lcdOk = false;
char lcdWanted[LCD_ROWS][LCD_COLS + 1];
char lcdShown[LCD_ROWS][LCD_COLS];
unsigned long lcdLastBuild = 0, lcdLastTry = 0, lcdLastInit = 0;

uint8_t readHexRaw(uint8_t which) {
  const HexSwitch &s = *hexSwitches[which];
  uint8_t value = 0;
  for (uint8_t k = 0; k < 4; k++) {
    const bool low = (digitalRead(s.pins[k]) == LOW);
    if (low == HEX_BIT_IS_LOW) value |= (uint8_t)(1 << k);
  }
  return value;
}

// Debounces a hex switch; returns true and sets `from` when a new stable position was reached.
bool pollHex(uint8_t which, unsigned long now, uint8_t &from) {
  HexSwitch &s = *hexSwitches[which];
  const uint8_t raw = readHexRaw(which);
  if (raw != s.candidate) { s.candidate = raw; s.since = now; return false; }
  if (s.candidate == s.stable) return false;
  if (now - s.since < HEX_DEBOUNCE_MS) return false;
  from = s.stable;
  s.stable = s.candidate;
  return true;
}

// Debounces a push button or slide switch; returns true when its state just changed.
bool pollButton(uint8_t which, unsigned long now) {
  Button &b = *buttons[which];
  const bool raw = (digitalRead(b.pin) == LOW);
  if (raw != b.candidate) { b.candidate = raw; b.since = now; return false; }
  if (b.candidate == b.stable) return false;
  if (now - b.since < BUTTON_DEBOUNCE_MS) return false;
  b.stable = b.candidate;
  return true;
}

void noteEvent(unsigned long now) {
  eventCount++;
  eventsNow++;
  anyEvent = true;
  lastEventAt = now;
}

// The slide switches and their reverse flags decide the Arrow and Casino values.
uint16_t arrowPosition() {                      // 0 = Left, 1 = Right
  bool left = switchArrow.stable == ARROW_POINTS_LEFT_WHEN_LOW;
  if (menuPos(P_arrowReverse) == 1) left = !left;
  return left ? 0 : 1;
}
uint16_t casinoPosition() {                     // 0 = Closed, 1 = Open
  bool closed = switchCasino.stable == CASINO_CLOSED_WHEN_LOW;
  if (menuPos(P_casinoReverse) == 1) closed = !closed;
  return closed ? 0 : 1;
}

bool shellApplySwitches = true;        // (the host difference test switches this off: it plays menu events without switches)

// Called by the menu engine after a property value changed.
void menuCallback(uint8_t cb, uint8_t prop) {
  callbackCount++;
  char name[24];
  menuStr(menuRd16(CB_NAME_STR, cb), name, sizeof(name));
  Serial.print(F("CB "));
  Serial.print(name);
  Serial.print(F(" prop="));
  char pname[24];
  menuStr(menuRd16(PP_NAME, prop), pname, sizeof(pname));
  Serial.print(pname);
  Serial.print(F(" pos="));
  Serial.println((int)menuPos(prop));
  if (shellApplySwitches && cb == CB_applySwitches) {   // the reverse flags changed: the switch values follow at once
    menuSet(P_arrow, arrowPosition());
    menuSet(P_casino, casinoPosition());
  }
}

void pollInputs(unsigned long now) {
  uint8_t from = 0;
  if (pollHex(HEX_PROGRAM, now, from)) {
    noteEvent(now);
    menuPhysical(P_program, programSwitch.stable, now);
  }
  if (pollHex(HEX_MENU, now, from)) {
    noteEvent(now);
    uint8_t delta = (uint8_t)((menuSwitch.stable - from) & 15);
    if (delta != 8) {                           // exactly half a turn: the direction cannot be told, ignored
      int steps = delta;
      if (delta > 8) steps = (int)delta - 16;
      if (MENU_SWITCH_REVERSED) steps = -steps;
      menuStepCount += (unsigned long)(steps < 0 ? -steps : steps);
      menuTurn(steps, buttonShift.stable, now);
    }
  }
  if (pollButton(BTN_SHIFT, now)) noteEvent(now);
  if (pollButton(BTN_OK, now) && buttonOk.stable) { keyCount++; noteEvent(now); menuKey(MENU_KEY_OK, now); }
  if (pollButton(BTN_BACK, now) && buttonBack.stable) { keyCount++; noteEvent(now); menuKey(MENU_KEY_BACK, now); }
  if (pollButton(BTN_ARROW, now)) { noteEvent(now); menuPhysical(P_arrow, arrowPosition(), now); }
  if (pollButton(BTN_CASINO, now)) { noteEvent(now); menuPhysical(P_casino, casinoPosition(), now); }
}

// Test only: the real sign changes the program after the delay property; here only the countdown is shown.
void updateRandomCountdown(unsigned long now) {
  if (menuPos(P_program) != MENU_STATUS_RANDOM_VALUE) { menuChangeInSec = -1; randomDelaySec = 0; return; }
  if (randomDelaySec == 0) {
    long minutes = (long)floorf(menuValue(P_randomDelay) + 0.5f);
    if (minutes == 0) minutes = 1 + (long)(millis() % 30UL);
    randomDelaySec = minutes * 60L;
    randomStartedAt = now;
  }
  const long elapsed = (long)((now - randomStartedAt) / 1000UL);
  if (elapsed >= randomDelaySec) { randomDelaySec = 0; menuChangeInSec = 0; return; }
  menuChangeInSec = randomDelaySec - elapsed;
}

// ============================== LCD DISPLAY ================================
void lcdQueue(uint8_t value, uint8_t rs) {
  const uint8_t hi = (uint8_t)((value & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  const uint8_t lo = (uint8_t)(((value << 4) & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  Wire.write((uint8_t)(hi | LCD_BIT_E));
  Wire.write(hi);
  Wire.write((uint8_t)(lo | LCD_BIT_E));
  Wire.write(lo);
}

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

bool lcdInit() {
  lcdOk = false;
  delay(50);
  if (!lcdNibble(0x3)) return false;
  delay(5);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x2);
  delay(1);
  lcdCommand(0x28);
  lcdCommand(0x0C);
  lcdCommand(0x06);
  lcdCommand(0x01);
  delay(3);
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) lcdShown[row][col] = ' ';
  }
  lcdOk = true;
  return true;
}

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
    Serial.println(F(": check SDA D20 / SCL D21 and the 5 V supply; set LCD_I2C_ADDRESS to the address the scan lists."));
  }
  return found;
}

void buildLcdText(unsigned long now) {
  char r0[LCD_COLS + 1], r1[LCD_COLS + 1];
  menuRows(now, r0, r1);
  memcpy(lcdWanted[0], r0, LCD_COLS + 1);
  memcpy(lcdWanted[1], r1, LCD_COLS + 1);
}

void lcdSendChanges() {
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) {
      if (lcdWanted[row][col] == lcdShown[row][col]) continue;
      Wire.beginTransmission(LCD_I2C_ADDRESS);
      lcdQueue((uint8_t)(0x80 | (row ? 0x40 : 0x00) | col), 0);
      uint8_t sent = 0;
      const uint8_t startCol = col;
      while (col < LCD_COLS && sent < LCD_CHARS_PER_LOOP) {
        lcdQueue((uint8_t)lcdWanted[row][col], LCD_BIT_RS);
        col++;
        sent++;
      }
      if (Wire.endTransmission() != 0) { lcdOk = false; return; }
      for (uint8_t c = startCol; c < col; c++) lcdShown[row][c] = lcdWanted[row][c];
      return;
    }
  }
}

void updateLcd(unsigned long now) {
  if (!lcdOk) {
    if (now - lcdLastTry >= LCD_RETRY_MS) {
      lcdLastTry = now;
      if (lcdInit()) lcdLastInit = now;
    }
    return;
  }
  if (LCD_REINIT_MS > 0 && now - lcdLastInit >= LCD_REINIT_MS) {
    lcdLastTry = now;
    lcdLastInit = now;
    lcdInit();
    return;
  }
  if (now - lcdLastBuild >= LCD_BUILD_MS) {
    lcdLastBuild = now;
    buildLcdText(now);
  }
  lcdSendChanges();
}

// ============================== SIGN OF LIFE ===============================
char modeLetter() {
  if (menuMode == MM_MENU) return 'M';
  if (menuMode == MM_EDIT) return 'E';
  if (menuMode == MM_CONFIRM) return 'C';
  return 'S';
}

void printDebug(unsigned long now) {
  Serial.print(F("DBG up="));
  Serial.print(now / 1000UL);
  Serial.print(F(" mode="));
  Serial.print(modeLetter());
  Serial.print(F(" prog="));
  Serial.print((int)menuPos(P_program), HEX);
  Serial.print(F(" arrow="));
  Serial.print(menuPos(P_arrow) == 0 ? 'L' : 'R');
  Serial.print(F(" casino="));
  Serial.print(menuPos(P_casino) == 0 ? 'C' : 'O');
  Serial.print(F(" menu="));
  Serial.print(menuStepCount);
  Serial.print(F(" keys="));
  Serial.print(keyCount);
  Serial.print(F(" cb="));
  Serial.print(callbackCount);
  Serial.print(F(" eeprom="));
  Serial.print(menuEepromValid ? F("saved") : F("defaults"));
  Serial.print(F(" lcd="));
  Serial.println(lcdOk ? 1 : 0);
}

void signOfLife(unsigned long now) {
  if (now - eventWindowStart >= 1000) {
    eventsPrev = eventsNow;
    eventsNow = 0;
    eventWindowStart = now;
  }
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebug(now);
  }
  const bool busy = (eventsNow + eventsPrev >= BUSY_EVENTS);
  const unsigned long beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
  if (now - lastBeat >= beatMs) {
    lastBeat = now;
    beatOn = !beatOn;
  }
  const bool inverted = anyEvent && (now - lastEventAt < INVERT_MS);
  digitalWrite(PIN_STATUS_LED, (beatOn != inverted) ? HIGH : LOW);
}

// ============================== SETUP / LOOP ===============================
void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (uint8_t k = 0; k < 4; k++) {
    pinMode(PROGRAM_PINS[k], INPUT_PULLUP);
    pinMode(MENU_PINS[k], INPUT_PULLUP);
  }
  pinMode(PIN_OK, INPUT_PULLUP);
  pinMode(PIN_BACK, INPUT_PULLUP);
  pinMode(PIN_SHIFT, INPUT_PULLUP);
  pinMode(PIN_ARROW_SWITCH, INPUT_PULLUP);
  pinMode(PIN_CASINO_SWITCH, INPUT_PULLUP);

  Serial.println(F("Menu test: see the top of sketch.ino."));
  Serial.print(F("Serial speed "));
  Serial.print(SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second, a CB line whenever a menu property calls its callback."));

  eepromWasValid = menuBegin();
  Serial.println(eepromWasValid ? F("EEPROM: saved menu values loaded.") : F("EEPROM: no saved values for this menu layout, defaults used."));

  programSwitch.stable = programSwitch.candidate = readHexRaw(HEX_PROGRAM);
  menuSwitch.stable = menuSwitch.candidate = readHexRaw(HEX_MENU);
  switchArrow.stable = switchArrow.candidate = (digitalRead(PIN_ARROW_SWITCH) == LOW);
  switchCasino.stable = switchCasino.candidate = (digitalRead(PIN_CASINO_SWITCH) == LOW);
  menuSet(P_program, programSwitch.stable);
  menuSet(P_arrow, arrowPosition());
  menuSet(P_casino, casinoPosition());

  Wire.begin();
  Wire.setClock(100000UL);
  scanI2cBus();
  if (lcdInit()) lcdLastInit = millis();
  else Serial.println(F("LCD did not answer at start-up; trying again every second."));

  const unsigned long now = millis();
  lastBeat = lastDebug = eventWindowStart = lcdLastTry = now;
  menuLastInput = menuSince = now;
  buildLcdText(now);
}

void loop() {
  const unsigned long now = millis();
  pollInputs(now);
  menuTick(now);
  updateRandomCountdown(now);
  signOfLife(now);
  updateLcd(now);
}
