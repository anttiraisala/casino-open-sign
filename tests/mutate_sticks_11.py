# Mutation test of sticks_11_test/sketch.ino: every mutant must make
# tests/sticks_11_test_harness.cpp report failures.   python3 tests/mutate_sticks_11.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'sticks_11_test', 'sketch.ino')).read()
muts = [
 ("pin stick 1", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3}", "{3, 2, 4, 5, 6, 7, 8, A0, A1, A2, A3}"),
 ("pin stick 8", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3}", "{2, 3, 4, 5, 6, 7, 8, A1, A0, A2, A3}"),
 ("closed on A2", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3}", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A3, A2}"),
 ("closed on A4", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3}", "{2, 3, 4, 5, 6, 7, 8, A0, A1, A2, 58}"),
 ("leds 9", "LEDS_PER_STICK = 10;", "LEDS_PER_STICK = 9;"),
 ("colour order", "COLOR_ORDER = NEO_GRB + NEO_KHZ800;", "COLOR_ORDER = NEO_RGB + NEO_KHZ800;"),
 ("arrow level", "ARROW_LEVEL = 80;", "ARROW_LEVEL = 81;"),
 ("closed level", "CLOSED_LEVEL = 80;", "CLOSED_LEVEL = 79;"),
 ("closed off", "if (s == ARROW_STICKS) level = CLOSED_LEVEL;", "if (s == ARROW_STICKS) level = 0;"),
 ("closed follows mode", "if (s == ARROW_STICKS) level = CLOSED_LEVEL;", "if (s == ARROW_STICKS && allOn) level = CLOSED_LEVEL;\n      else if (s == ARROW_STICKS) level = 0;"),
 ("start all on", "START_ALL_ON = false;", "START_ALL_ON = true;"),
 ("button pin", "PIN_BUTTON = 33;", "PIN_BUTTON = 32;"),
 ("no debounce", "BUTTON_MS = 30;", "BUTTON_MS = 0;"),
 ("press on release", "    if (buttonStable) {\n      allOn", "    if (!buttonStable) {\n      allOn"),
 ("no toggle", "      allOn = !allOn;\n", "      allOn = true;\n"),
 ("wave speed", "- t * 6)", "- t * 5)"),
 ("wave spacing", "led * 28", "led * 27"),
 ("wave stick offset", "stick * 37", "stick * 36"),
 ("wave triangle", "(uint8_t)(phase * 2) : (uint8_t)((255 - phase) * 2)", "(uint8_t)(phase * 2) : (uint8_t)((255 - phase) * 1)"),
 ("frame time", "FRAME_MS = 20;", "FRAME_MS = 25;"),
 ("estimate", "ESTIMATE_MA_PER_CHANNEL = 20;", "ESTIMATE_MA_PER_CHANNEL = 21;"),
 ("estimate lit", "        channelSum += 3UL * level;", "        channelSum += level;"),
 ("baud", "SERIAL_BAUD = 115200;", "SERIAL_BAUD = 9600;"),
 ("no inversion", "(beatOn != inverted)", "(beatOn != false)"),
 ("no heartbeat", "    beatOn = !beatOn;\n    beats++;", "    beats++;"),
 ("no debug line", "    printDebug(now);\n", ""),
 ("debug every 2 s", "lastDebug = now;\n    printDebug", "lastDebug = now + 1000;\n    printDebug"),
 ("banner baud text", "Serial speed ", "Serial rate "),
 ("no start estimate", "Estimated current now: ", "Est. now: "),
 ("dark at start", "  drawFrame();                               // lit at once, no dark pause\n", ""),
]
def run(i):
    name, old, new = muts[i]
    assert SRC.count(old) == 1, (name, SRC.count(old))
    d = '/tmp/mut_s11/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/sketch.ino', 'w').write(SRC.replace(old, new))
    r = subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-I' + ROOT + '/tests', '-DSKETCH_PATH="%s/sketch.ino"' % d, '-o', d + '/h', ROOT + '/tests/sticks_11_test_harness.cpp'], capture_output=True, text=True)
    if r.returncode: return name, 'COMPILE ERROR ' + r.stderr[:200]
    r = subprocess.run([d + '/h'], capture_output=True, text=True)
    last = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name, (last[-1] if last else 'no result')
with cf.ThreadPoolExecutor(6) as ex:
    for name, res in ex.map(run, range(len(muts))):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res == 'no result' else "SURVIVED ") + name + ": " + res)
