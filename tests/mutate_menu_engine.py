# (Equivalent mutants left out on purpose: this menu has only integer steps of 1, no negative values, no property with two callbacks, a preset with confirmText, only status properties given to menuPhysical, a menu no deeper than 3, an empty preset set list, and no submenu text override.)
# Mutation test of the menu engine and test shell in menu_test/sketch.ino: every mutant must be detected by the C++ checks
# (tests/menu_engine_checks.inc) or by the difference test against the JS engine.   python3 tests/mutate_menu_engine.py
import subprocess, os, tempfile, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'menu_test', 'sketch.ino'), encoding='utf-8').read()
MENU = os.path.join(ROOT, 'tools', 'menu-editor', 'menu-v2.json')
muts = [
 # engine: maths and texts
 ("linear value", "minV + (float)pos * (maxV - minV) / (float)aux;", "minV + (float)pos * (maxV - minV) / (float)(aux + 1);"),
 ("log value", "powf(maxV / minV, (float)pos / (float)aux)", "powf(maxV / minV, (float)pos / (float)(aux + 1))"),
 ("wrap flag", "if (menuRd8(PP_FLAGS, prop) & MF_WRAP) return", "if (true) return"),
 ("clamp low", "  if (q < 0) q = 0;\n  if (q > n - 1) q = n - 1;\n  return (uint16_t)q;", "  if (q > n - 1) q = n - 1;\n  return (uint16_t)q;"),
 ("clamp high", "  if (q < 0) q = 0;\n  if (q > n - 1) q = n - 1;\n  return (uint16_t)q;", "  if (q < 0) q = 0;\n  return (uint16_t)q;"),
 ("round half", "v = (unsigned long)floorf(a * (float)scale + 0.5f);", "v = (unsigned long)floorf(a * (float)scale);"),
 ("leading zero", "while (v > 0 || len <= dec);", "while (v > 0);"),
 ("hex digits", "const char hexDigits[] = \"0123456789ABCDEF\";\n    char tmp[9];", "const char hexDigits[] = \"0123456789abcdef\";\n    char tmp[9];"),
 ("unit appended", "if (unit != MENU_NO_STR16) {\n    char u[12];", "if (false) {\n    char u[12];"),
 ("value texts", "if (fabsf(v - menuRdF(VT_KEY, at)) < 0.0000005f)", "if (false)"),
 ("choice text offset", "menuRd16(PP_CH_FIRST, prop) + pos", "pos"),
 # engine: rows
 ("fit gap", "if (p < MENU_COLS) out[p++] = ' ';", ""),
 ("fit right edge", "int avail = MENU_COLS - rl - (rl ? 1 : 0);", "int avail = MENU_COLS - rl;"),
 ("marquee pause", "MENU_MARQUEE_PAUSE_MS = 1000UL", "MENU_MARQUEE_PAUSE_MS = 900UL"),
 ("marquee step", "MENU_MARQUEE_STEP_MS = 400UL", "MENU_MARQUEE_STEP_MS = 350UL"),
 ("marquee cap", "if (o > maxOff) o = maxOff;", ""),
 ("marquee from since", "unsigned long t = now >= since ? now - since : 0;", "unsigned long t = now;"),
 ("status hex", "hexDigits[(long)floorf(menuValue(MENU_STATUS_PROGRAM) + 0.5f) & 15]", "hexDigits[0]"),
 ("status arrow", "const bool left = menuPosArr[MENU_STATUS_ARROW] == 0;", "const bool left = menuPosArr[MENU_STATUS_ARROW] == 1;"),
 ("status arrow chars", "left ? MENU_LEFT_ARROW : MENU_RIGHT_ARROW", "left ? MENU_RIGHT_ARROW : MENU_LEFT_ARROW"),
 ("left arrow code", "MENU_LEFT_ARROW = 0x7F;", "MENU_LEFT_ARROW = 0x7E;"),
 ("right arrow code", "MENU_RIGHT_ARROW = 0x7E;", "MENU_RIGHT_ARROW = 0x7F;"),
 ("status alternation", "MENU_STATUS_ALT_MS = 3000UL", "MENU_STATUS_ALT_MS = 2000UL"),
 ("status alternation phase", "if ((since / MENU_STATUS_ALT_MS) % 2 == 1)", "if ((since / MENU_STATUS_ALT_MS) % 2 == 0)"),
 ("status countdown seconds", "menuChangeInSec / 60, menuChangeInSec % 60", "menuChangeInSec / 60, menuChangeInSec % 59"),
 ("status countdown needs value", "menuChangeInSec >= 0) {\n      const unsigned long since", "true) {\n      const unsigned long since"),
 ("status random value", "menuValue(MENU_STATUS_PROGRAM) == (float)MENU_STATUS_RANDOM_VALUE", "menuValue(MENU_STATUS_PROGRAM) >= (float)MENU_STATUS_RANDOM_VALUE - 1"),
 ("edit row prefix", "snprintf(b, sizeof(b), \"> %s\", v);", "snprintf(b, sizeof(b), \">%s\", v);"),
 ("confirm hint", "menuFit(row1, \"OK=yes BACK=no\"", "menuFit(row1, \"OK=yes\""),
 ("countdown rounding", "const long sec = rem <= 0 ? 0 : (rem + 999) / 1000;", "const long sec = rem <= 0 ? 0 : rem / 1000;"),
 ("timeout constant", "MENU_TIMEOUT_MS = 60000UL", "MENU_TIMEOUT_MS = 30000UL"),
 ("submenu marker", "menuFit(row1, a, \">\", now, menuSince);", "menuFit(row1, a, \"\", now, menuSince);"),
 ("title is opener", "menuStr(menuRd16(IT_NAME, menuLevelOpener[level]), a, sizeof(a));", "menuStr(MENU_ROOT_NAME_STR, a, sizeof(a));"),
 # engine: events
 ("no change no callback", "  if (pos == menuPosArr[prop]) return false;\n  menuPosArr[prop] = pos;", "  menuPosArr[prop] = pos;"),
 ("physical cancels edit", "if (isStatus && menuMode == MM_EDIT) menuSetPos(menuEditProp, menuEditStart);", ""),
 ("physical to status", "if (isStatus && menuMode != MM_STATUS) menuToStatus(now);", ""),
 ("status props", "return prop == MENU_STATUS_PROGRAM || prop == MENU_STATUS_CASINO || prop == MENU_STATUS_ARROW;", "return prop == MENU_STATUS_PROGRAM || prop == MENU_STATUS_CASINO;"),
 ("list clamp low", "    if (k < 0) k = 0;\n    if (k > n - 1) k = n - 1;\n    if (k != menuLevelItem[level])", "    if (k > n - 1) k = n - 1;\n    if (k != menuLevelItem[level])"),
 ("list clamp high", "    if (k < 0) k = 0;\n    if (k > n - 1) k = n - 1;\n    if (k != menuLevelItem[level])", "    if (k < 0) k = 0;\n    if (k != menuLevelItem[level])"),
 ("list wrap", "    if (k < 0) k = 0;\n    if (k > n - 1) k = n - 1;\n    if (k != menuLevelItem[level])", "    if (k < 0) k = n - 1;\n    if (k > n - 1) k = 0;\n    if (k != menuLevelItem[level])"),
 ("turn touches input", "    else menuTouch(now, false);\n  } else if (menuMode == MM_EDIT) {", "  } else if (menuMode == MM_EDIT) {"),
 ("shift factor", "const long factor = shift ? menuRd8(PP_SHIFT, menuEditProp) : 1;", "const long factor = 1;"),
 ("shift always", "const long factor = shift ? menuRd8(PP_SHIFT, menuEditProp) : 1;", "const long factor = menuRd8(PP_SHIFT, menuEditProp);"),
 ("preset setAll", "if (flags & 2) {", "if (false) {"),
 ("preset setAll readonly", "if (menuRd8(PP_FLAGS, p) & MF_READONLY) continue;", ""),
 ("preset changed only", "if (menuPosArr[p] != want[p]) { menuPosArr[p] = want[p]; changed[changedCount++] = p; }", "{ menuPosArr[p] = want[p]; changed[changedCount++] = p; }"),
 ("preset callbacks", "for (uint8_t j = 0; j < changedCount; j++) menuCallCallbacks(changed[j]);", ""),
 ("preset save", "menuSave(changed, changedCount);\n}", "}"),
 ("open root", "menuLevelMenu[0] = 0;\n      menuLevelItem[0] = 0;", "menuLevelMenu[0] = 0;"),
 ("root back leaves", "else menuToStatus(now);\n      return;", "else menuTouch(now, true);\n      return;"),
 ("pop level", "if (menuDepth > 1) { menuDepth--; menuTouch(now, true); }", "if (menuDepth > 1) { menuDepth--; menuLevelItem[menuDepth - 1] = 0; menuTouch(now, true); }"),
 ("push opener", "menuLevelOpener[menuDepth] = item;", ""),
 ("push item reset", "menuLevelItem[menuDepth] = 0;\n        menuLevelOpener", "menuLevelOpener"),
 ("readonly edit", "if (!(menuRd8(PP_FLAGS, a) & MF_READONLY)) {", "if (true) {"),
 ("edit start", "menuEditStart = menuPosArr[a];", "menuEditStart = 0;"),
 ("confirm flag", "} else if (menuRd8(PR_FLAGS, a) & 1) {", "} else if (false) {"),
 ("edit OK saves", "if (key == MENU_KEY_OK) menuSave(&menuEditProp, 1);\n    else menuSetPos(menuEditProp, menuEditStart);", "menuSave(&menuEditProp, 1);"),
 ("edit BACK restores", "if (key == MENU_KEY_OK) menuSave(&menuEditProp, 1);\n    else menuSetPos(menuEditProp, menuEditStart);", "if (key == MENU_KEY_OK) menuSave(&menuEditProp, 1);"),
 ("confirm OK", "if (key == MENU_KEY_OK) menuApplyPreset(menuConfirmPreset);", "menuApplyPreset(menuConfirmPreset);"),
 ("timeout only at root", "if (menuMode == MM_MENU && menuDepth == 1 && now - menuLastInput", "if (menuMode == MM_MENU && now - menuLastInput"),
 ("timeout compare", "now - menuLastInput >= MENU_TIMEOUT_MS) menuToStatus", "now - menuLastInput > MENU_TIMEOUT_MS) menuToStatus"),
 # engine: EEPROM
 ("tag byte order", "(uint8_t)(MENU_LAYOUT_TAG >> (8 * k)));\n}", "(uint8_t)(MENU_LAYOUT_TAG >> (8 * (3 - k))));\n}"),
 ("slot offset", "EEPROM.update(MENU_EEPROM_BASE + 4 + slot, (uint8_t)menuPosArr[prop]);", "EEPROM.update(MENU_EEPROM_BASE + 3 + slot, (uint8_t)menuPosArr[prop]);"),
 ("save filter persist", "if ((f & MF_PERSIST) && !(f & MF_READONLY)) list[count++] = props[k];", "list[count++] = props[k];"),
 ("save empty batch", "  if (count == 0) return;\n#ifdef MENU_TEST_LOG", "#ifdef MENU_TEST_LOG"),
 ("first save writes all", "  if (!menuEepromValid) {", "  if (false) {"),
 ("valid after save", "    menuEepromValid = true;\n    return;", "    return;"),
 ("begin tag check", "if (EEPROM.read(MENU_EEPROM_BASE + k) != (uint8_t)(MENU_LAYOUT_TAG >> (8 * k))) ok = false;", ""),
 ("begin range check", "EEPROM.read(MENU_EEPROM_BASE + 4 + slot) >= menuPropCount(p)) ok = false;", "EEPROM.read(MENU_EEPROM_BASE + 4 + slot) > menuPropCount(p)) ok = false;"),
 ("begin no range check", "EEPROM.read(MENU_EEPROM_BASE + 4 + slot) >= menuPropCount(p)) ok = false;", "false) ok = false;"),
 ("begin defaults", "menuPosArr[p] = (ok && slot != MENU_NO_SLOT) ? (uint16_t)EEPROM.read(MENU_EEPROM_BASE + 4 + slot) : menuRd16(PP_DEFPOS, p);", "menuPosArr[p] = (slot != MENU_NO_SLOT) ? (uint16_t)EEPROM.read(MENU_EEPROM_BASE + 4 + slot) : menuRd16(PP_DEFPOS, p);"),
 ("begin to status", "  menuEepromValid = ok;\n  menuMode = MM_STATUS;\n  menuDepth = 0;", "  menuEepromValid = ok;"),
 # shell
 ("program pins", "PROGRAM_PINS[4] = {34, 35, 36, 37};", "PROGRAM_PINS[4] = {35, 34, 36, 37};"),
 ("menu pins", "MENU_PINS[4] = {42, 43, 44, 45};", "MENU_PINS[4] = {43, 42, 44, 45};"),
 ("hex polarity", "HEX_BIT_IS_LOW = true;", "HEX_BIT_IS_LOW = false;"),
 ("hex debounce", "HEX_DEBOUNCE_MS = 10;", "HEX_DEBOUNCE_MS = 0;"),
 ("menu switch direction", "MENU_SWITCH_REVERSED = false;", "MENU_SWITCH_REVERSED = true;"),
 ("button pins ok", "PIN_OK = 48;", "PIN_OK = 49;"),
 ("button pins back", "PIN_BACK = 49;", "PIN_BACK = 48;"),
 ("shift pin", "PIN_SHIFT = 50;", "PIN_SHIFT = 51;"),
 ("arrow pin", "PIN_ARROW_SWITCH = 32;", "PIN_ARROW_SWITCH = 33;"),
 ("casino pin", "PIN_CASINO_SWITCH = 30;", "PIN_CASINO_SWITCH = 31;"),
 ("arrow polarity", "ARROW_POINTS_LEFT_WHEN_LOW = true;", "ARROW_POINTS_LEFT_WHEN_LOW = false;"),
 ("casino polarity", "CASINO_CLOSED_WHEN_LOW = true;", "CASINO_CLOSED_WHEN_LOW = false;"),
 ("button debounce", "BUTTON_DEBOUNCE_MS = 30;", "BUTTON_DEBOUNCE_MS = 0;"),
 ("ambiguous jump", "if (delta != 8) {", "if (delta != 9) {"),
 ("backward steps", "if (delta > 8) steps = (int)delta - 16;", ""),
 ("shift pressed", "menuTurn(steps, buttonShift.stable, now);", "menuTurn(steps, false, now);"),
 ("keys on press only", "if (pollButton(buttonOk, now) && buttonOk.stable)", "if (pollButton(buttonOk, now))"),
 ("arrow reverse", "if (menuPos(P_arrowReverse) == 1) left = !left;", ""),
 ("casino reverse", "if (menuPos(P_casinoReverse) == 1) closed = !closed;", ""),
 ("apply switches", "if (shellApplySwitches && cb == CB_applySwitches) {", "if (false) {"),
 ("lcd address", "LCD_I2C_ADDRESS = 0x27;", "LCD_I2C_ADDRESS = 0x3F;"),
 ("serial speed", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("heartbeat", "HEARTBEAT_MS = 500;", "HEARTBEAT_MS = 400;"),
 ("random countdown", "randomDelaySec = minutes * 60L;", "randomDelaySec = minutes * 30L;"),
 ("random program", "if (menuPos(P_program) != MENU_STATUS_RANDOM_VALUE) {", "if (menuPos(P_program) != 14) {"),
 ("debug mode letter", "if (menuMode == MM_EDIT) return 'E';", "if (menuMode == MM_EDIT) return 'M';"),
 ("eeprom base", "MENU_EEPROM_BASE = 0;", "MENU_EEPROM_BASE = 8;"),
]
print('building scenarios...')
tmp = tempfile.mkdtemp()
seeds = (1, 2, 3)
for s in seeds:
    subprocess.run(['node', os.path.join(ROOT, 'tests', 'menu_diff_scenario.js'), MENU, str(s), '3500', os.path.join(tmp, 'sc%d' % s), os.path.join(tmp, 'js%d' % s)], check=True)
def run(args):
    k, (name, old, new) = args
    n = SRC.count(old)
    if n != 1: return name, 'BAD (pattern found %d times)' % n
    d = os.path.join(tmp, 'm%d' % k); os.makedirs(d)
    sk = os.path.join(d, 'sketch.ino'); open(sk, 'w', encoding='utf-8').write(SRC.replace(old, new))
    exe = os.path.join(d, 'h')
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I', os.path.join(ROOT, 'tests'), '-DSKETCH_PATH="%s"' % sk, '-o', exe, os.path.join(ROOT, 'tests', 'menu_engine_harness.cpp')], capture_output=True, text=True)
    if r.returncode: return name, 'BAD (does not compile)'
    try:
        if subprocess.run([exe], capture_output=True, timeout=60).returncode != 0: return name, 'killed'
        for s in seeds:
            out = os.path.join(d, 'cc%d' % s)
            subprocess.run([exe, 'diff', os.path.join(tmp, 'sc%d' % s), out], capture_output=True, timeout=60)
            if not os.path.exists(out) or open(out).read() != open(os.path.join(tmp, 'js%d' % s)).read(): return name, 'killed'
    except subprocess.TimeoutExpired:
        return name, 'killed'
    return name, 'SURVIVED'
with cf.ThreadPoolExecutor(8) as ex:
    res = list(ex.map(run, list(enumerate(muts))))
bad = [r for r in res if r[1] != 'killed']
for n, s in bad: print(s + ': ' + n)
print('%d mutants, %d killed, %d problems' % (len(res), len(res) - len(bad), len(bad)))
raise SystemExit(1 if bad else 0)
