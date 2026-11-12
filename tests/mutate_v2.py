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
 ("frame pin","PIN_FRAME = 12","PIN_FRAME = 6"),
 ("letters pin on the status LED","PIN_LETTERS = 10","PIN_LETTERS = 13"),
 ("letters pin","PIN_LETTERS = 10","PIN_LETTERS = 11"),
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
 ("frame not mirrored","drawFrame(arrowPointsRight, activeFrame, frameStep)","drawFrame(true, activeFrame, frameStep)"),
 ("frame direction inverted","drawFrame(arrowPointsRight, activeFrame, frameStep)","drawFrame(!arrowPointsRight, activeFrame, frameStep)"),
 ("OPEN not reversed","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return letter;"),
 ("OPEN reversed off by one","return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));","return (uint8_t)(LETTER_COUNT - 1 - letter);"),
 ("first word reversed","if (letter < FIRST_WORD_LETTERS) return letter;","if (letter < FIRST_WORD_LETTERS) return (uint8_t)(FIRST_WORD_LETTERS - 1 - letter);"),
 ("startup window 3","STARTUP_TEST_WINDOW_LEDS = 2;","STARTUP_TEST_WINDOW_LEDS = 3;"),
 # equivalent, left out: the end of the skip notice flag (only matters after millis() wraps) and initInput(INPUT_CANCEL) at power-up (a zero-filled state behaves the same)
 ("cancel pin","PIN_CANCEL = 49;","PIN_CANCEL = 3;"),
 ("skip on release","    if (cancelChanged && inputStable[INPUT_CANCEL] == LOW) {\n      noteEvent(now);\n      finishStartup(now, true);","    if (cancelChanged) {\n      noteEvent(now);\n      finishStartup(now, true);"),
 ("skip not announced","    if (skipped) { skipNoticeOn = true;","    if (false) { skipNoticeOn = true;"),
 ("skip notice too short","LCD_SKIP_NOTICE_MS = 1500;","LCD_SKIP_NOTICE_MS = 15;"),
 ("skip notice too long","LCD_SKIP_NOTICE_MS = 1500;","LCD_SKIP_NOTICE_MS = 15000;"),
 ("skip text","const char marquee[] = \"press CANCEL to skip   \";","const char marquee[] = \"press D33 to skip   \";"),
 ("ok skips test","    if (cancelChanged && inputStable[INPUT_CANCEL] == LOW) {\n      noteEvent(now);\n      finishStartup(now, true);","    if (pollInput(INPUT_OK, PIN_OK, now) || cancelChanged && inputStable[INPUT_CANCEL] == LOW) {\n      noteEvent(now);\n      finishStartup(now, true);"),
 ("status led pin","PIN_STATUS_LED = 13","PIN_STATUS_LED = 12"),
 ("fast heartbeat","HEARTBEAT_FAST_MS = 125","HEARTBEAT_FAST_MS = 250"),
 ("no inversion","(beatOn != inverted) ? HIGH : LOW","(beatOn) ? HIGH : LOW"),
 ("debug interval","DEBUG_MS = 1000","DEBUG_MS = 2000"),
 ("baud","SERIAL_BAUD = 115200","SERIAL_BAUD = 9600"),
 ("no event on arrow","    menuPhysical(P_arrow, arrowPosition(), now);\n    noteEvent(now);","    menuPhysical(P_arrow, arrowPosition(), now);"),
 ("dark sticks not cleared","  if (arrowDarkDirty) {  // only touch the unlit sticks when the direction changed","  if (false) {"),
 ("sticks 9 leds","LEDS_PER_STICK = 10;","LEDS_PER_STICK = 8;"),
 ("loops not reset","    printDebug(now);\n    loopCount = 0;","    printDebug(now);"),
 ("analog seed on stick pin","randomSeed(analogRead(A15))","randomSeed(analogRead(A0))"),
 ("curve default exponent","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.67f, 2.67f}","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.2f, 2.67f, 2.67f}"),
 ("curve default exponent stick","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.67f, 2.67f}","CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.0f, 2.67f}"),
 ("curve default letters max","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 80}","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 79}"),
 ("curve default max stick","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 80}","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 90, 80}"),
 ("global brightness back","    allStrips[k]->begin();\n","    allStrips[k]->begin();\n    allStrips[k]->setBrightness(80);\n"),
 ("curve default min","CURVE_DEFAULT_MIN[LED_TYPE_COUNT] = {0, 0, 0}","CURVE_DEFAULT_MIN[LED_TYPE_COUNT] = {0, 2, 0}"),
 ("curve default max","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 80}","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {81, 60, 80}"),
 ("curve default mix","CURVE_DEFAULT_MIX[LED_TYPE_COUNT] = {1.0f, 1.0f, 1.0f}","CURVE_DEFAULT_MIX[LED_TYPE_COUNT] = {1.0f, 0.9f, 1.0f}"),
 ("curve default keep zero","CURVE_DEFAULT_KEEP_ZERO[LED_TYPE_COUNT] = {true, true, true}","CURVE_DEFAULT_KEEP_ZERO[LED_TYPE_COUNT] = {true, false, true}"),
 ("curve exponent max","CURVE_EXPONENT_MAX = 20.0f","CURVE_EXPONENT_MAX = 25.0f"),
 ("curve exponent min","CURVE_EXPONENT_MIN = 0.1f","CURVE_EXPONENT_MIN = 0.2f"),
 ("curve mix formula","(1.0f - mix) * t + mix * powf(t, exponent)","(1.0f - mix) * powf(t, exponent) + mix * t"),
 ("curve keep zero ignored","(keepZero && in == 0) ? 0.0f","(in == 0) ? 0.0f"),
 ("curve keep zero inverted","(keepZero && in == 0) ? 0.0f","(!keepZero && in == 0) ? 0.0f"),
 ("curve no rounding","brightnessLut[type][in] = (uint8_t)(out + 0.5f);","brightnessLut[type][in] = (uint8_t)(out);"),
 ("curve mix clamp","if (mix > 1.0f) mix = 1.0f;","if (mix > 2.0f) mix = 2.0f;"),
 ("shape ignores type","const uint8_t out = brightnessLut[type][v];","const uint8_t out = brightnessLut[0][v];"),
 ("shape hue lost","const uint16_t rr = ((uint16_t)r * out + v / 2) / v, gg = ((uint16_t)g * out + v / 2) / v, bb = ((uint16_t)b * out + v / 2) / v;","const uint16_t rr = out, gg = out, bb = out;"),
 ("shape uses min channel","if (b > v) v = b;\n  if (v == 0) return 0;","if (b < v) v = b;\n  if (v == 0) return 0;"),
 ("frame not shaped","frame.setPixelColor(i, shapeShow(LED_TYPE_STRIP, frame.Color(level, level, level), activeFrame == 14));","frame.setPixelColor(i, frame.Color(level, level, level));"),
 ("letters not shaped","shapeShow(LED_TYPE_LETTERS, letters.Color(level, level / 12, 0), activeLetters == MODE_GREY_BANDS)","letters.Color(level, level / 12, 0)"),
 ("letters wrong type","shapeShow(LED_TYPE_LETTERS, letters.Color(level, level / 12, 0)","shapeShow(LED_TYPE_STICK, letters.Color(level, level / 12, 0)"),
 ("closed not shaped","shapeShow(LED_TYPE_STICK, closedLight.Color(closedLevel, closedLevel, closedLevel), false)","closedLight.Color(closedLevel, closedLevel, closedLevel)"),
 ("arrow not shaped","shapeShow(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)), false)","arrowPixel(*sticks[k], s, stickLowerLane(k, led))"),
 ("arrow wrong type","shapeShow(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led))","shapeShow(LED_TYPE_STRIP, arrowPixel(*sticks[k], s, stickLowerLane(k, led))"),
 ("curves not rebuilt after the menu start","  const bool menuFromEeprom = menuBegin();   // saved menu values (speeds, curves, reverse flags) or the defaults\n  rebuildAllCurves();","  const bool menuFromEeprom = menuBegin();   // saved menu values (speeds, curves, reverse flags) or the defaults"),
 ("marquee text","press CANCEL to skip   \";","press CANCEL to skip  \";"),
 # --- program switch, new letter programs, frame patterns (added with the 15 programs) ---
 ("frame effect off by one","    showFramePattern((uint8_t)(effect - 1));","    showFramePattern((uint8_t)effect);"),
 ("letters effect off by one","    showLettersPattern((uint8_t)(effect - 1));","    showLettersPattern((uint8_t)effect);"),
 ("frame random keeps timer","    showFramePattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeFrame));\n    restartRandomTimer(now);","    showFramePattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeFrame));"),
 ("letters random keeps timer","    showLettersPattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeLetters));\n    restartRandomTimer(now);","    showLettersPattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeLetters));"),
 ("frame random may repeat","    showFramePattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeFrame));","    showFramePattern(pickDifferent(PATTERN_MODE_COUNT));"),
 ("letters random may repeat","    showLettersPattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeLetters));","    showLettersPattern(pickDifferent(PATTERN_MODE_COUNT));"),
 ("pick may repeat","  if (pick >= previous) pick++;  // skip the previous pattern","  // no skip"),
 ("pick not uniform","uint8_t pick = (uint8_t)random(PATTERN_MODE_COUNT - 1);","uint8_t pick = (uint8_t)random(PATTERN_MODE_COUNT - 2);"),
 ("timer fixed frame changes","  if (frameIsRandom()) showFramePattern(pickDifferent(activeFrame));","  showFramePattern(pickDifferent(activeFrame));"),
 ("timer fixed letters change","  if (lettersIsRandom()) showLettersPattern(pickDifferent(activeLetters));","  showLettersPattern(pickDifferent(activeLetters));"),
 ("timer frame never changes","  if (frameIsRandom()) showFramePattern(pickDifferent(activeFrame));","  if (false) showFramePattern(pickDifferent(activeFrame));"),
 ("timer letters never change","  if (lettersIsRandom()) showLettersPattern(pickDifferent(activeLetters));","  if (false) showLettersPattern(pickDifferent(activeLetters));"),
 ("timer runs when both fixed","  if (!frameIsRandom() && !lettersIsRandom()) return;\n  if (now - autoPickedAt","  if (now - autoPickedAt"),
 ("timer ignores delay","  if (now - autoPickedAt < autoDelayMs) return;\n  if (frameIsRandom())","  if (frameIsRandom())"),
 ("timer not restarted","  restartRandomTimer(now);\n  printEffects();\n}\n\n// A small deterministic hash","  printEffects();\n}\n\n// A small deterministic hash"),
 ("random group needs both","  if (!frameIsRandom() && !lettersIsRandom()) return;","  if (!frameIsRandom() || !lettersIsRandom()) return;"),
 ("frame random is effect 1","bool frameIsRandom() { return menuPos(P_frameEffect) == 0; }","bool frameIsRandom() { return menuPos(P_frameEffect) == 1; }"),
 ("letters random is effect 1","bool lettersIsRandom() { return menuPos(P_lettersEffect) == 0; }","bool lettersIsRandom() { return menuPos(P_lettersEffect) == 1; }"),
 ("status frame number","  menuActiveFrame = (uint8_t)(pattern + 1);","  menuActiveFrame = pattern;"),
 ("status letters number","  menuActiveLetters = (uint8_t)(pattern + 1);","  menuActiveLetters = pattern;"),
 ("letters pattern not restarted","  letterModeStart = letterClockMs;  // the new pattern starts from its beginning\n  menuActiveLetters","  menuActiveLetters"),
 ("effect callbacks swapped","  } else if (cb == CB_effectFrameChanged) {","  } else if (cb == CB_effectLettersChanged) {"),
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
 ("monochrome property ignored","(alwaysGrey || menuPos(P_monochrome) == 1)","(alwaysGrey)"),
 ("monochrome inverted","menuPos(P_monochrome) == 1) ? toGrey(color)","menuPos(P_monochrome) == 0) ? toGrey(color)"),
 ("arrow ignores monochrome","shapeShow(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)), false)","shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led)))"),
 ("letters ignore monochrome","shapeShow(LED_TYPE_LETTERS, letters.Color(level, level / 12, 0), activeLetters == MODE_GREY_BANDS)","shapeColor(LED_TYPE_LETTERS, letters.Color(level, level / 12, 0))"),
 ("brightness test follows monochrome","shapeColor(LED_TYPE_LETTERS, brightTestInput(letter))","shapeShow(LED_TYPE_LETTERS, brightTestInput(letter), false)"),
 ("grey phase length","LETTER_GREY_PHASE_MS = 6000;","LETTER_GREY_PHASE_MS = 5000;"),
 ("grey all-lit length","LETTER_GREY_ALL_MS = 1000;","LETTER_GREY_ALL_MS = 800;"),
 ("grey band step","LETTER_GREY_BAND_MS = 250;","LETTER_GREY_BAND_MS = 300;"),
 ("grey words step","LETTER_GREY_WORDS_MS = 400;","LETTER_GREY_WORDS_MS = 500;"),
 ("grey block step","LETTER_GREY_BLOCK_MS = 200;","LETTER_GREY_BLOCK_MS = 250;"),
 ("grey all-lit phase 1 full","if (local < LETTER_GREY_ALL_MS) return phase == 1 ? 128 : 255;","if (local < LETTER_GREY_ALL_MS) return 255;"),
 ("grey all-lit not at the start","if (local < LETTER_GREY_ALL_MS) return phase == 1 ? 128 : 255;","if (local >= LETTER_GREY_ALL_MS && local < 2 * LETTER_GREY_ALL_MS) return phase == 1 ? 128 : 255;"),
 ("grey letters ramp direction","return RAMP6[(uint8_t)((step + 6 - letter % 6) % 6)];","return RAMP6[(uint8_t)((step + letter % 6) % 6)];"),
 ("grey letters ramp modulo","return RAMP6[(uint8_t)((step + 6 - letter % 6) % 6)];","return RAMP6[(uint8_t)((step + 6 - letter % 5) % 6)];"),
 ("grey letters phase not local","const uint16_t local = (uint16_t)(t % LETTER_GREY_PHASE_MS);","const uint16_t local = (uint16_t)t;"),
 ("grey phase count","(t / LETTER_GREY_PHASE_MS) % 3);","(t / LETTER_GREY_PHASE_MS) % 2);"),
 ("grey second word direction","RAMP3[(uint8_t)((step + (letter - FIRST_WORD_LETTERS)) % 3)]","RAMP3[(uint8_t)((step + 3 - (letter - FIRST_WORD_LETTERS) % 3) % 3)]"),
 ("grey first word direction","RAMP3[(uint8_t)((step + 3 - letter % 3) % 3)]","RAMP3[(uint8_t)((step + letter % 3) % 3)]"),
 ("grey letters half level","static const uint8_t RAMP6[6] = {255, 255, 128, 128, 0, 0};\n      static const uint8_t RAMP3[3] = {255, 128, 0};\n      const uint8_t phase = (uint8_t)((t / LETTER","static const uint8_t RAMP6[6] = {255, 255, 100, 128, 0, 0};\n      static const uint8_t RAMP3[3] = {255, 128, 0};\n      const uint8_t phase = (uint8_t)((t / LETTER"),
 ("grey word ramp level","static const uint8_t RAMP3[3] = {255, 128, 0};\n      const uint8_t phase = (uint8_t)((t / LETTER","static const uint8_t RAMP3[3] = {255, 100, 0};\n      const uint8_t phase = (uint8_t)((t / LETTER"),
 ("grey block head","const uint8_t head = tri < 10 ? tri : (uint8_t)(18 - tri);","const uint8_t head = tri < 9 ? tri : (uint8_t)(17 - tri);"),
 ("grey block half","return distance == 0 ? 255 : (distance == 1 ? 128 : 0);","return distance == 0 ? 255 : (distance <= 2 ? 128 : 0);"),
 ("grey letters not always grey","letters.Color(level, level / 12, 0), activeLetters == MODE_GREY_BANDS);","letters.Color(level, level / 12, 0), false);"),
 ("frame grey phase length","const uint8_t phase = (uint8_t)((t / 36) % 4);","const uint8_t phase = (uint8_t)((t / 35) % 4);"),
 ("frame grey all-lit length","if (t % 36 < 8) level","if (t % 36 < 7) level"),
 ("frame grey all-lit phase 1","level = phase == 1 ? 128 : 255;\n      else if (phase == 0)","level = 255;\n      else if (phase == 0)"),
 ("frame grey phase 0 direction","level = RAMP6[(t + 6 - q % 6) % 6];","level = RAMP6[(t + q) % 6];"),
 ("frame grey phase 1 direction","else if (phase == 1) level = RAMP6[(t + q) % 6];","else if (phase == 1) level = RAMP6[(t + 6 - q % 6) % 6];"),
 ("frame grey phase 2 even direction","RAMP3[(t + 3 - (i / 2) % 3) % 3]","RAMP3[(t + (i / 2)) % 3]"),
 ("frame grey phase 2 odd direction","(i & 1) ? RAMP3[(t + i / 2) % 3]","(i & 1) ? RAMP3[(t + 3 - (i / 2) % 3) % 3]"),
 ("frame grey blocks width","level = BLOCKS[((q + t / 2) / 3) % 4];","level = BLOCKS[((q + t / 2) / 2) % 4];"),
 ("frame grey blocks speed","level = BLOCKS[((q + t / 2) / 3) % 4];","level = BLOCKS[((q + t) / 3) % 4];"),
 ("frame grey ramp level","static const uint8_t RAMP6[6] = {255, 255, 128, 128, 0, 0};\n      static const uint8_t RAMP3[3] = {255, 128, 0};\n      static const uint8_t BLOCKS","static const uint8_t RAMP6[6] = {255, 255, 128, 100, 0, 0};\n      static const uint8_t RAMP3[3] = {255, 128, 0};\n      static const uint8_t BLOCKS"),
 ("frame grey blocks levels","BLOCKS[4] = {255, 128, 0, 128};","BLOCKS[4] = {255, 0, 128, 128};"),
 ("frame grey flicker slot","const uint16_t slot = t / 50;","const uint16_t slot = t / 60;"),
 ("frame grey flicker length","if (t % 50 < 8 &&","if (t % 50 < 9 &&"),
 ("frame grey flicker rate","hash16(slot, 77) % 3 == 0","hash16(slot, 77) % 2 == 0"),
 ("frame grey broken pixel","i == hash16(slot, 5) % FRAME_PIXELS","i == hash16(slot, 6) % FRAME_PIXELS"),
 ("frame grey flicker levels","level = pick == 0 ? 0 : (pick == 1 ? 128 : 255);","level = pick == 0 ? 0 : (pick == 1 ? 128 : 128);"),
 ("stick max default","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 80};","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 80, 80};"),
 ("stick max default frame","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 60, 80};","CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {60, 60, 80};"),
 ("led test mono inverted","else if (menuPos(P_monochrome) == 1) {   // Monochrome: only greys","else if (menuPos(P_monochrome) == 0) {   // Monochrome: only greys"),
 ("led test mono ignored","else if (menuPos(P_monochrome) == 1) {   // Monochrome: only greys","else if (false) {   // Monochrome: only greys"),
 ("led test grey half","i % 3 == 0 ? 255 : (i % 3 == 1 ? 128 : 64);","i % 3 == 0 ? 255 : (i % 3 == 1 ? 127 : 64);"),
 ("led test grey quarter","i % 3 == 0 ? 255 : (i % 3 == 1 ? 128 : 64);","i % 3 == 0 ? 255 : (i % 3 == 1 ? 128 : 63);"),
 ("led test grey order","i % 3 == 0 ? 255 : (i % 3 == 1 ? 128 : 64);","i % 3 == 1 ? 255 : (i % 3 == 0 ? 128 : 64);"),
 ("frame blink length","if (t % 60 < 6 && hash16(t / 60, 91) % 2 == 0","if (t % 60 < 7 && hash16(t / 60, 91) % 2 == 0"),
 ("frame blink slots","hash16(t / 60, 91) % 2 == 0 && hash16(t, 3) % 3 != 0","hash16(t / 60, 91) % 3 == 0 && hash16(t, 3) % 3 != 0"),
 ("frame blink seed","hash16(t / 60, 91) % 2 == 0","hash16(t / 60, 92) % 2 == 0"),
 ("frame blink step rate","hash16(t, 3) % 3 != 0) level = 0;","hash16(t, 3) % 3 != 1) level = 0;"),
 ("frame blink slot size","if (t % 60 < 6 && hash16(t / 60, 91)","if (t % 60 < 6 && hash16(t / 50, 91)"),
 ("frame blink only pixel","hash16(t, 3) % 3 != 0) level = 0;","hash16(t, 3) % 3 != 0 && i == 0) level = 0;"),
 ("frame step wrap","FRAME_STEP_WRAP = 3600;","FRAME_STEP_WRAP = 3601;"),
 ("frame step not reset on pattern","  activeFrame = pattern;\n  frameStep = 0;\n","  activeFrame = pattern;\n"),
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
 ("frame q arrow right","(uint16_t)((FRAME_UPPER_LEFT + FRAME_PIXELS - i) % FRAME_PIXELS)","(uint16_t)((FRAME_UPPER_RIGHT + FRAME_PIXELS - i) % FRAME_PIXELS)"),
 ("frame q arrow left","(uint16_t)((i + FRAME_PIXELS - FRAME_UPPER_RIGHT) % FRAME_PIXELS)","(uint16_t)((i + FRAME_PIXELS - FRAME_UPPER_LEFT) % FRAME_PIXELS)"),
 ("frame long edge","i <= FRAME_LOWER_RIGHT || (i >= FRAME_UPPER_RIGHT && i <= FRAME_UPPER_LEFT)","i < FRAME_LOWER_RIGHT || (i >= FRAME_UPPER_RIGHT && i <= FRAME_UPPER_LEFT)"),

 # --- menu integration (OK/CANCEL/SHIFT, menu switch, EEPROM, speeds and curves as menu properties) ---
 ("menu switch pins","PIN_MENU_SWITCH[4] = {38, 41, 42, 45};","PIN_MENU_SWITCH[4] = {41, 38, 42, 45};"),
 ("menu switch not polled","  if (pollHexSwitch(SLOT_MENU, PIN_MENU_SWITCH, now)) {","  if (false) {"),
 ("menu switch half turn used","if (delta != 8) {","if (true) {"),
 ("menu switch direction","if (delta > 8) steps = (int)delta - 16;","if (delta > 8) steps = (int)delta;"),
 ("menu switch ignores shift","menuTurn(steps, inputStable[INPUT_SHIFT] == LOW, now);","menuTurn(steps, false, now);"),
 ("shift inverted","menuTurn(steps, inputStable[INPUT_SHIFT] == LOW, now);","menuTurn(steps, inputStable[INPUT_SHIFT] != LOW, now);"),
 ("ok key not sent","menuKey(MENU_KEY_OK, now); }","}"),
 ("back key not sent","menuKey(MENU_KEY_CANCEL, now); }","}"),
 ("ok sends back","menuKey(MENU_KEY_OK, now); }","menuKey(MENU_KEY_CANCEL, now); }"),
 ("ok on release","if (pollInput(INPUT_OK, PIN_OK, now) && inputStable[INPUT_OK] == LOW)","if (pollInput(INPUT_OK, PIN_OK, now) && inputStable[INPUT_OK] != LOW)"),
 ("menu timeout 30 s","MENU_TIMEOUT_MS = 60000UL;","MENU_TIMEOUT_MS = 30000UL;"),
 ("menu timeout in submenus","menuDepth == 1 && now - menuLastInput >= MENU_TIMEOUT_MS","now - menuLastInput >= MENU_TIMEOUT_MS"),
 ("menu eeprom base","MENU_EEPROM_BASE = 0;","MENU_EEPROM_BASE = 8;"),
 ("eeprom tag not checked","if (EEPROM.read(MENU_EEPROM_BASE + k) != (uint8_t)(MENU_LAYOUT_TAG >> (8 * k))) ok = false;","if (false) ok = false;"),
 ("eeprom range not checked","if (slot != MENU_NO_SLOT && EEPROM.read(MENU_EEPROM_BASE + 4 + slot) >= menuPropCount(p)) ok = false;","if (false) ok = false;"),
 ("eeprom not read","menuPosArr[p] = (ok && slot != MENU_NO_SLOT) ?","menuPosArr[p] = (false && slot != MENU_NO_SLOT) ?"),
 ("eeprom write skipped","EEPROM.update(MENU_EEPROM_BASE + 4 + slot, (uint8_t)menuPosArr[prop]);","(void)slot;"),
 ("arrow reverse ignored","if (menuPos(P_arrowReverse) == 1) pointsLeft = !pointsLeft;",""),
 ("arrow reverse inverted","if (menuPos(P_arrowReverse) == 1) pointsLeft = !pointsLeft;","if (menuPos(P_arrowReverse) == 0) pointsLeft = !pointsLeft;"),
 ("casino reverse ignored","if (menuPos(P_casinoReverse) == 1) closed = !closed;",""),
 ("casino reverse inverted","if (menuPos(P_casinoReverse) == 1) closed = !closed;","if (menuPos(P_casinoReverse) == 0) closed = !closed;"),
 ("frame speed not applied","frameIntervalMs = intervalFor(FRAME_SPEED_MS, multiplier256(P_speedFrame));","frameIntervalMs = FRAME_SPEED_MS;"),
 ("arrow speed not applied","arrowIntervalMs = intervalFor(ARROW_SPEED_MS, multiplier256(P_speedArrow));","arrowIntervalMs = ARROW_SPEED_MS;"),
 ("letters speed not applied","letterMult256 = multiplier256(P_speedLetters);","letterMult256 = 256;"),
 ("arrow speed uses frame","arrowIntervalMs = intervalFor(ARROW_SPEED_MS, multiplier256(P_speedArrow));","arrowIntervalMs = intervalFor(ARROW_SPEED_MS, multiplier256(P_speedFrame));"),
 ("speed divides wrongly","uint32_t ms = ((uint32_t)baseMs * 256UL) / mult256;","uint32_t ms = ((uint32_t)baseMs * mult256) / 256UL;"),
 ("min update ms","MIN_UPDATE_MS = 8;","MIN_UPDATE_MS = 20;"),
 ("strip curve not rebuilt","rebuildCurve(LED_TYPE_STRIP, P_stripMin, P_stripMax, P_stripExp, P_stripMix);\n  rebuildCurve(LED_TYPE_LETTERS, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);\n  rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);\n}","rebuildCurve(LED_TYPE_LETTERS, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);\n  rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);\n}"),
 ("letters curve from strip props","rebuildCurve(LED_TYPE_LETTERS, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);\n  rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);\n}","rebuildCurve(LED_TYPE_LETTERS, P_stripMin, P_stripMax, P_stripExp, P_stripMix);\n  rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);\n}"),
 ("stick curve from letters props","rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);\n}","rebuildCurve(LED_TYPE_STICK, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);\n}"),
 ("random delay ignored","long minutes = (long)(menuValue(P_randomDelay) + 0.5f);","long minutes = 2;"),
 ("random delay zero","if (minutes <= 0) minutes = 1 + (long)random(30);","if (minutes <= 0) minutes = 0;"),
 ("random delay in seconds","autoDelayMs = (unsigned long)minutes * 60000UL;","autoDelayMs = (unsigned long)minutes * 1000UL;"),
 ("countdown rounds down","(long)((autoDelayMs - elapsed + 999UL) / 1000UL)","(long)((autoDelayMs - elapsed) / 1000UL)"),
 ("countdown only for frame","  if (frameIsRandom() || lettersIsRandom()) {\n    const unsigned long elapsed","  if (frameIsRandom()) {\n    const unsigned long elapsed"),
 ("countdown only for letters","  if (frameIsRandom() || lettersIsRandom()) {\n    const unsigned long elapsed","  if (lettersIsRandom()) {\n    const unsigned long elapsed"),
 ("countdown always","  if (frameIsRandom() || lettersIsRandom()) {\n    const unsigned long elapsed","  if (true) {\n    const unsigned long elapsed"),
 ("status alternate","(since / MENU_STATUS_ALT_MS) % 2 == 1","(since / MENU_STATUS_ALT_MS) % 2 == 0"),
 ("hex debounce","HEX_DEBOUNCE_MS = 10;","HEX_DEBOUNCE_MS = 0;"),
 # monochrome (left out as equivalent: the frame already draws white, so replacing its shapeShow by shapeColor changes nothing)
 # brightness test mode (left out as equivalent: removing the MM_STATUS check of brightTestActive(), the depth is 0 in the status page)
 ("test ramp levels","{26, 51, 77, 102, 128, 153, 179, 204, 230, 255}","{25, 50, 75, 100, 125, 150, 175, 200, 225, 250}"),
 ("test ramp last step","179, 204, 230, 255}","179, 204, 230, 250}"),
 ("test frame step rounding","(((i + 1UL) * TEST_RAMP_STEPS + FRAME_PIXELS - 1) / FRAME_PIXELS) - 1","(((i + 0UL) * TEST_RAMP_STEPS + FRAME_PIXELS - 1) / FRAME_PIXELS) - 0"),
 ("test frame step floor","(((i + 1UL) * TEST_RAMP_STEPS + FRAME_PIXELS - 1) / FRAME_PIXELS) - 1","(((i + 1UL) * TEST_RAMP_STEPS) / FRAME_PIXELS) - 1"),
 ("test letters in strip order","letters.setPixelColor(letterPixel(letter), shapeColor(LED_TYPE_LETTERS, brightTestInput(letter)));","letters.setPixelColor(letter, shapeColor(LED_TYPE_LETTERS, brightTestInput(letter)));"),
 ("test letters without curve","shapeColor(LED_TYPE_LETTERS, brightTestInput(letter))","brightTestInput(letter)"),
 ("test frame without curve","frame.setPixelColor(i, shapeColor(LED_TYPE_STRIP, brightTestInput(step)));","frame.setPixelColor(i, brightTestInput(step));"),
 ("test sticks without curve","const uint32_t color = shapeColor(LED_TYPE_STICK, brightTestInput(led));","const uint32_t color = brightTestInput(led);"),
 ("test frame wrong curve","shapeColor(LED_TYPE_STRIP, brightTestInput(step))","shapeColor(LED_TYPE_LETTERS, brightTestInput(step))"),
 ("test only active sticks shown","for (uint8_t k = 0; k < STICK_COUNT; k++) sticks[k]->show();  // all ten sticks","for (uint8_t k = 0; k < STICK_COUNT; k++) if (stickActive(k)) sticks[k]->show();  // all ten sticks"),
 ("test closed stick not shown","  closedLight.show();\n}\n\nvoid loop","  \n}\n\nvoid loop"),
 ("test frame not shown","  frame.show();\n  for (uint8_t letter = 0; letter < LETTER_COUNT; letter++) {\n    letters.setPixelColor(letterPixel","  for (uint8_t letter = 0; letter < LETTER_COUNT; letter++) {\n    letters.setPixelColor(letterPixel"),
 ("test letters not shown","  letters.show();\n  for (uint8_t led = 0; led < TEST_RAMP_STEPS; led++) {","  for (uint8_t led = 0; led < TEST_RAMP_STEPS; led++) {"),
 ("test white 254","case TEST_PATTERN_WHITE: r = g = b = 255; break;","case TEST_PATTERN_WHITE: r = g = b = 254; break;"),
 ("test red wrong","case TEST_PATTERN_RED: r = 255; break;","case TEST_PATTERN_RED: g = 255; break;"),
 ("test green wrong","case TEST_PATTERN_GREEN: g = 255; break;","case TEST_PATTERN_GREEN: b = 255; break;"),
 ("test blue wrong","case TEST_PATTERN_BLUE: b = 255; break;","case TEST_PATTERN_BLUE: r = 255; break;"),
 ("test level property","(uint8_t)(menuValue(P_testLevel) + 0.5f)","(uint8_t)(menuValue(P_testPattern) + 0.5f)"),
 ("test level pattern index","case TEST_PATTERN_LEVEL: r = g = b","case 6: r = g = b"),
 ("test active always","if (menuLevelMenu[level] == MENU_REF_brightnessTest) return true;","return true;"),
 ("test active wrong menu","if (menuLevelMenu[level] == MENU_REF_brightnessTest) return true;","if (menuLevelMenu[level] == MENU_REF_stripBrightness) return true;"),
 ("test active only at the top level","for (uint8_t level = 0; level < menuDepth; level++) {\n    if (menuLevelMenu[level] == MENU_REF_brightnessTest)","for (uint8_t level = menuDepth - 1; level < menuDepth; level++) {\n    if (menuLevelMenu[level] == MENU_REF_brightnessTest)"),
 ("test interval long","BRIGHT_TEST_INTERVAL_MS = 50;","BRIGHT_TEST_INTERVAL_MS = 5000;"),
 ("test does not replace the effects","    updateLcd(now);\n    return;\n  }\n\n  // Independent, non-blocking timers.","    updateLcd(now);\n  }\n\n  // Independent, non-blocking timers."),
 ("test exit keeps dark sticks lit","      arrowDarkDirty = true;           // the sticks that are dark","      //arrowDarkDirty = true;           // the sticks that are dark"),
 ("test exit letter clock jumps","      lastLetterClockTick = now;       // the letter clock","      //lastLetterClockTick = now;       // the letter clock"),
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
    res=(last[-1] if last else 'no result')
    if 'failures' not in res and res!='no result':   # the boot-mode run (saved EEPROM image at power-up) is part of the test
        try:
            for bm in ('1','tag','range','colour'):
                r2=subprocess.run([d+'/h'],capture_output=True,text=True,timeout=60,env=dict(os.environ,**({'V2_LED_COLOUR':'1'} if bm=='colour' else {'V2_BOOT':bm})))
                last2=[l for l in r2.stdout.splitlines() if l.startswith('RESULT')]
                res2=(last2[-1] if last2 else 'no result')
                if 'failures' in res2 or res2=='no result': return name,res2+' (boot mode %s)'%bm
        except subprocess.TimeoutExpired: return name,'RESULT: timeout in boot mode: 1 failures'
    return name,res
# Variant with the ball bit false: the harness (EXPECT_BALL_AT_DIN=0) must pass on it and fail when the bit is ignored.
def run_variant(name,old,new,expect,extra=()):
    d='/tmp/mut_v2/v_'+name.replace(' ','_'); os.makedirs(d,exist_ok=True)
    open(d+'/sketch.ino','w').write(SRC.replace(old,new))
    r=subprocess.run(['g++','-std=c++17','-O1','-w','-I'+ROOT+'/tests','-DEXPECT_BALL_AT_DIN=%d'%expect,*extra,'-DSKETCH_PATH="%s/sketch.ino"'%d,'-o',d+'/h',ROOT+'/tests/casino_open_sign_v2_harness.cpp'],capture_output=True,text=True)
    if r.returncode: return name,'COMPILE ERROR '+r.stderr[:200]
    r=subprocess.run([d+'/h'],capture_output=True,text=True)
    last=[l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    return name,(last[-1] if last else 'no result')


ONLY=os.environ.get('MUT_ONLY')   # MUT_ONLY=new runs only the mutants added with the 15 programs (after the 'frame effect off by one' one)
IDX=range(len(muts)) if not ONLY else range([m[0] for m in muts].index('frame effect off by one'),len(muts))
with cf.ThreadPoolExecutor(8) as ex:
    for name,res in ex.map(run,IDX):
        print(("DETECTED " if 'failures' in res or 'ERROR' in res or 'no result' in res else "SURVIVED ")+name+": "+res)

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
