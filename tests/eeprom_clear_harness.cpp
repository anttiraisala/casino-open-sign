// Host-side test of eeprom_clear/sketch.ino (clears the whole EEPROM to 0xFF once at start-up).
//   g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH="\"$PWD/eeprom_clear/sketch.ino\"" -o /tmp/eeprom_clear_harness tests/eeprom_clear_harness.cpp && /tmp/eeprom_clear_harness
// All expected values are written out here on purpose (they are not taken from the sketch constants),
// so a changed constant or rule in the sketch is detected. tests/mutate_eeprom_clear.py checks that.
#include "Arduino.h"
#include "EEPROM.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
EEPROMClass EEPROM;
#include SKETCH_PATH

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (t=%lu)\n", msg, g_millis); fails++; } } while (0)

static void run(unsigned long ms) {
  for (unsigned long k = 0; k < ms; k++) {
    g_millis++;
    loop();
  }
}

// Counts the toggles of the LED pin during the next ms milliseconds.
static int togglesIn(unsigned long ms) {
  int toggles = 0;
  uint8_t prev = g_pinOut[13];
  for (unsigned long k = 0; k < ms; k++) {
    run(1);
    if (g_pinOut[13] != prev) { toggles++; prev = g_pinOut[13]; }
  }
  return toggles;
}

// The last "DBG ..." line the sketch printed.
static std::string lastDbg() {
  const std::string &c = Serial.captured;
  size_t at = c.rfind("DBG up=");
  if (at == std::string::npos) return "";
  return c.substr(at, c.find('\n', at) - at);
}

static bool allErased() {
  for (int i = 0; i < 1024; i++) if (EEPROM.mem[i] != 0xFF) return false;
  return true;
}

// Starts a fresh power-up: the EEPROM holds what the test put there, the clock starts at 0.
static void powerUp() {
  g_millis = 0;
  Serial.captured.clear();
  Serial.capture = true;
  beatCount = 0; beatOn = false;
  memset(g_pinOut, 0, sizeof g_pinOut);
  setup();
}

static void resetEeprom() {
  memset(EEPROM.mem, 0xFF, sizeof EEPROM.mem);
  EEPROM.writes = 0;
  EEPROM.stuck = -1;
}

int main() {
  // --- a few bytes hold settings: 7 bytes are not 0xFF
  resetEeprom();
  const int used[7] = {0, 1, 2, 3, 4, 100, 1023};
  for (int a : used) EEPROM.mem[a] = (uint8_t)(0x12 + a % 7);
  powerUp();
  CHECK(Serial.baud == 115200, "serial speed is 115200");
  CHECK(Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "the start text states the 115200 baud speed");
  CHECK(allErased(), "every EEPROM byte is 0xFF after the start-up");
  CHECK(EEPROM.writes == 7, "only the 7 bytes that differed were written (update, not write)");
  CHECK(Serial.captured.find("EEPROM size 1024 bytes, 7 of them were not 0xFF.") != std::string::npos, "the size and the number of used bytes are printed");
  CHECK(Serial.captured.find("EEPROM cleared: all bytes are 0xFF. Upload the sign program now.") != std::string::npos, "the success message is printed");
  CHECK(Serial.captured.find("FAILED") == std::string::npos, "no failure message when the clearing worked");
  CHECK(Serial.captured.find("DBG up=") == std::string::npos, "no debug line before one second has passed");
  // sign of life: slow LED (500 ms) and one DBG line per second with changing values
  run(999);
  CHECK(Serial.captured.find("DBG up=") == std::string::npos, "still no debug line at t=999");
  run(1);
  CHECK(lastDbg() == "DBG up=1 size=1024 before=7 bad=0 state=OK beats=2", "t=1000: first debug line");
  run(1000);
  CHECK(lastDbg() == "DBG up=2 size=1024 before=7 bad=0 state=OK beats=4", "t=2000: second debug line with changed uptime and beats");
  { size_t lines = 0, at = 0;
    while ((at = Serial.captured.find("DBG up=", at)) != std::string::npos) { lines++; at++; }
    CHECK(lines == 2, "exactly one debug line per second (2 lines in 2 s)"); }
  { const int toggles = togglesIn(2000);
    CHECK(toggles == 4, "the LED toggles every 500 ms while the EEPROM is clear (4 toggles in 2 s)"); }
  CHECK(allErased() && EEPROM.writes == 7, "the loop does not write the EEPROM again");

  // --- an EEPROM that is already empty: nothing is written
  resetEeprom();
  powerUp();
  CHECK(allErased() && EEPROM.writes == 0, "an empty EEPROM is left alone (nothing written)");
  CHECK(Serial.captured.find("EEPROM size 1024 bytes, 0 of them were not 0xFF.") != std::string::npos, "0 used bytes are reported");
  run(1000);
  CHECK(lastDbg() == "DBG up=1 size=1024 before=0 bad=0 state=OK beats=2", "debug line of an empty EEPROM");

  // --- a full EEPROM: every byte is written
  resetEeprom();
  memset(EEPROM.mem, 0x00, sizeof EEPROM.mem);
  powerUp();
  CHECK(allErased() && EEPROM.writes == 1024, "all 1024 bytes cleared and written once");
  CHECK(Serial.captured.find("EEPROM size 1024 bytes, 1024 of them were not 0xFF.") != std::string::npos, "1024 used bytes are reported");

  // --- the very last byte and the very first byte are cleared too
  resetEeprom();
  EEPROM.mem[0] = 0x00; EEPROM.mem[1023] = 0x00;
  powerUp();
  CHECK(allErased() && EEPROM.writes == 2, "the first and the last byte are cleared");

  // --- a byte that cannot be cleared: failure message, fast LED, FAIL in the debug line
  resetEeprom();
  EEPROM.mem[500] = 0x55; EEPROM.mem[501] = 0x66; EEPROM.stuck = 500;
  powerUp();
  CHECK(EEPROM.mem[500] == 0x55 && EEPROM.mem[501] == 0xFF, "the stuck byte stays, the other byte is cleared");
  CHECK(Serial.captured.find("EEPROM clear FAILED: 1 byte(s) are still not 0xFF.") != std::string::npos, "the failure message names the number of bad bytes");
  CHECK(Serial.captured.find("EEPROM cleared") == std::string::npos, "no success message when a byte is stuck");
  CHECK(Serial.captured.find("EEPROM size 1024 bytes, 2 of them were not 0xFF.") != std::string::npos, "2 used bytes reported");
  { const int toggles = togglesIn(1000);
    CHECK(toggles == 10, "the LED toggles every 100 ms when clearing failed (10 toggles in 1 s)"); }
  CHECK(lastDbg() == "DBG up=1 size=1024 before=2 bad=1 state=FAIL beats=10", "debug line of the failed clearing");

  if (fails == 0) printf("RESULT: all checks passed\n");
  else printf("RESULT: %d failures\n", fails);
  return fails ? 1 : 0;
}
