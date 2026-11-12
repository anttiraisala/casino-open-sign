/*
 * Casino Open Sign V2 - Arduino Mega 2560 + two 12 V WS2811 pixel strips + an arrow of 10 Grove RGB LED sticks
 * (Wokwi project). V2 has exactly 2 strips and 11 sticks; version 1 (the folder above this one) has four strips.
 *
 * Physical sign:
 *   - Black rectangular plate (60 x 30 cm) with an LED frame around its edge: ONE 12 V WS2811 strip of 18 pixels
 *     (one pixel = one chip = 3 LEDs = 10 cm), pixel 1 in the lower-left corner and then counter-clockwise, seen from
 *     the front: pixels 1-6 bottom edge left to right, 7-9 right edge going up, 10-15 top edge right to left,
 *     16-18 left edge going down. While the arrow points left a bar pattern starts in the customer's upper-right
 *     corner (pixel 10) and runs both ways round the frame (clockwise and counter-clockwise) to the lower-left corner
 *     (pixel 1); while it points right it starts in the upper-left corner (pixel 15) and runs both ways to the
 *     lower-right corner (pixel 6). Both paths are 9 steps long and meet in the bottom corner.
 *   - Red plexiglass letters, "CASINO" above "OPEN", floating in front of the
 *     plate. Behind each letter sits ONE pixel (3 LEDs, one colour) of the second 12 V WS2811 strip (10 pixels).
 *     Strip pixels 1-6 are C, A, S, I, N, O; pixels 7-10 are N, E, P, O: the OPEN letters are wired in REVERSED
 *     order on the strip (O is the last pixel). The program maps the letters, so every pattern still runs in the
 *     natural reading order for the viewer.
 *   - A hole text "CLOSED" cut into the plate under the OPEN row; the 11th Grove stick (10 white LEDs) behind the
 *     plate light it up while the casino is closed (and then the OPEN letters are off).
 *   - A left/right arrow under that, made of 10 Grove RGB LED sticks (10 LEDs each, plugged into the
 *     connectors of the Grove Base Shield; exact geometry and drill template in arrow_sticks/): a vertical
 *     cap stick at each end, four rail sticks between the caps (two upper, two lower) and four wing
 *     sticks (30 degrees) that form an arrowhead at each end. Only the head the arrow points at is lit.
 *     The cap at the tail lights its middle 6 LEDs (2 dark at each end), the cap at the head lights
 *     only the 3 LEDs at each end (middle 4 dark). (The older arrow made of six LED strips is kept as a
 *     backup in backup/old_arrow_strips/; it is not wired any more.)
 *
 * Strip overview (one Adafruit_NeoPixel object and one data pin each; the strips run on a separate 12 V supply,
 * the Arduino, the Grove shield and the sticks on the 5 V supply, all grounds are joined; D13 is the status LED):
 *   frame            D11  12 V WS2811, 18 pixels (see above)
 *   letters          D12  12 V WS2811, 10 pixels: C A S I N O N E P O (OPEN reversed)
 *   closedLight      A0   the 11th Grove stick (connector A0 of the shield): 10 white LEDs behind the plate
 *                         that light the "CLOSED" hole text
 *   Arrow sticks (Grove Base Shield connectors; LED 1 of a stick is its DIN end):
 *   (V2 wiring, chosen to be natural on the real sign; differs from version 1)
 *   L-CAP D2   L-WING-UP D6   L-WING-DN D5   RAIL-UP-L D7   RAIL-UP-R D8   RAIL-DN-L D3   RAIL-DN-R D4
 *   R-CAP A3   R-WING-UP A1   R-WING-DN A2   (connector A0: the CLOSED light stick)
 *
 * Inputs (both use the internal pull-up resistor, the switch pulls the pin to GND):
 *   D32 slide switch  arrow direction (handle on the GND side = LOW = the arrow points to the customer's LEFT,
 *                     seen from the front). The switch is on the back of the sign: mount it so that, seen from
 *                     the back, the GND contact (1) is on the right-hand side; then the handle points the same
 *                     way as the arrow does in real space (toward the customer's left = the back's right).
 *   D33 push button   skips the power-on LED test (it has no other function)
 *   D30 slide switch  SW5 "casino closed" (handle on the GND side = LOW = closed): the OPEN
 *                     letters go dark and the CLOSED light turns on
 *   D42-D45  menu switch: 16-position hex rotary switch (pins 1, 2, 4, 8), free-running (relative): moves the
 *                 menu selection or changes a value
 *   D48 OK, D49 BACK, D50 SHIFT  push buttons to GND: OK opens the menu from the status page, enters a submenu,
 *                 starts and accepts an edit; BACK goes one level up or cancels an edit; SHIFT held while turning
 *                 makes the step bigger
 *   (the common pin of each hex switch goes to GND; the speeds and brightness curves are no longer set with switches:
 *   they are menu properties, saved in EEPROM)
 *
 * Power-on LED test: for 10 s after power-up all strips run a short window of lit LEDs from the first
 * to the last LED at the same time (2 passes: white, then red-green-blue), so every LED and the
 * colour order can be checked. The LCD shows the seconds in a progress bar and a scrolling
 * "press D33 to skip" (the two rows swap places every 2.5 s). Pressing D33 skips the test.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as
 * fast while many state changes happen; it is shortly inverted on every state change: arrow direction,
 * effect change, casino open/closed, menu key or switch). Once per second (DEBUG_MS) the Serial Monitor (115200
 * baud, SERIAL_BAUD) gets a line "DBG up=.. loops=.. fx=<frame>/<letters> arrow=R|L (customer view) closed=.. menu=S|M|E|C|V events=..
 * beats=.. tick=..", so the values change from line to line. The baud rate is also printed at start-up.
 *
 * LCD display (I2C, SDA = D20, SCL = D21, 5 V, GND): the status page (row 0 "F12 L03 Open|Closed" = frame and letters
 * effect, a star before the number for a random effect, e.g. "F*07"; row 1 the arrow symbol and "Customer right|left", it
 * alternates with "Change in M:SS" while an effect is random) or the settings menu
 * (OK opens it; the menu is described in tools/menu-editor/menu-v2.json and generated into the block below).
 * Turning a slide switch (arrow, casino) closes an open menu. At the menu root the display returns to the
 * status page after 60 s without input.
 * At boot an I2C scan (addresses 0x08-0x77) prints every device that answers to the Serial Monitor, and
 * a hint if nothing answers on the LCD address (many displays use 0x3F instead of 0x27).
 *
 * Effects (menu Effects > Frame effect / Letters effect, 0..15, saved in EEPROM; effect 0 = random, default 1):
 * letters: 1 breathe  2 wave  3 typewriter  4 ignite  5 word blink  6 alternate
 *   7 scanner (a bright spot with a tail sweeps back and forth)  8 sparkle (random twinkle)
 *   9 chase (one dim letter runs through each word)  10 pulse (heartbeat)
 *   11 converge (each word lights from both ends towards the middle)  12 words swap (the words breathe in opposite phase)
 *   13 slot machine (letters flicker like reels and lock in one by one)  14 twin chase (two dim spots run in opposite directions)
 *   15 marquee (three letters lit, one dark, moving)
 * frame (white): 1 bar  2 comet  3 fill  4 ignite  5 sides blink  6 odd even  7 ping-pong  8 twinkle  9 runner
 *   10 heartbeat  11 corners  12 edges swap  13 slot machine  14 twin comets  15 dashes (see frameLevel())
 * Frame and letters are chosen independently. Random (0): the group picks a pattern, never the same twice in a row;
 * one shared timer (menu Effects > Random delay, default 2 minutes, 0 = 1..30 minutes at random) changes every group that
 * is set to random at the same moment. Patterns 1, 2, 3, 7, 9, 14 and 15 of the frame follow the arrow direction.
 *
 * To change the sign edit ONLY the CONFIGURATION section below and, for
 * Wokwi, regenerate diagram.json with: python3 tools/generate_v2_diagram.py
 *
 * Arrow sticks: see arrow_sticks/README.txt (drill template, layout, wiring). Sketch settings are
 * STICK_PINS and LEDS_PER_STICK below.
 *
 * Note: the Arduino build inserts function prototypes at the top of this file,
 * so function signatures only use types that exist before the first function
 * (built-in types and Adafruit_NeoPixel). Do not add custom structs to them.
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>  // built into the Arduino IDE; used for the I2C LCD (no extra library)
#include <EEPROM.h>  // built into the Arduino IDE; stores the menu values
#include <math.h>
#include <string.h>
#include <stdio.h>

// ============================== CONFIGURATION ==============================

// --- Strips (12 V WS2811: one chip = one "pixel" = 3 LEDs = 10 cm, green-red-blue order) -----
// Frame: ONE strip of 18 pixels around the plate (60 x 30 cm): 6 pixels along the bottom and the top edge, 3 along
// each side edge. Index 0 (pixel 1) is the lower-left corner; the strip then runs counter-clockwise, seen from the
// front: bottom edge left to right, right edge upwards, top edge right to left, left edge downwards.
constexpr uint8_t FRAME_EDGE_LONG = 6;     // pixels along the bottom edge and along the top edge
constexpr uint8_t FRAME_EDGE_SHORT = 3;    // pixels along the left edge and along the right edge

// Text: two stacked lines. Letters are counted left to right, line 1 first.
constexpr char LINE_1_TEXT[] = "CASINO";
constexpr char LINE_2_TEXT[] = "OPEN";
// Letters: ONE pixel per letter on a 10-pixel strip. Strip order: C A S I N O, then the OPEN letters REVERSED
// (N E P O): the strip enters the sign at the OPEN row's right end. letterPixel() maps a letter to its pixel.

// The arrow: 10 Grove RGB LED sticks of 10 LEDs each (see arrow_sticks/README.txt for the geometry).
constexpr uint8_t STICK_COUNT = 10;
constexpr uint16_t LEDS_PER_STICK = 10;    // must be even (the caps are split into an upper and a lower half)

// Which end of a real Grove stick carries the "ball" (the round marker on the stick's board). The layout and
// drill drawings (layout-diagram, arrow_sticks/) show a black dot on every stick: that is where the BALL of the
// real stick goes. Every stick is mounted exactly as drawn, with its ball at the dot.
//   true  = the ball of a real stick is at its DIN end (LED 1, where the data goes in). The LEDs are numbered
//           from the dot, so the program uses the LED numbers as they are.
//   false = the ball is at the OTHER end of the stick (LED 10). The program counts the LEDs of every stick from
//           the other end, so the animation still runs the right way although the sticks stay as drawn.
// If the animation runs backwards along the sticks on the real sign, flip this value (do not re-mount the sticks).
// The CLOSED light stick is always white, so this value does not matter for it.
constexpr bool STICK_BALL_AT_DIN_END = true;

// The white light behind the "CLOSED" hole text: the 11th Grove RGB LED stick (10 LEDs, connector A0 of the
// Grove Base Shield), always white. It sits behind the plate; its LED window is 74 x 4 mm.
constexpr uint16_t CLOSED_LIGHT_LEDS = 10;

// --- Data pins -------------------------------------------------------------
constexpr uint8_t PIN_FRAME = 11;          // strip 1: the frame (18 pixels)
constexpr uint8_t PIN_LETTERS = 12;        // strip 2: the letters (10 pixels); D13 stays the status LED
constexpr uint8_t PIN_CLOSED_LIGHT = A0;   // Grove Base Shield connector A0 (the 11th stick)

// Arrow sticks. The data pins are the signal pins of the Grove Base Shield connectors D2..D8 and
// A1..A3 (the connector A0 carries the CLOSED light stick). Stick numbers = positions in this list.
constexpr uint8_t STICK_PINS[STICK_COUNT] = {2, 6, 5, 7, 8, 3, 4, A3, A1, A2};
constexpr uint8_t STICK_L_CAP = 0;       // vertical bar, left end        (D2)
constexpr uint8_t STICK_L_WING_UP = 1;   // left arrowhead, upper wing    (D6)
constexpr uint8_t STICK_L_WING_DN = 2;   // left arrowhead, lower wing    (D5)
constexpr uint8_t STICK_RAIL_UP_L = 3;   // upper rail, left stick        (D7)
constexpr uint8_t STICK_RAIL_UP_R = 4;   // upper rail, right stick       (D8)
constexpr uint8_t STICK_RAIL_DN_L = 5;   // lower rail, left stick        (D3)
constexpr uint8_t STICK_RAIL_DN_R = 6;   // lower rail, right stick       (D4)
constexpr uint8_t STICK_R_CAP = 7;       // vertical bar, right end       (A3)
constexpr uint8_t STICK_R_WING_UP = 8;   // right arrowhead, upper wing   (A1)
constexpr uint8_t STICK_R_WING_DN = 9;   // right arrowhead, lower wing   (A2)
// Logical LED 0 of a stick is its BALL end (the black dot in the drawings); see STICK_BALL_AT_DIN_END.

// --- Input pins ------------------------------------------------------------
constexpr uint8_t PIN_ARROW_SWITCH = 32;   // slide switch: arrow direction
constexpr uint8_t PIN_SKIP_BUTTON = 33;    // push button: skips the power-on LED test only
// Direction of the arrow switch (D32, slide switch, internal pull-up). It depends on how the mechanical switch
// is wired and mounted, so it is a source-code setting:
//   true  = pin LOW (handle on the GND side) -> the arrow points to the customer's LEFT, pin HIGH -> RIGHT
//   false = reversed: pin LOW -> the arrow points to the customer's RIGHT, pin HIGH -> LEFT
// If the arrow points the wrong way for a handle position, flip this one constant.
constexpr bool ARROW_POINTS_LEFT_WHEN_LOW = true;
constexpr uint8_t PIN_CASINO_CLOSED_SWITCH = 30;   // slide switch SW5: casino closed
constexpr bool CASINO_CLOSED_WHEN_LOW = true;      // handle on the GND side = closed (false: reverse)
constexpr uint8_t DEBOUNCE_MS = 30;        // an input must stay stable this long

// Hex rotary switch (the menu switch): 4 pins (bit values 1, 2, 4, 8) plus a
// common pin that goes to GND. The pins use the internal pull-ups.
constexpr bool HEX_BIT_IS_LOW = true;      // a bit counts as set when its pin reads LOW (the switch
                                           // connects it to the common pin = GND). Set to false if
                                           // your switch is complement coded (reads 15 - position).
constexpr unsigned long HEX_DEBOUNCE_MS = 10;   // a hex switch position must stay unchanged this long

// Settings menu controls (the menu is described in tools/menu-editor/menu-v2.json):
constexpr uint8_t PIN_MENU_SWITCH[4] = {42, 43, 44, 45};  // menu hex switch (free-running, relative): selection / value
constexpr bool MENU_SWITCH_REVERSED = false;              // true: turning it clockwise moves the other way
constexpr uint8_t PIN_OK = 48;             // push button OK: opens the menu, enters a submenu, starts / accepts an edit
constexpr uint8_t PIN_BACK = 49;           // push button BACK: one level up, cancels an edit
constexpr uint8_t PIN_SHIFT = 50;          // push button SHIFT: held while turning makes the step bigger
constexpr int MENU_EEPROM_BASE = 0;        // the saved menu image starts at this EEPROM address

// --- Sign of life (see the top of this file) ---------------------------------
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED on the Mega board
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated here, at the top, and printed at start-up)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" line
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a state change

// --- LCD display: 2x16 character LCD with an I2C backpack (PCF8574), SDA = D20, SCL = D21 ---
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;  // Wokwi uses 0x27; if a real display stays blank try 0x3F
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint8_t LCD_CHARS_PER_LOOP = 4;  // characters sent per loop() pass, so animations never wait

// --- Power-on LED test -----------------------------------------------------
// At power-up every LED is lit once per pass (all strips at the same time, each with a short
// moving window) so wiring and colours can be checked. Pressing D33 skips the test.
constexpr uint32_t STARTUP_TEST_MS = 10000;      // length of the test (0 = no test)
constexpr uint8_t STARTUP_TEST_PASSES = 2;       // pass 1 is white, the other passes red-green-blue
constexpr uint8_t STARTUP_TEST_WINDOW_LEDS = 2;  // pixels lit at the same time in each strip (the strips are short)
constexpr uint16_t LCD_TEST_SWAP_MS = 2500;      // the two LCD rows swap places this often during the test
constexpr uint16_t LCD_TEST_SCROLL_MS = 250;     // the "press D33 to skip" text scrolls one character per step

// --- Look and timing -------------------------------------------------------
// There is no global brightness: the library brightness stays at 255 and the maximum of each brightness curve
// below is the brightness limit of its LED type (default 80 of 255 = the same limit as the old global setting).

// --- Brightness curves (post-processing of the brightness of every LED) -----
// The colours are computed as before; the brightness of each colour (its largest channel) is then mapped through a
// 256-entry lookup table, one table per LED type, so every type can be tuned on its own. A new LED type needs one new
// LED_TYPE_* constant (and a bigger LED_TYPE_COUNT) and one more value in each array below.
//   out = min + (max - min) * ((1 - mix) * t + mix * t^exponent),  t = in / 255,  rounded to 0..255
// If keepZero is true, an input of 0 always stays 0 (unlit LEDs stay dark even when min > 0).
// The tables are built at start-up and rebuilt by setBrightnessCurve() whenever a value is changed.
constexpr uint8_t LED_TYPE_STRIP = 0;  // the frame strip
constexpr uint8_t LED_TYPE_STICK = 1;  // Grove RGB LED sticks: the arrow and the CLOSED light
constexpr uint8_t LED_TYPE_LETTERS = 2;  // the letters strip
constexpr uint8_t LED_TYPE_COUNT = 3;
// Defaults (same values for every type for now):
constexpr uint8_t CURVE_DEFAULT_MIN[LED_TYPE_COUNT] = {0, 0, 0};              // 0..255, output for the dimmest lit LED
constexpr uint8_t CURVE_DEFAULT_MAX[LED_TYPE_COUNT] = {80, 80, 80};              // 0..255, output for input 255 = the brightness limit
constexpr float CURVE_DEFAULT_EXPONENT[LED_TYPE_COUNT] = {2.67f, 2.67f, 2.67f};   // 0.1..20, 1 = linear
constexpr float CURVE_DEFAULT_MIX[LED_TYPE_COUNT] = {1.0f, 1.0f, 1.0f};          // 0..1, 0 = linear, 1 = full power curve
constexpr bool CURVE_DEFAULT_KEEP_ZERO[LED_TYPE_COUNT] = {true, true, true};
constexpr float CURVE_EXPONENT_MIN = 0.1f;
constexpr float CURVE_EXPONENT_MAX = 20.0f;

// Speeds: the multipliers (x0.125 .. x4) are menu properties; all the times below are the times at multiplier 1.0.
constexpr uint8_t MIN_UPDATE_MS = 8;       // frame and arrow never update more often than this

constexpr uint16_t FRAME_SPEED_MS = 200;   // time between frame animation steps (at multiplier 1)
constexpr uint8_t FRAME_BAR_LEDS = 2;      // pixels lit, then the same number dark, repeating

constexpr uint16_t LETTERS_SPEED_MS = 30;  // time between letter backlight updates (the letter patterns
                                           // themselves follow the letters speed)
constexpr uint32_t FLICKER_MIN_GAP_MS = 5000;     // shortest pause between flicker events
constexpr uint32_t FLICKER_MAX_GAP_MS = 12000;    // longest pause between flicker events
constexpr uint32_t FLICKER_DURATION_MS = 700;     // how long the faulty letter flickers

// Letter mode timing
constexpr uint16_t LETTER_BREATH_MS = 4000;       // breathe: one breathing cycle
constexpr uint16_t LETTER_WAVE_MS = 2400;         // wave: time for the glow to cross the text
constexpr uint16_t LETTER_TYPE_STEP_MS = 350;     // typewriter: time per letter
constexpr uint16_t LETTER_IGNITE_STEP_MS = 350;   // ignite: delay between letters starting up
constexpr uint16_t LETTER_BLINK_PERIOD_MS = 1600; // word blink: full on/off period of the second word
constexpr uint16_t LETTER_BLINK_ON_MS = 1000;     // word blink: how long it stays bright
constexpr uint16_t LETTER_ALTERNATE_MS = 2400;    // alternate: cycle of the odd/even swap
constexpr uint16_t LETTER_SCANNER_MS = 2400;      // scanner: one trip from the first to the last letter and back
constexpr uint16_t LETTER_SPARKLE_STEP_MS = 140;  // sparkle: how often the set of sparkling letters changes
constexpr uint16_t LETTER_CHASE_STEP_MS = 300;    // chase: time the dim spot stays on one letter
constexpr uint16_t LETTER_PULSE_MS = 1400;        // pulse: one heartbeat (two beats and a pause)
constexpr uint16_t LETTER_CONVERGE_STEP_MS = 400; // converge: time per step (grow 3 steps, hold 3, off 2)
constexpr uint16_t LETTER_SWAP_MS = 2400;         // words swap: one breathing cycle of the two words
constexpr uint16_t LETTER_SLOT_REEL_MS = 80;      // slot machine: a spinning letter changes this often
constexpr uint16_t LETTER_SLOT_FIRST_LOCK_MS = 1500;  // slot machine: the first letter locks after this long
constexpr uint16_t LETTER_SLOT_LOCK_STEP_MS = 300;    // slot machine: time between two letters locking
constexpr uint16_t LETTER_SLOT_HOLD_END_MS = 7200;    // slot machine: all letters go dark at this time ...
constexpr uint16_t LETTER_SLOT_PERIOD_MS = 8000;      // ... and the cycle restarts at this time
constexpr uint16_t LETTER_TWIN_STEP_MS = 250;     // twin chase: time the dim spots stay on a letter
constexpr uint16_t LETTER_MARQUEE_STEP_MS = 250;  // marquee: time per step

constexpr uint16_t ARROW_SPEED_MS = 35;    // time between arrow animation steps (at multiplier 1)
constexpr uint16_t ARROW_WAVE_LEDS = 24;   // LEDs per full colour cycle

// CLOSED light, like a faulty fluorescent tube. Times are in pattern milliseconds, so they
// follow the letters speed (menu) (at multiplier 2 everything happens twice as fast).
constexpr uint16_t CLOSED_STARTUP_MS = 1200;       // switching on: the tube tries to start, then stays on
constexpr uint32_t CLOSED_STEADY_MIN_MS = 5000;    // steady light for at least this long
constexpr uint32_t CLOSED_STEADY_MAX_MS = 14000;   // ... and at most this long
constexpr uint16_t CLOSED_FLICKER_MIN_MS = 400;    // a flicker burst lasts at least this long
constexpr uint16_t CLOSED_FLICKER_MAX_MS = 1500;   // ... and at most this long
constexpr uint16_t CLOSED_HOLD_MIN_MS = 30;        // during a burst a brightness is held this long at least
constexpr uint16_t CLOSED_HOLD_MAX_MS = 140;       // ... and at most this long

// ================================ DERIVED ==================================

constexpr uint8_t FIRST_WORD_LETTERS = sizeof(LINE_1_TEXT) - 1;  // letters in line 1
constexpr uint8_t LETTER_COUNT = FIRST_WORD_LETTERS + sizeof(LINE_2_TEXT) - 1;  // = pixels of the letter strip

// Frame: pixel count and the four corners (strip index of the corner pixel, seen from the front).
constexpr uint16_t FRAME_PIXELS = 2 * (FRAME_EDGE_LONG + FRAME_EDGE_SHORT);                 // 18
constexpr uint16_t FRAME_LOWER_LEFT = 0;                                                    // pixel 1
constexpr uint16_t FRAME_LOWER_RIGHT = FRAME_EDGE_LONG - 1;                                 // pixel 6
constexpr uint16_t FRAME_UPPER_RIGHT = FRAME_EDGE_LONG + FRAME_EDGE_SHORT;                  // pixel 10
constexpr uint16_t FRAME_UPPER_LEFT = 2 * FRAME_EDGE_LONG + FRAME_EDGE_SHORT - 1;           // pixel 15

// The rails are attached to LED 3 and LED 8 of a cap (index 2 and 7). The tail cap lights the LEDs between
// and including those two (the middle 6); the head cap lights the 3 LEDs at each of its ends (the rail LED and
// the two beyond it) and leaves the middle 4 dark.
constexpr uint16_t ARROW_TAIL_DARK_ENDS = 2;                // dark LEDs at each end of the tail cap
constexpr uint16_t ARROW_HEAD_CAP_LIT = 3;                  // lit LEDs at each end of the head cap
constexpr uint16_t ARROW_TAIL_HALF = LEDS_PER_STICK / 2 - ARROW_TAIL_DARK_ENDS;  // lit tail cap LEDs per lane (3)
constexpr uint16_t ARROW_RAIL_LEDS = 2 * LEDS_PER_STICK;    // LEDs of one rail lane (two sticks in a row)
constexpr uint16_t ARROW_HEAD_START = ARROW_TAIL_HALF + ARROW_RAIL_LEDS;  // flow coordinate of the first head cap LED
constexpr uint16_t ARROW_WING_START = ARROW_HEAD_START + ARROW_HEAD_CAP_LIT;  // flow coordinate where a wing starts
constexpr uint16_t ARROW_PATH_LEDS = ARROW_WING_START + LEDS_PER_STICK;  // whole path per lane (36), tail to wing tip
constexpr uint16_t ARROW_PULSE_TURN = ARROW_PATH_LEDS;      // the lower pulse starts when the upper one reaches the tip
constexpr uint16_t ARROW_PULSE_PERIOD = 2 * ARROW_PULSE_TURN;  // upper pulse, then lower pulse, then again
constexpr uint8_t ARROW_PULSE_TAIL = 5;                     // LEDs of a pulse (head plus tail)
constexpr uint8_t FRAME_PERIOD = 2 * FRAME_BAR_LEDS;

// Letter modes
constexpr uint8_t MODE_BREATHE = 0;      // all letters breathe slowly, slightly out of phase
constexpr uint8_t MODE_WAVE = 1;         // a glow runs through the letters from left to right
constexpr uint8_t MODE_TYPEWRITER = 2;   // letters switch on one by one, hold, then all off
constexpr uint8_t MODE_IGNITE = 3;       // letters start up like cold neon tubes, flickering
constexpr uint8_t MODE_WORD_BLINK = 4;   // first word steady, second word blinks
constexpr uint8_t MODE_ALTERNATE = 5;    // odd and even letters breathe in opposite phase
constexpr uint8_t MODE_SCANNER = 6;      // a bright spot with a tail sweeps back and forth over the letters
constexpr uint8_t MODE_SPARKLE = 7;      // letters twinkle at random and fade out softly
constexpr uint8_t MODE_CHASE = 8;        // one dim letter runs through each word (the words run separately)
constexpr uint8_t MODE_PULSE = 9;        // all letters beat together like a heart (two beats, a pause)
constexpr uint8_t MODE_CONVERGE = 10;    // each word lights up from both ends towards its middle, holds, goes dark
constexpr uint8_t MODE_WORDS_SWAP = 11;  // the two words breathe in opposite phase
constexpr uint8_t MODE_SLOT = 12;        // letters flicker like slot machine reels and lock in one by one
constexpr uint8_t MODE_TWIN_CHASE = 13;  // two dim spots run through all letters in opposite directions
constexpr uint8_t MODE_MARQUEE = 14;     // three letters lit, one dark, moving along the letters
constexpr uint8_t PATTERN_MODE_COUNT = 15;  // patterns 0..14 = effects 1..15 of the menu; effect 0 = random (menu "Random delay")

// The ignite cycle: off, letters start one by one, hold, off again.
constexpr uint32_t IGNITE_FIRST_MS = 500;
constexpr uint32_t IGNITE_FLICKER_MS = 600;
constexpr uint32_t IGNITE_HOLD_END_MS =
    IGNITE_FIRST_MS + (uint32_t)LETTER_COUNT * LETTER_IGNITE_STEP_MS + IGNITE_FLICKER_MS + 3000UL;
constexpr uint32_t IGNITE_PERIOD_MS = IGNITE_HOLD_END_MS + 1500UL;

static_assert(LEDS_PER_STICK % 2 == 0, "LEDS_PER_STICK must be even");
static_assert(LETTER_COUNT > 1, "At least two letters are required");
static_assert(FRAME_PIXELS % 2 == 0 && FRAME_UPPER_RIGHT < FRAME_PIXELS && FRAME_UPPER_LEFT < FRAME_PIXELS,
              "the frame needs an even number of pixels (both light paths must be equally long)");
static_assert(PATTERN_MODE_COUNT == 15, "the menu has 15 effects per group (plus 0 = random)");
static_assert(STARTUP_TEST_PASSES > 0 && STARTUP_TEST_WINDOW_LEDS > 0, "the start-up test needs a pass and a window");
static_assert(CLOSED_LIGHT_LEDS > 0, "CLOSED_LIGHT_LEDS must be at least 1");

// ================================ STRIPS ===================================

Adafruit_NeoPixel frame(FRAME_PIXELS, PIN_FRAME, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel letters(LETTER_COUNT, PIN_LETTERS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel closedLight(CLOSED_LIGHT_LEDS, PIN_CLOSED_LIGHT, NEO_GRB + NEO_KHZ800);

// The arrow sticks (Grove RGB LED sticks, WS2813 mini: green-red-blue order like the strips).
Adafruit_NeoPixel stickLCap(LEDS_PER_STICK, STICK_PINS[STICK_L_CAP], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickLWingUp(LEDS_PER_STICK, STICK_PINS[STICK_L_WING_UP], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickLWingDn(LEDS_PER_STICK, STICK_PINS[STICK_L_WING_DN], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRailUpL(LEDS_PER_STICK, STICK_PINS[STICK_RAIL_UP_L], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRailUpR(LEDS_PER_STICK, STICK_PINS[STICK_RAIL_UP_R], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRailDnL(LEDS_PER_STICK, STICK_PINS[STICK_RAIL_DN_L], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRailDnR(LEDS_PER_STICK, STICK_PINS[STICK_RAIL_DN_R], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRCap(LEDS_PER_STICK, STICK_PINS[STICK_R_CAP], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRWingUp(LEDS_PER_STICK, STICK_PINS[STICK_R_WING_UP], NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stickRWingDn(LEDS_PER_STICK, STICK_PINS[STICK_R_WING_DN], NEO_GRB + NEO_KHZ800);

// The sticks in the order of STICK_PINS (NeoPixel objects must not be copied: pointers only).
Adafruit_NeoPixel *const sticks[STICK_COUNT] = {&stickLCap,    &stickLWingUp, &stickLWingDn, &stickRailUpL, &stickRailUpR,
                                                &stickRailDnL, &stickRailDnR, &stickRCap,    &stickRWingUp, &stickRWingDn};

constexpr uint8_t STRIP_COUNT = 3 + STICK_COUNT;
Adafruit_NeoPixel *const allStrips[STRIP_COUNT] = {&frame,         &letters,      &closedLight,
                                                   &stickLCap,     &stickLWingUp, &stickLWingDn, &stickRailUpL,
                                                   &stickRailUpR,  &stickRailDnL, &stickRailDnR, &stickRCap,
                                                   &stickRWingUp,  &stickRWingDn};

// =============================== SHARED STATE ==============================

uint8_t sineTable[256];  // 0..255, one full sine period, filled in setup()

bool arrowPointsRight = true;  // set from the slide switch; also steers the frame animation
bool casinoClosed = false;     // set from the "casino closed" switch SW5

unsigned long lastFrameUpdate = 0;
unsigned long lastLettersUpdate = 0;
unsigned long lastArrowUpdate = 0;

// ============================== SETTINGS MENU ==============================
// Tables and engine of the settings menu (described in tools/menu-editor/menu-v2.json). The block is generated:
// edit the JSON with the Menu Editor and run `node tools/generate_v2_menu.js`. The sketch supplies menuCallback().
//@V2_MENU_BEGIN (generated by tools/generate_v2_menu.js from tools/menu-editor/menu-v2.json and tools/menu_engine/engine.inc; do not edit by hand)
// BEGIN GENERATED MENU TABLES (do not edit: made by tools/menu_engine/generate_menu_tables.js from menu-v2.json)
// 22 properties, 50 items, 11 menus, 1 presets; EEPROM layout tag adc94b94, 20 stored properties
constexpr uint8_t MENU_PROP_COUNT = 22;
constexpr uint8_t MENU_STORED_COUNT = 20;
constexpr uint32_t MENU_LAYOUT_TAG = 0xADC94B94UL;
constexpr uint16_t MENU_NO_STR16 = 0xFFFF;
constexpr uint8_t P_frameEffect = 0;
constexpr uint8_t P_lettersEffect = 1;
constexpr uint8_t P_arrow = 2;
constexpr uint8_t P_casino = 3;
constexpr uint8_t P_arrowReverse = 4;
constexpr uint8_t P_casinoReverse = 5;
constexpr uint8_t P_speedFrame = 6;
constexpr uint8_t P_speedArrow = 7;
constexpr uint8_t P_speedLetters = 8;
constexpr uint8_t P_randomDelay = 9;
constexpr uint8_t P_stripMin = 10;
constexpr uint8_t P_stripMax = 11;
constexpr uint8_t P_stripExp = 12;
constexpr uint8_t P_stickMin = 13;
constexpr uint8_t P_stickMax = 14;
constexpr uint8_t P_stickExp = 15;
constexpr uint8_t P_stripMix = 16;
constexpr uint8_t P_lettersMin = 17;
constexpr uint8_t P_lettersMax = 18;
constexpr uint8_t P_lettersExp = 19;
constexpr uint8_t P_lettersMix = 20;
constexpr uint8_t P_stickMix = 21;
constexpr uint8_t CB_effectFrameChanged = 0;
constexpr uint8_t CB_effectLettersChanged = 1;
constexpr uint8_t CB_applySwitches = 2;
constexpr uint8_t CB_applySpeeds = 3;
constexpr uint8_t CB_rebuildStripCurve = 4;
constexpr uint8_t CB_rebuildStickCurve = 5;
constexpr uint8_t CB_rebuildLettersCurve = 6;
constexpr uint8_t MENU_CB_COUNT = 7;
constexpr uint8_t MENU_STATUS_CASINO = P_casino;
constexpr uint8_t MENU_STATUS_ARROW = P_arrow;
constexpr uint8_t MENU_STATUS_EFFECT_FRAME = P_frameEffect;
constexpr uint8_t MENU_STATUS_EFFECT_LETTERS = P_lettersEffect;
constexpr bool MENU_STATUS_ARROW_ASCII = false;
constexpr uint16_t MENU_ROOT_NAME_STR = 59;
// strings
const char MENU_STR[] PROGMEM =
  "Frame effect\0"
  "Random\0"
  "1 Bar\0"
  "2 Comet\0"
  "3 Fill\0"
  "4 Ignite\0"
  "5 Sides blink\0"
  "6 Odd even\0"
  "7 Ping-pong\0"
  "8 Twinkle\0"
  "9 Runner\0"
  "10 Heartbeat\0"
  "11 Corners\0"
  "12 Edges swap\0"
  "13 Slot machine\0"
  "14 Twin comets\0"
  "15 Dashes\0"
  "Letters effect\0"
  "1 Breathe\0"
  "2 Wave\0"
  "3 Typewriter\0"
  "5 Word blink\0"
  "6 Alternate\0"
  "7 Scanner\0"
  "8 Sparkle\0"
  "9 Chase\0"
  "10 Pulse\0"
  "11 Converge\0"
  "12 Words swap\0"
  "14 Twin chase\0"
  "15 Marquee\0"
  "Arrow\0"
  "Left\0"
  "Right\0"
  "Casino\0"
  "Closed\0"
  "Open\0"
  "Arrow reverse\0"
  "No\0"
  "Yes\0"
  "Casino reverse\0"
  "Frame speed\0"
  "x\0"
  "Arrow speed\0"
  "Letters speed\0"
  "Random delay\0"
  " min\0"
  "random\0"
  "Min\0"
  "Max\0"
  "Exponent\0"
  "Mix\0"
  "effectFrameChanged\0"
  "effectLettersChanged\0"
  "applySwitches\0"
  "applySpeeds\0"
  "rebuildStripCurve\0"
  "rebuildStickCurve\0"
  "rebuildLettersCurve\0"
  "Menu\0"
  "Controls\0"
  "Effects\0"
  "Speed\0"
  "Frame\0"
  "Letters\0"
  "Brightness\0"
  "Restore defaults\0"
  "Restore defaults?\0";
const uint16_t MENU_STR_OFF[] PROGMEM = {0, 13, 20, 26, 34, 41, 50, 64, 75, 87, 97, 106, 119, 130, 144, 160, 175, 185, 200, 210, 217, 230, 243, 255, 265, 275, 283, 292, 304, 318, 332, 343, 349, 354, 360, 367, 374, 379, 393, 396, 400, 415, 427, 429, 441, 455, 468, 473, 480, 484, 488, 497, 501, 520, 541, 555, 567, 585, 603, 623, 628, 637, 645, 651, 657, 665, 676, 693};
// properties
const uint8_t PP_KIND[] PROGMEM = {3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 2, 0, 0, 2, 1, 0, 0, 2, 1, 1};
const uint8_t PP_FLAGS[] PROGMEM = {2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
const uint8_t PP_SHIFT[] PROGMEM = {1, 1, 1, 1, 1, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
const uint8_t PP_DEC[] PROGMEM = {0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 2, 2, 2};
const uint16_t PP_COUNT[] PROGMEM = {16, 16, 2, 2, 2, 2, 31, 31, 31, 31, 256, 256, 51, 256, 256, 51, 51, 256, 256, 51, 51, 51};
const uint16_t PP_DEFPOS[] PROGMEM = {1, 1, 0, 0, 0, 0, 18, 18, 18, 2, 0, 80, 31, 0, 80, 31, 50, 0, 80, 31, 50, 50};
const float PP_MIN[] PROGMEM = {0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.125000000f, 0.125000000f, 0.125000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.100000000f, 0.00000000f, 0.00000000f};
const float PP_MAX[] PROGMEM = {0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 0.00000000f, 4.00000000f, 4.00000000f, 4.00000000f, 30.0000000f, 255.000000f, 255.000000f, 20.0000000f, 255.000000f, 255.000000f, 20.0000000f, 1.00000000f, 255.000000f, 255.000000f, 20.0000000f, 1.00000000f, 1.00000000f};
const uint16_t PP_AUX[] PROGMEM = {0, 0, 0, 0, 0, 0, 30, 30, 30, 1, 1, 1, 50, 1, 1, 50, 50, 1, 1, 50, 50, 50};
const uint16_t PP_NAME[] PROGMEM = {0, 17, 31, 34, 37, 40, 41, 43, 44, 45, 48, 49, 50, 48, 49, 50, 51, 48, 49, 50, 51, 51};
const uint16_t PP_UNIT[] PROGMEM = {MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, 42, 42, 42, 46, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
const uint8_t PP_VT_FIRST[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t PP_VT_COUNT[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const uint16_t PP_CH_FIRST[] PROGMEM = {0, 16, 32, 34, 36, 38, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40};
const uint8_t PP_CB_FIRST[] PROGMEM = {0, 1, 2, 2, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
const uint8_t PP_CB_COUNT[] PROGMEM = {1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t PP_SLOT[] PROGMEM = {0, 1, 255, 255, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
const float VT_KEY[] PROGMEM = {0.00000000f};
const uint16_t VT_STR[] PROGMEM = {47};
const uint16_t CH_STR[] PROGMEM = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1, 18, 19, 20, 5, 21, 22, 23, 24, 25, 26, 27, 28, 14, 29, 30, 32, 33, 35, 36, 38, 39, 38, 39};
const uint8_t CB_LIST[] PROGMEM = {0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 4, 6, 6, 6, 6, 5};
const uint16_t CB_NAME_STR[] PROGMEM = {52, 53, 54, 55, 56, 57, 58};
// menus and items
const uint8_t MN_FIRST[] PROGMEM = {0, 8, 12, 15, 18, 24, 30, 35, 38, 42, 46};
const uint8_t MN_COUNT[] PROGMEM = {8, 4, 3, 3, 6, 6, 5, 3, 4, 4, 4};
const uint8_t IT_TYPE[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const uint8_t IT_A[] PROGMEM = {1, 2, 3, 4, 5, 6, 7, 0, 2, 4, 3, 5, 0, 1, 9, 6, 8, 7, 0, 6, 10, 11, 12, 16, 1, 8, 17, 18, 19, 20, 7, 13, 14, 15, 21, 8, 9, 10, 10, 11, 12, 16, 17, 18, 19, 20, 13, 14, 15, 21};
const uint16_t IT_NAME[] PROGMEM = {60, 61, 62, 63, 64, 31, 65, 66, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, 63, 64, 31, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
const uint16_t IT_TEXT[] PROGMEM = {MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16, MENU_NO_STR16};
// presets
const uint16_t PR_NAME[] PROGMEM = {66};
const uint16_t PR_TEXT[] PROGMEM = {MENU_NO_STR16};
const uint16_t PR_CONFIRM[] PROGMEM = {67};
const uint8_t PR_FLAGS[] PROGMEM = {3};
const uint8_t PR_FIRST[] PROGMEM = {0};
const uint8_t PR_COUNT[] PROGMEM = {0};
const uint8_t PV_PROP[] PROGMEM = {0};
const uint16_t PV_POS[] PROGMEM = {0};
// END GENERATED MENU TABLES

// ============================== MENU ENGINE ===============================
// Same rules as tools/menu-editor/menu-core.js (the reference); tests/menu_diff_test.py checks that both give identical
// display rows, callbacks and EEPROM writes for the same key events. The tables above are generated from the menu JSON.
// Time is passed in as `now` (millis()). All text rows are exactly MENU_COLS characters.

constexpr uint8_t MENU_COLS = 16;
constexpr unsigned long MENU_TIMEOUT_MS = 60000UL;        // at the root level without input: back to the status page
constexpr unsigned long MENU_MARQUEE_PAUSE_MS = 1000UL;   // a long text stays at its start this long
constexpr unsigned long MENU_MARQUEE_STEP_MS = 400UL;     // then moves one character per step
constexpr unsigned long MENU_STATUS_ALT_MS = 3000UL;      // status row 1 alternates with the countdown (an effect is random)
constexpr uint8_t MENU_MAX_DEPTH = 8;
constexpr char MENU_LEFT_ARROW = 0x7F;                    // HD44780 ROM arrows
constexpr char MENU_RIGHT_ARROW = 0x7E;
constexpr char MENU_LOCK_CHAR = 1;                        // custom character 1 (a padlock) drawn by the sketch; 0 is not usable in C strings
constexpr uint8_t MENU_KEY_OK = 0;
constexpr uint8_t MENU_KEY_BACK = 1;
constexpr uint8_t MENU_NO_SLOT = 255;                    // property is not stored in EEPROM

constexpr uint8_t MM_STATUS = 0, MM_MENU = 1, MM_EDIT = 2, MM_CONFIRM = 3, MM_VIEW = 4;   // MM_VIEW: a read-only property shown like an edit page
constexpr uint8_t MK_INT = 0, MK_FLOAT = 1, MK_LOG = 2, MK_CHOICE = 3;
constexpr uint8_t MF_READONLY = 1, MF_PERSIST = 2, MF_WRAP = 4, MF_HEX = 8;

uint16_t menuPosArr[MENU_PROP_COUNT];            // current position of every property
uint8_t menuMode = MM_STATUS;
uint8_t menuDepth = 0;                           // number of menu levels open (1 = root)
uint8_t menuLevelMenu[MENU_MAX_DEPTH];           // menu table index of each level
uint8_t menuLevelItem[MENU_MAX_DEPTH];           // selected item (0-based, inside the menu) of each level
uint8_t menuLevelOpener[MENU_MAX_DEPTH];         // global item index that opened the level (its name is the title)
uint8_t menuEditProp = 0;
uint16_t menuEditStart = 0;
uint8_t menuConfirmPreset = 0;
unsigned long menuLastInput = 0;
unsigned long menuSince = 0;
constexpr unsigned long MENU_UNDO_MS = 300UL;   // a move straight back to the item just left (contact bounce) keeps that item's scroll
bool menuUndoValid = false;
uint8_t menuUndoItem = 0;
unsigned long menuUndoSince = 0, menuUndoAt = 0;
uint8_t menuActiveFrame = 0, menuActiveLetters = 0;   // effect numbers (1..15) really showing; 0 = unknown (shown as --)
long menuChangeInSec = -1;                       // seconds until the random effects change (-1 = unknown)
bool menuEepromValid = false;                    // the stored image matches this layout

#ifdef MENU_TEST_LOG
struct MenuLogCb { uint8_t cb, prop; uint16_t pos; };
MenuLogCb menuLogCb[4000];
uint16_t menuLogCbCount = 0;
uint8_t menuLogBatch[600];                       // batches: count, then the property indexes
uint16_t menuLogBatchLen = 0;
#endif

void menuCallback(uint8_t cb, uint8_t prop);     // supplied by the sketch: apply the new value of `prop`

// ---- table access
uint8_t menuRd8(const uint8_t *table, uint16_t i) { return pgm_read_byte(&table[i]); }
uint16_t menuRd16(const uint16_t *table, uint16_t i) { return pgm_read_word(&table[i]); }
float menuRdF(const float *table, uint16_t i) { return pgm_read_float(&table[i]); }

// Copies string number `index` into out (at most max-1 characters).
void menuStr(uint16_t index, char *out, uint8_t max) {
  out[0] = 0;
  if (index == MENU_NO_STR16) return;
  const char *p = MENU_STR + menuRd16(MENU_STR_OFF, index);
  uint8_t n = 0;
  while (n + 1 < max) {
    const char c = (char)pgm_read_byte(p + n);
    if (c == 0) break;
    out[n++] = c;
  }
  out[n] = 0;
}

// ---- property maths
uint16_t menuPropCount(uint8_t prop) { return menuRd16(PP_COUNT, prop); }

float menuValueAt(uint8_t prop, uint16_t pos) {
  const uint8_t kind = menuRd8(PP_KIND, prop);
  const float minV = menuRdF(PP_MIN, prop), maxV = menuRdF(PP_MAX, prop);
  const uint16_t aux = menuRd16(PP_AUX, prop);
  if (kind == MK_INT) return minV + (float)pos * (float)aux;
  if (kind == MK_FLOAT) return minV + (float)pos * (maxV - minV) / (float)aux;
  if (kind == MK_LOG) return minV * powf(maxV / minV, (float)pos / (float)aux);
  return (float)pos;
}

float menuValue(uint8_t prop) { return menuValueAt(prop, menuPosArr[prop]); }
uint16_t menuPos(uint8_t prop) { return menuPosArr[prop]; }

uint16_t menuMovePos(uint8_t prop, uint16_t pos, long delta) {
  const long n = menuPropCount(prop);
  long q = (long)pos + delta;
  if (menuRd8(PP_FLAGS, prop) & MF_WRAP) return (uint16_t)(((q % n) + n) % n);
  if (q < 0) q = 0;
  if (q > n - 1) q = n - 1;
  return (uint16_t)q;
}

// Appends the decimal text of x with `dec` decimals (round half away from zero) to out at position n.
uint8_t menuFmtNum(char *out, uint8_t n, float x, uint8_t dec) {
  bool neg;
  unsigned long v;
  if (dec == 0) {                                // same as Math.round: a half rounds up
    const long r = (long)floorf(x + 0.5f);
    neg = r < 0;
    v = (unsigned long)(neg ? -r : r);
  } else {
    neg = x < 0;
    const float a = neg ? -x : x;
    unsigned long scale = 1;
    for (uint8_t k = 0; k < dec; k++) scale *= 10UL;
    v = (unsigned long)floorf(a * (float)scale + 0.5f);
  }
  char digits[12];
  uint8_t len = 0;
  do { digits[len++] = (char)('0' + (v % 10)); v /= 10; } while (v > 0 || len <= dec);
  if (neg) out[n++] = '-';
  while (len > 0) {
    len--;
    out[n++] = digits[len];
    if (len == dec && dec > 0) out[n++] = '.';
  }
  out[n] = 0;
  return n;
}

// The text of a property value, for example "1.12x", "random", "B", "Yes".
void menuValueText(uint8_t prop, uint16_t pos, char *out) {
  const uint8_t kind = menuRd8(PP_KIND, prop);
  out[0] = 0;
  if (kind == MK_CHOICE) {
    menuStr(menuRd16(CH_STR, menuRd16(PP_CH_FIRST, prop) + pos), out, 24);
    return;
  }
  const float v = menuValueAt(prop, pos);
  const uint8_t vtCount = menuRd8(PP_VT_COUNT, prop);
  for (uint8_t k = 0; k < vtCount; k++) {
    const uint8_t at = menuRd8(PP_VT_FIRST, prop) + k;
    if (fabsf(v - menuRdF(VT_KEY, at)) < 0.0000005f) { menuStr(menuRd16(VT_STR, at), out, 24); return; }
  }
  uint8_t n = 0;
  if (menuRd8(PP_FLAGS, prop) & MF_HEX) {
    long r = (long)floorf(v + 0.5f);
    char tmp[9];
    uint8_t t = 0;
    const char hexDigits[] = "0123456789ABCDEF";
    do { tmp[t++] = hexDigits[r & 15]; r >>= 4; } while (r > 0 && t < 8);
    while (t > 0) out[n++] = tmp[--t];
    out[n] = 0;
  } else {
    n = menuFmtNum(out, 0, v, menuRd8(PP_DEC, prop));
  }
  const uint16_t unit = menuRd16(PP_UNIT, prop);
  if (unit != MENU_NO_STR16) {
    char u[12];
    menuStr(unit, u, sizeof(u));
    for (uint8_t k = 0; u[k] != 0 && n + 1 < 24; k++) out[n++] = u[k];
    out[n] = 0;
  }
}

// ---- row building
// Writes into out (MENU_COLS characters + 0): left text, then a gap and the right text aligned to the right edge.
// A left text that does not fit scrolls (marquee) inside the free width.
void menuFit(char *out, const char *left, const char *right, unsigned long now, unsigned long since) {
  const uint8_t ll = (uint8_t)strlen(left), rl = (uint8_t)strlen(right);
  int avail = MENU_COLS - rl - (rl ? 1 : 0);
  if (avail < 0) avail = 0;
  uint8_t off = 0;
  if (ll > avail) {
    const unsigned long maxOff = (unsigned long)(ll - avail);
    unsigned long t = now >= since ? now - since : 0;
    const unsigned long total = MENU_MARQUEE_PAUSE_MS + maxOff * MENU_MARQUEE_STEP_MS + MENU_MARQUEE_PAUSE_MS;
    t = t % total;
    unsigned long o = t < MENU_MARQUEE_PAUSE_MS ? 0 : (t - MENU_MARQUEE_PAUSE_MS) / MENU_MARQUEE_STEP_MS;
    if (o > maxOff) o = maxOff;
    off = (uint8_t)o;
  }
  for (uint8_t k = 0; k < MENU_COLS; k++) out[k] = ' ';
  for (int k = 0; k < avail && k < MENU_COLS; k++) {
    const int src = off + k;
    if (src < ll) out[k] = left[src];
  }
  uint8_t p = (uint8_t)avail;
  if (rl) {
    if (p < MENU_COLS) out[p++] = ' ';
    for (uint8_t k = 0; k < rl && p < MENU_COLS; k++) out[p++] = right[k];
  }
  out[MENU_COLS] = 0;
}

uint8_t menuItemBase(uint8_t menuIndex) { return menuRd8(MN_FIRST, menuIndex); }

void menuTouch(unsigned long now, bool newView) {
  menuLastInput = now;
  if (newView) menuSince = now;
}

void menuToStatus(unsigned long now) {
  menuUndoValid = false;
  menuMode = MM_STATUS;
  menuDepth = 0;
  menuSince = now;
}

// Status page text of one effect: two digits, with a star before them when the effect is random (then the number
// of the effect really showing, or -- when it is not known).
void menuEffectText(uint8_t prop, uint8_t active, char *out) {
  const bool random = menuPosArr[prop] == 0;
  const unsigned n = random ? active : menuPosArr[prop];
  if (n > 0 && n < 100) snprintf(out, 8, "%s%02u", random ? "*" : "", n);
  else snprintf(out, 8, "%s--", random ? "*" : "");
}

// Builds the two display rows for time `now`.
void menuRows(unsigned long now, char *row0, char *row1) {
  char a[48], b[48];
  if (menuMode == MM_STATUS) {
    char valueTextBuf[24];
    menuValueText(MENU_STATUS_CASINO, menuPosArr[MENU_STATUS_CASINO], valueTextBuf);
    char ef[8], el[8];
    menuEffectText(MENU_STATUS_EFFECT_FRAME, menuActiveFrame, ef);
    menuEffectText(MENU_STATUS_EFFECT_LETTERS, menuActiveLetters, el);
    snprintf(a, sizeof(a), "F%s L%s %s", ef, el, valueTextBuf);
    const bool left = menuPosArr[MENU_STATUS_ARROW] == 0;
    if (MENU_STATUS_ARROW_ASCII) snprintf(b, sizeof(b), "%s", left ? "<-Customer right" : "->Customer left");
    else snprintf(b, sizeof(b), "%c Customer %s", left ? MENU_LEFT_ARROW : MENU_RIGHT_ARROW, left ? "right" : "left");
    if ((menuPosArr[MENU_STATUS_EFFECT_FRAME] == 0 || menuPosArr[MENU_STATUS_EFFECT_LETTERS] == 0) && menuChangeInSec >= 0) {
      const unsigned long since = now >= menuSince ? now - menuSince : 0;
      if ((since / MENU_STATUS_ALT_MS) % 2 == 1) snprintf(b, sizeof(b), "Change in %ld:%02ld", menuChangeInSec / 60, menuChangeInSec % 60);
    }
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, b, "", now, menuSince);
    return;
  }
  if (menuMode == MM_EDIT || menuMode == MM_VIEW) {
    menuStr(menuRd16(PP_NAME, menuEditProp), a, sizeof(a));
    char v[24];
    menuValueText(menuEditProp, menuPosArr[menuEditProp], v);
    snprintf(b, sizeof(b), "%c %s", menuMode == MM_VIEW ? MENU_LOCK_CHAR : '>', v);
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, b, "", now, menuSince);
    return;
  }
  if (menuMode == MM_CONFIRM) {
    const uint8_t pr = menuConfirmPreset;
    const uint16_t confirm = menuRd16(PR_CONFIRM, pr);
    if (confirm != MENU_NO_STR16) menuStr(confirm, a, sizeof(a));
    else {
      const uint16_t text = menuRd16(PR_TEXT, pr);
      menuStr(text != MENU_NO_STR16 ? text : menuRd16(PR_NAME, pr), a, sizeof(a) - 1);
      const size_t l = strlen(a);
      a[l] = '?';
      a[l + 1] = 0;
    }
    menuFit(row0, a, "", now, menuSince);
    menuFit(row1, "OK=yes BACK=no", "", now, menuSince);
    return;
  }
  const uint8_t level = menuDepth - 1;
  const uint8_t menuIndex = menuLevelMenu[level];
  const uint8_t item = menuItemBase(menuIndex) + menuLevelItem[level];
  if (level == 0) {
    const long rem = (long)MENU_TIMEOUT_MS - (long)(now - menuLastInput);
    const long sec = rem <= 0 ? 0 : (rem + 999) / 1000;
    char s[20];
    snprintf(s, sizeof(s), "%lds", sec);
    menuStr(MENU_ROOT_NAME_STR, a, sizeof(a));
    menuFit(row0, a, s, now, menuSince);
  } else {
    menuStr(menuRd16(IT_NAME, menuLevelOpener[level]), a, sizeof(a));
    menuFit(row0, a, "", now, menuSince);
  }
  const uint8_t type = menuRd8(IT_TYPE, item);
  const uint16_t text = menuRd16(IT_TEXT, item);
  if (type == 0) {                                  // submenu
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(IT_NAME, item), a, sizeof(a));
    menuFit(row1, a, ">", now, menuSince);
  } else if (type == 1) {                           // property
    const uint8_t prop = menuRd8(IT_A, item);
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(PP_NAME, prop), a, sizeof(a));
    char v[24];
    menuValueText(prop, menuPosArr[prop], v);
    if (strlen(a) + 1 + strlen(v) <= MENU_COLS) menuFit(row1, a, v, now, menuSince);
    else {                                          // name and value that do not fit together scroll as one text
      snprintf(b, sizeof(b), "%s: %s", a, v);
      menuFit(row1, b, "", now, menuSince);
    }
  } else {                                          // preset
    menuStr(text != MENU_NO_STR16 ? text : menuRd16(IT_NAME, item), a, sizeof(a));
    menuFit(row1, a, "", now, menuSince);
  }
}

// ---- values, callbacks, EEPROM
void menuCallCallbacks(uint8_t prop) {
  const uint8_t n = menuRd8(PP_CB_COUNT, prop);
  for (uint8_t k = 0; k < n; k++) {
    const uint8_t cb = menuRd8(CB_LIST, menuRd8(PP_CB_FIRST, prop) + k);
#ifdef MENU_TEST_LOG
    if (menuLogCbCount < 4000) { menuLogCb[menuLogCbCount].cb = cb; menuLogCb[menuLogCbCount].prop = prop; menuLogCb[menuLogCbCount].pos = menuPosArr[prop]; menuLogCbCount++; }
#endif
    menuCallback(cb, prop);
  }
}

bool menuSetPos(uint8_t prop, uint16_t pos) {       // changes the value and calls the callbacks
  if (pos == menuPosArr[prop]) return false;
  menuPosArr[prop] = pos;
  menuCallCallbacks(prop);
  return true;
}

void menuEepromWriteTag() {
  for (uint8_t k = 0; k < 4; k++) EEPROM.update(MENU_EEPROM_BASE + k, (uint8_t)(MENU_LAYOUT_TAG >> (8 * k)));
}
void menuEepromWriteSlot(uint8_t prop) {
  const uint8_t slot = menuRd8(PP_SLOT, prop);
  if (slot != MENU_NO_SLOT) EEPROM.update(MENU_EEPROM_BASE + 4 + slot, (uint8_t)menuPosArr[prop]);
}

// Writes the given properties (only the stored ones) to EEPROM as one batch.
void menuSave(const uint8_t *props, uint8_t n) {
  uint8_t list[MENU_PROP_COUNT];
  uint8_t count = 0;
  for (uint8_t k = 0; k < n; k++) {
    const uint8_t f = menuRd8(PP_FLAGS, props[k]);
    if ((f & MF_PERSIST) && !(f & MF_READONLY)) list[count++] = props[k];
  }
  if (count == 0) return;
#ifdef MENU_TEST_LOG
  if (menuLogBatchLen + count + 1 < 600) { menuLogBatch[menuLogBatchLen++] = count; for (uint8_t k = 0; k < count; k++) menuLogBatch[menuLogBatchLen++] = list[k]; }
#endif
  if (!menuEepromValid) {                           // first save after a layout change: write the whole image
    menuEepromWriteTag();
    for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) menuEepromWriteSlot(p);
    menuEepromValid = true;
    return;
  }
  for (uint8_t k = 0; k < count; k++) menuEepromWriteSlot(list[k]);
}

// Start-up: values from EEPROM when the stored layout matches and every slot is valid, otherwise all defaults.
bool menuBegin() {
  bool ok = true;
  for (uint8_t k = 0; k < 4; k++) if (EEPROM.read(MENU_EEPROM_BASE + k) != (uint8_t)(MENU_LAYOUT_TAG >> (8 * k))) ok = false;
  for (uint8_t p = 0; p < MENU_PROP_COUNT && ok; p++) {
    const uint8_t slot = menuRd8(PP_SLOT, p);
    if (slot != MENU_NO_SLOT && EEPROM.read(MENU_EEPROM_BASE + 4 + slot) >= menuPropCount(p)) ok = false;
  }
  for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) {
    const uint8_t slot = menuRd8(PP_SLOT, p);
    menuPosArr[p] = (ok && slot != MENU_NO_SLOT) ? (uint16_t)EEPROM.read(MENU_EEPROM_BASE + 4 + slot) : menuRd16(PP_DEFPOS, p);
  }
  menuEepromValid = ok;
  menuMode = MM_STATUS;
  menuDepth = 0;
  return ok;
}

// ---- events
void menuSet(uint8_t prop, uint16_t pos) { menuSetPos(prop, pos); }   // a value changed outside the menu (no EEPROM write)

bool menuIsStatusProp(uint8_t prop) {
  return prop == MENU_STATUS_CASINO || prop == MENU_STATUS_ARROW;
}

// A physical control changed: the value follows; an open menu closes for the status properties.
void menuPhysical(uint8_t prop, uint16_t pos, unsigned long now) {
  const bool isStatus = menuIsStatusProp(prop);
  if (isStatus && menuMode == MM_EDIT) menuSetPos(menuEditProp, menuEditStart);
  menuSetPos(prop, pos);
  if (isStatus && menuMode != MM_STATUS) menuToStatus(now);
}

void menuTurn(int steps, bool shift, unsigned long now) {
  if (steps == 0) return;
  if (menuMode == MM_MENU) {
    const uint8_t level = menuDepth - 1;
    const long n = menuRd8(MN_COUNT, menuLevelMenu[level]);
    long k = (long)menuLevelItem[level] + steps;
    if (k < 0) k = 0;
    if (k > n - 1) k = n - 1;
    if (k == menuLevelItem[level]) menuTouch(now, false);        // already at the first or last item: nothing moves, the scroll goes on
    else if (menuUndoValid && menuUndoItem == (uint8_t)k && now - menuUndoAt <= MENU_UNDO_MS) {
      menuLevelItem[level] = (uint8_t)k; menuSince = menuUndoSince; menuUndoValid = false; menuTouch(now, false);   // straight back: its scroll goes on
    } else {
      menuUndoValid = true; menuUndoItem = menuLevelItem[level]; menuUndoSince = menuSince; menuUndoAt = now;
      menuLevelItem[level] = (uint8_t)k; menuTouch(now, true);
    }
  } else if (menuMode == MM_EDIT) {
    const long factor = shift ? menuRd8(PP_SHIFT, menuEditProp) : 1;
    menuSetPos(menuEditProp, menuMovePos(menuEditProp, menuPosArr[menuEditProp], (long)steps * factor));
    menuTouch(now, false);
  }
}

// Applies a preset: all values first, then the callbacks once per changed property, then one EEPROM batch.
void menuApplyPreset(uint8_t preset) {
  uint8_t order[MENU_PROP_COUNT];
  uint16_t want[MENU_PROP_COUNT];
  uint8_t n = 0;
  const uint8_t flags = menuRd8(PR_FLAGS, preset);
  if (flags & 2) {                                  // setAll: every property that is not read-only gets its default
    for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) {
      if (menuRd8(PP_FLAGS, p) & MF_READONLY) continue;
      order[n] = p;
      want[p] = menuRd16(PP_DEFPOS, p);
      n++;
    }
  }
  const uint8_t first = menuRd8(PR_FIRST, preset), count = menuRd8(PR_COUNT, preset);
  for (uint8_t k = 0; k < count; k++) {
    const uint8_t p = menuRd8(PV_PROP, first + k);
    bool present = false;
    for (uint8_t j = 0; j < n; j++) if (order[j] == p) present = true;
    if (!present) order[n++] = p;
    want[p] = menuRd16(PV_POS, first + k);
  }
  uint8_t changed[MENU_PROP_COUNT];
  uint8_t changedCount = 0;
  for (uint8_t j = 0; j < n; j++) {
    const uint8_t p = order[j];
    if (menuPosArr[p] != want[p]) { menuPosArr[p] = want[p]; changed[changedCount++] = p; }
  }
  for (uint8_t j = 0; j < changedCount; j++) menuCallCallbacks(changed[j]);
  menuSave(changed, changedCount);
}

void menuKey(uint8_t key, unsigned long now) {
  menuUndoValid = false;
  if (menuMode == MM_STATUS) {
    if (key == MENU_KEY_OK) {
      menuMode = MM_MENU;
      menuDepth = 1;
      menuLevelMenu[0] = 0;
      menuLevelItem[0] = 0;
      menuTouch(now, true);
    }
    return;
  }
  if (menuMode == MM_MENU) {
    if (key == MENU_KEY_BACK) {
      if (menuDepth > 1) { menuDepth--; menuTouch(now, true); }
      else menuToStatus(now);
      return;
    }
    const uint8_t level = menuDepth - 1;
    const uint8_t item = menuItemBase(menuLevelMenu[level]) + menuLevelItem[level];
    const uint8_t type = menuRd8(IT_TYPE, item);
    const uint8_t a = menuRd8(IT_A, item);
    if (type == 0) {
      if (menuDepth < MENU_MAX_DEPTH) {
        menuLevelMenu[menuDepth] = a;
        menuLevelItem[menuDepth] = 0;
        menuLevelOpener[menuDepth] = item;
        menuDepth++;
        menuTouch(now, true);
      }
    } else if (type == 1) {
      menuMode = (menuRd8(PP_FLAGS, a) & MF_READONLY) ? MM_VIEW : MM_EDIT;
      menuEditProp = a;
      menuEditStart = menuPosArr[a];
      menuTouch(now, true);
    } else if (menuRd8(PR_FLAGS, a) & 1) {          // preset with confirmation
      menuMode = MM_CONFIRM;
      menuConfirmPreset = a;
      menuTouch(now, true);
    } else {
      menuApplyPreset(a);
      menuTouch(now, false);
    }
    return;
  }
  if (menuMode == MM_VIEW) {                        // a read-only property: any key goes back
    menuMode = MM_MENU;
    menuTouch(now, true);
    return;
  }
  if (menuMode == MM_EDIT) {
    if (key == MENU_KEY_OK) menuSave(&menuEditProp, 1);
    else menuSetPos(menuEditProp, menuEditStart);
    menuMode = MM_MENU;
    menuTouch(now, true);
    return;
  }
  if (key == MENU_KEY_OK) menuApplyPreset(menuConfirmPreset);   // confirm page
  menuMode = MM_MENU;
  menuTouch(now, true);
}

void menuTick(unsigned long now) {
  if (menuMode == MM_MENU && menuDepth == 1 && now - menuLastInput >= MENU_TIMEOUT_MS) menuToStatus(now);
}
//@V2_MENU_END

// ================================ HELPERS ==================================

// Linear blend between two channel values, f = 0..255.
uint8_t mixChannel(uint8_t a, uint8_t b, uint8_t f) {
  return (uint8_t)(((uint16_t)a * (255 - f) + (uint16_t)b * f) / 255);
}

// Position 0..255 inside a repeating cycle of periodMs milliseconds.
uint8_t cycleOf(unsigned long t, uint16_t periodMs) {
  return (uint8_t)(((t % periodMs) * 256UL) / periodMs);
}

// Pixel of the letter strip behind a letter (letters are counted in reading order: C A S I N O, O P E N).
// The first word sits on pixels 0..5 in the same order; the strip then continues with the OPEN letters
// REVERSED, so O P E N are pixels 9, 8, 7, 6.
uint8_t letterPixel(uint8_t letter) {
  if (letter < FIRST_WORD_LETTERS) return letter;
  return (uint8_t)(FIRST_WORD_LETTERS + (LETTER_COUNT - 1 - letter));
}

// ============================== LCD STATE ==================================
// The display shows the status pages or the settings menu: menuRows() of the menu engine builds both rows
// (see the LCD DISPLAY section further down).

bool lcdRebuild = true;                   // rebuild the text at once

// ================================= INPUTS ==================================
// Debounced inputs with the internal pull-up: a closed switch reads LOW.

constexpr uint8_t INPUT_ARROW = 0;
constexpr uint8_t INPUT_SKIP = 1;
constexpr uint8_t INPUT_CLOSED = 2;
constexpr uint8_t INPUT_OK = 3;
constexpr uint8_t INPUT_BACK = 4;
constexpr uint8_t INPUT_SHIFT = 5;

uint8_t inputStable[6];            // debounced level (HIGH or LOW)
uint8_t inputLastRaw[6];           // last raw reading
unsigned long inputChangedAt[6];   // when the raw reading last changed

// Reads the pin and returns true when the debounced level just changed.
bool pollInput(uint8_t slot, uint8_t pin, unsigned long now) {
  const uint8_t raw = digitalRead(pin);
  if (raw != inputLastRaw[slot]) {
    inputLastRaw[slot] = raw;
    inputChangedAt[slot] = now;
  }
  if (raw != inputStable[slot] && now - inputChangedAt[slot] >= DEBOUNCE_MS) {
    inputStable[slot] = raw;
    return true;
  }
  return false;
}

void initInput(uint8_t slot, uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
  inputStable[slot] = digitalRead(pin);
  inputLastRaw[slot] = inputStable[slot];
  inputChangedAt[slot] = millis();
}

// ============================== HEX SWITCHES ===============================
// One 16-position hex rotary switch (positions 0..F, binary coded on four pins):
//   menu switch     free-running: only the steps between two positions count (relative)
// A turning switch passes through other positions, so a position only counts once it has stayed unchanged
// for HEX_DEBOUNCE_MS.

constexpr uint8_t SLOT_MENU = 0;

uint8_t hexStable[1];              // debounced switch positions
uint8_t hexLastRaw[1];
unsigned long hexChangedAt[1];

// Reads the four pins of a hex switch (pins[0] = bit value 1 ... pins[3] = bit value 8).
uint8_t readHexSwitch(const uint8_t *pins) {
  uint8_t code = 0;
  for (uint8_t bit = 0; bit < 4; bit++) {
    const bool low = digitalRead(pins[bit]) == LOW;
    if (low == HEX_BIT_IS_LOW) code |= (uint8_t)(1 << bit);
  }
  return code;
}

void initHexSwitch(uint8_t slot, const uint8_t *pins) {
  for (uint8_t bit = 0; bit < 4; bit++) pinMode(pins[bit], INPUT_PULLUP);
  hexStable[slot] = readHexSwitch(pins);
  hexLastRaw[slot] = hexStable[slot];
  hexChangedAt[slot] = millis();
}

// Returns true when the debounced position just changed.
bool pollHexSwitch(uint8_t slot, const uint8_t *pins, unsigned long now) {
  const uint8_t raw = readHexSwitch(pins);
  if (raw != hexLastRaw[slot]) {
    hexLastRaw[slot] = raw;
    hexChangedAt[slot] = now;
  }
  if (raw != hexStable[slot] && now - hexChangedAt[slot] >= HEX_DEBOUNCE_MS) {
    hexStable[slot] = raw;
    return true;
  }
  return false;
}

// ================================ SPEEDS ===================================
// The speeds are menu properties (Speed > Frame, Letters, Arrow; multiplier x0.125 .. x4 in 30 log steps, default x1). The letter
// patterns run on their own clock (letterClockMs) that advances by real time x multiplier, so changing the speed
// does not make a jump.

uint16_t frameIntervalMs = FRAME_SPEED_MS;
uint16_t arrowIntervalMs = ARROW_SPEED_MS;
uint16_t letterMult256 = 256;
unsigned long letterClockMs = 0;   // pattern time of the letters (milliseconds)
uint32_t letterClockFrac = 0;      // fraction of a millisecond, in 1/256 ms
unsigned long lastLetterClockTick = 0;

// Update interval for a pattern whose interval is baseMs at multiplier 1.
uint16_t intervalFor(uint16_t baseMs, uint16_t mult256) {
  uint32_t ms = ((uint32_t)baseMs * 256UL) / mult256;
  if (ms < MIN_UPDATE_MS) ms = MIN_UPDATE_MS;
  if (ms > 65535UL) ms = 65535UL;
  return (uint16_t)ms;
}

// The multiplier of a speed property, times 256.
uint16_t multiplier256(uint8_t prop) {
  return (uint16_t)(menuValue(prop) * 256.0f + 0.5f);
}

// Called by the menu when a speed changed (and at start-up).
void applySpeeds() {
  frameIntervalMs = intervalFor(FRAME_SPEED_MS, multiplier256(P_speedFrame));
  arrowIntervalMs = intervalFor(ARROW_SPEED_MS, multiplier256(P_speedArrow));
  letterMult256 = multiplier256(P_speedLetters);
  Serial.print(F("Speeds: frame x"));
  Serial.print(menuValue(P_speedFrame), 2);
  Serial.print(F(" ("));
  Serial.print((unsigned int)frameIntervalMs);
  Serial.print(F(" ms), arrow x"));
  Serial.print(menuValue(P_speedArrow), 2);
  Serial.print(F(" ("));
  Serial.print((unsigned int)arrowIntervalMs);
  Serial.print(F(" ms), letters x"));
  Serial.println(menuValue(P_speedLetters), 2);
}

// Advances the pattern clock of the letters by the real time that has passed times the multiplier.
void advanceLetterClock(unsigned long now) {
  const unsigned long dt = now - lastLetterClockTick;
  lastLetterClockTick = now;
  letterClockFrac += (uint32_t)dt * letterMult256;
  letterClockMs += letterClockFrac >> 8;
  letterClockFrac &= 0xFF;
}

// ================================= FRAME ===================================
// One strip of FRAME_PIXELS pixels. The pattern (FRAME_BAR_LEDS lit, FRAME_BAR_LEDS dark, repeating) runs from a
// start corner to the opposite bottom corner along BOTH ways round the frame at once:
//   arrow right: start = upper-left corner (pixel 15), end = lower-right corner (pixel 6)
//   arrow left:  start = upper-right corner (pixel 10), end = lower-left corner (pixel 1)
// (corners as the customer sees the sign). The clockwise path and the counter-clockwise path are both
// FRAME_PIXELS / 2 steps long and meet at the end corner. The pattern is a function of (distance - phase) modulo
// FRAME_PERIOD, where "distance" is how many steps a pixel is from the start corner along its path.

// Frame step counter: counts the frame updates, wraps at FRAME_STEP_WRAP (a multiple of every pattern period) and
// restarts from 0 when the program changes.
constexpr uint16_t FRAME_STEP_WRAP = 3600;
uint16_t frameStep = 0;

// Distance of pixel i from the start corner, along the shorter way round (clockwise = decreasing index).
uint16_t frameDistance(uint16_t i, bool arrowRight) {
  const uint16_t start = arrowRight ? FRAME_UPPER_LEFT : FRAME_UPPER_RIGHT;
  const uint16_t clockwise = (start + FRAME_PIXELS - i) % FRAME_PIXELS;  // steps going clockwise
  return clockwise <= FRAME_PIXELS / 2 ? clockwise : (uint16_t)(FRAME_PIXELS - clockwise);
}

// ---- Brightness curves ----
uint8_t brightnessLut[LED_TYPE_COUNT][256];  // the lookup tables (RAM: 512 bytes)

// Builds the lookup table of one LED type from the given curve values (all clamped to their ranges).
// 256 powf() calls: fine at start-up and when a value is changed, far too slow to do per LED per frame.
void setBrightnessCurve(uint8_t type, uint8_t minValue, uint8_t maxValue, float exponent, float mix, bool keepZero) {
  if (type >= LED_TYPE_COUNT) return;
  if (exponent < CURVE_EXPONENT_MIN) exponent = CURVE_EXPONENT_MIN;
  if (exponent > CURVE_EXPONENT_MAX) exponent = CURVE_EXPONENT_MAX;
  if (mix < 0.0f) mix = 0.0f;
  if (mix > 1.0f) mix = 1.0f;
  for (uint16_t in = 0; in < 256; in++) {
    const float t = in / 255.0f;
    const float curve = (1.0f - mix) * t + mix * powf(t, exponent);
    float out = (keepZero && in == 0) ? 0.0f : minValue + ((float)maxValue - (float)minValue) * curve;
    if (out < 0.0f) out = 0.0f;
    if (out > 255.0f) out = 255.0f;
    brightnessLut[type][in] = (uint8_t)(out + 0.5f);
  }
}

// Builds the tables of all LED types from the defaults.
void buildDefaultBrightnessCurves() {
  for (uint8_t type = 0; type < LED_TYPE_COUNT; type++) {
    setBrightnessCurve(type, CURVE_DEFAULT_MIN[type], CURVE_DEFAULT_MAX[type], CURVE_DEFAULT_EXPONENT[type],
                       CURVE_DEFAULT_MIX[type], CURVE_DEFAULT_KEEP_ZERO[type]);
  }
}

// Rebuilds the curve of one LED type from its four menu properties (Min, Max, Exponent, Mix).
void rebuildCurve(uint8_t type, uint8_t propMin, uint8_t propMax, uint8_t propExponent, uint8_t propMix) {
  setBrightnessCurve(type, (uint8_t)(menuValue(propMin) + 0.5f), (uint8_t)(menuValue(propMax) + 0.5f),
                     menuValue(propExponent), menuValue(propMix), CURVE_DEFAULT_KEEP_ZERO[type]);
}

void rebuildAllCurves() {
  rebuildCurve(LED_TYPE_STRIP, P_stripMin, P_stripMax, P_stripExp, P_stripMix);
  rebuildCurve(LED_TYPE_LETTERS, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);
  rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);
}

// Maps the brightness (largest channel) of a packed 0xRRGGBB colour through the table of the LED type;
// the three channels are scaled by the same factor, so the hue stays the same.
uint32_t shapeColor(uint8_t type, uint32_t color) {
  const uint8_t r = (uint8_t)(color >> 16), g = (uint8_t)(color >> 8), b = (uint8_t)color;
  uint8_t v = r > g ? r : g;
  if (b > v) v = b;
  if (v == 0) return 0;
  const uint8_t out = brightnessLut[type][v];
  if (out == v) return color;
  const uint16_t rr = ((uint16_t)r * out + v / 2) / v, gg = ((uint16_t)g * out + v / 2) / v, bb = ((uint16_t)b * out + v / 2) / v;
  return ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | (uint32_t)bb;
}

// ================================ LETTERS ==================================
// Every letter is its own group of LEDs with its own brightness (0..255) that
// depends on the current mode. On top of every mode, now and then one random letter flickers like a
// faulty neon tube.

constexpr uint8_t NO_LETTER = 0xFF;

uint8_t activeFrame = 0;                // the frame pattern really shown (0..14 = effect 1..15)
uint8_t activeLetters = 0;              // the letter pattern really shown (0..14 = effect 1..15)
unsigned long autoPickedAt = 0;         // real time when the random effects last picked patterns
unsigned long autoDelayMs = 120000;     // how long they last (menu "Random delay")
unsigned long letterModeStart = 0;      // pattern clock (letterClockMs) when the letter pattern started

uint8_t flickerLetter = NO_LETTER;
bool flickerLit = true;
unsigned long flickerEndsAt = 0;
unsigned long nextFlickerAt = 0;

void updateFlickerState(unsigned long now) {
  if (flickerLetter == NO_LETTER) {
    if ((long)(now - nextFlickerAt) >= 0) {
      // While the casino is closed only the letters of the first word are lit, so only they flicker.
      flickerLetter = (uint8_t)random(casinoClosed ? FIRST_WORD_LETTERS : LETTER_COUNT);
      flickerEndsAt = now + FLICKER_DURATION_MS;
    }
  } else if ((long)(now - flickerEndsAt) >= 0) {
    flickerLetter = NO_LETTER;
    nextFlickerAt = now + (unsigned long)random((long)FLICKER_MIN_GAP_MS, (long)FLICKER_MAX_GAP_MS + 1);
  } else {
    flickerLit = random(100) < 40;  // irregular on/off while the fault lasts
  }
}

void printModeName(uint8_t mode) {
  switch (mode) {
    case MODE_BREATHE:     Serial.print(F("breathe")); break;
    case MODE_WAVE:        Serial.print(F("wave")); break;
    case MODE_TYPEWRITER:  Serial.print(F("typewriter")); break;
    case MODE_IGNITE:      Serial.print(F("ignite")); break;
    case MODE_WORD_BLINK:  Serial.print(F("word blink")); break;
    case MODE_ALTERNATE:   Serial.print(F("alternate")); break;
    case MODE_SCANNER:     Serial.print(F("scanner")); break;
    case MODE_SPARKLE:     Serial.print(F("sparkle")); break;
    case MODE_CHASE:       Serial.print(F("chase")); break;
    case MODE_PULSE:       Serial.print(F("pulse")); break;
    case MODE_CONVERGE:    Serial.print(F("converge")); break;
    case MODE_WORDS_SWAP:  Serial.print(F("words swap")); break;
    case MODE_SLOT:        Serial.print(F("slot machine")); break;
    case MODE_TWIN_CHASE:  Serial.print(F("twin chase")); break;
    case MODE_MARQUEE:     Serial.print(F("marquee")); break;
    default:               Serial.print(F("?")); break;
  }
}

void printEffects() {
  Serial.print(F("Effects: frame "));
  Serial.print((unsigned int)activeFrame + 1);
  Serial.print(F(", letters "));
  Serial.print((unsigned int)activeLetters + 1);
  Serial.print(F(": "));
  printModeName(activeLetters);
  Serial.println();
}

bool frameIsRandom() { return menuPos(P_frameEffect) == 0; }
bool lettersIsRandom() { return menuPos(P_lettersEffect) == 0; }

// A random pick from 0..PATTERN_MODE_COUNT-1, never the pattern that is showing now (previous >= PATTERN_MODE_COUNT = none).
uint8_t pickDifferent(uint8_t previous) {
  if (previous >= PATTERN_MODE_COUNT) return (uint8_t)random(PATTERN_MODE_COUNT);
  uint8_t pick = (uint8_t)random(PATTERN_MODE_COUNT - 1);
  if (pick >= previous) pick++;  // skip the previous pattern
  return pick;
}

// Both random groups change together: one shared timer, its length is the menu property "Random delay" (0 = 1..30 minutes at random).
void restartRandomTimer(unsigned long now) {
  autoPickedAt = now;
  long minutes = (long)(menuValue(P_randomDelay) + 0.5f);
  if (minutes <= 0) minutes = 1 + (long)random(30);
  autoDelayMs = (unsigned long)minutes * 60000UL;
}

void showFramePattern(uint8_t pattern) {
  activeFrame = pattern;
  frameStep = 0;
  menuActiveFrame = (uint8_t)(pattern + 1);
}

void showLettersPattern(uint8_t pattern) {
  activeLetters = pattern;
  letterModeStart = letterClockMs;  // the new pattern starts from its beginning
  menuActiveLetters = (uint8_t)(pattern + 1);
}

// The Frame effect property changed (or start-up): effect N shows pattern N-1, 0 picks a random pattern and
// restarts the shared timer so that both random groups keep changing at the same moment.
void applyFrameEffect(unsigned long now, bool atStart) {
  const uint16_t effect = menuPos(P_frameEffect);
  if (effect == 0) {
    showFramePattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeFrame));
    restartRandomTimer(now);
  } else {
    showFramePattern((uint8_t)(effect - 1));
  }
  printEffects();
}

void applyLettersEffect(unsigned long now, bool atStart) {
  const uint16_t effect = menuPos(P_lettersEffect);
  if (effect == 0) {
    showLettersPattern(pickDifferent(atStart ? PATTERN_MODE_COUNT : activeLetters));
    restartRandomTimer(now);
  } else {
    showLettersPattern((uint8_t)(effect - 1));
  }
  printEffects();
}

// The shared timer ran out: every group that is set to random picks a new pattern.
void updateRandomEffects(unsigned long now) {
  if (!frameIsRandom() && !lettersIsRandom()) return;
  if (now - autoPickedAt < autoDelayMs) return;
  if (frameIsRandom()) showFramePattern(pickDifferent(activeFrame));
  if (lettersIsRandom()) showLettersPattern(pickDifferent(activeLetters));
  restartRandomTimer(now);
  printEffects();
}

// A small deterministic hash for the stateless sparkle pattern.
uint16_t hash16(uint16_t a, uint16_t b) {
  uint32_t h = (uint32_t)a * 40503UL + (uint32_t)b * 9973UL + 12345UL;
  h ^= h >> 7;
  h *= 2654435761UL;
  h ^= h >> 13;
  return (uint16_t)(h >> 8);
}

// Heartbeat bump: jumps to full at `start` and fades out over `length` (all in 1/1000 of the period).
uint8_t pulseBump(uint16_t u, uint16_t start, uint16_t length) {
  if (u < start || u >= start + length) return 0;
  return (uint8_t)(255UL * (length - (u - start)) / length);
}

// ---- Frame patterns: effects 1..15 of the Frame effect property (the frame follows activeFrame) ----
// frameLevel() gives the brightness (0..255, white) of pixel i at step t. "q" is the position of a pixel along the
// direction of travel, counted from the start corner of the arrow (arrow right: upper-left corner, pixel 15, going
// clockwise; arrow left: upper-right corner, pixel 10, going counter-clockwise), so q = 0..17. The patterns of the
// patterns that follow the arrow (0, 1, 2, 6, 8, 13, 14; effects 1, 2, 3, 7, 9, 14, 15) use q or frameDistance(); the others ignore the direction.
//   0 bar (2 lit, 2 dark) runs from the start corner both ways to the opposite bottom corner (period 4)
//   1 comet with a 5-pixel tail runs once round the frame (18)      2 fills from the start corner both ways, holds, off (16)
//   3 pixels start up one by one, flickering, like cold tubes (36)  4 top and bottom edges steady, side edges blink (8)
//   5 odd and even pixels alternate (4)                             6 one comet ping-pongs round the frame (36)
//   7 random twinkle                                                8 one lit pixel in three runs round the frame (3)
//   9 heartbeat of the whole frame (10)                             10 corners light first, then the edges fill in (8)
//   11 long edges and side edges alternate (10)                     12 slot machine: pixels spin, then lock one by one (40)
//   13 twin comets run apart from the start corner (18)             14 two lit, one dark, running round the frame (3)
constexpr uint8_t FRAME_CORNERS[4] = {FRAME_LOWER_LEFT, FRAME_LOWER_RIGHT, FRAME_UPPER_RIGHT, FRAME_UPPER_LEFT};

uint8_t frameLevel(uint16_t i, bool arrowRight, uint8_t mode, uint16_t t) {
  const uint16_t q = arrowRight ? (uint16_t)((FRAME_UPPER_LEFT + FRAME_PIXELS - i) % FRAME_PIXELS)
                                : (uint16_t)((i + FRAME_PIXELS - FRAME_UPPER_RIGHT) % FRAME_PIXELS);
  const bool longEdge = i <= FRAME_LOWER_RIGHT || (i >= FRAME_UPPER_RIGHT && i <= FRAME_UPPER_LEFT);
  switch (mode) {
    case 1: {
      const uint16_t k = (uint16_t)((t % FRAME_PIXELS + FRAME_PIXELS - q) % FRAME_PIXELS);
      return k < 5 ? (uint8_t)(255 - 50 * k) : 0;
    }
    case 2: {
      const uint16_t p = t % 16;
      if (p < 10) return frameDistance(i, arrowRight) <= p ? 255 : 0;
      return p < 14 ? 255 : 0;
    }
    case 3: {
      const uint16_t p = t % 36;
      if (p < i) return 0;
      if (p < i + 3) return (hash16(i, t) & 1) ? 255 : 0;
      return p < 30 ? 230 : 0;
    }
    case 4:
      if (longEdge) return 200;
      return (t % 8) < 5 ? 255 : 20;
    case 5:
      return ((i + t / 2) & 1) ? 255 : 0;
    case 6: {
      const uint16_t tri = t % 36;
      const bool forward = tri < 18;
      const int16_t head = forward ? (int16_t)tri : (int16_t)(35 - tri);
      const int16_t behind = forward ? head - (int16_t)q : (int16_t)q - head;
      if (behind == 0) return 255;
      return (behind > 0 && behind < 4) ? (uint8_t)(255 - 70 * behind) : 0;
    }
    case 7:
      return (hash16(i, t) & 7) < 2 ? 255 : 30;
    case 8:
      return ((q + 3 - t % 3) % 3) == 0 ? 255 : 0;
    case 9: {
      const uint16_t s = t % 10;
      return s == 0 ? 255 : (s == 1 ? 120 : (s == 2 ? 200 : (s == 3 ? 60 : 20)));
    }
    case 10: {
      uint16_t nearest = FRAME_PIXELS;
      for (uint8_t c = 0; c < 4; c++) {
        uint16_t d = i > FRAME_CORNERS[c] ? i - FRAME_CORNERS[c] : FRAME_CORNERS[c] - i;
        if (d > FRAME_PIXELS - d) d = FRAME_PIXELS - d;   // the strip is a ring: pixel 17 is next to pixel 0
        if (d < nearest) nearest = d;
      }
      const uint16_t s = t % 8;
      if (s >= 6) return 0;
      return nearest <= s ? 255 : 0;
    }
    case 11:
      return ((t % 10) < 5) == longEdge ? 255 : 0;
    case 12: {
      const uint16_t p = t % 40;
      if (p < 6 + i) return (hash16(i, t) & 1) ? 255 : 0;
      return p < 36 ? 255 : 0;
    }
    case 13: {
      const uint16_t a = t % FRAME_PIXELS;
      const uint16_t b = (FRAME_PIXELS - a) % FRAME_PIXELS;
      const uint16_t k1 = (a + FRAME_PIXELS - q) % FRAME_PIXELS;   // distance behind the clockwise comet
      const uint16_t k2 = (q + FRAME_PIXELS - b) % FRAME_PIXELS;   // distance behind the counter-clockwise comet
      const uint8_t l1 = k1 < 3 ? (uint8_t)(255 - 80 * k1) : 0;
      const uint8_t l2 = k2 < 3 ? (uint8_t)(255 - 80 * k2) : 0;
      return l1 > l2 ? l1 : l2;
    }
    case 14:
      return ((q + 3 - t % 3) % 3) != 0 ? 255 : 0;
    default: {  // 0: the bar pattern
      const uint16_t distance = frameDistance(i, arrowRight);
      const uint8_t pos = (uint8_t)((distance % FRAME_PERIOD + FRAME_PERIOD - (t % FRAME_PERIOD)) % FRAME_PERIOD);
      return pos < FRAME_BAR_LEDS ? 255 : 0;
    }
  }
}

void drawFrame(bool arrowRight, uint8_t mode, uint16_t t) {
  for (uint16_t i = 0; i < FRAME_PIXELS; i++) {
    const uint8_t level = frameLevel(i, arrowRight, mode, t);
    frame.setPixelColor(i, shapeColor(LED_TYPE_STRIP, frame.Color(level, level, level)));
  }
}

void updateFrame() {
  drawFrame(arrowPointsRight, activeFrame, frameStep);
  frame.show();
  frameStep = (frameStep + 1) % FRAME_STEP_WRAP;
}

// Brightness of one letter in the current mode. t = milliseconds since the mode started.
uint8_t baseLetterLevel(uint8_t letter, unsigned long t) {
  switch (activeLetters) {
    case MODE_SCANNER: {
      // The head moves over the letters (position in 1/255 letter steps); letters behind it
      // glow on as a long tail, letters ahead of it only a little.
      const uint8_t c = cycleOf(t, LETTER_SCANNER_MS);
      const bool movingRight = c < 128;
      const int32_t tri = movingRight ? (int32_t)c * 2 : (int32_t)(255 - c) * 2;  // 0..254
      const int32_t signedDist = tri * (LETTER_COUNT - 1) - (int32_t)letter * 254;
      const bool behind = movingRight ? signedDist > 0 : signedDist < 0;
      const int32_t dist = signedDist < 0 ? -signedDist : signedDist;
      const int32_t spread = behind ? 254L * 3 : 254L * 7 / 10;
      if (dist >= spread) return 20;
      return (uint8_t)(20 + (235L * (spread - dist)) / spread);
    }
    case MODE_SPARKLE: {
      // Every step a random set of letters flashes up and then fades out until the next step.
      const uint16_t slot = (uint16_t)(t / LETTER_SPARKLE_STEP_MS);
      const uint16_t frac = (uint16_t)(t % LETTER_SPARKLE_STEP_MS);
      if ((hash16(letter, slot) & 7) < 2) {
        return (uint8_t)(35 + (220UL * (LETTER_SPARKLE_STEP_MS - frac)) / LETTER_SPARKLE_STEP_MS);
      }
      return 35;
    }
    case MODE_CHASE: {
      // One letter per word is dim and the dim spot moves on; each word runs on its own.
      const uint8_t second = LETTER_COUNT - FIRST_WORD_LETTERS;
      const uint16_t step = (uint16_t)(t / LETTER_CHASE_STEP_MS);
      const uint8_t dimLetter = letter < FIRST_WORD_LETTERS ? (uint8_t)(step % FIRST_WORD_LETTERS)
                                                            : (uint8_t)(FIRST_WORD_LETTERS + step % second);
      return letter == dimLetter ? 30 : 230;
    }
    case MODE_PULSE: {
      // Heartbeat: a strong beat, a weaker second beat, then a pause.
      const uint16_t u = (uint16_t)((t % LETTER_PULSE_MS) * 1000UL / LETTER_PULSE_MS);
      const uint8_t first = pulseBump(u, 0, 200);
      const uint8_t second = (uint8_t)((uint16_t)pulseBump(u, 260, 240) * 3 / 4);
      const uint8_t beat = first > second ? first : second;
      return (uint8_t)(50 + ((uint16_t)beat * 205) / 255);
    }
    case MODE_CONVERGE: {
      // Each word lights from both of its ends towards the middle (3 steps), holds (3 steps), goes dark (2 steps).
      const uint8_t step = (uint8_t)((t / LETTER_CONVERGE_STEP_MS) % 8);
      if (step >= 6) return 0;
      const uint8_t wordStart = letter < FIRST_WORD_LETTERS ? 0 : FIRST_WORD_LETTERS;
      const uint8_t wordLength = letter < FIRST_WORD_LETTERS ? FIRST_WORD_LETTERS : LETTER_COUNT - FIRST_WORD_LETTERS;
      const uint8_t inWord = letter - wordStart;
      const uint8_t fromEnd = inWord < wordLength - 1 - inWord ? inWord : (uint8_t)(wordLength - 1 - inWord);
      return fromEnd <= step ? 230 : 25;
    }
    case MODE_WORDS_SWAP: {
      // The first word rises while the second falls.
      const uint8_t cycle = cycleOf(t, LETTER_SWAP_MS);
      const uint8_t breath = sineTable[(uint8_t)(cycle + (letter < FIRST_WORD_LETTERS ? 0 : 128))];
      return (uint8_t)(40 + ((uint16_t)breath * 215) / 255);
    }
    case MODE_SLOT: {
      // Slot machine: every letter spins (random on/off) until it locks, from the first to the last letter.
      const uint32_t p = t % LETTER_SLOT_PERIOD_MS;
      const uint32_t lockAt = LETTER_SLOT_FIRST_LOCK_MS + (uint32_t)letter * LETTER_SLOT_LOCK_STEP_MS;
      if (p < lockAt) return (hash16(letter, (uint16_t)(p / LETTER_SLOT_REEL_MS)) & 1) ? 255 : 30;
      return p < LETTER_SLOT_HOLD_END_MS ? 230 : 0;
    }
    case MODE_TWIN_CHASE: {
      // Two dim letters run through all the letters, one forwards and one backwards.
      const uint8_t step = (uint8_t)((t / LETTER_TWIN_STEP_MS) % LETTER_COUNT);
      return (letter == step || letter == LETTER_COUNT - 1 - step) ? 30 : 230;
    }
    case MODE_MARQUEE: {
      // Marquee lights: three letters lit, one dark, moving along the letters.
      const uint8_t step = (uint8_t)((t / LETTER_MARQUEE_STEP_MS) % 4);
      return ((letter + step) % 4) != 0 ? 230 : 40;
    }
    default: break;
  }
  switch (activeLetters) {
    case MODE_WAVE: {
      // A bright crest travels from the first to the last letter.
      const uint8_t cycle = cycleOf(t, LETTER_WAVE_MS);
      const uint8_t v = sineTable[(uint8_t)(cycle - letter * (256 / LETTER_COUNT))];
      const uint16_t sharp = ((uint16_t)v * v) / 255;  // squared: narrower crest
      return (uint8_t)(30 + (sharp * 225UL) / 255);
    }
    case MODE_TYPEWRITER: {
      // Steps 0..COUNT-1 switch the letters on, then 4 steps hold, then 2 steps off.
      const uint16_t step = (t / LETTER_TYPE_STEP_MS) % (LETTER_COUNT + 6);
      if (step < LETTER_COUNT) return letter < step ? 210 : (letter == step ? 255 : 0);
      if (step < LETTER_COUNT + 4) return 210;
      return 0;
    }
    case MODE_IGNITE: {
      // Each letter starts up later than the previous one; while starting it
      // stutters on and off, then it burns steadily until everything goes out.
      const unsigned long p = t % IGNITE_PERIOD_MS;
      const unsigned long startAt = IGNITE_FIRST_MS + (unsigned long)letter * LETTER_IGNITE_STEP_MS;
      if (p < startAt) return 0;
      if (p < startAt + IGNITE_FLICKER_MS) return random(100) < 55 ? 255 : 0;
      if (p < IGNITE_HOLD_END_MS) return 230;
      return 0;
    }
    case MODE_WORD_BLINK:
      if (letter < FIRST_WORD_LETTERS) return 210;  // first word steady
      return (t % LETTER_BLINK_PERIOD_MS) < LETTER_BLINK_ON_MS ? 255 : 25;
    case MODE_ALTERNATE: {
      // Even letters rise while odd letters fall.
      const uint8_t cycle = cycleOf(t, LETTER_ALTERNATE_MS);
      const uint8_t breath = sineTable[(uint8_t)(cycle + ((letter & 1) ? 128 : 0))];
      return (uint8_t)(40 + ((uint16_t)breath * 215) / 255);
    }
    default: {  // MODE_BREATHE
      const uint8_t cycle = cycleOf(t, LETTER_BREATH_MS);
      const uint8_t breath = sineTable[(uint8_t)(cycle + letter * 24)];
      return (uint8_t)(70 + ((uint16_t)breath * 185) / 255);  // 70..255
    }
  }
}

uint8_t letterLevel(uint8_t letter) {
  if (casinoClosed && letter >= FIRST_WORD_LETTERS) return 0;  // OPEN is dark while the casino is closed
  const uint8_t level = baseLetterLevel(letter, letterClockMs - letterModeStart);
  if (letter == flickerLetter && !flickerLit && level > 6) return 6;  // almost dark
  return level;
}

void drawLetters() {
  for (uint8_t letter = 0; letter < LETTER_COUNT; letter++) {
    const uint8_t level = letterLevel(letter);
    const uint32_t color = shapeColor(LED_TYPE_LETTERS, letters.Color(level, level / 12, 0));  // deep red, slightly warm
    letters.setPixelColor(letterPixel(letter), color);
  }
}

// ============================== CLOSED LIGHT ===============================
// The 11th Grove stick (10 white LEDs) behind the plate light the "CLOSED" hole text while the casino is closed.
// They behave like a faulty fluorescent tube:
//   startup  the tube tries to start: quick flashes, more and more of them stay on
//   steady   full white for a random time
//   flicker  a burst of irregular flicker (full, dim and dark phases), then steady again
// The timing runs on the pattern clock of the letters (letterClockMs), so the letters speed (menu) speeds it up.
// While the casino is open the light is off.

constexpr uint8_t CL_OFF = 0;
constexpr uint8_t CL_STARTUP = 1;
constexpr uint8_t CL_STEADY = 2;
constexpr uint8_t CL_FLICKER = 3;

uint8_t closedState = CL_OFF;
unsigned long closedStateStart = 0;    // pattern clock when the state began
unsigned long closedStateLength = 0;   // how long the state lasts (pattern ms)
unsigned long closedHoldUntil = 0;     // pattern clock until the current brightness is held
uint8_t closedLevel = 0;               // brightness of the 10 LEDs

void enterClosedState(uint8_t state, unsigned long length) {
  closedState = state;
  closedStateStart = letterClockMs;
  closedStateLength = length;
  closedHoldUntil = letterClockMs;  // pick a new brightness right away
}

void setCasinoClosed(bool closed) {
  casinoClosed = closed;
  if (closed) {
    enterClosedState(CL_STARTUP, CLOSED_STARTUP_MS);
  } else {
    closedState = CL_OFF;
    closedLevel = 0;
  }
  Serial.println(closed ? F("Casino CLOSED") : F("Casino OPEN"));
}

// Level of the "casino closed" switch: true when the casino is closed.
bool closedFromSwitch() {
  bool closed = (inputStable[INPUT_CLOSED] == LOW) == CASINO_CLOSED_WHEN_LOW;
  if (menuPos(P_casinoReverse) == 1) closed = !closed;   // the "Casino reverse" menu flag
  return closed;
}

void updateClosedLight() {
  if (closedState != CL_OFF) {
    if (letterClockMs - closedStateStart >= closedStateLength) {
      if (closedState == CL_STEADY) {
        enterClosedState(CL_FLICKER, (unsigned long)random((long)CLOSED_FLICKER_MIN_MS, (long)CLOSED_FLICKER_MAX_MS + 1));
      } else {  // after the startup or after a flicker burst
        enterClosedState(CL_STEADY, (unsigned long)random((long)CLOSED_STEADY_MIN_MS, (long)CLOSED_STEADY_MAX_MS + 1));
      }
    }
    const unsigned long elapsed = letterClockMs - closedStateStart;
    const bool newLevel = (long)(letterClockMs - closedHoldUntil) >= 0;
    if (closedState == CL_STEADY) {
      closedLevel = 255;
    } else if (closedState == CL_STARTUP && newLevel) {
      const long chance = 20 + (long)(75UL * elapsed / closedStateLength);  // 20 % rising to 95 %
      closedLevel = random(100) < chance ? 255 : 0;
      closedHoldUntil = letterClockMs + (unsigned long)random((long)CLOSED_HOLD_MIN_MS, (long)CLOSED_HOLD_MAX_MS + 1);
    } else if (closedState == CL_FLICKER && newLevel) {
      const long r = random(100);
      closedLevel = r < 40 ? 255 : (r < 65 ? 0 : (uint8_t)random(40, 170));
      closedHoldUntil = letterClockMs + (unsigned long)random((long)CLOSED_HOLD_MIN_MS, (long)CLOSED_HOLD_MAX_MS + 1);
    }
  }
  const uint32_t white = shapeColor(LED_TYPE_STICK, closedLight.Color(closedLevel, closedLevel, closedLevel));  // always white
  for (uint16_t i = 0; i < CLOSED_LIGHT_LEDS; i++) {
    closedLight.setPixelColor(i, white);
  }
  closedLight.show();
}

void updateLetters(unsigned long now) {
  updateFlickerState(now);
  advanceLetterClock(now);
  drawLetters();
  letters.show();
  updateClosedLight();
}

// ================================= ARROW ===================================
// The arrow is one flowing light. Every LED gets a "flow coordinate" s that says how far along
// the path the light has travelled (for the direction "points right"):
//
//   tail cap (L-CAP)   0 .. TAIL_HALF-1      the middle 6 LEDs, from the middle of the cap out to the rail LEDs
//   rails              TAIL_HALF .. +20      left rail stick, then right rail stick, toward the head
//   head cap (R-CAP)   HEAD_START .. +3      the 3 LEDs at each end, from the rail LED outward (middle 4 dark)
//   wing (R-WING-*)    WING_START .. +10     from the cap side out to the tip
//
// The arrow is two routes, an upper and a lower one, 36 LEDs each, bent into the shape of the arrow.
// Both carry a slowly moving colour wave (with different phases for a layered look), and a bright pulse
// runs from the tail to the wing tip first on the upper route and then on the lower route, taking turns.
// "Points left" is the mirror image (tail cap R-CAP, the rails run right to left, the left cap and the left
// wings carry the head). The wings of the other head stay dark. The direction comes from the slide switch.
//
// LED 1 (index 0) of every stick is its DIN end (see STICK_* above), so the index order is:
//   caps    top to bottom        left rails   left to right      right rails   right to left
//   wings   from the tip to the cap   (the black dots of the drawings: left half dots point left, right half
//   dots point right, the caps point up; see STICK_BALL_AT_DIN_END)
// The caps are symmetric top to bottom, so the end of a cap does not matter.

// Palette stops as 0xRRGGBB: blue, cyan, violet, ice white.
const uint32_t PALETTE[4] = {0x143CFF, 0x00DCFF, 0x9628FF, 0xC8DCFF};

bool arrowDarkDirty = true;  // the unlit sticks must be cleared and shown
uint16_t arrowTick = 0;

// Colour for t = 0..255, cycling through the palette. Returns 0xRRGGBB.
uint32_t paletteAt(uint8_t t) {
  const uint8_t segment = t >> 6;              // 4 segments of 64 steps
  const uint8_t f = (uint8_t)((t & 63) << 2);  // 0..252 inside the segment
  const uint32_t a = PALETTE[segment];
  const uint32_t b = PALETTE[(segment + 1) & 3];
  const uint8_t r = mixChannel((a >> 16) & 0xFF, (b >> 16) & 0xFF, f);
  const uint8_t g = mixChannel((a >> 8) & 0xFF, (b >> 8) & 0xFF, f);
  const uint8_t bl = mixChannel(a & 0xFF, b & 0xFF, f);
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
}

// Colour of the pixel at flow coordinate s on the given lane (false = top).
uint32_t arrowPixel(Adafruit_NeoPixel &strip, uint16_t s, bool bottomLane) {
  const uint8_t wavePhase = (uint8_t)(arrowTick * 3 + (bottomLane ? 85 : 0));
  const uint8_t t = (uint8_t)(((uint32_t)s * 256UL / ARROW_WAVE_LEDS) - wavePhase);
  const uint32_t base = paletteAt(t);
  uint8_t r = (base >> 16) & 0xFF;
  uint8_t g = (base >> 8) & 0xFF;
  uint8_t b = base & 0xFF;

  // Slow shimmer: brightness ripples along the strip.
  const uint8_t shimmer = sineTable[(uint8_t)(s * 21 + arrowTick * 5)];
  const uint16_t gain = 150 + ((uint16_t)shimmer * 105) / 255;  // 150..255
  r = (uint8_t)(((uint16_t)r * gain) / 255);
  g = (uint8_t)(((uint16_t)g * gain) / 255);
  b = (uint8_t)(((uint16_t)b * gain) / 255);

  // White pulse with a tail (head 255, then 200, 150, 100, 55): the upper route is first, the lower route
  // starts when the upper pulse reaches the tip, so exactly one pulse head is on the arrow at a time.
  const uint16_t pulse = (arrowTick + (bottomLane ? ARROW_PULSE_TURN : 0)) % ARROW_PULSE_PERIOD;
  if (s <= pulse && pulse - s < ARROW_PULSE_TAIL) {
    static const uint8_t PULSE_STRENGTH[ARROW_PULSE_TAIL] = {255, 200, 150, 100, 55};
    const uint8_t strength = PULSE_STRENGTH[pulse - s];
    r = mixChannel(r, 255, strength);
    g = mixChannel(g, 255, strength);
    b = mixChannel(b, 255, strength);
  }
  return strip.Color(r, g, b);
}

constexpr uint16_t NO_FLOW = 0xFFFF;  // the LED (stick) is dark in the current direction

// Flow coordinate of LED `led` (0 = DIN end) of stick number `stick` for the current arrow direction,
// or NO_FLOW when that stick stays dark.
uint16_t stickFlow(uint8_t stick, uint8_t led) {
  const uint16_t last = LEDS_PER_STICK - 1;
  if (stick == STICK_L_CAP || stick == STICK_R_CAP) {
    const bool tailCap = (stick == STICK_L_CAP) == arrowPointsRight;
    const uint16_t half = LEDS_PER_STICK / 2;
    if (tailCap) {
      // The light starts in the middle of the cap and runs out to the rail LEDs; the ends stay dark.
      const uint16_t fromMiddle = led >= half ? (led - half) : (half - 1 - led);  // 0 at the two middle LEDs
      return fromMiddle < ARROW_TAIL_HALF ? fromMiddle : NO_FLOW;
    }
    // Head cap: from the rail LED outward to the cap ends (the rail LED is the third from each end).
    if (led < ARROW_HEAD_CAP_LIT) return ARROW_HEAD_START + (ARROW_HEAD_CAP_LIT - 1 - led);
    if (led > last - ARROW_HEAD_CAP_LIT) return ARROW_HEAD_START + (led - (last - ARROW_HEAD_CAP_LIT + 1));
    return NO_FLOW;
  }
  if (stick == STICK_RAIL_UP_L || stick == STICK_RAIL_DN_L || stick == STICK_RAIL_UP_R || stick == STICK_RAIL_DN_R) {
    const bool leftStick = (stick == STICK_RAIL_UP_L || stick == STICK_RAIL_DN_L);
    // The stick next to the tail cap comes first on the path. LED 0 (the ball end) of a left rail stick is at
    // its left end and of a right rail stick at its right end, so on the first stick the light runs from LED 0
    // and on the second stick it runs towards LED 0.
    const bool first = (leftStick == arrowPointsRight);
    return ARROW_TAIL_HALF + (first ? 0 : LEDS_PER_STICK) + (first ? led : last - led);
  }
  // Wings: only the two wings of the head the arrow points at are lit; the light runs to the tip.
  const bool rightWing = (stick == STICK_R_WING_UP || stick == STICK_R_WING_DN);
  if (rightWing != arrowPointsRight) return NO_FLOW;
  return ARROW_WING_START + (last - led);  // LED 0 (the ball end) of a wing is at its tip, the light runs to the tip
}

// True for the sticks that have at least one lit LED in the current direction (both caps and all rails
// always do; of the wings only the pair at the head).
bool stickActive(uint8_t stick) {
  if (stick == STICK_L_WING_UP || stick == STICK_L_WING_DN) return !arrowPointsRight;
  if (stick == STICK_R_WING_UP || stick == STICK_R_WING_DN) return arrowPointsRight;
  return true;
}

// True for the LEDs of the lower lane (they run half a colour cycle behind the upper lane).
bool stickLowerLane(uint8_t stick, uint8_t led) {
  if (stick == STICK_L_CAP || stick == STICK_R_CAP) return led >= LEDS_PER_STICK / 2;
  return stick == STICK_L_WING_DN || stick == STICK_RAIL_DN_L || stick == STICK_RAIL_DN_R || stick == STICK_R_WING_DN;
}

void drawArrow() {
  for (uint8_t k = 0; k < STICK_COUNT; k++) {
    for (uint8_t led = 0; led < LEDS_PER_STICK; led++) {
      const uint16_t s = stickFlow(k, led);
      // `led` counts from the ball (the black dot of the drawings); the physical index follows STICK_BALL_AT_DIN_END.
      const uint8_t physical = STICK_BALL_AT_DIN_END ? led : (uint8_t)(LEDS_PER_STICK - 1 - led);
      sticks[k]->setPixelColor(physical, s == NO_FLOW ? 0 : shapeColor(LED_TYPE_STICK, arrowPixel(*sticks[k], s, stickLowerLane(k, led))));
    }
  }
}

// The menu property "Arrow": 0 = Left, 1 = Right as seen from BEHIND the sign (where the LCD is), so an arrow that
// points to the customer's right is position 0 (the status page then shows "<- Customer right").
uint16_t arrowPosition() {
  return arrowPointsRight ? 0 : 1;
}

// True when the arrow points to the customer's right: the debounced switch level, swapped by the "Arrow reverse" menu flag.
bool arrowRightFromSwitch() {
  bool pointsLeft = (inputStable[INPUT_ARROW] == LOW) == ARROW_POINTS_LEFT_WHEN_LOW;
  if (menuPos(P_arrowReverse) == 1) pointsLeft = !pointsLeft;
  return !pointsLeft;
}

// Sets the arrow direction from the debounced switch level.
void applyArrowSwitch() {
  const bool pointsRight = arrowRightFromSwitch();
  if (pointsRight != arrowPointsRight) {  // direction changes
    arrowPointsRight = pointsRight;
    arrowDarkDirty = true;
    frameStep = 0;  // the frame light restarts from its new start corner
    Serial.println(arrowPointsRight ? F("Arrow points to customer right") : F("Arrow points to customer left"));
  }
}

void updateArrow() {
  drawArrow();
  for (uint8_t k = 0; k < STICK_COUNT; k++) {
    if (stickActive(k)) sticks[k]->show();
  }
  if (arrowDarkDirty) {  // only touch the unlit sticks when the direction changed
    for (uint8_t k = 0; k < STICK_COUNT; k++) {
      if (!stickActive(k)) {
        sticks[k]->clear();
        sticks[k]->show();
      }
    }
    arrowDarkDirty = false;
  }

  arrowTick++;
}

// ============================ POWER-ON LED TEST ============================
// For STARTUP_TEST_MS after power-up every strip runs a short window of lit LEDs from its first
// to its last LED, all strips at the same time and all finishing together (a short strip moves
// slower than a long one). The test has STARTUP_TEST_PASSES passes: pass 1 is white, the others
// light the LEDs red, green, blue, red, ... (by LED number) so a wrong colour order shows.
// At most STRIP_COUNT x STARTUP_TEST_WINDOW_LEDS LEDs are lit at a time, which keeps the current low.
// The LCD shows the time and a progress bar (see the LCD DISPLAY section). The push button D33
// skips the test (only a new press counts, so a button held during power-up does not skip it).
// The other inputs are read again when the test ends, so they take effect only after it.

bool startupTestActive = false;
unsigned long startupTestStart = 0;
uint32_t startupLast[STRIP_COUNT];           // last drawn (pass, window position) of each strip

// The normal start of the sign: read the switches and start the patterns.
void finishStartup(unsigned long now, bool skipped) {
  if (startupTestActive) {
    startupTestActive = false;
    for (uint8_t k = 0; k < STRIP_COUNT; k++) {
      allStrips[k]->clear();
      allStrips[k]->show();
    }
    Serial.println(skipped ? F("LED test skipped") : F("LED test done"));
  }
  initInput(INPUT_ARROW, PIN_ARROW_SWITCH);
  initInput(INPUT_CLOSED, PIN_CASINO_CLOSED_SWITCH);
  initInput(INPUT_OK, PIN_OK);
  initInput(INPUT_BACK, PIN_BACK);
  initInput(INPUT_SHIFT, PIN_SHIFT);
  initHexSwitch(SLOT_MENU, PIN_MENU_SWITCH);
  applySpeeds();
  lastLetterClockTick = now;

  arrowPointsRight = arrowRightFromSwitch();
  arrowDarkDirty = true;
  menuSet(P_arrow, arrowPosition());

  applyFrameEffect(now, true);
  applyLettersEffect(now, true);
  setCasinoClosed(closedFromSwitch());
  menuSet(P_casino, casinoClosed ? 0 : 1);
  nextFlickerAt = now + FLICKER_MIN_GAP_MS;
  menuLastInput = now;
  menuSince = now;
  lcdRebuild = true;
}

void beginStartupTest(unsigned long now) {
  startupTestActive = true;
  startupTestStart = now;
  for (uint8_t k = 0; k < STRIP_COUNT; k++) startupLast[k] = 0xFFFFFFFFUL;
  Serial.println(F("Power-on LED test (press D33 to skip)"));
}

// Draws one step of the test; ends the test when the time is up.
void updateStartupTest(unsigned long now) {
  const unsigned long elapsed = now - startupTestStart;
  if (elapsed >= STARTUP_TEST_MS) {
    finishStartup(now, false);
    return;
  }
  const unsigned long passMs = STARTUP_TEST_MS / STARTUP_TEST_PASSES;
  uint8_t pass = (uint8_t)(elapsed / passMs);
  if (pass >= STARTUP_TEST_PASSES) pass = STARTUP_TEST_PASSES - 1;
  const unsigned long inPass = elapsed - (unsigned long)pass * passMs;
  for (uint8_t k = 0; k < STRIP_COUNT; k++) {
    Adafruit_NeoPixel &strip = *allStrips[k];
    const uint16_t n = strip.numPixels();
    const uint16_t w = n < STARTUP_TEST_WINDOW_LEDS ? n : STARTUP_TEST_WINDOW_LEDS;
    // The window's front moves from 0 to n + w, so every LED is lit and then dark again.
    const uint16_t front = (uint16_t)((uint32_t)inPass * (n + w) / passMs);
    const uint32_t key = ((uint32_t)pass << 16) | front;
    if (key == startupLast[k]) continue;  // nothing moved: no need to send the strip again
    startupLast[k] = key;
    for (uint16_t i = 0; i < n; i++) {
      uint32_t color = 0;
      if (i < front && i + w >= front) {
        if (pass == 0) color = strip.Color(255, 255, 255);
        else if (i % 3 == 0) color = strip.Color(255, 0, 0);
        else if (i % 3 == 1) color = strip.Color(0, 255, 0);
        else color = strip.Color(0, 0, 255);
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
  }
}

// ============================== LCD DISPLAY ================================
// A 2x16 HD44780 character LCD behind a PCF8574 I2C backpack, driven with the Wire library
// only (no extra library). Standard backpack wiring: P0 = RS, P1 = RW, P2 = E, P3 = backlight,
// P4..P7 = D4..D7. The text is built every 20 ms and only the characters that changed are sent,
// LCD_CHARS_PER_LOOP at a time, so a screen update never blocks the LED animations.
//
// The display shows the status page (row 0 "#<program> Casino Open|Closed", row 1 arrow symbol and customer direction;
// in program F row 1 alternates with "Change in M:SS") or the settings menu (OK opens it). The rows are built by the menu
// engine, see tools/menu_engine/engine.inc.
// If no display answers at start-up, the display is simply left out and the sign works as usual.

constexpr uint8_t LCD_BIT_RS = 0x01;
constexpr uint8_t LCD_BIT_E = 0x04;
constexpr uint8_t LCD_BIT_BACKLIGHT = 0x08;

bool lcdOk = false;                           // a display answered during start-up
char lcdWanted[LCD_ROWS][LCD_COLS + 1];       // the text that should be on screen
char lcdShown[LCD_ROWS][LCD_COLS];            // what the display shows now
unsigned long lcdLastBuild = 0;
unsigned long lcdLastTry = 0;                 // when the display was last (re)initialised
constexpr unsigned long LCD_RETRY_MS = 1000;  // a lost display is initialised again this often

// Adds one byte (two nibbles, each with an E pulse) to the open Wire transmission.
void lcdQueue(uint8_t value, uint8_t rs) {
  const uint8_t hi = (uint8_t)((value & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  const uint8_t lo = (uint8_t)(((value << 4) & 0xF0) | LCD_BIT_BACKLIGHT | rs);
  Wire.write((uint8_t)(hi | LCD_BIT_E));
  Wire.write(hi);
  Wire.write((uint8_t)(lo | LCD_BIT_E));
  Wire.write(lo);
}

// Sends a single nibble (only used while the display is being switched to 4-bit mode).
bool lcdNibble(uint8_t nibble) {
  const uint8_t b = (uint8_t)((nibble << 4) | LCD_BIT_BACKLIGHT);
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  Wire.write((uint8_t)(b | LCD_BIT_E));
  Wire.write(b);
  return Wire.endTransmission() == 0;
}

bool lcdCommand(uint8_t command) {
  Wire.beginTransmission(LCD_I2C_ADDRESS);
  lcdQueue(command, 0);
  return Wire.endTransmission() == 0;
}

// Loads the padlock into custom character 1 (shown in front of the value of a read-only property).
// Custom character 0 is not used: code 0 would end a C string.
void lcdLoadLock() {
  static const uint8_t lock[8] = {0x0E, 0x11, 0x11, 0x1F, 0x1B, 0x1B, 0x1F, 0x00};
  lcdCommand(0x48);                         // CGRAM address of character 1
  for (uint8_t k = 0; k < 8; k++) {
    Wire.beginTransmission(LCD_I2C_ADDRESS);
    lcdQueue(lock[k], LCD_BIT_RS);
    Wire.endTransmission();
  }
  lcdCommand(0x80);                         // back to the display memory
}

// Boot-time I2C scan: prints every device that acknowledges its address (an empty write, so the display
// is not disturbed) and tells whether the LCD address answered. Returns the number of devices found.
uint8_t scanI2cBus() {
  Serial.println(F("I2C scan (0x08-0x77):"));
  uint8_t found = 0;
  bool lcdFound = false;
  for (uint8_t address = 0x08; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) continue;
    found++;
    if (address == LCD_I2C_ADDRESS) lcdFound = true;
    Serial.print(F("  found 0x"));
    if (address < 16) Serial.print(F("0"));
    Serial.print((int)address, HEX);
    Serial.println(address == LCD_I2C_ADDRESS ? F("  (LCD address in this sketch)") : F(""));
  }
  Serial.print(F("I2C scan done: "));
  Serial.print((int)found);
  Serial.println(F(" device(s)"));
  if (!lcdFound) {
    Serial.print(F("No device on the LCD address 0x"));
    Serial.print((int)LCD_I2C_ADDRESS, HEX);
    Serial.println(F(": check SDA D20 / SCL D21; if the scan lists another address, set LCD_I2C_ADDRESS to it."));
  }
  return found;
}

// The start-up sequence of the HD44780 (4-bit mode). It runs once in setup(), so the short
// delays here do not affect the animations.
void lcdInit() {
  delay(50);
  if (!lcdNibble(0x3)) return;  // nobody answers: leave the display out
  delay(5);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x3);
  delay(1);
  lcdNibble(0x2);               // switch to 4-bit mode
  delay(1);
  lcdCommand(0x28);             // 4-bit, two lines, 5x8 font
  lcdCommand(0x0C);             // display on, no cursor
  lcdCommand(0x06);             // cursor moves right
  lcdCommand(0x01);             // clear
  delay(3);
  lcdLoadLock();
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) lcdShown[row][col] = ' ';
  }
  lcdOk = true;
}

// Copies text into a row of lcdWanted and pads it with spaces.
void lcdPut(uint8_t row, const char *text) {
  uint8_t col = 0;
  while (col < LCD_COLS && text[col] != 0) {
    lcdWanted[row][col] = text[col];
    col++;
  }
  while (col < LCD_COLS) lcdWanted[row][col++] = ' ';
  lcdWanted[row][LCD_COLS] = 0;
}

// Builds the two text rows (status page or menu) with the menu engine.
void lcdBuildText(unsigned long now) {
  char row0[LCD_COLS + 1], row1[LCD_COLS + 1];
  menuRows(now, row0, row1);
  lcdPut(0, row0);
  lcdPut(1, row1);
}

// The LCD during the power-on test: one row scrolls "press D33 to skip", the other shows the
// elapsed seconds in the middle of a progress bar. The rows swap places every LCD_TEST_SWAP_MS,
// so every character gets shown on both rows.
void lcdBuildTestText(unsigned long now) {
  unsigned long elapsed = now - startupTestStart;
  if (elapsed > STARTUP_TEST_MS) elapsed = STARTUP_TEST_MS;
  const uint8_t counterRow = (uint8_t)((elapsed / LCD_TEST_SWAP_MS) % 2) == 0 ? 1 : 0;
  const uint8_t skipRow = 1 - counterRow;

  // Scrolling text: the text and three spaces repeat around the row.
  const char marquee[] = "press D33 to skip   ";
  const uint8_t marqueeLen = sizeof(marquee) - 1;
  const uint8_t shift = (uint8_t)((elapsed / LCD_TEST_SCROLL_MS) % marqueeLen);
  char line[LCD_COLS + 1];
  for (uint8_t c = 0; c < LCD_COLS; c++) line[c] = marquee[(shift + c) % marqueeLen];
  line[LCD_COLS] = 0;
  lcdPut(skipRow, line);

  // Seconds in the middle of the progress bar (the text covers the bar where they overlap).
  char label[12];
  snprintf(label, sizeof(label), "%u/%u s", (unsigned int)(elapsed / 1000UL), (unsigned int)(STARTUP_TEST_MS / 1000UL));
  const uint8_t labelLen = (uint8_t)strlen(label);
  const uint8_t labelStart = (LCD_COLS - labelLen) / 2;
  const uint8_t filled = (uint8_t)((elapsed * LCD_COLS) / STARTUP_TEST_MS);
  for (uint8_t c = 0; c < LCD_COLS; c++) {
    if (c >= labelStart && c < labelStart + labelLen) line[c] = label[c - labelStart];
    else line[c] = c < filled ? '#' : '.';
  }
  lcdPut(counterRow, line);
}

// Sends the next few characters that differ from what the display shows.
void lcdSendChanges() {
  for (uint8_t row = 0; row < LCD_ROWS; row++) {
    for (uint8_t col = 0; col < LCD_COLS; col++) {
      if (lcdWanted[row][col] == lcdShown[row][col]) continue;
      Wire.beginTransmission(LCD_I2C_ADDRESS);
      lcdQueue((uint8_t)(0x80 | (row ? 0x40 : 0x00) | col), 0);  // set the cursor
      uint8_t sent = 0;
      while (col < LCD_COLS && sent < LCD_CHARS_PER_LOOP) {
        lcdQueue((uint8_t)lcdWanted[row][col], LCD_BIT_RS);
        lcdShown[row][col] = lcdWanted[row][col];
        col++;
        sent++;
      }
      if (Wire.endTransmission() != 0) { lcdOk = false; return; }  // the display stopped answering: retried by updateLcd()
      return;  // the rest follows on the next pass
    }
  }
}

void updateLcd(unsigned long now) {
  if (!lcdOk) {  // the display did not answer (or stopped answering): try again every LCD_RETRY_MS
    if (now - lcdLastTry >= LCD_RETRY_MS) {
      lcdLastTry = now;
      lcdInit();
    }
    return;
  }
  if (startupTestActive) {  // the power-on test has its own screen
    lcdBuildTestText(now);
    lcdSendChanges();
    return;
  }
  if (lcdRebuild || now - lcdLastBuild >= 20) {
    lcdRebuild = false;
    lcdLastBuild = now;
    lcdBuildText(now);
  }
  lcdSendChanges();
}

// ============================== SIGN OF LIFE ===============================
// The debug LED on pin 13 blinks while the program runs; it is shortly inverted on every state change
// and blinks faster while many changes happen. Once per second a "DBG" line goes to the Serial Monitor.

unsigned long lastBeat = 0;
bool beatOn = false;
unsigned long beatCount = 0;      // LED toggles since power-up
unsigned long lastDebug = 0;
unsigned long loopCount = 0;      // loop() passes since the last debug line
unsigned long lastEventAt = 0;    // when the last state change happened
bool anyEvent = false;
unsigned int eventsNow = 0;       // state changes in the current second
unsigned int eventsPrev = 0;      // state changes in the second before
unsigned long eventWindowStart = 0;
unsigned long eventsShown = 0;    // total state changes since power-up

// Records a state change (arrow direction, letter mode, casino open/closed, menu key or switch).
void noteEvent(unsigned long now) {
  anyEvent = true;
  lastEventAt = now;
  eventsNow++;
  eventsShown++;
}

// Prints the once-per-second debug line (uptime, loops, beats and tick change every time).
void printDebug(unsigned long now) {
  Serial.print(F("DBG up="));
  Serial.print(now / 1000UL);
  Serial.print(F("s loops="));
  Serial.print(loopCount);
  Serial.print(F(" fx="));
  Serial.print((unsigned int)activeFrame + 1);
  Serial.print('/');
  Serial.print((unsigned int)activeLetters + 1);
  Serial.print(F(" arrow="));
  Serial.print(arrowPointsRight ? F("R") : F("L"));
  Serial.print(F(" closed="));
  Serial.print(casinoClosed ? 1 : 0);
  Serial.print(F(" menu="));
  Serial.print(menuMode == MM_MENU ? 'M' : menuMode == MM_EDIT ? 'E' : menuMode == MM_CONFIRM ? 'C' : menuMode == MM_VIEW ? 'V' : 'S');
  Serial.print(F(" events="));
  Serial.print(eventsShown);
  Serial.print(F(" beats="));
  Serial.print(beatCount);
  Serial.print(F(" tick="));
  Serial.println((unsigned int)arrowTick);
}

// Called on every loop() pass, also during the power-on test.
void signOfLife(unsigned long now) {
  loopCount++;
  if (now - eventWindowStart >= 1000) {  // sliding one-second window of state changes
    eventsPrev = eventsNow;
    eventsNow = 0;
    eventWindowStart = now;
  }
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebug(now);
    loopCount = 0;
  }
  const bool busy = (eventsNow + eventsPrev >= 2);
  const unsigned long beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
  if (now - lastBeat >= beatMs) {  // the LED blinks all the time, faster when busy
    lastBeat = now;
    beatOn = !beatOn;
    beatCount++;
  }
  const bool inverted = anyEvent && (now - lastEventAt < INVERT_MS);  // shortly inverted after a state change
  digitalWrite(PIN_STATUS_LED, (beatOn != inverted) ? HIGH : LOW);
}

// ============================ MENU CALLBACKS ===============================
// The menu engine calls this after a property value changed (in the menu, by a quick-pick or Restore defaults).
void menuCallback(uint8_t cb, uint8_t prop) {
  if (cb == CB_applySpeeds) {
    applySpeeds();
  } else if (cb == CB_rebuildStripCurve) {
    rebuildCurve(LED_TYPE_STRIP, P_stripMin, P_stripMax, P_stripExp, P_stripMix);
  } else if (cb == CB_rebuildLettersCurve) {
    rebuildCurve(LED_TYPE_LETTERS, P_lettersMin, P_lettersMax, P_lettersExp, P_lettersMix);
  } else if (cb == CB_rebuildStickCurve) {
    rebuildCurve(LED_TYPE_STICK, P_stickMin, P_stickMax, P_stickExp, P_stickMix);
  } else if (cb == CB_effectFrameChanged) {
    applyFrameEffect(millis(), false);
  } else if (cb == CB_effectLettersChanged) {
    applyLettersEffect(millis(), false);
  } else if (cb == CB_applySwitches) {          // a reverse flag changed: the switch values follow at once
    applyArrowSwitch();
    menuSet(P_arrow, arrowPosition());
    const bool closed = closedFromSwitch();
    if (closed != casinoClosed) setCasinoClosed(closed);
    menuSet(P_casino, casinoClosed ? 0 : 1);
  }
}

// ============================== SETUP / LOOP ===============================

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  randomSeed(analogRead(A15));  // a floating pin that is not used for anything else

  for (uint16_t i = 0; i < 256; i++) {
    sineTable[i] = (uint8_t)(128.0f + 127.0f * sin(i * 6.2831853f / 256.0f) + 0.5f);
  }

  buildDefaultBrightnessCurves();
  const bool menuFromEeprom = menuBegin();   // saved menu values (speeds, curves, reverse flags) or the defaults
  rebuildAllCurves();

  for (uint8_t k = 0; k < STRIP_COUNT; k++) {
    allStrips[k]->begin();
    allStrips[k]->clear();
    allStrips[k]->show();
  }

  initInput(INPUT_SKIP, PIN_SKIP_BUTTON);

  Wire.begin();
  Wire.setClock(400000UL);
  scanI2cBus();
  lcdInit();

  Serial.println(F("Casino Open Sign V2: see the top of sketch.ino."));
  Serial.print(F("Serial speed "));
  Serial.print(SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));
  Serial.println(menuFromEeprom ? F("EEPROM: saved menu values loaded.") : F("EEPROM: no saved values for this menu layout, defaults used."));
  lastBeat = millis();
  lastDebug = lastBeat;
  eventWindowStart = lastBeat;

  // The other inputs and the patterns are started by finishStartup(), after the LED test.
  if (STARTUP_TEST_MS > 0) {
    beginStartupTest(millis());
  } else {
    finishStartup(millis(), false);
  }
}

void loop() {
  const unsigned long now = millis();
  signOfLife(now);
  const bool buttonChanged = pollInput(INPUT_SKIP, PIN_SKIP_BUTTON, now);

  // Power-on LED test: only the button (skip) is watched; everything else waits for the end.
  if (startupTestActive) {
    if (buttonChanged && inputStable[INPUT_SKIP] == LOW) {
      noteEvent(now);
      finishStartup(now, true);  // this press only skips the test, it does not change the mode
    } else {
      updateStartupTest(now);
    }
    updateLcd(now);
    return;
  }

  // Inputs: the slide switches set the arrow direction and casino open/closed (the menu flags may reverse them).
  // A change of one of these two closes an open menu and shows the status page.
  if (pollInput(INPUT_ARROW, PIN_ARROW_SWITCH, now)) {
    applyArrowSwitch();
    menuPhysical(P_arrow, arrowPosition(), now);
    noteEvent(now);
  }
  if (pollInput(INPUT_CLOSED, PIN_CASINO_CLOSED_SWITCH, now)) {
    setCasinoClosed(closedFromSwitch());
    menuPhysical(P_casino, casinoClosed ? 0 : 1, now);
    noteEvent(now);
  }

  // Menu controls: the menu switch moves the selection or changes the value, OK and BACK are the keys, SHIFT makes
  // the step bigger while it is held.
  const uint8_t menuFrom = hexStable[SLOT_MENU];
  if (pollHexSwitch(SLOT_MENU, PIN_MENU_SWITCH, now)) {
    noteEvent(now);
    const uint8_t delta = (uint8_t)((hexStable[SLOT_MENU] - menuFrom) & 15);
    if (delta != 8) {                           // exactly half a turn: the direction cannot be told, ignored
      int steps = delta;
      if (delta > 8) steps = (int)delta - 16;
      if (MENU_SWITCH_REVERSED) steps = -steps;
      menuTurn(steps, inputStable[INPUT_SHIFT] == LOW, now);
    }
  }
  if (pollInput(INPUT_SHIFT, PIN_SHIFT, now)) noteEvent(now);
  if (pollInput(INPUT_OK, PIN_OK, now) && inputStable[INPUT_OK] == LOW) { noteEvent(now); menuKey(MENU_KEY_OK, now); }
  if (pollInput(INPUT_BACK, PIN_BACK, now) && inputStable[INPUT_BACK] == LOW) { noteEvent(now); menuKey(MENU_KEY_BACK, now); }
  menuTick(now);
  menuChangeInSec = -1;                         // status page, a random effect: seconds until the next change
  updateRandomEffects(now);
  if (frameIsRandom() || lettersIsRandom()) {
    const unsigned long elapsed = now - autoPickedAt;
    menuChangeInSec = elapsed >= autoDelayMs ? 0 : (long)((autoDelayMs - elapsed + 999UL) / 1000UL);
  }

  // Independent, non-blocking timers. show() is synchronous, so the updates
  // still run one after another, but no animation ever waits on delay().
  if (now - lastFrameUpdate >= frameIntervalMs) {
    lastFrameUpdate = now;
    updateFrame();
  }
  if (now - lastLettersUpdate >= LETTERS_SPEED_MS) {
    lastLettersUpdate = now;
    updateLetters(now);
  }
  if (now - lastArrowUpdate >= arrowIntervalMs) {
    lastArrowUpdate = now;
    updateArrow();
  }

  updateLcd(now);
}
