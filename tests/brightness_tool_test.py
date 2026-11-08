# Checks that the defaults of the phone brightness tool equal the CURVE_DEFAULT_* values of the sketch and that
# both LED types have the same defaults (the tool has one set of sliders). Expected values are hard-coded.
import os, re, sys
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sk = open(os.path.join(root, 'sketch.ino')).read()
tool = open(os.path.join(root, 'tools', 'brightness-tool', 'index.html')).read()
fails = 0
def check(c, m):
    global fails
    if not c: print('FAIL:', m); fails += 1
def arr(name):
    m = re.search(name + r'\[LED_TYPE_COUNT\] = \{([^}]*)\}', sk)
    return [v.strip().rstrip('f') for v in m.group(1).split(',')]
want = {'MIN': ['0', '0'], 'MAX': ['80', '80'], 'EXPONENT': ['2.67', '2.67'], 'MIX': ['1.0', '1.0'], 'KEEP_ZERO': ['true', 'true']}
for k, v in want.items(): check(arr('CURVE_DEFAULT_' + k) == v, 'sketch default ' + k)
d = re.search(r'var DEF=\{([^}]*)\}', tool).group(1)
check(d == 'min:0,max:80,exp:2.67,mix:100,kz:1,probe:128', 'tool defaults ' + d)
print('RESULT: all checks passed' if not fails else 'RESULT: %d failures' % fails)
sys.exit(1 if fails else 0)
