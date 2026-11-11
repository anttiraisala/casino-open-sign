# Arduino IDE builds function prototypes before the struct definitions, so a function signature with a user-defined type
# (struct/class/enum) fails to compile ("'X' does not name a type"). This lint checks every sketch given on the command line
# (default: menu_test/sketch.ino; library types such as Adafruit_NeoPixel are reported too but are fine): all parameter and return types must be built-in.   python3 tests/arduino_signature_lint.py
import re, sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILTIN = {'void','bool','char','int','long','short','float','double','unsigned','signed','const','uint8_t','uint16_t','uint32_t','int8_t','int16_t','int32_t','size_t','byte','static','inline','constexpr','volatile'}
def bad_types(text):
    bad = []
    for m in re.finditer(r'^([A-Za-z_][\w \*&:<>]*?)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*\{', text, re.M):
        ret, name, params = m.groups()
        if name in ('if', 'for', 'while', 'switch', 'return'): continue
        for part in [ret] + [p for p in params.split(',') if p.strip()]:
            words = re.findall(r'[A-Za-z_]\w*', part)
            if part is not ret and len(words) > 1: words = words[:-1]      # drop the parameter name
            for w in words:
                if w not in BUILTIN: bad.append('%s: type "%s"' % (name, w))
    return bad
files = sys.argv[1:] or [os.path.join(ROOT, 'menu_test', 'sketch.ino')]
fail = 0
for f in files:
    b = bad_types(open(f, encoding='utf-8').read())
    for x in b: print('%s: %s' % (f, x))
    fail += len(b)
print('signature lint: %d problem(s)' % fail)
sys.exit(1 if fail else 0)
