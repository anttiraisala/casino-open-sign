// Host-side test of the menu engine in menu_test/sketch.ino (generated from tools/menu-editor/menu-v2.json).
//   g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH="\"$PWD/menu_test/sketch.ino\"" -o /tmp/menu_engine_harness tests/menu_engine_harness.cpp
//   /tmp/menu_engine_harness diff <scenario> <trace-out>    plays a scenario and writes the trace (compared with the JS trace)
//   /tmp/menu_engine_harness                                runs the C++-specific checks (EEPROM, start-up, strings)
// Expected values are written out here on purpose, not taken from the sketch constants.
#include "Arduino.h"
#include "Wire.h"
#include "EEPROM.h"
unsigned long g_millis = 0;
uint8_t g_pinLevel[64];
uint8_t g_pinOut[64];
MockSerial Serial;
TwoWire Wire;
EEPROMClass EEPROM;
#define MENU_TEST_LOG
#include SKETCH_PATH
#include <string>
#include <vector>

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { if (fails < 40) printf("FAIL: %s (line %d)\n", msg, __LINE__); fails++; } } while (0)

static std::string hexOf(const char *s) {
  std::string o; char b[4];
  for (int k = 0; k < 16; k++) { snprintf(b, sizeof b, "%02x", (unsigned char)s[k]); o += b; }
  return o;
}

static int diffMode(const char *scenario, const char *traceOut) {
  FILE *in = fopen(scenario, "r"), *out = fopen(traceOut, "w");
  if (!in || !out) { printf("cannot open files\n"); return 2; }
  shellApplySwitches = false;
  menuBegin();
  menuLastInput = 0; menuSince = 0;
  char line[256];
  uint16_t cbSeen = 0, batchSeen = 0;
  while (fgets(line, sizeof line, in)) {
    unsigned long t; char op[16]; long a = 0, b = 0;
    int n = sscanf(line, "%lu %15s %ld %ld", &t, op, &a, &b);
    if (n < 2) continue;
    menuTick(t);
    if (!strcmp(op, "turn")) menuTurn((int)a, b != 0, t);
    else if (!strcmp(op, "key")) menuKey((uint8_t)a, t);
    else if (!strcmp(op, "phys")) menuPhysical((uint8_t)a, (uint16_t)b, t);
    else if (!strcmp(op, "set")) menuSet((uint8_t)a, (uint16_t)b);
    else if (!strcmp(op, "rt")) menuChangeInSec = a;
    char r0[17], r1[17], q0[17], q1[17];
    menuRows(t, r0, r1);
    menuRows(t + 1700, q0, q1);
    fprintf(out, "%s %s %s %s ", hexOf(r0).c_str(), hexOf(r1).c_str(), hexOf(q0).c_str(), hexOf(q1).c_str());
    if (menuLogCbCount == cbSeen) fprintf(out, "-");
    for (uint16_t k = cbSeen; k < menuLogCbCount; k++) fprintf(out, "%s%d:%d:%d", k > cbSeen ? "," : "", menuLogCb[k].cb, menuLogCb[k].prop, menuLogCb[k].pos);
    cbSeen = menuLogCbCount;
    fprintf(out, " ");
    if (menuLogBatchLen == batchSeen) fprintf(out, "-");
    bool first = true;
    for (uint16_t k = batchSeen; k < menuLogBatchLen; ) {
      const uint8_t cnt = menuLogBatch[k++];
      if (!first) fprintf(out, ",");
      first = false;
      for (uint8_t j = 0; j < cnt; j++) fprintf(out, "%s%d", j ? "." : "", menuLogBatch[k++]);
    }
    batchSeen = menuLogBatchLen;
    fprintf(out, " ");
    for (uint8_t p = 0; p < MENU_PROP_COUNT; p++) fprintf(out, "%s%u", p ? "." : "", (unsigned)menuPosArr[p]);
    fprintf(out, " %c\n", "smecv"[menuMode]);
  }
  fclose(in); fclose(out);
  return 0;
}

int main(int argc, char **argv) {
  if (argc >= 4 && !strcmp(argv[1], "diff")) return diffMode(argv[2], argv[3]);
  // The C++-specific checks live in menu_engine_checks.inc (included below).
#include "menu_engine_checks.inc"
  if (fails) { printf("%d check(s) failed\n", fails); return 1; }
  printf("menu engine checks passed\n");
  return 0;
}
