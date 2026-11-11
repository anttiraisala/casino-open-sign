// Host-side stand-in for EEPROM.h: a byte array that counts the real writes (update() writes only a changed value).
#pragma once
#include <cstdint>
#include <cstring>
struct EEPROMClass {
  uint8_t mem[1024];
  long writes = 0;
  EEPROMClass() { memset(mem, 0xFF, sizeof mem); }
  uint8_t read(int address) const { return mem[address]; }
  void update(int address, uint8_t value) { if (mem[address] != value) { mem[address] = value; writes++; } }
  void write(int address, uint8_t value) { mem[address] = value; writes++; }
};
extern EEPROMClass EEPROM;
