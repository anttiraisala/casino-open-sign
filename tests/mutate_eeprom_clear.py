# Mutation test of eeprom_clear/sketch.ino: every mutant must make tests/eeprom_clear_harness.cpp report failures.
#   python3 tests/mutate_eeprom_clear.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'eeprom_clear', 'sketch.ino')).read()
muts = [
 # what is written and where
 ("erased value 0x00", "ERASED_VALUE = 0xFF;", "ERASED_VALUE = 0x00;"),
 ("erased value 0xFE", "ERASED_VALUE = 0xFF;", "ERASED_VALUE = 0xFE;"),
 ("write instead of update", "EEPROM.update(address, ERASED_VALUE);", "EEPROM.write(address, ERASED_VALUE);"),
 ("last byte skipped", "  for (unsigned long address = 0; address < eepromSize; address++) {\n    EEPROM.update", "  for (unsigned long address = 0; address + 1 < eepromSize; address++) {\n    EEPROM.update"),
 ("first byte skipped", "  for (unsigned long address = 0; address < eepromSize; address++) {\n    EEPROM.update", "  for (unsigned long address = 1; address < eepromSize; address++) {\n    EEPROM.update"),
 ("size not from the library", "eepromSize = EEPROM.length();", "eepromSize = 512;"),
 ("size too big", "eepromSize = EEPROM.length();", "eepromSize = 1000UL;"),
 ("nothing written", "    EEPROM.update(address, ERASED_VALUE);\n", ""),
 # counting
 ("count wrong test", "if (EEPROM.read(address) != ERASED_VALUE) count++;", "if (EEPROM.read(address) == ERASED_VALUE) count++;"),
 ("count nothing", "if (EEPROM.read(address) != ERASED_VALUE) count++;", ""),
 ("count last byte missing", "  for (unsigned long address = 0; address < eepromSize; address++) {\n    if (EEPROM.read", "  for (unsigned long address = 0; address + 1 < eepromSize; address++) {\n    if (EEPROM.read"),
 ("count first byte missing", "  for (unsigned long address = 0; address < eepromSize; address++) {\n    if (EEPROM.read", "  for (unsigned long address = 1; address < eepromSize; address++) {\n    if (EEPROM.read"),
 ("before not counted", "bytesBefore = countNotErased();", "bytesBefore = 0;"),
 ("after not counted", "bytesBad = countNotErased();", "bytesBad = 0;"),
 ("after counted before the clearing", "  bytesBad = countNotErased();\n  clearOk", "  bytesBad = bytesBefore;\n  clearOk"),
 ("ok always", "clearOk = (bytesBad == 0);", "clearOk = true;"),
 ("ok never", "clearOk = (bytesBad == 0);", "clearOk = false;"),
 ("ok inverted", "clearOk = (bytesBad == 0);", "clearOk = (bytesBad != 0);"),
 ("ok tolerates one bad byte", "clearOk = (bytesBad == 0);", "clearOk = (bytesBad <= 1);"),
 # messages
 ("banner baud text", "Serial speed ", "Serial rate "),
 ("size text", 'Serial.print(F("EEPROM size "));', 'Serial.print(F("EEPROM bytes "));'),
 ("used text", "of them were not 0xFF.", "were not 0xFF."),
 ("success text", "EEPROM cleared: all bytes are 0xFF. Upload the sign program now.", "EEPROM cleared."),
 ("failure text", "EEPROM clear FAILED: ", "EEPROM clear failed: "),
 ("failure count text", "byte(s) are still not 0xFF.", "bytes left."),
 ("success printed always", "  if (clearOk) {\n    Serial.println(F(\"EEPROM cleared", "  if (true) {\n    Serial.println(F(\"EEPROM cleared"),
 # sign of life
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("heartbeat ok 400", "HEARTBEAT_OK_MS = 500;", "HEARTBEAT_OK_MS = 400;"),
 ("heartbeat ok 600", "HEARTBEAT_OK_MS = 500;", "HEARTBEAT_OK_MS = 600;"),
 ("heartbeat fail 200", "HEARTBEAT_FAIL_MS = 100;", "HEARTBEAT_FAIL_MS = 200;"),
 ("heartbeat fail 50", "HEARTBEAT_FAIL_MS = 100;", "HEARTBEAT_FAIL_MS = 50;"),
 ("heartbeat speed swapped", "(clearOk ? HEARTBEAT_OK_MS : HEARTBEAT_FAIL_MS)", "(clearOk ? HEARTBEAT_FAIL_MS : HEARTBEAT_OK_MS)"),
 ("heartbeat always slow", "(clearOk ? HEARTBEAT_OK_MS : HEARTBEAT_FAIL_MS)", "HEARTBEAT_OK_MS"),
 ("no heartbeat toggle", "    beatOn = !beatOn;\n    beatCount++;", "    beatCount++;"),
 ("beat count off", "    beatOn = !beatOn;\n    beatCount++;", "    beatOn = !beatOn;"),
 ("LED not written", "    digitalWrite(PIN_STATUS_LED, beatOn ? HIGH : LOW);\n  }", "  }"),
 ("LED inverted", "digitalWrite(PIN_STATUS_LED, beatOn ? HIGH : LOW);", "digitalWrite(PIN_STATUS_LED, beatOn ? LOW : HIGH);"),
 ("LED does not start from off", "  digitalWrite(PIN_STATUS_LED, LOW);                // the heartbeat starts from LED off\n", ""),
 ("debug 2 s", "DEBUG_MS = 1000;", "DEBUG_MS = 2000;"),
 ("debug 500 ms", "DEBUG_MS = 1000;", "DEBUG_MS = 500;"),
 ("no debug line", "    printDebugLine(now);\n", ""),
 ("debug timer not restarted", "    lastDebug = now;\n    printDebugLine(now);", "    printDebugLine(now);"),
 ("debug up", 'Serial.print(F("DBG up="));', 'Serial.print(F("DBG time="));'),
 ("debug up in ms", "Serial.print(now / 1000UL);", "Serial.print(now);"),
 ("debug size", 'Serial.print(F(" size="));', 'Serial.print(F(" bytes="));'),
 ("debug before", 'Serial.print(F(" before="));', 'Serial.print(F(" was="));'),
 ("debug bad", 'Serial.print(F(" bad="));', 'Serial.print(F(" left="));'),
 ("debug state", 'Serial.print(F(" state="));', 'Serial.print(F(" result="));'),
 ("debug state swapped", 'Serial.print(clearOk ? F("OK") : F("FAIL"));', 'Serial.print(clearOk ? F("FAIL") : F("OK"));'),
 ("debug beats", 'Serial.print(F(" beats="));', 'Serial.print(F(" beat="));'),
 ("debug beats value", "Serial.println(beatCount);", "Serial.println(beatOn);"),
 ("debug bad value", "Serial.print(bytesBad);\n  Serial.print(F(\" state=\"));", "Serial.print(bytesBefore);\n  Serial.print(F(\" state=\"));"),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_eeprom/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/eeprom_clear_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    try:
        r = subprocess.run([d + '/h'], capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=60)
    except subprocess.TimeoutExpired:
        return name, 'failures (timeout)'
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
survived = 0
problems = 0
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        if 'COMPILE ERROR' in res or res == 'no result':
            problems += 1; print("PROBLEM " + name + ": " + res)
            continue
        ok = 'failures' in res
        if not ok: survived += 1
        print(("DETECTED " if ok else "SURVIVED ") + name + ": " + res)
print("%d mutants, %d survived, %d problems" % (len(muts), survived, problems))
