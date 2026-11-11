// Host-side stand-in for Wire.h with a fake HD44780 LCD behind a PCF8574 backpack (standard
// wiring: P0 RS, P1 RW, P2 E, P3 backlight, P4..P7 D4..D7), used by tests/harness.cpp.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <cstddef>

struct TwoWire {
  // ---- what the sketch sees ----
  void begin() { begun = true; }
  void setClock(unsigned long hz) { clock = hz; }
  void beginTransmission(uint8_t address) { cur = address; buf.clear(); }
  size_t write(uint8_t b) { buf.push_back(b); return 1; }
  uint8_t endTransmission() {
    transactions++;
    lastBytes = buf.size();
    totalBytes += buf.size();
    if (buf.size() > 32) oversize = true;           // the AVR Wire buffer holds 32 bytes
    if (cur != address || !present) return 2;       // NACK on address
    for (uint8_t b : buf) feed(b);
    return 0;
  }

  // ---- the fake display ----
  uint8_t address = 0x27;
  bool present = true;
  bool begun = false;
  unsigned long clock = 0;
  long transactions = 0, totalBytes = 0;
  size_t lastBytes = 0;
  bool oversize = false;
  bool fourBit = false;
  char ddram[2][16];
  int addr = 0;                 // 0x00..0x0F row 0, 0x40..0x4F row 1
  long dataWrites = 0, clears = 0;
  bool backlight = false;
  uint8_t cgram[64] = {0};      // custom characters (8 bytes each); written after a command 0x40..0x7F
  bool inCgram = false;

  TwoWire() { memset(ddram, ' ', sizeof(ddram)); }
  std::string row(int r) const { return std::string(ddram[r], 16); }

 private:
  uint8_t cur = 0;
  std::vector<uint8_t> buf;
  bool prevE = false, haveHi = false;
  uint8_t hi = 0, hiRs = 0;
  void feed(uint8_t b) {
    const bool e = b & 0x04;
    backlight = b & 0x08;
    if (prevE && !e) latch(b >> 4, b & 1);       // data is taken on the falling edge of E
    prevE = e;
  }
  void latch(uint8_t nibble, uint8_t rs) {
    if (!fourBit) { if (nibble == 0x2) fourBit = true; return; }   // 0x3 nibbles are ignored
    if (!haveHi) { hi = nibble; hiRs = rs; haveHi = true; return; }
    haveHi = false;
    const uint8_t value = (uint8_t)((hi << 4) | nibble);
    if (hiRs == 0) command(value); else data(value);
  }
  void command(uint8_t v) {
    if (v == 0x01) { memset(ddram, ' ', sizeof(ddram)); addr = 0; clears++; }
    else if (v & 0x80) { addr = v & 0x7F; inCgram = false; }
    else if (v & 0x40) { addr = v & 0x3F; inCgram = true; }
  }
  void data(uint8_t v) {
    if (inCgram) { cgram[addr & 0x3F] = v; addr++; dataWrites++; return; }
    const int r = addr >= 0x40 ? 1 : 0, c = addr & 0x3F;
    if (c < 16) ddram[r][c] = (char)v;
    addr++;
    dataWrites++;
  }
};
extern TwoWire Wire;
