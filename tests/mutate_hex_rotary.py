# Mutation test of hex_rotary_test/sketch.ino: every mutant must make tests/hex_rotary_test_harness.cpp report failures.
#   python3 tests/mutate_hex_rotary.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'hex_rotary_test', 'sketch.ino')).read()
muts = [
 # switch pins and polarity
 ("pins 1<->2", "HEX_PINS[4] = {34, 35, 36, 37};", "HEX_PINS[4] = {35, 34, 36, 37};"),
 ("pins 2<->4", "HEX_PINS[4] = {34, 35, 36, 37};", "HEX_PINS[4] = {34, 36, 35, 37};"),
 ("pins 4<->8", "HEX_PINS[4] = {34, 35, 36, 37};", "HEX_PINS[4] = {34, 35, 37, 36};"),
 ("pins shifted", "HEX_PINS[4] = {34, 35, 36, 37};", "HEX_PINS[4] = {35, 36, 37, 38};"),
 ("bit polarity", "HEX_BIT_IS_LOW = true;", "HEX_BIT_IS_LOW = false;"),
 ("debounce 9", "HEX_DEBOUNCE_MS = 10;", "HEX_DEBOUNCE_MS = 9;"),
 ("debounce 11", "HEX_DEBOUNCE_MS = 10;", "HEX_DEBOUNCE_MS = 11;"),
 ("debounce 0", "HEX_DEBOUNCE_MS = 10;", "HEX_DEBOUNCE_MS = 0;"),
 ("timing not restarted", "    candidatePos = raw;\n    candidateSince = now;\n    return;", "    candidatePos = raw;\n    return;"),
 ("no early return on same position", "  if (candidatePos == stablePos) return;\n", ""),
 ("stable position not stored", "  applyMove(stablePos, candidatePos);\n  stablePos = candidatePos;", "  applyMove(stablePos, candidatePos);"),
 ("start position not read", "  stablePos = readRawPosition();\n  candidatePos = stablePos;", "  candidatePos = readRawPosition();"),
 # the number A
 ("A_MIN 1", "A_MIN = 2;", "A_MIN = 1;"),
 ("A_MIN 3", "A_MIN = 2;", "A_MIN = 3;"),
 ("A_MAX 50", "A_MAX = 51;", "A_MAX = 50;"),
 ("A_MAX 52", "A_MAX = 51;", "A_MAX = 52;"),
 ("A_STEP 2", "A_STEP = 1;", "A_STEP = 2;"),
 ("A_START 25", "A_START = 26;", "A_START = 25;"),
 ("no lower clamp", "  if (next < A_MIN) next = A_MIN;\n", ""),
 ("no upper clamp", "  if (next > A_MAX) next = A_MAX;\n", ""),
 ("lower wrap", "if (next < A_MIN) next = A_MIN;", "if (next < A_MIN) next = A_MAX;"),
 ("upper wrap", "if (next > A_MAX) next = A_MAX;", "if (next > A_MAX) next = A_MIN;"),
 ("A not stored", "  valueA = next;\n", ""),
 # direction and step count from the position change
 ("reverse bit starts true", "bool reverseDirection = false;", "bool reverseDirection = true;"),
 ("reverse bit ignored", "  if (reverseDirection) dir = -dir;\n", ""),
 ("delta mask", "(uint8_t)((to - from) & 15)", "(uint8_t)((to - from) & 7)"),
 ("delta from/to swapped", "(uint8_t)((to - from) & 15)", "(uint8_t)((from - to) & 15)"),
 ("ambiguous at 9", "if (delta == 8) {", "if (delta == 9) {"),
 ("ambiguous at 7", "if (delta == 8) {", "if (delta == 7) {"),
 ("ambiguous counts forward", "if (delta == 8) {                          // exactly half a turn: forward or backward cannot be told\n    ambiguousCount++;\n    return;\n  }", "ambiguousCount++;"),
 ("ambiguous not counted", "    ambiguousCount++;\n", ""),
 ("backward from 10", "if (delta > 8) {", "if (delta > 9) {"),
 ("backward steps 15", "steps = 16 - delta;", "steps = 15 - delta;"),
 ("backward steps 17", "steps = 16 - delta;", "steps = 17 - delta;"),
 ("dir not negated", "    dir = -1;\n  }", "    dir = 1;\n  }"),
 ("step counter +1", "stepCount += (unsigned long)steps;", "stepCount += 1;"),
 ("last direction fixed", "lastDir = (int8_t)dir;", "lastDir = 1;"),
 ("last direction not stored", "  lastDir = (int8_t)dir;\n", ""),
 ("change counter off", "  changeCount++;\n  lastChangeAt", "  lastChangeAt"),
 ("change window off", "  changesNow++;\n", ""),
 ("change time off", "  lastChangeAt = millis();\n", ""),
 ("any change off", "  anyChange = true;\n", ""),
 # LCD text
 ("row 0 format", 'snprintf(line, sizeof(line), "A=%-4d(%d..%d)", valueA, A_MIN, A_MAX);', 'snprintf(line, sizeof(line), "A=%d(%d..%d)", valueA, A_MIN, A_MAX);'),
 ("row 0 label", '"A=%-4d(%d..%d)"', '"B=%-4d(%d..%d)"'),
 ("row 1 hex lower case", '"Pos:%X %-3s n:%u"', '"Pos:%x %-3s n:%u"'),
 ("row 1 label", '"Pos:%X %-3s n:%u"', '"Pos:%X %-3s m:%u"'),
 ("row 1 shows raw not stable", "(unsigned int)stablePos, directionName()", "(unsigned int)readRawPosition(), directionName()"),
 ("cap 99", "(stepCount > 9999UL ? 9999UL : stepCount)", "(stepCount > 99UL ? 99UL : stepCount)"),
 ("cap none", "(stepCount > 9999UL ? 9999UL : stepCount)", "stepCount"),
 ("cap big", "stepCount > 9999UL ? 9999UL : stepCount", "stepCount > 99999UL ? 99999UL : stepCount"),
 ("CW text", 'return "CW";', 'return "CCW";'),
 ("CCW text", 'return "CCW";', 'return "CW";'),
 ("none text", 'return "--";', 'return "..";'),
 ("direction sign", "if (lastDir > 0) return", "if (lastDir < 0) return"),
 # LCD driver
 ("LCD address", "LCD_I2C_ADDRESS = 0x27;", "LCD_I2C_ADDRESS = 0x3F;"),
 ("chars per loop 8", "LCD_CHARS_PER_LOOP = 6;", "LCD_CHARS_PER_LOOP = 8;"),
 ("chars per loop 1", "LCD_CHARS_PER_LOOP = 6;", "LCD_CHARS_PER_LOOP = 1;"),
 ("4-bit switch", "lcdNibble(0x2);               // switch to 4-bit mode", "lcdNibble(0x3);               // switch to 4-bit mode"),
 ("clear missing", "  lcdCommand(0x01);             // clear\n", ""),
 ("shown not reset", "    for (uint8_t col = 0; col < LCD_COLS; col++) lcdShown[row][col] = ' ';", "    for (uint8_t col = 0; col < LCD_COLS; col++) lcdShown[row][col] = lcdWanted[row][col];"),
 ("lcdOk never true", "  lcdOk = true;\n  return true;", "  return true;"),
 ("failure not noticed", "      if (Wire.endTransmission() != 0) {     // nobody answered: start again later\n        lcdOk = false;\n        return;\n      }", "      Wire.endTransmission();"),
 ("shown not updated", "      for (uint8_t c = startCol; c < col; c++) lcdShown[row][c] = lcdWanted[row][c];\n", ""),
 ("retry 3 s", "LCD_RETRY_MS = 1000;", "LCD_RETRY_MS = 3000;"),
 ("retry never lastTry", "      lcdLastTry = now;\n      if (lcdInit()) lcdLastInit = now;", "      if (lcdInit()) lcdLastInit = now;"),
 ("retry init time not stored", "      if (lcdInit()) lcdLastInit = now;", "      lcdInit();"),
 ("reinit 4 s", "LCD_REINIT_MS = 5000;", "LCD_REINIT_MS = 4000;"),
 ("reinit 6 s", "LCD_REINIT_MS = 5000;", "LCD_REINIT_MS = 6000;"),
 ("reinit off", "LCD_REINIT_MS = 5000;", "LCD_REINIT_MS = 0;"),
 ("reinit time not stored", "    lcdLastTry = now;\n    lcdLastInit = now;\n    lcdInit();", "    lcdLastTry = now;\n    lcdInit();"),
 ("start-up retry text", "LCD did not answer at start-up; trying again every second.", "LCD did not answer."),
 ("start-up init time", "    lcdLastInit = millis();\n  } else {", "  } else {"),
 ("wire speed", "Wire.setClock(100000UL);", "Wire.setClock(400000UL);"),
 # I2C scan
 ("scan start", "address = 0x08; address <= 0x77", "address = 0x09; address <= 0x77"),
 ("scan end", "address = 0x08; address <= 0x77", "address = 0x08; address < 0x77"),
 ("scan no leading zero", '    if (address < 16) Serial.print(F("0"));\n', ""),
 ("scan header", "I2C scan (0x08-0x77):", "I2C scan:"),
 ("scan done text", "I2C scan done: ", "I2C done: "),
 ("scan lcd mark", "(LCD address in this sketch)", "(LCD)"),
 ("scan warning", "No device on the LCD address 0x", "No LCD on 0x"),
 ("scan warning always", "  if (!lcdFound) {", "  if (true) {"),
 ("scan found flag", "    if (address == LCD_I2C_ADDRESS) lcdFound = true;\n", ""),
 # sign of life
 ("beat 400", "HEARTBEAT_MS = 500;", "HEARTBEAT_MS = 400;"),
 ("beat fast 250", "HEARTBEAT_FAST_MS = 125;", "HEARTBEAT_FAST_MS = 250;"),
 ("beat fast same", "const unsigned long beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;", "const unsigned long beatMs = HEARTBEAT_MS;"),
 ("busy threshold 4", "BUSY_CHANGES = 3;", "BUSY_CHANGES = 4;"),
 ("busy threshold 2", "BUSY_CHANGES = 3;", "BUSY_CHANGES = 2;"),
 ("busy previous window", "(changesNow + changesPrev >= BUSY_CHANGES)", "(changesNow >= BUSY_CHANGES)"),
 ("invert 20", "INVERT_MS = 80;", "INVERT_MS = 20;"),
 ("invert 150", "INVERT_MS = 80;", "INVERT_MS = 150;"),
 ("no invert", "(beatOn != inverted) ? HIGH : LOW", "beatOn ? HIGH : LOW"),
 ("no heartbeat", "    beatOn = !beatOn;\n    beatCount++;", "    beatCount++;"),
 ("beat count off", "    beatOn = !beatOn;\n    beatCount++;", "    beatOn = !beatOn;"),
 ("debug 2 s", "DEBUG_MS = 1000;", "DEBUG_MS = 2000;"),
 ("no debug line", "    printDebug(now);\n", ""),
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("banner baud text", "Serial speed ", "Serial rate "),
 ("debug up", 'Serial.print(F("DBG up="));', 'Serial.print(F("DBG time="));'),
 ("debug pos", 'Serial.print(F(" pos="));', 'Serial.print(F(" p="));'),
 ("debug A", 'Serial.print(F(" A="));', 'Serial.print(F(" a="));'),
 ("debug dir", 'Serial.print(F(" dir="));', 'Serial.print(F(" d="));'),
 ("debug steps", 'Serial.print(F(" steps="));', 'Serial.print(F(" n="));'),
 ("debug chg", 'Serial.print(F(" chg="));', 'Serial.print(F(" changes="));'),
 ("debug raw", 'Serial.print(F(" raw="));', 'Serial.print(F(" bits="));'),
 ("debug beats", 'Serial.print(F(" beats="));', 'Serial.print(F(" beat="));'),
 ("debug lcd", 'Serial.print(F(" lcd="));', 'Serial.print(F(" display="));'),
 ("debug raw value", "Serial.print((int)readRawPosition(), HEX);", "Serial.print((int)stablePos + 1, HEX);"),
 ("debug window off", "    changesPrev = changesNow;\n    changesNow = 0;", "    changesNow = 0;"),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_hex/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/hex_rotary_test_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    try:
        r = subprocess.run([d + '/h'], capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=60)
    except subprocess.TimeoutExpired:
        return name, 'failures (timeout)'
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
survived = 0
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        ok = 'failures' in res or 'ERROR' in res or res == 'no result'
        if not ok: survived += 1
        print(("DETECTED " if ok else "SURVIVED ") + name + ": " + res)
print("%d mutants, %d survived" % (len(muts), survived))
