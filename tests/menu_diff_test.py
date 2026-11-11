# Differential test: the JS menu engine and the C++ engine of menu_test/sketch.ino must give identical display rows,
# callbacks and EEPROM batches for the same event sequences.   python3 tests/menu_diff_test.py [events-per-seed]
import os, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
N = sys.argv[1] if len(sys.argv) > 1 else '6000'
MENU = os.path.join(ROOT, 'tools', 'menu-editor', 'menu-v2.json')
SKETCH = os.environ.get('MENU_SKETCH', os.path.join(ROOT, 'menu_test', 'sketch.ino'))
tmp = tempfile.mkdtemp()
exe = os.path.join(tmp, 'harness')
r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I', os.path.join(ROOT, 'tests'), '-DSKETCH_PATH="%s"' % SKETCH, '-o', exe, os.path.join(ROOT, 'tests', 'menu_engine_harness.cpp')], capture_output=True, text=True)
if r.returncode: print(r.stderr[:3000]); sys.exit(2)
bad = 0; total = 0
for seed in (1, 2, 3, 4, 5, 6):
    sc, js, cc = [os.path.join(tmp, '%s%d' % (n, seed)) for n in ('sc', 'js', 'cc')]
    subprocess.run(['node', os.path.join(ROOT, 'tests', 'menu_diff_scenario.js'), MENU, str(seed), N, sc, js], check=True)
    subprocess.run([exe, 'diff', sc, cc], check=True)
    a = open(js).read().split('\n'); b = open(cc).read().split('\n'); s = open(sc).read().split('\n')
    total += len(a)
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            bad += 1
            if bad <= 5:
                print('MISMATCH seed %d event %d: %s' % (seed, i, s[i]))
                for k, (p, q) in enumerate(zip(x.split(' '), y.split(' '))):
                    if p != q: print('   field %d JS=%s C++=%s' % (k, p if k > 3 else bytes.fromhex(p).decode('latin1'), q if k > 3 else bytes.fromhex(q).decode('latin1')))
    if len(a) != len(b): bad += 1; print('trace length differs for seed %d: %d vs %d' % (seed, len(a), len(b)))
print('%d events compared, %d mismatches' % (total, bad))
sys.exit(1 if bad else 0)
