/*
 * Casino sign "CASINO OPEN" - Arduino Mega 2560 + 10 WS2812B LED strips (Wokwi project)
 *
 * Physical sign:
 *   - Black rectangular plate with an LED frame around its edge (2 strips,
 *     one runs clockwise, one counter-clockwise). The light runs from the top-left
 *     to the bottom-right corner while the arrow points right, and from the
 *     top-right to the bottom-left corner while it points left.
 *   - Red plexiglass letters, "CASINO" above "OPEN", floating in front of the
 *     plate. Behind each letter sits a small group of LEDs (any number per
 *     letter, set in LETTER_LEDS). The groups are cut from the strip and joined
 *     with wire jumpers into one data chain, so no LEDs are wasted between letters.
 *   - A hole text "CLOSED" cut into the plate under the OPEN row; six white LEDs behind the
 *     plate light it up while the casino is closed (and then the OPEN letters are off).
 *   - A left/right arrow under that. Its shaft is a rectangle (top edge,
 *     bottom edge and a cap at each end) that is closed at the tail and open
 *     toward the head. A triangular head sits on each end: its base is the shaft
 *     cap plus one short strip piece above and below it. Only the head the arrow
 *     points at is lit, and only the cap at the tail end is lit.
 *
 * Strip overview (one Adafruit_NeoPixel object and one data pin each):
 *   frameCw          D6   frame, clockwise half (top edge, then right edge)
 *   frameCcw         D7   frame, counter-clockwise half (left edge, then bottom edge)
 *   letters          D8   backlight behind all letters, line 1 then line 2
 *   arrowBodyTop     D9   shaft, top edge
 *   arrowBodyBottom  D10  shaft, bottom edge
 *   arrowHeadLeft    D11  left arrowhead  (upper base piece, upper wing, lower wing, lower base piece)
 *   arrowHeadRight   D12  right arrowhead (same order)
 *   arrowCapLeft     D4   shaft, left end cap  (top to bottom)
 *   arrowCapRight    D5   shaft, right end cap (top to bottom)
 *   closedLight      D31  6 white LEDs behind the plate that light the "CLOSED" hole text
 *
 * Inputs (both use the internal pull-up resistor, the switch pulls the pin to GND):
 *   D2  slide switch  arrow direction (handle left = LOW = arrow points left)
 *   D3  push button   next letter lighting mode
 *   D30 slide switch  SW5 "casino closed" (handle on the GND side = LOW = closed): the OPEN
 *                     letters go dark and the CLOSED light turns on
 *   D22-D25  SW3  16-position hex rotary switch (0..F, pins 1, 2, 4, 8): speed of the
 *                 frame and arrow patterns
 *   D26-D29  SW4  16-position hex rotary switch (0..F, pins 1, 2, 4, 8): speed of the
 *                 letter patterns
 *   (the common pin of each hex switch goes to GND)
 *
 * Power-on LED test: for 10 s after power-up all strips run a short window of lit LEDs from the first
 * to the last LED at the same time (2 passes: white, then red-green-blue), so every LED and the
 * colour order can be checked. The LCD shows the seconds in a progress bar and a scrolling
 * "press D3 to skip" (the two rows swap places every 2.5 s). Pressing D3 skips the test.
 *
 * LCD display (I2C, SDA = D20, SCL = D21, 5 V, GND): four rotating pages with the current
 * settings (mode, SW3/SW4 position and speed, casino open/closed, arrow direction). A changed
 * setting shows its page at once and holds it for 6 s; the pages rotate every 3 s.
 *
 * Letter modes (the push button steps through them, then wraps to 0; the sign starts in mode 0):
 *   0 breathe  1 wave  2 typewriter  3 ignite  4 word blink  5 alternate
 *   6 scanner (a bright spot with a tail sweeps back and forth)  7 sparkle (random twinkle)
 *   8 chase (one dim letter runs through each word)  9 pulse (heartbeat)
 *   10 random auto: a random pattern from 0..9, changed every LETTER_RANDOM_AUTO_MS (2 minutes)
 *
 * To change strip lengths edit ONLY the CONFIGURATION section below and, for
 * Wokwi, regenerate diagram.json with: python3 tools/generate_diagram.py
 *
 * Note: the Arduino build inserts function prototypes at the top of this file,
 * so function signatures only use types that exist before the first function
 * (built-in types and Adafruit_NeoPixel). Do not add custom structs to them.
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>  // built into the Arduino IDE; used for the I2C LCD (no extra library)

// ============================== CONFIGURATION ==============================

// --- Strip lengths (the only values you need to change for other sizes) ----
// The defaults are for 60 LEDs per metre, a 50 x 45 cm sign and a 40 x 15 cm arrow.
// For other sizes run: python3 tools/size_calculator.py --help
// The frame is a ring of four edges. The CW strip covers the top edge, then the right edge; the
// CCW strip covers the left edge, then the bottom edge (both listed from the top-left corner).
// The light runs from the top-left to the bottom-right corner (arrow right) or from the
// top-right to the bottom-left corner (arrow left).
constexpr uint16_t FRAME_HALF_LEDS = 53;   // LEDs in EACH frame strip (CW and CCW are equal)
constexpr uint16_t FRAME_TOP_LEDS = 28;    // LEDs along the top and the bottom edge (the left and
                                           // right edges get FRAME_HALF_LEDS - FRAME_TOP_LEDS)

// Text: two stacked lines. Letters are counted left to right, line 1 first.
constexpr char LINE_1_TEXT[] = "CASINO";
constexpr char LINE_2_TEXT[] = "OPEN";
// LEDs behind each letter: one entry per letter of LINE_1_TEXT, then LINE_2_TEXT.
// Any number of LEDs per letter is fine (usually 4 to 8): in Wokwi each group is drawn as
// a circle of single LEDs, on the real sign the LEDs can be laid out behind the letter in
// any shape. "I" is narrow, so it gets only 4.
constexpr uint8_t LETTER_LEDS[] = {6, 6, 6, 4, 6, 6,   6, 6, 6, 6};

constexpr uint16_t ARROW_BODY_LEDS = 10;   // LEDs in EACH shaft edge strip (top and bottom)
constexpr uint16_t ARROW_CAP_LEDS = 4;     // LEDs in EACH shaft end cap (must be even)
constexpr uint16_t ARROW_HEAD_BASE_LEDS = 2;  // LEDs in each base piece of an arrowhead (above and
                                              // below the cap, so 2 pieces per head)
constexpr uint16_t ARROW_HEAD_WING_LEDS = 8;  // LEDs in each wing of an arrowhead (2 wings per head)

// The white light behind the "CLOSED" hole text: one short piece of strip, always white.
constexpr uint16_t CLOSED_LIGHT_LEDS = 6;

// --- Data pins -------------------------------------------------------------
constexpr uint8_t PIN_FRAME_CW = 6;
constexpr uint8_t PIN_FRAME_CCW = 7;
constexpr uint8_t PIN_LETTERS = 8;
constexpr uint8_t PIN_ARROW_BODY_TOP = 9;
constexpr uint8_t PIN_ARROW_BODY_BOTTOM = 10;
constexpr uint8_t PIN_ARROW_HEAD_LEFT = 11;
constexpr uint8_t PIN_ARROW_HEAD_RIGHT = 12;
constexpr uint8_t PIN_ARROW_CAP_LEFT = 4;
constexpr uint8_t PIN_ARROW_CAP_RIGHT = 5;
constexpr uint8_t PIN_CLOSED_LIGHT = 31;

// --- Input pins ------------------------------------------------------------
constexpr uint8_t PIN_ARROW_SWITCH = 2;    // slide switch: arrow direction
constexpr uint8_t PIN_MODE_BUTTON = 3;     // push button: next letter mode
constexpr bool ARROW_POINTS_LEFT_WHEN_LOW = true;  // false would reverse the switch
constexpr uint8_t PIN_CASINO_CLOSED_SWITCH = 30;   // slide switch SW5: casino closed
constexpr bool CASINO_CLOSED_WHEN_LOW = true;      // handle on the GND side = closed (false: reverse)
constexpr uint8_t DEBOUNCE_MS = 30;        // an input must stay stable this long

// Speed switches: two hex rotary switches, 4 pins each (bit values 1, 2, 4, 8) plus a
// common pin that goes to GND. The pins use the internal pull-ups.
constexpr uint8_t PIN_SPEED_FRAME_ARROW[4] = {22, 23, 24, 25};  // SW3: frame and arrow speed
constexpr uint8_t PIN_SPEED_LETTERS[4] = {26, 27, 28, 29};      // SW4: letters speed
constexpr bool SPEED_BIT_IS_LOW = true;    // a bit counts as set when its pin reads LOW (the switch
                                           // connects it to the common pin = GND). Set to false if
                                           // your switch is complement coded (reads 15 - position).

// --- LCD display: 2x16 character LCD with an I2C backpack (PCF8574), SDA = D20, SCL = D21 ---
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;  // Wokwi uses 0x27; if a real display stays blank try 0x3F
constexpr uint16_t LCD_PAGE_MS = 3000;     // time one page is shown while the pages rotate
constexpr uint16_t LCD_HOLD_MS = 6000;     // a page stays on screen this long after its setting changed
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint8_t LCD_CHARS_PER_LOOP = 4;  // characters sent per loop() pass, so animations never wait

// --- Power-on LED test -----------------------------------------------------
// At power-up every LED is lit once per pass (all strips at the same time, each with a short
// moving window) so wiring and colours can be checked. Pressing D3 skips the test.
constexpr uint32_t STARTUP_TEST_MS = 10000;      // length of the test (0 = no test)
constexpr uint8_t STARTUP_TEST_PASSES = 2;       // pass 1 is white, the other passes red-green-blue
constexpr uint8_t STARTUP_TEST_WINDOW_LEDS = 5;  // LEDs lit at the same time in each strip
constexpr uint16_t LCD_TEST_SWAP_MS = 2500;      // the two LCD rows swap places this often during the test
constexpr uint16_t LCD_TEST_SCROLL_MS = 250;     // the "press D3 to skip" text scrolls one character per step

// --- Look and timing -------------------------------------------------------
constexpr uint8_t BRIGHTNESS = 80;         // global brightness 0..255

// Speed range of the two speed switches. The multiplier grows geometrically from
// position 0 to position F; all the times below are the times at multiplier 1.0.
constexpr float SPEED_MIN_MULTIPLIER = 0.25f;  // position 0: patterns run 4 times slower
constexpr float SPEED_MAX_MULTIPLIER = 4.0f;   // position F: patterns run 4 times faster
constexpr uint8_t MIN_UPDATE_MS = 8;       // frame and arrow never update more often than this

constexpr uint16_t FRAME_SPEED_MS = 100;   // time between frame animation steps (at multiplier 1)
constexpr uint8_t FRAME_BAR_LEDS = 5;      // LEDs lit, then the same number dark, repeating

constexpr uint16_t LETTERS_SPEED_MS = 30;  // time between letter backlight updates (the letter patterns
                                           // themselves follow the speed switch SW4)
constexpr uint8_t LETTER_START_MODE = 0;   // letter mode after power-up (see MODE_* below)
constexpr uint32_t LETTER_MODE_AUTO_MS = 0;       // step to the next mode automatically this often (0 = never)
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
constexpr uint32_t LETTER_RANDOM_AUTO_MS = 120000;  // random auto: pick a new random mode this often (2 min)

constexpr uint16_t ARROW_SPEED_MS = 35;    // time between arrow animation steps (at multiplier 1)
constexpr uint16_t ARROW_WAVE_LEDS = 24;   // LEDs per full colour cycle

// CLOSED light, like a faulty fluorescent tube. Times are in pattern milliseconds, so they
// follow the letter speed switch SW4 (at multiplier 2 everything happens twice as fast).
constexpr uint16_t CLOSED_STARTUP_MS = 1200;       // switching on: the tube tries to start, then stays on
constexpr uint32_t CLOSED_STEADY_MIN_MS = 5000;    // steady light for at least this long
constexpr uint32_t CLOSED_STEADY_MAX_MS = 14000;   // ... and at most this long
constexpr uint16_t CLOSED_FLICKER_MIN_MS = 400;    // a flicker burst lasts at least this long
constexpr uint16_t CLOSED_FLICKER_MAX_MS = 1500;   // ... and at most this long
constexpr uint16_t CLOSED_HOLD_MIN_MS = 30;        // during a burst a brightness is held this long at least
constexpr uint16_t CLOSED_HOLD_MAX_MS = 140;       // ... and at most this long

// ================================ DERIVED ==================================

constexpr uint8_t LETTER_COUNT = sizeof(LETTER_LEDS) / sizeof(LETTER_LEDS[0]);

constexpr uint8_t FIRST_WORD_LETTERS = sizeof(LINE_1_TEXT) - 1;  // letters in line 1
constexpr uint16_t FRAME_SIDE_LEDS = FRAME_HALF_LEDS - FRAME_TOP_LEDS;  // left and right edge

constexpr uint16_t ARROW_CAP_HALF = ARROW_CAP_LEDS / 2;     // LEDs per cap half
constexpr uint16_t ARROW_HEAD_HALF = ARROW_HEAD_BASE_LEDS + ARROW_HEAD_WING_LEDS;  // one side of a head
constexpr uint16_t ARROW_HEAD_LEDS = 2 * ARROW_HEAD_HALF;   // LEDs in EACH arrowhead
constexpr uint16_t ARROW_HEAD_START = ARROW_CAP_HALF + ARROW_BODY_LEDS;  // flow coordinate of the head side
constexpr uint16_t ARROW_PATH_LEDS = ARROW_HEAD_START + ARROW_HEAD_HALF;
constexpr uint16_t ARROW_SPARKLE_CYCLE = ARROW_PATH_LEDS + 8;  // sparkle travel incl. pause
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
constexpr uint8_t MODE_RANDOM_AUTO = 10; // picks a random mode from 0..9 every LETTER_RANDOM_AUTO_MS
constexpr uint8_t PATTERN_MODE_COUNT = 10;  // modes 0..9 are real patterns, mode 10 chooses among them
constexpr uint8_t LETTER_MODE_COUNT = 11;

// The ignite cycle: off, letters start one by one, hold, off again.
constexpr uint32_t IGNITE_FIRST_MS = 500;
constexpr uint32_t IGNITE_FLICKER_MS = 600;
constexpr uint32_t IGNITE_HOLD_END_MS =
    IGNITE_FIRST_MS + (uint32_t)LETTER_COUNT * LETTER_IGNITE_STEP_MS + IGNITE_FLICKER_MS + 3000UL;
constexpr uint32_t IGNITE_PERIOD_MS = IGNITE_HOLD_END_MS + 1500UL;

static_assert(ARROW_HEAD_BASE_LEDS > 0 && ARROW_HEAD_WING_LEDS > 0, "arrowhead pieces need at least one LED");
static_assert(ARROW_CAP_LEDS % 2 == 0, "ARROW_CAP_LEDS must be even");
static_assert(LETTER_COUNT > 1, "At least two letters are required");
static_assert(sizeof(LINE_1_TEXT) - 1 + sizeof(LINE_2_TEXT) - 1 == LETTER_COUNT,
              "LETTER_LEDS needs one entry per letter of LINE_1_TEXT and LINE_2_TEXT");
static_assert(LETTER_START_MODE < LETTER_MODE_COUNT, "LETTER_START_MODE is out of range");
static_assert(STARTUP_TEST_PASSES > 0 && STARTUP_TEST_WINDOW_LEDS > 0, "the start-up test needs a pass and a window");
static_assert(CLOSED_LIGHT_LEDS > 0, "CLOSED_LIGHT_LEDS must be at least 1");
static_assert(SPEED_MIN_MULTIPLIER > 0.0f && SPEED_MIN_MULTIPLIER < SPEED_MAX_MULTIPLIER,
              "SPEED_MIN_MULTIPLIER must be above 0 and below SPEED_MAX_MULTIPLIER");
static_assert(FRAME_TOP_LEDS > 0 && FRAME_TOP_LEDS < FRAME_HALF_LEDS,
              "FRAME_TOP_LEDS must be between 1 and FRAME_HALF_LEDS - 1");

// ================================ STRIPS ===================================

Adafruit_NeoPixel frameCw(FRAME_HALF_LEDS, PIN_FRAME_CW, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel frameCcw(FRAME_HALF_LEDS, PIN_FRAME_CCW, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel letters(0, PIN_LETTERS, NEO_GRB + NEO_KHZ800);  // length is set in setup()
Adafruit_NeoPixel arrowBodyTop(ARROW_BODY_LEDS, PIN_ARROW_BODY_TOP, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel arrowBodyBottom(ARROW_BODY_LEDS, PIN_ARROW_BODY_BOTTOM, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel arrowHeadLeft(ARROW_HEAD_LEDS, PIN_ARROW_HEAD_LEFT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel arrowHeadRight(ARROW_HEAD_LEDS, PIN_ARROW_HEAD_RIGHT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel arrowCapLeft(ARROW_CAP_LEDS, PIN_ARROW_CAP_LEFT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel arrowCapRight(ARROW_CAP_LEDS, PIN_ARROW_CAP_RIGHT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel closedLight(CLOSED_LIGHT_LEDS, PIN_CLOSED_LIGHT, NEO_GRB + NEO_KHZ800);

constexpr uint8_t STRIP_COUNT = 10;
Adafruit_NeoPixel *const allStrips[STRIP_COUNT] = {&frameCw,         &frameCcw,      &letters,        &arrowBodyTop,
                                                   &arrowBodyBottom, &arrowHeadLeft, &arrowHeadRight, &arrowCapLeft,
                                                   &arrowCapRight,   &closedLight};

// =============================== SHARED STATE ==============================

uint8_t sineTable[256];  // 0..255, one full sine period, filled in setup()

bool arrowPointsRight = true;  // set from the slide switch; also steers the frame animation
bool casinoClosed = false;     // set from the "casino closed" switch SW5

unsigned long lastFrameUpdate = 0;
unsigned long lastLettersUpdate = 0;
unsigned long lastArrowUpdate = 0;

// ================================ HELPERS ==================================

// Linear blend between two channel values, f = 0..255.
uint8_t mixChannel(uint8_t a, uint8_t b, uint8_t f) {
  return (uint8_t)(((uint16_t)a * (255 - f) + (uint16_t)b * f) / 255);
}

// Position 0..255 inside a repeating cycle of periodMs milliseconds.
uint8_t cycleOf(unsigned long t, uint16_t periodMs) {
  return (uint8_t)(((t % periodMs) * 256UL) / periodMs);
}

// All LEDs of all letters.
uint16_t lettersTotalLeds() {
  uint16_t total = 0;
  for (uint8_t letter = 0; letter < LETTER_COUNT; letter++) {
    total += LETTER_LEDS[letter];
  }
  return total;
}

// ============================== LCD STATE ==================================
// The display shows four pages (see the LCD DISPLAY section further down). The pages rotate;
// when a setting changes, its page is shown at once and held for LCD_HOLD_MS.

constexpr uint8_t LCD_PAGE_MODE = 0;      // letter mode (and the pattern shown in random auto)
constexpr uint8_t LCD_PAGE_SW3 = 1;       // SW3 position, frame and arrow speed
constexpr uint8_t LCD_PAGE_SW4 = 2;       // SW4 position, letter speed
constexpr uint8_t LCD_PAGE_STATUS = 3;    // casino open/closed and arrow direction
constexpr uint8_t LCD_PAGE_COUNT = 4;

uint8_t lcdPage = LCD_PAGE_MODE;
unsigned long lcdPageSince = 0;           // when the current page was shown
bool lcdHolding = false;                  // true while a changed setting's page is held
unsigned long lcdHoldFrom = 0;
bool lcdRebuild = true;                   // rebuild the text at once (a page was just selected)

// Shows a page at once. hold = true: keep it for LCD_HOLD_MS (a setting was changed).
void lcdShowPage(uint8_t page, unsigned long now, bool hold) {
  lcdPage = page;
  lcdPageSince = now;
  lcdHolding = hold;
  lcdHoldFrom = now;
  lcdRebuild = true;
}

// ================================= INPUTS ==================================
// Debounced inputs with the internal pull-up: a closed switch reads LOW.

constexpr uint8_t INPUT_ARROW = 0;
constexpr uint8_t INPUT_MODE = 1;
constexpr uint8_t INPUT_CLOSED = 2;

uint8_t inputStable[3];            // debounced level (HIGH or LOW)
uint8_t inputLastRaw[3];           // last raw reading
unsigned long inputChangedAt[3];   // when the raw reading last changed

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

// ============================= SPEED SWITCHES ==============================
// Two 16-position hex rotary switches (positions 0..F, binary coded on four pins) set the
// speed of the patterns:
//   SW3  frame and arrow: sets how often the frame and the arrow animation step
//   SW4  letters:         sets how fast the letter patterns (breathing, wave, ...) run
// Position 0 is the slowest and F the fastest. The speed multiplier grows geometrically
// from SPEED_MIN_MULTIPLIER (position 0) to SPEED_MAX_MULTIPLIER (position F).
// The letter patterns run on their own clock (letterClockMs) that advances by
// real time x multiplier, so turning the switch changes the speed without a jump.

constexpr uint8_t SLOT_SPEED_MAIN = 0;
constexpr uint8_t SLOT_SPEED_LETTERS = 1;

uint8_t speedStable[2];            // debounced switch positions
uint8_t speedLastRaw[2];
unsigned long speedChangedAt[2];
uint16_t speedMult256[16];         // speed multiplier x 256 for each position
uint16_t frameIntervalMs = FRAME_SPEED_MS;
uint16_t arrowIntervalMs = ARROW_SPEED_MS;
uint16_t letterMult256 = 256;
unsigned long letterClockMs = 0;   // pattern time of the letters (milliseconds)
uint32_t letterClockFrac = 0;      // fraction of a millisecond, in 1/256 ms
unsigned long lastLetterClockTick = 0;

void buildSpeedTable() {
  for (uint8_t code = 0; code < 16; code++) {
    const float mult = SPEED_MIN_MULTIPLIER * pow(SPEED_MAX_MULTIPLIER / SPEED_MIN_MULTIPLIER, code / 15.0f);
    speedMult256[code] = (uint16_t)(mult * 256.0f + 0.5f);
  }
}

// Update interval for a pattern whose interval is baseMs at multiplier 1.
uint16_t intervalFor(uint16_t baseMs, uint16_t mult256) {
  uint32_t ms = ((uint32_t)baseMs * 256UL) / mult256;
  if (ms < MIN_UPDATE_MS) ms = MIN_UPDATE_MS;
  if (ms > 65535UL) ms = 65535UL;
  return (uint16_t)ms;
}

// Reads the four pins of a hex switch (pins[0] = bit value 1 ... pins[3] = bit value 8).
uint8_t readSpeedSwitch(const uint8_t *pins) {
  uint8_t code = 0;
  for (uint8_t bit = 0; bit < 4; bit++) {
    const bool low = digitalRead(pins[bit]) == LOW;
    if (low == SPEED_BIT_IS_LOW) code |= (uint8_t)(1 << bit);
  }
  return code;
}

void initSpeedSwitch(uint8_t slot, const uint8_t *pins) {
  for (uint8_t bit = 0; bit < 4; bit++) pinMode(pins[bit], INPUT_PULLUP);
  speedStable[slot] = readSpeedSwitch(pins);
  speedLastRaw[slot] = speedStable[slot];
  speedChangedAt[slot] = millis();
}

// Returns true when the debounced position just changed. A turning switch passes through
// other positions, so a position only counts once it has stayed unchanged for DEBOUNCE_MS.
bool pollSpeedSwitch(uint8_t slot, const uint8_t *pins, unsigned long now) {
  const uint8_t raw = readSpeedSwitch(pins);
  if (raw != speedLastRaw[slot]) {
    speedLastRaw[slot] = raw;
    speedChangedAt[slot] = now;
  }
  if (raw != speedStable[slot] && now - speedChangedAt[slot] >= DEBOUNCE_MS) {
    speedStable[slot] = raw;
    return true;
  }
  return false;
}

void printSpeed(const __FlashStringHelper *label, uint8_t code) {
  Serial.print(label);
  Serial.print(code, HEX);
  Serial.print(F(" (x"));
  Serial.print(speedMult256[code] / 256.0f, 2);
  Serial.println(F(")"));
}

void applyMainSpeed() {
  const uint8_t code = speedStable[SLOT_SPEED_MAIN];
  frameIntervalMs = intervalFor(FRAME_SPEED_MS, speedMult256[code]);
  arrowIntervalMs = intervalFor(ARROW_SPEED_MS, speedMult256[code]);
  printSpeed(F("Frame and arrow speed, position "), code);
}

void applyLettersSpeed() {
  const uint8_t code = speedStable[SLOT_SPEED_LETTERS];
  letterMult256 = speedMult256[code];
  printSpeed(F("Letters speed, position "), code);
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
// Both light paths show the same pattern: FRAME_BAR_LEDS lit, FRAME_BAR_LEDS dark, repeating.
// The pattern is a function of (distance - phase) modulo FRAME_PERIOD, where "distance" is
// how far an LED is from the start corner along its path, so it also works when the path
// length is not a multiple of FRAME_PERIOD.
//
// Strip index order (as installed):
//   frameCw   0 .. TOP-1          top edge, left to right
//             TOP .. HALF-1       right edge, top to bottom
//   frameCcw  0 .. SIDE-1         left edge, top to bottom
//             SIDE .. HALF-1      bottom edge, left to right
// Arrow right: start corner top-left, end corner bottom-right. Each strip is one path
//   and the distance is simply the LED index.
// Arrow left: start corner top-right, end corner bottom-left. The two paths are
//   top edge (reversed) + left edge, and right edge + bottom edge (reversed), so they
//   cross from one strip to the other.

uint8_t framePhase = 0;

// Distance of LED i of a frame strip from the start corner (both paths are HALF long).
uint16_t frameDistance(bool cwStrip, uint16_t i, bool mirrored) {
  if (!mirrored) return i;
  if (cwStrip) {
    return i < FRAME_TOP_LEDS ? (FRAME_TOP_LEDS - 1 - i)  // top edge, from the right
                              : (i - FRAME_TOP_LEDS);     // right edge, from the top
  }
  return i < FRAME_SIDE_LEDS ? (FRAME_TOP_LEDS + i)                                     // left edge
                             : (FRAME_SIDE_LEDS + (FRAME_TOP_LEDS - 1 - (i - FRAME_SIDE_LEDS)));  // bottom edge
}

void drawFrameHalf(Adafruit_NeoPixel &strip, bool cwStrip, bool mirrored, uint8_t phase) {
  const uint16_t count = strip.numPixels();
  const uint32_t white = strip.Color(255, 255, 255);
  for (uint16_t i = 0; i < count; i++) {
    const uint16_t distance = frameDistance(cwStrip, i, mirrored);
    const uint8_t pos = (uint8_t)((distance % FRAME_PERIOD + FRAME_PERIOD - phase) % FRAME_PERIOD);
    strip.setPixelColor(i, pos < FRAME_BAR_LEDS ? white : 0);
  }
}

void updateFrame() {
  const bool mirrored = !arrowPointsRight;
  drawFrameHalf(frameCw, true, mirrored, framePhase);
  drawFrameHalf(frameCcw, false, mirrored, framePhase);
  frameCw.show();
  frameCcw.show();
  framePhase = (framePhase + 1) % FRAME_PERIOD;
}

// ================================ LETTERS ==================================
// Every letter is its own group of LEDs with its own brightness (0..255) that
// depends on the current mode. On top of every mode, now and then one random letter flickers like a
// faulty neon tube.

constexpr uint8_t NO_LETTER = 0xFF;

uint8_t letterMode = LETTER_START_MODE;
uint8_t activeMode = 0;                 // the pattern really shown (differs from letterMode only in random auto)
unsigned long autoPickedAt = 0;         // real time when random auto last picked a pattern
unsigned long letterModeStart = 0;      // pattern clock (letterClockMs) when the mode started
unsigned long letterModeStartReal = 0;  // real time when the mode started (for the automatic change)

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
    default:               Serial.print(F("random auto")); break;
  }
}

void printLetterMode() {
  Serial.print(F("Letter mode "));
  Serial.print(letterMode);
  Serial.print(F(": "));
  printModeName(letterMode);
  if (letterMode == MODE_RANDOM_AUTO) {
    Serial.print(F(" (now showing "));
    printModeName(activeMode);
    Serial.print(F(")"));
  }
  Serial.println();
}

// Random auto: choose a pattern from 0..PATTERN_MODE_COUNT-1, never the one that is showing now.
void pickAutoMode(unsigned long now) {
  const uint8_t previous = activeMode;
  uint8_t pick;
  if (previous < PATTERN_MODE_COUNT) {
    pick = (uint8_t)random(PATTERN_MODE_COUNT - 1);
    if (pick >= previous) pick++;  // skip the previous pattern
  } else {
    pick = (uint8_t)random(PATTERN_MODE_COUNT);
  }
  activeMode = pick;
  autoPickedAt = now;
  letterModeStart = letterClockMs;  // the new pattern starts from its beginning
}

void setLetterMode(uint8_t mode, unsigned long now) {
  letterMode = mode % LETTER_MODE_COUNT;
  letterModeStart = letterClockMs;
  letterModeStartReal = now;
  if (letterMode == MODE_RANDOM_AUTO) {
    activeMode = PATTERN_MODE_COUNT;  // no previous pattern to avoid
    pickAutoMode(now);
  } else {
    activeMode = letterMode;
  }
  printLetterMode();
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

// Brightness of one letter in the current mode. t = milliseconds since the mode started.
uint8_t baseLetterLevel(uint8_t letter, unsigned long t) {
  switch (activeMode) {
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
    default: break;
  }
  switch (activeMode) {
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
  uint16_t pos = 0;
  for (uint8_t letter = 0; letter < LETTER_COUNT; letter++) {
    const uint8_t level = letterLevel(letter);
    const uint32_t color = letters.Color(level, level / 12, 0);  // deep red, slightly warm
    for (uint8_t k = 0; k < LETTER_LEDS[letter]; k++) {
      letters.setPixelColor(pos + k, color);
    }
    pos += LETTER_LEDS[letter];
  }
}

// ============================== CLOSED LIGHT ===============================
// Six white LEDs behind the plate light the "CLOSED" hole text while the casino is closed.
// They behave like a faulty fluorescent tube:
//   startup  the tube tries to start: quick flashes, more and more of them stay on
//   steady   full white for a random time
//   flicker  a burst of irregular flicker (full, dim and dark phases), then steady again
// The timing runs on the pattern clock of the letters (letterClockMs), so SW4 speeds it up.
// While the casino is open the light is off.

constexpr uint8_t CL_OFF = 0;
constexpr uint8_t CL_STARTUP = 1;
constexpr uint8_t CL_STEADY = 2;
constexpr uint8_t CL_FLICKER = 3;

uint8_t closedState = CL_OFF;
unsigned long closedStateStart = 0;    // pattern clock when the state began
unsigned long closedStateLength = 0;   // how long the state lasts (pattern ms)
unsigned long closedHoldUntil = 0;     // pattern clock until the current brightness is held
uint8_t closedLevel = 0;               // brightness of the six LEDs

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
  return (inputStable[INPUT_CLOSED] == LOW) == CASINO_CLOSED_WHEN_LOW;
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
  const uint32_t white = closedLight.Color(closedLevel, closedLevel, closedLevel);  // always white
  for (uint16_t i = 0; i < CLOSED_LIGHT_LEDS; i++) {
    closedLight.setPixelColor(i, white);
  }
  closedLight.show();
}

void updateLetters(unsigned long now) {
  if (LETTER_MODE_AUTO_MS > 0 && now - letterModeStartReal >= LETTER_MODE_AUTO_MS) {
    setLetterMode(letterMode + 1, now);
    lcdShowPage(LCD_PAGE_MODE, now, false);
  }
  if (letterMode == MODE_RANDOM_AUTO && now - autoPickedAt >= LETTER_RANDOM_AUTO_MS) {
    pickAutoMode(now);
    printLetterMode();
    lcdShowPage(LCD_PAGE_MODE, now, false);  // show the new pattern, no hold
  }
  updateFlickerState(now);
  advanceLetterClock(now);
  drawLetters();
  letters.show();
  updateClosedLight();
}

// ================================= ARROW ===================================
// The arrow is one flowing light. Every pixel gets a "flow coordinate" s that
// says how far along the path the light has travelled:
//
//   tail cap     0 .. CAP_HALF-1      from the cap middle out to its corners
//   shaft edges  CAP_HALF .. +BODY    along the shaft toward the arrowhead
//   arrowhead    HEAD_START ..        from the shaft corners up the base pieces and along
//                                     the wings to the tip
//
// The top lane (top edge, upper cap half, upper base piece and wing) and the bottom lane run
// with different colour phases for a layered, 3D look.
//
// Strip layout (physical wiring order):
//   shaft edges  left to right
//   caps         top to bottom
//   arrowheads   upper base piece (starting next to the cap), upper wing to the
//                tip, lower wing from the tip, lower base piece ending next to
//                the cap
// The shaft is closed at the tail by the tail cap and open toward the head, so the
// cap at the head end stays dark. Only the arrowhead the arrow points at is lit; the
// other head stays dark. The direction comes from the slide switch.

// Palette stops as 0xRRGGBB: blue, cyan, violet, ice white.
const uint32_t PALETTE[4] = {0x143CFF, 0x00DCFF, 0x9628FF, 0xC8DCFF};

bool arrowDarkDirty = true;  // the unlit head and cap must be cleared and shown
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

  // White sparkle with a short tail; the bottom lane is half a cycle behind.
  const uint16_t offset = bottomLane ? ARROW_SPARKLE_CYCLE / 2 : 0;
  const uint16_t sparkle = (arrowTick + offset) % ARROW_SPARKLE_CYCLE;
  if (s <= sparkle && sparkle - s < 3) {
    const uint8_t strength = (sparkle - s == 0) ? 255 : (sparkle - s == 1 ? 170 : 90);
    r = mixChannel(r, 255, strength);
    g = mixChannel(g, 255, strength);
    b = mixChannel(b, 255, strength);
  }
  return strip.Color(r, g, b);
}

// Flow coordinate of tail cap LED j (0 = top corner): the light starts in the middle
// of the cap and runs out to both corners, where the shaft edges continue.
uint16_t capFlow(uint16_t j) {
  const bool lower = j >= ARROW_CAP_HALF;
  const uint16_t fromCorner = lower ? (ARROW_CAP_LEDS - 1 - j) : j;  // 0 at the corner
  return ARROW_CAP_HALF - 1 - fromCorner;
}

void drawArrowCap(Adafruit_NeoPixel &cap) {
  for (uint16_t j = 0; j < ARROW_CAP_LEDS; j++) {
    cap.setPixelColor(j, arrowPixel(cap, capFlow(j), j >= ARROW_CAP_HALF));
  }
}

void drawArrow() {
  for (uint16_t i = 0; i < ARROW_BODY_LEDS; i++) {
    const uint16_t s = ARROW_CAP_HALF + (arrowPointsRight ? i : (ARROW_BODY_LEDS - 1 - i));
    arrowBodyTop.setPixelColor(i, arrowPixel(arrowBodyTop, s, false));
    arrowBodyBottom.setPixelColor(i, arrowPixel(arrowBodyBottom, s, true));
  }

  // The cap at the tail end is lit; the cap at the head end stays dark.
  drawArrowCap(arrowPointsRight ? arrowCapLeft : arrowCapRight);

  Adafruit_NeoPixel &litHead = arrowPointsRight ? arrowHeadRight : arrowHeadLeft;
  for (uint16_t j = 0; j < ARROW_HEAD_LEDS; j++) {
    // Both sides of the head flow toward the tip, which sits between index HALF-1 and HALF.
    const bool lowerSide = j >= ARROW_HEAD_HALF;
    const uint16_t s = lowerSide ? (ARROW_HEAD_START + ARROW_HEAD_LEDS - 1 - j)
                                 : (ARROW_HEAD_START + j);
    litHead.setPixelColor(j, arrowPixel(litHead, s, lowerSide));
  }
}

// Sets the arrow direction from the debounced switch level.
void applyArrowSwitch() {
  const bool pointsLeft = (inputStable[INPUT_ARROW] == LOW) == ARROW_POINTS_LEFT_WHEN_LOW;
  if (pointsLeft == arrowPointsRight) {  // direction changes
    arrowPointsRight = !pointsLeft;
    arrowDarkDirty = true;
    framePhase = 0;  // the frame light restarts from its new start corner
    Serial.println(arrowPointsRight ? F("Arrow points right") : F("Arrow points left"));
  }
}

void updateArrow() {
  drawArrow();
  arrowBodyTop.show();
  arrowBodyBottom.show();

  Adafruit_NeoPixel &litHead = arrowPointsRight ? arrowHeadRight : arrowHeadLeft;
  Adafruit_NeoPixel &darkHead = arrowPointsRight ? arrowHeadLeft : arrowHeadRight;
  Adafruit_NeoPixel &litCap = arrowPointsRight ? arrowCapLeft : arrowCapRight;
  Adafruit_NeoPixel &darkCap = arrowPointsRight ? arrowCapRight : arrowCapLeft;
  litHead.show();
  litCap.show();
  if (arrowDarkDirty) {  // only touch the unlit head and cap when the direction changed
    darkHead.clear();
    darkHead.show();
    darkCap.clear();
    darkCap.show();
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
// The LCD shows the time and a progress bar (see the LCD DISPLAY section). The push button D3
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
  initSpeedSwitch(SLOT_SPEED_MAIN, PIN_SPEED_FRAME_ARROW);
  initSpeedSwitch(SLOT_SPEED_LETTERS, PIN_SPEED_LETTERS);
  applyMainSpeed();
  applyLettersSpeed();
  lastLetterClockTick = now;

  arrowPointsRight = !((inputStable[INPUT_ARROW] == LOW) == ARROW_POINTS_LEFT_WHEN_LOW);
  arrowDarkDirty = true;

  setLetterMode(LETTER_START_MODE, now);
  setCasinoClosed(closedFromSwitch());
  nextFlickerAt = now + FLICKER_MIN_GAP_MS;
  lcdShowPage(LCD_PAGE_MODE, now, false);
}

void beginStartupTest(unsigned long now) {
  startupTestActive = true;
  startupTestStart = now;
  for (uint8_t k = 0; k < STRIP_COUNT; k++) startupLast[k] = 0xFFFFFFFFUL;
  Serial.println(F("Power-on LED test (press D3 to skip)"));
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
// Pages (rotating every LCD_PAGE_MS):
//   0 Mode        "Mode 7" / "sparkle"        (random auto: "Mode 10 (auto)" / "now: chase")
//   1 SW3         "SW3:8  x1.10" / "Fr 91ms Ar 31ms"   (position, speed, frame and arrow interval)
//   2 SW4         "SW4:8  x1.10" / "Letter patterns"
//   3 Status      "Casino: CLOSED" / "Arrow: <- LEFT"
// Changing a setting shows its page at once and holds it for LCD_HOLD_MS.
// If no display answers at start-up, the display is simply left out and the sign works as usual.

constexpr uint8_t LCD_BIT_RS = 0x01;
constexpr uint8_t LCD_BIT_E = 0x04;
constexpr uint8_t LCD_BIT_BACKLIGHT = 0x08;

bool lcdOk = false;                           // a display answered during start-up
char lcdWanted[LCD_ROWS][LCD_COLS + 1];       // the text that should be on screen
char lcdShown[LCD_ROWS][LCD_COLS];            // what the display shows now
unsigned long lcdLastBuild = 0;

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

const char *letterModeName(uint8_t mode) {
  switch (mode) {
    case MODE_BREATHE:    return "breathe";
    case MODE_WAVE:       return "wave";
    case MODE_TYPEWRITER: return "typewriter";
    case MODE_IGNITE:     return "ignite";
    case MODE_WORD_BLINK: return "word blink";
    case MODE_ALTERNATE:  return "alternate";
    case MODE_SCANNER:    return "scanner";
    case MODE_SPARKLE:    return "sparkle";
    case MODE_CHASE:      return "chase";
    case MODE_PULSE:      return "pulse";
    default:              return "random auto";
  }
}

// Builds the two text rows of the current page.
void lcdBuildText() {
  char line[24];
  switch (lcdPage) {
    case LCD_PAGE_MODE:
      if (letterMode == MODE_RANDOM_AUTO) {
        snprintf(line, sizeof(line), "Mode %u (auto)", (unsigned int)letterMode);
        lcdPut(0, line);
        snprintf(line, sizeof(line), "now: %s", letterModeName(activeMode));
        lcdPut(1, line);
      } else {
        snprintf(line, sizeof(line), "Mode %u", (unsigned int)letterMode);
        lcdPut(0, line);
        lcdPut(1, letterModeName(letterMode));
      }
      break;
    case LCD_PAGE_SW3: {
      const unsigned int hundredths = (unsigned int)(((uint32_t)speedMult256[speedStable[SLOT_SPEED_MAIN]] * 100UL + 128UL) / 256UL);
      snprintf(line, sizeof(line), "SW3:%X  x%u.%02u", (unsigned int)speedStable[SLOT_SPEED_MAIN],
               hundredths / 100, hundredths % 100);
      lcdPut(0, line);
      snprintf(line, sizeof(line), "Fr%ums Ar%ums", (unsigned int)frameIntervalMs, (unsigned int)arrowIntervalMs);
      lcdPut(1, line);
      break;
    }
    case LCD_PAGE_SW4: {
      const unsigned int hundredths = (unsigned int)(((uint32_t)letterMult256 * 100UL + 128UL) / 256UL);
      snprintf(line, sizeof(line), "SW4:%X  x%u.%02u", (unsigned int)speedStable[SLOT_SPEED_LETTERS],
               hundredths / 100, hundredths % 100);
      lcdPut(0, line);
      lcdPut(1, "Letter patterns");
      break;
    }
    default:
      lcdPut(0, casinoClosed ? "Casino: CLOSED" : "Casino: OPEN");
      lcdPut(1, arrowPointsRight ? "Arrow: RIGHT ->" : "Arrow: <- LEFT");
      break;
  }
}

// The LCD during the power-on test: one row scrolls "press D3 to skip", the other shows the
// elapsed seconds in the middle of a progress bar. The rows swap places every LCD_TEST_SWAP_MS,
// so every character gets shown on both rows.
void lcdBuildTestText(unsigned long now) {
  unsigned long elapsed = now - startupTestStart;
  if (elapsed > STARTUP_TEST_MS) elapsed = STARTUP_TEST_MS;
  const uint8_t counterRow = (uint8_t)((elapsed / LCD_TEST_SWAP_MS) % 2) == 0 ? 1 : 0;
  const uint8_t skipRow = 1 - counterRow;

  // Scrolling text: the text and three spaces repeat around the row.
  const char marquee[] = "press D3 to skip   ";
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
      Wire.endTransmission();
      return;  // the rest follows on the next pass
    }
  }
}

void updateLcd(unsigned long now) {
  if (!lcdOk) return;
  if (startupTestActive) {  // the power-on test has its own screen and no page rotation
    lcdBuildTestText(now);
    lcdSendChanges();
    return;
  }
  // Page rotation: a held page stays for LCD_HOLD_MS, then the pages rotate again.
  if (lcdHolding) {
    if (now - lcdHoldFrom >= LCD_HOLD_MS) {
      lcdHolding = false;
      lcdPage = (lcdPage + 1) % LCD_PAGE_COUNT;
      lcdPageSince = now;
      lcdRebuild = true;
    }
  } else if (now - lcdPageSince >= LCD_PAGE_MS) {
    lcdPage = (lcdPage + 1) % LCD_PAGE_COUNT;
    lcdPageSince = now;
    lcdRebuild = true;
  }
  if (lcdRebuild || now - lcdLastBuild >= 20) {
    lcdRebuild = false;
    lcdLastBuild = now;
    lcdBuildText();
  }
  lcdSendChanges();
}

// ============================== SETUP / LOOP ===============================

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));

  for (uint16_t i = 0; i < 256; i++) {
    sineTable[i] = (uint8_t)(128.0f + 127.0f * sin(i * 6.2831853f / 256.0f) + 0.5f);
  }

  letters.updateLength(lettersTotalLeds());

  for (uint8_t k = 0; k < STRIP_COUNT; k++) {
    allStrips[k]->begin();
    allStrips[k]->setBrightness(BRIGHTNESS);
    allStrips[k]->clear();
    allStrips[k]->show();
  }

  initInput(INPUT_MODE, PIN_MODE_BUTTON);
  buildSpeedTable();

  Wire.begin();
  Wire.setClock(400000UL);
  lcdInit();

  // The other inputs and the patterns are started by finishStartup(), after the LED test.
  if (STARTUP_TEST_MS > 0) {
    beginStartupTest(millis());
  } else {
    finishStartup(millis(), false);
  }
}

void loop() {
  const unsigned long now = millis();
  const bool buttonChanged = pollInput(INPUT_MODE, PIN_MODE_BUTTON, now);

  // Power-on LED test: only the button (skip) is watched; everything else waits for the end.
  if (startupTestActive) {
    if (buttonChanged && inputStable[INPUT_MODE] == LOW) {
      finishStartup(now, true);  // this press only skips the test, it does not change the mode
    } else {
      updateStartupTest(now);
    }
    updateLcd(now);
    return;
  }

  // Inputs: the slide switches set the arrow direction and casino open/closed, the button steps
  // through the letter modes.
  if (pollInput(INPUT_ARROW, PIN_ARROW_SWITCH, now)) {
    applyArrowSwitch();
    lcdShowPage(LCD_PAGE_STATUS, now, true);
  }
  if (buttonChanged && inputStable[INPUT_MODE] == LOW) {
    setLetterMode(letterMode + 1, now);
    lcdShowPage(LCD_PAGE_MODE, now, true);
  }
  if (pollInput(INPUT_CLOSED, PIN_CASINO_CLOSED_SWITCH, now)) {
    setCasinoClosed(closedFromSwitch());
    lcdShowPage(LCD_PAGE_STATUS, now, true);
  }

  // Speed switches: SW3 sets the frame and arrow speed, SW4 the letter speed.
  if (pollSpeedSwitch(SLOT_SPEED_MAIN, PIN_SPEED_FRAME_ARROW, now)) {
    applyMainSpeed();
    lcdShowPage(LCD_PAGE_SW3, now, true);
  }
  if (pollSpeedSwitch(SLOT_SPEED_LETTERS, PIN_SPEED_LETTERS, now)) {
    applyLettersSpeed();
    lcdShowPage(LCD_PAGE_SW4, now, true);
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
