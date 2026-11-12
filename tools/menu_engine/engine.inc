// ============================== MENU ENGINE ===============================
// Same rules as tools/menu-editor/menu-core.js (the reference); tests/menu_diff_test.py checks that both give identical
// display rows, callbacks and EEPROM writes for the same key events. The tables above are generated from the menu JSON.
// Time is passed in as `now` (millis()). All text rows are exactly MENU_COLS characters.

constexpr uint8_t MENU_COLS = 16;
constexpr unsigned long MENU_TIMEOUT_MS = 60000UL;        // at the root level without input: back to the status page
constexpr unsigned long MENU_MARQUEE_PAUSE_MS = 1000UL;   // a long text stays at its start this long
constexpr unsigned long MENU_MARQUEE_STEP_MS = 400UL;     // then moves one character per step
constexpr unsigned long MENU_STATUS_ALT_MS = 3000UL;      // status row 1 alternates with the countdown (an effect is random)
constexpr uint8_t MENU_MAX_DEPTH = 8;
constexpr char MENU_LEFT_ARROW = 0x7F;                    // HD44780 ROM arrows
constexpr char MENU_RIGHT_ARROW = 0x7E;
constexpr char MENU_LOCK_CHAR = 1;                        // custom character 1 (a padlock) drawn by the sketch; 0 is not usable in C strings
constexpr uint8_t MENU_KEY_OK = 0;
constexpr uint8_t MENU_KEY_BACK = 1;
constexpr uint8_t MENU_NO_SLOT = 255;                    // property is not stored in EEPROM

constexpr uint8_t MM_STATUS = 0, MM_MENU = 1, MM_EDIT = 2, MM_CONFIRM = 3, MM_VIEW = 4;   // MM_VIEW: a read-only property shown like an edit page
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
constexpr unsigned long MENU_UNDO_MS = 300UL;   // a move straight back to the item just left (contact bounce) keeps that item's scroll
bool menuUndoValid = false;
uint8_t menuUndoItem = 0;
unsigned long menuUndoSince = 0, menuUndoAt = 0;
uint8_t menuActiveFrame = 0, menuActiveLetters = 0;   // effect numbers (1..15) really showing; 0 = unknown (shown as --)
long menuChangeInSec = -1;                       // seconds until the random effects change (-1 = unknown)
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
    char tmp[9];
    uint8_t t = 0;
    const char hexDigits[] = "0123456789ABCDEF";
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
  menuUndoValid = false;
  menuMode = MM_STATUS;
  menuDepth = 0;
  menuSince = now;
}

// Status page text of one effect: two digits, with a star before them when the effect is random (then the number
// of the effect really showing, or -- when it is not known).
void menuEffectText(uint8_t prop, uint8_t active, char *out) {
  const bool random = menuPosArr[prop] == 0;
  const unsigned n = random ? active : menuPosArr[prop];
  if (n > 0 && n < 100) snprintf(out, 8, "%s%02u", random ? "*" : "", n);
  else snprintf(out, 8, "%s--", random ? "*" : "");
}

// Builds the two display rows for time `now`.
void menuRows(unsigned long now, char *row0, char *row1) {
  char a[48], b[48];
  if (menuMode == MM_STATUS) {
    char valueTextBuf[24];
    menuValueText(MENU_STATUS_CASINO, menuPosArr[MENU_STATUS_CASINO], valueTextBuf);
    char ef[8], el[8];
    menuEffectText(MENU_STATUS_EFFECT_FRAME, menuActiveFrame, ef);
    menuEffectText(MENU_STATUS_EFFECT_LETTERS, menuActiveLetters, el);
    snprintf(a, sizeof(a), "F%s L%s %s", ef, el, valueTextBuf);
    const bool left = menuPosArr[MENU_STATUS_ARROW] == 0;
    if (MENU_STATUS_ARROW_ASCII) snprintf(b, sizeof(b), "%s", left ? "<-Customer right" : "->Customer left");
    else snprintf(b, sizeof(b), "%c Customer %s", left ? MENU_LEFT_ARROW : MENU_RIGHT_ARROW, left ? "right" : "left");
    if ((menuPosArr[MENU_STATUS_EFFECT_FRAME] == 0 || menuPosArr[MENU_STATUS_EFFECT_LETTERS] == 0) && menuChangeInSec >= 0) {
      const unsigned long since = now >= menuSince ? now - menuSince : 0;
      if ((since / MENU_STATUS_ALT_MS) % 2 == 1) snprintf(b, sizeof(b), "Change in %ld:%02ld", menuChangeInSec / 60, menuChangeInSec % 60);
    }
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, b, "", now, menuSince);
    return;
  }
  if (menuMode == MM_EDIT || menuMode == MM_VIEW) {
    menuStr(menuRd16(PP_NAME, menuEditProp), a, sizeof(a));
    char v[24];
    menuValueText(menuEditProp, menuPosArr[menuEditProp], v);
    snprintf(b, sizeof(b), "%c %s", menuMode == MM_VIEW ? MENU_LOCK_CHAR : '>', v);
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
    if (strlen(a) + 1 + strlen(v) <= MENU_COLS) menuFit(row1, a, v, now, menuSince);
    else {                                          // name and value that do not fit together scroll as one text
      snprintf(b, sizeof(b), "%s: %s", a, v);
      menuFit(row1, b, "", now, menuSince);
    }
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
  return prop == MENU_STATUS_CASINO || prop == MENU_STATUS_ARROW;
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
    if (k == menuLevelItem[level]) menuTouch(now, false);        // already at the first or last item: nothing moves, the scroll goes on
    else if (menuUndoValid && menuUndoItem == (uint8_t)k && now - menuUndoAt <= MENU_UNDO_MS) {
      menuLevelItem[level] = (uint8_t)k; menuSince = menuUndoSince; menuUndoValid = false; menuTouch(now, false);   // straight back: its scroll goes on
    } else {
      menuUndoValid = true; menuUndoItem = menuLevelItem[level]; menuUndoSince = menuSince; menuUndoAt = now;
      menuLevelItem[level] = (uint8_t)k; menuTouch(now, true);
    }
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
  menuUndoValid = false;
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
      menuMode = (menuRd8(PP_FLAGS, a) & MF_READONLY) ? MM_VIEW : MM_EDIT;
      menuEditProp = a;
      menuEditStart = menuPosArr[a];
      menuTouch(now, true);
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
  if (menuMode == MM_VIEW) {                        // a read-only property: any key goes back
    menuMode = MM_MENU;
    menuTouch(now, true);
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
