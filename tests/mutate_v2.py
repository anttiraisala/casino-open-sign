# Mutation test of the Casino Open Sign V2 sketch: every mutant must make tests/casino_open_sign_v2_harness.cpp report failures.
# Run from anywhere:  python3 tests/mutate.py   (needs g++; prints DETECTED / SURVIVED per mutant)
import subprocess, sys, shutil, os, concurrent.futures as cf
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC=open(os.path.join(ROOT,'casino_open_sign_v2','sketch.ino')).read()
muts=[
 ("wing side swapped","if (rightWing != arrowPointsRight) return NO_FLOW;","if (rightWing == arrowPointsRight) return NO_FLOW;"),
 ("rail order swapped","const bool first = (leftStick == arrowPointsRight);","const bool first = (leftStick != arrowPointsRight);"),
 ("rail direction","(first ? led : last - led)","(first ? last - led : led)"),
 ("wing led order","return ARROW_WING_START + (last - led);","return ARROW_WING_START + led;"),
 ("ball bit inverted","const uint8_t physical = STICK_BALL_AT_DIN_END ? led :","const uint8_t physical = !STICK_BALL_AT_DIN_END ? led :"),
 ("lcd text swapped","arrowPointsRight ? \"<-Customer right\" : \"->Customer left\"","arrowPointsRight ? \"->Customer left\" : \"<-Customer right\""),
 ("lcd old text","\"<-Customer right\"","\"Customer right<-\""),
 ("tail cap wrong","const bool tailCap = (stick == STICK_L_CAP) == arrowPointsRight;","const bool tailCap = (stick == STICK_L_CAP) != arrowPointsRight;"),
 ("tail dark ends 1","ARROW_TAIL_DARK_ENDS = 2;","ARROW_TAIL_DARK_ENDS = 1;"),
 ("tail dark ends 3","ARROW_TAIL_DARK_ENDS = 2;","ARROW_TAIL_DARK_ENDS = 3;"),
 ("head cap lit 2","ARROW_HEAD_CAP_LIT = 3;","ARROW_HEAD_CAP_LIT = 2;"),
 ("head cap lit 4","ARROW_HEAD_CAP_LIT = 3;","ARROW_HEAD_CAP_LIT = 4;"),
 ("head cap flow reversed","return ARROW_HEAD_START + (ARROW_HEAD_CAP_LIT - 1 - led);","return ARROW_HEAD_START + led;"),
 ("head cap lower flow reversed","return ARROW_HEAD_START + (led - (last - ARROW_HEAD_CAP_LIT + 1));","return ARROW_HEAD_START + (last - led);"),
 ("tail flow from the ends","return fromMiddle < ARROW_TAIL_HALF ? fromMiddle : NO_FLOW;","return fromMiddle < ARROW_TAIL_HALF ? ARROW_TAIL_HALF - 1 - fromMiddle : NO_FLOW;"),
 ("wing start without cap","return ARROW_WING_START + (last - led);","return ARROW_HEAD_START + (last - led);"),
 ("lower lane wrong","return stick == STICK_L_WING_DN || stick == STICK_RAIL_DN_L || stick == STICK_RAIL_DN_R || stick == STICK_R_WING_DN;","return stick == STICK_L_WING_DN || stick == STICK_RAIL_DN_L || stick == STICK_R_WING_DN;"),
 ("lower cap lane wrong","return led >= LEDS_PER_STICK / 2;","return led > LEDS_PER_STICK / 2;"),
 ("pulse both lanes together","(arrowTick + (bottomLane ? ARROW_PULSE_TURN : 0))","(arrowTick + (bottomLane ? 0 : 0))"),
 ("pulse turn short","ARROW_PULSE_TURN = ARROW_PATH_LEDS;","ARROW_PULSE_TURN = ARROW_PATH_LEDS - 6;"),
 ("pulse head not white","{255, 200, 150, 100, 55}","{250, 200, 150, 100, 55}"),
 ("inactive wings shown","if (!stickActive(k)) {","if (stickActive(k)) {"),
 ("stick lit test","  if (stick == STICK_L_WING_UP || stick == STICK_L_WING_DN) return !arrowPointsRight;","  if (stick == STICK_L_WING_UP || stick == STICK_L_WING_DN) return arrowPointsRight;"),
 ("closed light back on D31","PIN_CLOSED_LIGHT = A0;","PIN_CLOSED_LIGHT = 31;"),
 ("closed light on A2","PIN_CLOSED_LIGHT = A0;","PIN_CLOSED_LIGHT = A2;"),
 ("i2c scan not called","  scanI2cBus();\n  lcdInit();","  lcdInit();"),
 ("i2c scan misses a device","if (Wire.endTransmission() != 0) continue;\n    found++;","if (Wire.endTransmission() != 0 || address == LCD_I2C_ADDRESS) continue;\n    found++;"),
 ("i2c scan counts every address","if (Wire.endTransmission() != 0) continue;\n    found++;","Wire.endTransmission();\n    found++;"),
 ("i2c scan no lcd marker","Serial.println(address == LCD_I2C_ADDRESS ? F(\"  (LCD address in this sketch)\") : F(\"\"));","Serial.println(F(\"\"));"),
 ("i2c scan hint always","  if (!lcdFound) {\n    Serial.print(F(\"No device","  if (true) {\n    Serial.print(F(\"No device"),
 ("i2c scan hint never","  if (!lcdFound) {\n    Serial.print(F(\"No device","  if (false) {\n    Serial.print(F(\"No device"),
 ("i2c scan lower limit","address = 0x08; address <= 0x77","address = 0x28; address <= 0x77"),
 ("i2c scan upper limit","address = 0x08; address <= 0x77","address = 0x08; address <= 0x26"),
 ("i2c scan no leading zero","if (address < 16) Serial.print(F(\"0\"));","if (address < 0) Serial.print(F(\"0\"));"),
 ("closed light 6 LEDs","CLOSED_LIGHT_LEDS = 10;","CLOSED_LIGHT_LEDS = 6;"),
 ("stick pins swapped","{2, 6, 5, 7, 8, 3, 4, A3, A1, A2}","{2, 6, 7, 5, 8, 3, 4, A3, A1, A2}"),
 ("last stick pin","{2, 6, 5, 7, 8, 3, 4, A3, A1, A2}","{2, 6, 5, 7, 8, 3, 4, A3, A1, A0}"),
 ("frame pin","PIN_FRAME = 11","PIN_FRAME = 6"),
 ("letters pin on the status LED","PIN_LETTERS = 12","PIN_LETTERS = 13"),
 ("letters pin","PIN_LETTERS = 12","PIN_LETTERS = 11"),
 ("frame 17 pixels","FRAME_EDGE_SHORT = 3;","FRAME_EDGE_SHORT = 2;"),
 ("frame long edge 5","FRAME_EDGE_LONG = 6;","FRAME_EDGE_LONG = 5;"),
 ("start corner right","const uint16_t start = arrowRight ? FRAME_UPPER_LEFT : FRAME_UPPER_RIGHT;","const uint16_t start = arrowRight ? FRAME_UPPER_RIGHT : FRAME_UPPER_LEFT;"),
 ("start corner lower","const uint16_t start = arrowRight ? FRAME_UPPER_LEFT : FRAME_UPPER_RIGHT;","const uint16_t start = arrowRight ? FRAME_LOWER_LEFT : FRAME_UPPER_RIGHT;"),
 ("upper-left corner off by one","FRAME_UPPER_LEFT = 2 * FRAME_EDGE_LONG + FRAME_EDGE_SHORT - 1;","FRAME_UPPER_LEFT = 2 * FRAME_EDGE_LONG + FRAME_EDGE_SHORT;"),
 ("upper-right corner off by one","FRAME_UPPER_RIGHT = FRAME_EDGE_LONG + FRAME_EDGE_SHORT;","FRAME_UPPER_RIGHT = FRAME_EDGE_LONG + FRAME_EDGE_SHORT - 1;"),
 ("only one way round","return clockwise <= FRAME_PIXELS / 2 ? clockwise : (uint16_t)(FRAME_PIXELS - clockwise);","return clockwise;"),
 ("frame runs backwards","const uint8_t pos = (uint8_t)((distance % FRAME_PERIOD + FRAME_PERIOD - (t % FRAME_PERIOD)) % FRAME_PERIOD);","const uint8_t pos = (uint8_t)((distance % FRAME_PERIOD + (t % FRAME_PERIOD)) % FRAME_PERIOD);"),
 ("frame bar 3","FRAME_BAR_LEDS = 2;","FRAME_BAR_LEDS = 3;"),
 ("frame speed","FRAME_SPEED_MS = 200;","FRAME_SPEED_MS = 100;"),
 ("frame not mirrored","drawFrame(arrowPointsRight, activeMode, frameStep)","drawFrame(true, activeMode, frameStep)"),
 ("frame direction inverted","drawFrame(arrowPointsRight, activeMode, frameStep)","drawFrame(!arrowPointsRight, activeMode, frameStep)"),
 ("OPEN not reversed","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return letter;"),
 ("OPEN reversed off by one","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return (uint8_t)(LETTER_COUNT - 1 - letter);"),
 ("first word reversed","if (letter < FIRST_WORD_LETTERS) return letter;","if (letter < FIRST_WORD_LETTERS) return (uint8_t)(FIRST_WORD_LETTERS - 1 - letter);"),
 ("startup window 3","STARTUP_TEST_WINDOW_LEDS = 2;","STARTUP_TEST_WINDOW_LEDS = 3;"),
 ("button pin","PIN_SKIP_BUTTON = 33;","PIN_SKIP_BUTTON = 3;"),
 ("status led pin","PIN_STATUS_LED = 13","PIN_STATUS_LED = 12"),
 ("fast heartbeat","HEARTBEAT_FAST_MS = 125","HEARTBEAT_FAST_MS = 250"),
 ("no inversion","(beatOn != inverted) ? HIGH : LOW","(beatOn) ? HIGH : LOW"),
 ("debug interval","DEBUG_MS = 1000","DEBUG_MS = 2000"),
 ("baud","SERIAL_BAUD = 115200","SERIAL_BAUD = 9600"),
 ("no event on arrow","    applyArrowSwitch();\n    noteEvent(now);","    applyArrowSwitch();"),
 ("dark sticks not cleared","  if (arrowDarkDirty) {  // only touch the unlit sticks when the direction changed","  if (false) {"),
 ("sticks 9 leds","LEDS_PER_STICK = 10;","LEDS_PER_STICK = 8;"),
 ("loops not reset","    printDebug(now);\n    loopCount = 0;","    printDebug(now);"),
 ("analog seed on stick pin","randomSeed(analogRead(A15))","randomSeed(analogRead(A0))"),
 ("curve default exponent","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.67f}","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.2f, 2.67f}"),
 ("curve default exponent stick","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.67f}","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.0f}"),
 ("curve default max stick","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 80}","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 90}"),
 ("global brightness back","    allStrips[k]->begin();\n","    allStrips[k]->begin();\n    allStrips[k]->setBrightness(80);\n"),
 ("curve default min","CURVE_DEFAULT_MIN[LED_TYPE_COUNT] = {0, 0}","CURVE_DEFAULT_MIN[LED_TYPE_COUNT] = {0, 2}"),
 ("curve default max","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 80}","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {81, 80}"),
 ("curve default mix","CURVE_DEFAULT_MIX[LED_TYPE_COUNT] = {1.0f, 1.0f}","CURVE_DEFAULT_MIX[LED_TYPE_COUNT] = {1.0f, 0.9f}"),
 ("curve default keep zero","CURVE_DEFAULT_KEEP_ZERO[LED_TYPE_COUNT] = {true, true}","CURVE_DEFAULT_KEEP_ZERO[LED_TYPE_COUNT] = {true, false}"),
 ("curve exponent max","CURVE_EXPONENT_MAX = 20.0f","CURVE_EXPONENT_MAX = 25.0f"),
 ("curve exponent min","CURVE_EXPONENT_MIN = 0.1f","CURVE_EXPONENT_MIN = 0.2f"),
 ("curve mix formula","(1.0f - mix) * t + mix * powf(t, exponent)","(1.0f - mix) * powf(t, exponent) + mix * t"),
 ("curve keep zero ignored","(keepZero && in == 0) ? 0.0f","(in == 0) ? 0.0f"),
 ("curve keep zero inverted","(keepZero && in == 0) ? 0.0f","(!keepZero && in == 0) ? 0.0f"),
 ("curve no rounding","brightnessLut[type][in] = (uint8_t)(out + 0.5f);","brightnessLut[type][in] = (uint8_t)(out);"),
 ("curve mix clamp","if (mix > 1.0f) mix = 1.0f;","if (mix > 2.0f) mix = 2.0f;"),
 ("shape ignores type","const uint8_t out = brightnessLut[type][v];","const uint8_t out = brightnessLut[0][v];"),
 ("shape hue lost","const uint16_t rr = ((uint16_t)r * out + v / 2) / v, gg = ((uint16_t)g * out + v / 2) / v, bb = ((uint16_t)b * out + v / 2) / v;","const uint16_t rr = out, gg = out, bb = out;"),
 ("shape uses min channel","if (b > v) v = b;","if (b < v) v = b;"),
 ("frame not shaped","frame.setPixelColor(i, shapeColor(LED_TYPE_STRIP, frame.Color(level, level, level)));","frame.setPixelColor(i, frame.Color(level, level, level));"),
 ("letters not shaped","shapeColor(LED_TYPE_STRIP, letters.Color(level, level / 12, 0))","letters.Color(level, level / 12, 0)"),
 ("letters wrong type","shapeColor(LED_TYPE_STRIP, letters.Color(level, level / 12, 0))","shapeColor(LED_TYPE_STICK, letters.Color(level, level / 12, 0))"),
 ("closed not shaped","shapeColor(LED_TYPE_STICK, closedLight.Color(closedLevel, closedLevel, closedLevel))","closedLight.Color(closedLevel, closedLevel, closedLevel)"),
 ("arrow not shaped","shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))","arrowPixel(*sticks[k], s, stickLowerLane(k, led))"),
 ("arrow wrong type","shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))","shapeColor(LED_TYPE_STRIP, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))"),
 ("tables not built at start","  buildDefaultBrightnessCurves();\n",""),
 ("marquee text","press D33 to skip   \";","press D3 to skip   \";"),
 # --- program switch, new letter programs, frame patterns (added with the 15 programs) ---
 ("program pins","PIN_PROGRAM[4] = {34, 35, 36, 37};","PIN_PROGRAM[4] = {35, 34, 36, 37};"),
 ("program not polled","  if (pollSpeedSwitch(SLOT_PROGRAM, PIN_PROGRAM, now)) {","  if (false) {"),
 ("program not applied at start","setLetterMode(speedStable[SLOT_PROGRAM], now);\n  setCasinoClosed","setLetterMode(0, now);\n  setCasinoClosed"),
 ("converge step","LETTER_CONVERGE_STEP_MS = 400;","LETTER_CONVERGE_STEP_MS = 300;"),
 ("converge off phase","if (step >= 6) return 0;","if (step >= 7) return 0;"),
 ("converge from one end","const uint8_t fromEnd = inWord < wordLength - 1 - inWord ? inWord : (uint8_t)(wordLength - 1 - inWord);","const uint8_t fromEnd = inWord;"),
 ("converge word start","const uint8_t wordStart = letter < FIRST_WORD_LETTERS ? 0 : FIRST_WORD_LETTERS;","const uint8_t wordStart = 0;"),
 ("words swap by letter","(letter < FIRST_WORD_LETTERS ? 0 : 128))];\n      return (uint8_t)(40 +","(letter & 1 ? 0 : 128))];\n      return (uint8_t)(40 +"),
 ("words swap cycle","LETTER_SWAP_MS = 2400;","LETTER_SWAP_MS = 2000;"),
 ("slot first lock","LETTER_SLOT_FIRST_LOCK_MS = 1500;","LETTER_SLOT_FIRST_LOCK_MS = 1200;"),
 ("slot lock step","LETTER_SLOT_LOCK_STEP_MS = 300;","LETTER_SLOT_LOCK_STEP_MS = 250;"),
 ("slot hold end","LETTER_SLOT_HOLD_END_MS = 7200;","LETTER_SLOT_HOLD_END_MS = 7000;"),
 ("slot period","LETTER_SLOT_PERIOD_MS = 8000;","LETTER_SLOT_PERIOD_MS = 7500;"),
 ("slot reel levels","(hash16(letter, (uint16_t)(p / LETTER_SLOT_REEL_MS)) & 1) ? 255 : 30","(hash16(letter, (uint16_t)(p / LETTER_SLOT_REEL_MS)) & 1) ? 255 : 0"),
 ("slot lock order","const uint32_t lockAt = LETTER_SLOT_FIRST_LOCK_MS + (uint32_t)letter * LETTER_SLOT_LOCK_STEP_MS;","const uint32_t lockAt = LETTER_SLOT_FIRST_LOCK_MS + (uint32_t)(LETTER_COUNT - 1 - letter) * LETTER_SLOT_LOCK_STEP_MS;"),
 ("twin chase one spot","return (letter == step || letter == LETTER_COUNT - 1 - step) ? 30 : 230;","return (letter == step) ? 30 : 230;"),
 ("twin chase step","LETTER_TWIN_STEP_MS = 250;","LETTER_TWIN_STEP_MS = 200;"),
 ("marquee modulo","return ((letter + step) % 4) != 0 ? 230 : 40;","return ((letter + step) % 3) != 0 ? 230 : 40;"),
 ("marquee direction","return ((letter + step) % 4) != 0 ? 230 : 40;","return ((letter + 4 - step) % 4) != 0 ? 230 : 40;"),
 ("marquee step","LETTER_MARQUEE_STEP_MS = 250;","LETTER_MARQUEE_STEP_MS = 300;"),
 ("random auto not 15","MODE_RANDOM_AUTO = 15;","MODE_RANDOM_AUTO = 14;"),
 ("frame step wrap","FRAME_STEP_WRAP = 3600;","FRAME_STEP_WRAP = 3601;"),
 ("frame step not reset on program","  letterModeStart = letterClockMs;\n  frameStep = 0;\n","  letterModeStart = letterClockMs;\n"),
 ("frame wave tail","return k < 5 ? (uint8_t)(255 - 50 * k) : 0;","return k < 5 ? (uint8_t)(255 - 40 * k) : 0;"),
 ("frame fill hold","return p < 14 ? 255 : 0;\n    }\n    case 3:","return p < 13 ? 255 : 0;\n    }\n    case 3:"),
 ("frame ignite hold","return p < 30 ? 230 : 0;","return p < 28 ? 230 : 0;"),
 ("frame sides blink","return (t % 8) < 5 ? 255 : 20;","return (t % 8) < 4 ? 255 : 20;"),
 ("frame alternate","return ((i + t / 2) & 1) ? 255 : 0;","return ((i + t) & 1) ? 255 : 0;"),
 ("frame pingpong tail","return (behind > 0 && behind < 4) ? (uint8_t)(255 - 70 * behind) : 0;","return (behind > 0 && behind < 3) ? (uint8_t)(255 - 70 * behind) : 0;"),
 ("frame twinkle","return (hash16(i, t) & 7) < 2 ? 255 : 30;","return (hash16(i, t) & 7) < 2 ? 255 : 0;"),
 ("frame chase direction","return ((q + 3 - t % 3) % 3) == 0 ? 255 : 0;","return ((q + t % 3) % 3) == 0 ? 255 : 0;"),
 ("frame heartbeat","return s == 0 ? 255 : (s == 1 ? 120 : (s == 2 ? 200 : (s == 3 ? 60 : 20)));","return s == 0 ? 255 : (s == 1 ? 120 : (s == 2 ? 200 : (s == 3 ? 60 : 30)));"),
 ("frame corners linear distance","if (d > FRAME_PIXELS - d) d = FRAME_PIXELS - d;","(void)d;"),
 ("frame corners dark phase","if (s >= 6) return 0;\n      return nearest","if (s >= 7) return 0;\n      return nearest"),
 ("frame edges swap","return ((t % 10) < 5) == longEdge ? 255 : 0;","return ((t % 10) < 4) == longEdge ? 255 : 0;"),
 ("frame slot lock","if (p < 6 + i) return","if (p < 5 + i) return"),
 ("frame twin second comet","const uint16_t b = (FRAME_PIXELS - a) % FRAME_PIXELS;","const uint16_t b = a;"),
 ("frame marquee","return ((q + 3 - t % 3) % 3) != 0 ? 255 : 0;","return ((q + 3 - t % 3) % 3) != 1 ? 255 : 0;"),
 ("frame q arrow right","(uint16_t)((FRAME_UPPER_LEFT + FRAME_PIXELS - i) % FRAME_PIXELS)","(uint16_t)((FRAME_UPPER_RIGHT + FRAME_PIXELS - i) % FRAME_PIXELS)"),
 ("frame q arrow left","(uint16_t)((i + FRAME_PIXELS - FRAME_UPPER_RIGHT) % FRAME_PIXELS)","(uint16_t)((i + FRAME_PIXELS - FRAME_UPPER_LEFT) % FRAME_PIXELS)"),
 ("frame long edge","i <= FRAME_LOWER_RIGHT || (i >= FRAME_UPPER_RIGHT && i <= FRAME_UPPER_LEFT)","i < FRAME_LOWER_RIGHT || (i >= FRAME_UPPER_RIGHT && i <= FRAME_UPPER_LEFT)"),
 ("lcd program hex","\"Prog %X\"","\"Prog %u\""),
]
def run(i):
    name,old,new=muts[i]
    assert SRC.count(old)==1,(name,SRC.count(old))
    d='/tmp/mut_v2/%d'%i; os.makedirs(d,exist_ok=True)
    open(d+'/sketch.ino','w').write(SRC.replace(old,new))
    r=subprocess.run(['g++','-std=c++17','-O1','-w','-I'+ROOT+'/tests','-DSKETCH_PATH="%s/sketch.ino"'%d,'-o',d+'/h',ROOT+'/tests/casino_open_sign_v2_harness.cpp'],capture_output=True,text=True)
    if r.returncode: return name,'COMPILE ERROR '+r.stderr[:200]
    try: r=subprocess.run([d+'/h'],capture_output=True,text=True,timeout=60)   # a hang (the harness waits for a change that never comes) counts as detected
    except subprocess.TimeoutExpired: return name,'RESULT: timeout, harness hangs: 1 failures'
    last=[l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name,(last[-1] if last else 'no result')
# Variant with the ball bit false: the harness (EXPECT_BALL_AT_DIN=0) must pass on it and fail when the bit is ignored.
def run_variant(name,old,new,expect,extra=()):
    d='/tmp/mut_v2/v_'+name.replace(' ','_'); os.makedirs(d,exist_ok=True)
    open(d+'/sketch.ino','w').write(SRC.replace(old,new))
    r=subprocess.run(['g++','-std=c++17','-O1','-w','-I'+ROOT+'/tests','-DEXPECT_BALL_AT_DIN=%d'%expect,*extra,'-DSKETCH_PATH="%s/sketch.ino"'%d,'-o',d+'/h',ROOT+'/tests/casino_open_sign_v2_harness.cpp'],capture_output=True,text=True)
    if r.returncode: return name,'COMPILE ERROR '+r.stderr[:200]
    r=subprocess.run([d+'/h'],capture_output=True,text=True)
    last=[l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name,(last[-1] if last else 'no result')


ONLY=os.environ.get('MUT_ONLY')   # MUT_ONLY=new runs only the mutants added with the 15 programs (after the 'program pins' one)
IDX=range(len(muts)) if not ONLY else range([m[0] for m in muts].index('program pins'),len(muts))
with cf.ThreadPoolExecutor(8) as ex:
    for name,res in ex.map(run,IDX):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res=='no result' else "SURVIVED ")+name+": "+res)

if ONLY: sys.exit(0)

FO="constexpr bool STICK_BALL_AT_DIN_END = true;"; FN="constexpr bool STICK_BALL_AT_DIN_END = false;"
assert SRC.count(FO)==1
PH="const uint8_t physical = STICK_BALL_AT_DIN_END ? led : (uint8_t)(LEDS_PER_STICK - 1 - led);"
assert SRC.count(PH)==1
n,res=run_variant("bit false",FO,FN,0)
print(("OK       " if res.startswith('RESULT: all checks passed') else "BROKEN   ")+"variant bit=false must pass: "+res)
SRCX=SRC
SRC=SRCX.replace(FO,FN).replace(PH,"const uint8_t physical = led;")
n,res=run_variant("bit false flip missing",FN,FN,0)
print(("DETECTED " if 'failures' in res else "SURVIVED ")+"variant bit=false with the flip removed: "+res)

SO="constexpr bool ARROW_POINTS_LEFT_WHEN_LOW = true;"; SN="constexpr bool ARROW_POINTS_LEFT_WHEN_LOW = false;"
assert SRCX.count(SO)==1
SRC=SRCX
n,res=run_variant("switch false",SO,SN,1,("-DEXPECT_LOW_IS_LEFT=0",))
print(("OK       " if res.startswith('RESULT: all checks passed') else "BROKEN   ")+"variant switch bit=false must pass: "+res)
n,res=run_variant("switch false expected true",SO,SN,1)
print(("DETECTED " if 'failures' in res else "SURVIVED ")+"switch bit=false but harness expects true: "+res)
SRC=SRCX.replace(SO,SN).replace("(inputStable[INPUT_ARROW] == LOW) == ARROW_POINTS_LEFT_WHEN_LOW","(inputStable[INPUT_ARROW] == LOW)")
n,res=run_variant("switch false ignored",SN,SN,1,("-DEXPECT_LOW_IS_LEFT=0",))
print(("DETECTED " if 'failures' in res else "SURVIVED ")+"switch bit=false but ignored by the sketch: "+res)
