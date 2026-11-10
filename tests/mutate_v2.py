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
 ("closed light back on D31","PIN_CLOSED_LIGHT = A3;","PIN_CLOSED_LIGHT = 31;"),
 ("closed light on A2","PIN_CLOSED_LIGHT = A3;","PIN_CLOSED_LIGHT = A2;"),
 ("closed light 6 LEDs","CLOSED_LIGHT_LEDS = 10;","CLOSED_LIGHT_LEDS = 6;"),
 ("stick pins swapped","{2, 3, 4, 5, 6, 7, 8, A0, A1, A2}","{2, 3, 4, 6, 5, 7, 8, A0, A1, A2}"),
 ("last stick pin","{2, 3, 4, 5, 6, 7, 8, A0, A1, A2}","{2, 3, 4, 5, 6, 7, 8, A0, A1, A3}"),
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
 ("frame runs backwards","pos = (uint8_t)((distance % FRAME_PERIOD + FRAME_PERIOD - phase) % FRAME_PERIOD)","pos = (uint8_t)((distance % FRAME_PERIOD + phase) % FRAME_PERIOD)"),
 ("frame bar 3","FRAME_BAR_LEDS = 2;","FRAME_BAR_LEDS = 3;"),
 ("frame speed","FRAME_SPEED_MS = 200;","FRAME_SPEED_MS = 100;"),
 ("frame not mirrored","drawFrame(arrowPointsRight, framePhase)","drawFrame(true, framePhase)"),
 ("frame direction inverted","drawFrame(arrowPointsRight, framePhase)","drawFrame(!arrowPointsRight, framePhase)"),
 ("OPEN not reversed","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return letter;"),
 ("OPEN reversed off by one","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return (uint8_t)(LETTER_COUNT - 1 - letter);"),
 ("first word reversed","if (letter < FIRST_WORD_LETTERS) return letter;","if (letter < FIRST_WORD_LETTERS) return (uint8_t)(FIRST_WORD_LETTERS - 1 - letter);"),
 ("startup window 3","STARTUP_TEST_WINDOW_LEDS = 2;","STARTUP_TEST_WINDOW_LEDS = 3;"),
 ("button pin","PIN_MODE_BUTTON = 33","PIN_MODE_BUTTON = 3"),
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
 ("frame not shaped","shapeColor(LED_TYPE_STRIP, frame.Color(255, 255, 255))","frame.Color(255, 255, 255)"),
 ("letters not shaped","shapeColor(LED_TYPE_STRIP, letters.Color(level, level / 12, 0))","letters.Color(level, level / 12, 0)"),
 ("letters wrong type","shapeColor(LED_TYPE_STRIP, letters.Color(level, level / 12, 0))","shapeColor(LED_TYPE_STICK, letters.Color(level, level / 12, 0))"),
 ("closed not shaped","shapeColor(LED_TYPE_STICK, closedLight.Color(closedLevel, closedLevel, closedLevel))","closedLight.Color(closedLevel, closedLevel, closedLevel)"),
 ("arrow not shaped","shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))","arrowPixel(*sticks[k], s, stickLowerLane(k, led))"),
 ("arrow wrong type","shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))","shapeColor(LED_TYPE_STRIP, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))"),
 ("tables not built at start","  buildDefaultBrightnessCurves();\n",""),
 ("marquee text","press D33 to skip   \";","press D3 to skip   \";"),
]
def run(i):
    name,old,new=muts[i]
    assert SRC.count(old)==1,(name,SRC.count(old))
    d='/tmp/mut_v2/%d'%i; os.makedirs(d,exist_ok=True)
    open(d+'/sketch.ino','w').write(SRC.replace(old,new))
    r=subprocess.run(['g++','-std=c++17','-O1','-w','-I'+ROOT+'/tests','-DSKETCH_PATH="%s/sketch.ino"'%d,'-o',d+'/h',ROOT+'/tests/casino_open_sign_v2_harness.cpp'],capture_output=True,text=True)
    if r.returncode: return name,'COMPILE ERROR '+r.stderr[:200]
    r=subprocess.run([d+'/h'],capture_output=True,text=True)
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


with cf.ThreadPoolExecutor(6) as ex:
    for name,res in ex.map(run,range(len(muts))):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or res=='no result' else "SURVIVED ")+name+": "+res)

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
