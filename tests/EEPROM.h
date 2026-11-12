// Host-side stand-in for EEPROM.h: a byte array that counts the real writes (update() writes only a changed value).
#pragma once
#include <cstdint>
#include <cstring>
struct EEPROMClass {
  uint8_t mem[1024];
  long writes = 0;
  int stuck = -1;                  // test hook: a write to this address is ignored (a worn-out byte)
  EEPROMClass() { memset(mem, 0xFF, sizeof mem); }
  int length() const { return (int)sizeof mem; }
  uint8_t read(int address) const { return mem[address]; }
  void update(int address, uint8_t value) { if (mem[address] != value) { writes++; if (address != stuck) mem[address] = value; } }
  void write(int address, uint8_t value) { writes++; if (address != stuck) mem[address] = value; }
};
extern EEPROMClass EEPROM;
