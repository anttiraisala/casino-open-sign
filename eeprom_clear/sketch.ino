/*
 * EEPROM clear tool - sets every byte of the Arduino Mega's EEPROM to 0xFF (the state of a new chip)
 *
 * Why: uploading a sketch over USB does NOT touch the EEPROM, so the menu settings of the casino sign
 * (casino_open_sign_v2, saved in the EEPROM) survive an upload of a new version of the sign program. Upload THIS
 * sketch once when you want a really empty EEPROM, for example before trying a new menu layout or to see how the
 * sign starts from nothing. Afterwards upload the sign program again: it finds no valid saved settings (its layout tag
 * is missing) and starts with the default values of the menu. (The menu item "Restore defaults" of the sign does the
 * same for the values that the menu knows; this tool clears every byte.)
 *
 * WHAT IT DOES: at start-up it counts the bytes that are not 0xFF, writes 0xFF to every byte (EEPROM.update(): only
 * bytes that differ are really written, which spares the EEPROM wear), reads everything back and checks that all bytes
 * are 0xFF. It does that once, at start-up. Nothing else is connected or needed: no wiring, power it over USB.
 *
 * SIGN OF LIFE: the LED on pin 13 blinks all the time while the program runs: slowly (HEARTBEAT_OK_MS) when the
 * EEPROM is clear, fast (HEARTBEAT_FAIL_MS) if some byte could not be cleared. Once per second (DEBUG_MS) the Serial
 * Monitor (115200 baud, SERIAL_BAUD) gets a line
 *     DBG up=<s> size=<bytes> before=<bytes that were not 0xFF> bad=<bytes that are still not 0xFF> state=<OK|FAIL> beats=<n>
 * The start-up text states the baud rate and, when the clearing is done, "EEPROM cleared" (or "EEPROM clear FAILED").
 *
 * Only built-in types are used in function signatures (Arduino's automatic prototypes).
 */

#include <EEPROM.h>

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr uint8_t PIN_STATUS_LED = 13;          // onboard LED, sign of life
constexpr uint8_t ERASED_VALUE = 0xFF;          // the value of an erased EEPROM byte
constexpr unsigned long HEARTBEAT_OK_MS = 500;  // LED toggle interval when the EEPROM is clear
constexpr unsigned long HEARTBEAT_FAIL_MS = 100;  // LED toggle interval when clearing failed
constexpr unsigned long DEBUG_MS = 1000;        // one debug line per second

unsigned long eepromSize = 0;                   // number of bytes of the EEPROM
unsigned long bytesBefore = 0;                  // bytes that were not 0xFF before the clearing
unsigned long bytesBad = 0;                     // bytes that are not 0xFF after the clearing
bool clearOk = false;

unsigned long lastBeat = 0;
unsigned long lastDebug = 0;
unsigned long beatCount = 0;
bool beatOn = false;

// Number of EEPROM bytes that are not 0xFF.
unsigned long countNotErased() {
  unsigned long count = 0;
  for (unsigned long address = 0; address < eepromSize; address++) {
    if (EEPROM.read(address) != ERASED_VALUE) count++;
  }
  return count;
}

void printDebugLine(unsigned long now) {
  Serial.print(F("DBG up="));
  Serial.print(now / 1000UL);
  Serial.print(F(" size="));
  Serial.print(eepromSize);
  Serial.print(F(" before="));
  Serial.print(bytesBefore);
  Serial.print(F(" bad="));
  Serial.print(bytesBad);
  Serial.print(F(" state="));
  Serial.print(clearOk ? F("OK") : F("FAIL"));
  Serial.print(F(" beats="));
  Serial.println(beatCount);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PIN_STATUS_LED, OUTPUT);
  Serial.println(F("EEPROM clear tool: every EEPROM byte is set to 0xFF once, now."));
  Serial.print(F("Serial speed "));
  Serial.print(SERIAL_BAUD);
  Serial.println(F(" baud; a DBG line is printed every second."));

  eepromSize = EEPROM.length();
  bytesBefore = countNotErased();
  for (unsigned long address = 0; address < eepromSize; address++) {
    EEPROM.update(address, ERASED_VALUE);
    if ((address & 63UL) == 0) digitalWrite(PIN_STATUS_LED, (address & 64UL) ? HIGH : LOW);   // the LED flickers while it works
  }
  digitalWrite(PIN_STATUS_LED, LOW);                // the heartbeat starts from LED off
  bytesBad = countNotErased();
  clearOk = (bytesBad == 0);

  Serial.print(F("EEPROM size "));
  Serial.print(eepromSize);
  Serial.print(F(" bytes, "));
  Serial.print(bytesBefore);
  Serial.println(F(" of them were not 0xFF."));
  if (clearOk) {
    Serial.println(F("EEPROM cleared: all bytes are 0xFF. Upload the sign program now."));
  } else {
    Serial.print(F("EEPROM clear FAILED: "));
    Serial.print(bytesBad);
    Serial.println(F(" byte(s) are still not 0xFF."));
  }

  const unsigned long now = millis();
  lastBeat = now;
  lastDebug = now;
}

void loop() {
  const unsigned long now = millis();
  if (now - lastBeat >= (clearOk ? HEARTBEAT_OK_MS : HEARTBEAT_FAIL_MS)) {
    lastBeat = now;
    beatOn = !beatOn;
    beatCount++;
    digitalWrite(PIN_STATUS_LED, beatOn ? HIGH : LOW);
  }
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    printDebugLine(now);
  }
}
