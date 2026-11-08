/*
 * 11 sticks - the arrow (10 Grove RGB LED sticks) and the CLOSED light (the 11th stick) on one Grove Base Shield
 *
 * Purpose: keep all 11 sticks lit as in the casino sign and check that nothing gets hot (the shield, the Grove
 * connectors, the sticks, the wires, the supply). Nothing here has anything to do with the casino sign's own
 * program, but the stick order and connectors are the same as in the sign (sketch.ino and arrow_sticks/).
 *
 *   Sticks 1..10 = the arrow, plugged into the Grove Base Shield connectors in this order:
 *     1 L-CAP D2   2 L-WING-UP D3   3 L-WING-DN D4   4 RAIL-UP-L D5   5 RAIL-UP-R D6
 *     6 RAIL-DN-L D7   7 RAIL-DN-R D8   8 R-CAP A0   9 R-WING-UP A1   10 R-WING-DN A2
 *   Stick 11   = the CLOSED light, connector A3 (the one connector left over): all 10 LEDs white all the time.
 *
 * WHAT IT DOES: from power-up the CLOSED light (stick 11) is steady white at CLOSED_LEVEL, and the 10 arrow sticks
 * show a white wave that flows along every stick (the average brightness is about half of ARROW_LEVEL, like the
 * flowing arrow of the sign). The push button on pin 33 (to GND, short press) switches between two modes:
 *   - FLOW (default): the wave described above. A realistic load for hours of running.
 *   - ALL ON: all 110 LEDs steady white at the levels below. The worst case, to see how hot things can get.
 * The brightness levels (ARROW_LEVEL, CLOSED_LEVEL; 0..255 per colour channel) are set to 80 like the curve
 * maximum of the sign. Estimated current (a guess, printed at start-up and in the DBG line): see ESTIMATE_*.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs (every HEARTBEAT_MS, twice as fast
 * while many state changes happen; it is shortly inverted when the mode changes). Once per second (DEBUG_MS) the
 * Serial Monitor (115200 baud, SERIAL_BAUD) gets a line "DBG up=.. mode=flow|all fps=.. lit=.. ma=.. events=..
 * beats=..": uptime, mode, frames per second, lit LEDs (of 110), the estimated current in mA, mode changes in the
 * last second and LED toggles. The values change from line to line. (With the USB cable unplugged, as the supply is
 * on, there is no Serial Monitor; the pin 13 LED is then the only sign of life.)
 *
 * All timing uses millis(); the sketch never calls delay(). Function signatures use only built-in types.
 */

#include <Adafruit_NeoPixel.h>

// ============================== CONFIGURATION ==============================
constexpr uint8_t STICK_COUNT = 11;          // 10 arrow sticks + the CLOSED light stick (do not change)
constexpr uint8_t ARROW_STICKS = 10;         // sticks 1..10 are the arrow, stick 11 is the CLOSED light
constexpr uint8_t LEDS_PER_STICK = 10;
// Data pins = the signal pins of the Grove connectors D2..D8, A0..A2 (arrow) and A3 (CLOSED light).
constexpr uint8_t STICK_PINS[STICK_COUNT] = {2, 3, 4, 5, 6, 7, 8, A0, A1, A2, A3};
constexpr uint8_t PIN_BUTTON = 33;           // push button to GND: FLOW <-> ALL ON (same pin as the sign's button)
constexpr uint8_t PIN_STATUS_LED = 13;       // debug LED: sign of life
constexpr uint16_t COLOR_ORDER = NEO_GRB + NEO_KHZ800;   // WS2813: green-red-blue order, 800 kHz
constexpr uint8_t ARROW_LEVEL = 80;          // 0..255, brightness of an arrow LED at the crest of the wave / in ALL ON
constexpr uint8_t CLOSED_LEVEL = 80;         // 0..255, brightness of the CLOSED light LEDs
constexpr bool START_ALL_ON = false;         // true: start in ALL ON mode (the button still toggles)
constexpr uint32_t FRAME_MS = 20;            // data refresh interval
constexpr uint32_t BUTTON_MS = 30;           // the button must be stable this long
constexpr uint32_t HEARTBEAT_MS = 250;       // LED toggle interval
constexpr uint32_t HEARTBEAT_FAST_MS = 125;  // LED toggle interval while many state changes happen
constexpr uint32_t INVERT_MS = 120;          // the LED is inverted this long after a mode change
constexpr uint32_t SERIAL_BAUD = 115200;     // Serial Monitor speed (stated at the top of this file and printed at start)
constexpr uint32_t DEBUG_MS = 1000;          // interval of the "DBG" debug line
constexpr uint8_t FAST_EVENTS = 2;           // this many mode changes in the last two seconds = "many"
constexpr uint8_t ESTIMATE_MA_PER_CHANNEL = 20;   // guess: mA of one colour channel at 255 (a white LED = 3 channels)
// ===========================================================================

// One named object per stick (NeoPixel objects must not be copied) and a pointer array to loop over them.
Adafruit_NeoPixel stick1(LEDS_PER_STICK, STICK_PINS[0], COLOR_ORDER);
Adafruit_NeoPixel stick2(LEDS_PER_STICK, STICK_PINS[1], COLOR_ORDER);
Adafruit_NeoPixel stick3(LEDS_PER_STICK, STICK_PINS[2], COLOR_ORDER);
Adafruit_NeoPixel stick4(LEDS_PER_STICK, STICK_PINS[3], COLOR_ORDER);
Adafruit_NeoPixel stick5(LEDS_PER_STICK, STICK_PINS[4], COLOR_ORDER);
Adafruit_NeoPixel stick6(LEDS_PER_STICK, STICK_PINS[5], COLOR_ORDER);
Adafruit_NeoPixel stick7(LEDS_PER_STICK, STICK_PINS[6], COLOR_ORDER);
Adafruit_NeoPixel stick8(LEDS_PER_STICK, STICK_PINS[7], COLOR_ORDER);
Adafruit_NeoPixel stick9(LEDS_PER_STICK, STICK_PINS[8], COLOR_ORDER);
Adafruit_NeoPixel stick10(LEDS_PER_STICK, STICK_PINS[9], COLOR_ORDER);
Adafruit_NeoPixel stick11(LEDS_PER_STICK, STICK_PINS[10], COLOR_ORDER);
Adafruit_NeoPixel *const sticks[STICK_COUNT] = {&stick1, &stick2, &stick3, &stick4, &stick5, &stick6, &stick7, &stick8, &stick9, &stick10, &stick11};

bool allOn = START_ALL_ON;        // mode: false = FLOW, true = ALL ON
uint32_t tick = 0;                // frames drawn since power-up (the wave moves one step per frame)
uint32_t lastFrame = 0, lastBeat = 0, lastDebug = 0, lastEventAt = 0, eventWindowStart = 0;
bool anyEvent = false;
bool beatOn = false;
uint32_t beats = 0;
uint16_t frameCount = 0;          // frames since the last debug line
uint16_t litLeds = 0;             // lit LEDs of the last frame
uint32_t channelSum = 0;          // sum of all colour channel values of the last frame (for the current estimate)
uint8_t eventsNow = 0, eventsPrev = 0, eventsShown = 0;
bool buttonStable = false;        // debounced button state (true = pressed)
bool buttonRaw = false;
uint32_t buttonChangedAt = 0;

// Triangle wave 0..255..0 over a phase of 0..255.
uint8_t triangle(uint8_t phase) {
  return phase < 128 ? (uint8_t)(phase * 2) : (uint8_t)((255 - phase) * 2);
}

// Brightness 0..255 of LED `led` of arrow stick `stick` (0-based) at frame `t`: a wave that flows along every stick.
uint8_t waveOf(uint8_t stick, uint8_t led, uint32_t t) {
  const uint8_t phase = (uint8_t)(led * 28 + stick * 37 - t * 6);
  return triangle(phase);
}

void noteEvent(uint32_t now) {
  lastEventAt = now;
  anyEvent = true;
  eventsNow++;
  eventsShown++;
}

void drawFrame() {
  litLeds = 0;
  channelSum = 0;
  for (uint8_t s = 0; s < STICK_COUNT; s++) {
    for (uint8_t i = 0; i < LEDS_PER_STICK; i++) {
      uint8_t level;
      if (s == ARROW_STICKS) level = CLOSED_LEVEL;
      else if (allOn) level = ARROW_LEVEL;
      else level = (uint8_t)(((uint16_t)waveOf(s, i, tick) * ARROW_LEVEL) / 255);
      sticks[s]->setPixelColor(i, Adafruit_NeoPixel::Color(level, level, level));
      if (level) {
        litLeds++;
        channelSum += 3UL * level;
      }
    }
    sticks[s]->show();
  }
  tick++;
  frameCount++;
}

// Estimated current of the last frame in mA (a guess: ESTIMATE_MA_PER_CHANNEL mA per channel at 255).
uint16_t estimateMilliamps() {
  return (uint16_t)((channelSum * ESTIMATE_MA_PER_CHANNEL) / 255UL);
}

void printDebug(uint32_t now) {
  Serial.print(F("DBG up="));
  Serial.print((unsigned long)(now / 1000));
  Serial.print(F(" mode="));
  Serial.print(allOn ? F("all") : F("flow"));
  Serial.print(F(" fps="));
  Serial.print(frameCount);
  Serial.print(F(" lit="));
  Serial.print(litLeds);
  Serial.print(F(" ma="));
  Serial.print(estimateMilliamps());
  Serial.print(F(" events="));
  Serial.print(eventsShown);
  Serial.print(F(" beats="));
  Serial.println((unsigned long)beats);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (uint8_t s = 0; s < STICK_COUNT; s++) {
    sticks[s]->begin();
  }
  Serial.println(F("11 sticks: 10 arrow sticks (flowing white wave) + CLOSED light stick on A3 (steady white)"));
  Serial.print(F("Serial speed "));
  Serial.print((unsigned long)SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second. Button on pin 33: flow <-> all on."));
  const uint32_t now = millis();
  lastFrame = now;
  lastBeat = now;
  lastDebug = now;
  eventWindowStart = now;
  buttonChangedAt = now;
  drawFrame();                               // lit at once, no dark pause
  Serial.print(F("Estimated current now: "));
  Serial.print(estimateMilliamps());
  Serial.println(F(" mA (a guess, not a measurement)."));
}

void loop() {
  const uint32_t now = millis();

  // Push button (pin 33 to GND), debounced: a press switches the mode
  const bool raw = (digitalRead(PIN_BUTTON) == LOW);
  if (raw != buttonRaw) {
    buttonRaw = raw;
    buttonChangedAt = now;
  }
  if (buttonRaw != buttonStable && now - buttonChangedAt >= BUTTON_MS) {
    buttonStable = buttonRaw;
    if (buttonStable) {
      allOn = !allOn;
      noteEvent(now);
    }
  }

  if (now - eventWindowStart >= 1000) {      // sliding one-second window of state changes
    eventsPrev = eventsNow;
    eventsNow = 0;
    eventWindowStart = now;
  }

  if (now - lastFrame >= FRAME_MS) {
    lastFrame += FRAME_MS;
    drawFrame();
  }

  // Sign of life: LED 13 blinks all the time, faster when many mode changes happen, shortly inverted on a change
  const bool busy = (eventsNow + eventsPrev) >= FAST_EVENTS;
  const uint32_t beatMs = busy ? HEARTBEAT_FAST_MS : HEARTBEAT_MS;
  if (now - lastBeat >= beatMs) {
    lastBeat = now;
    beatOn = !beatOn;
    beats++;
  }
  const bool inverted = anyEvent && (now - lastEventAt < INVERT_MS);
  digitalWrite(PIN_STATUS_LED, (beatOn != inverted) ? HIGH : LOW);

  // Serial debug line once per second
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebug(now);
    frameCount = 0;
  }
}
