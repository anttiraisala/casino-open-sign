// Minimal host-side stand-in for Arduino.h, only what the sketch uses.
#pragma once
#include <string>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#define A0 54
#define A1 55
#define A2 56
#define A3 57
#define A15 69
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define HEX 16
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_float(a) (*(const float *)(a))
struct __FlashStringHelper {};
#define F(x) (reinterpret_cast<const __FlashStringHelper*>(x))
typedef uint8_t byte;
extern unsigned long g_millis;
extern uint8_t g_pinLevel[64];       // test-controlled pin levels (pull-ups: default HIGH)
inline unsigned long millis() { return g_millis; }
inline void delay(unsigned long) {}
inline void randomSeed(unsigned long s) { srand((unsigned)s); }
inline long random(long max) { return rand() % max; }
inline long random(long min, long max) { return min + rand() % (max - min); }
inline int g_analogPin = -1;              // pin of the last analogRead() (checked by the harness)
inline int analogRead(int pin) { g_analogPin = pin; return 42; }
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t pin) { return g_pinLevel[pin]; }
extern uint8_t g_pinOut[64];         // levels written by the sketch with digitalWrite()
inline void digitalWrite(uint8_t pin, uint8_t v) { g_pinOut[pin] = v; }
struct MockSerial {
  bool quiet = true;
  bool capture = false;              // when true, everything printed is also appended to captured
  long baud = 0;                     // value given to begin()
  std::string captured;
  void begin(long b) { baud = b; }
  void out(const std::string &t) { if (!quiet) printf("%s", t.c_str()); if (capture) captured += t; }
  void print(const char *s) { out(s); }
  void print(const __FlashStringHelper *s) { out(reinterpret_cast<const char *>(s)); }
  void print(char c) { out(std::string(1, c)); }
  void print(int v) { out(std::to_string(v)); }
  void print(unsigned int v) { out(std::to_string(v)); }
  void print(unsigned long v) { out(std::to_string(v)); }
  void print(int v, int base) { char b[32]; snprintf(b, sizeof b, base == 16 ? "%X" : "%d", v); out(b); }
  void print(double v, int digits) { char b[64]; snprintf(b, sizeof b, "%.*f", digits, v); out(b); }
  void println(double v, int digits) { char b[64]; snprintf(b, sizeof b, "%.*f", digits, v); out(std::string(b) + "\n"); }
  void println(const char *s) { out(std::string(s) + "\n"); }
  void println(const __FlashStringHelper *s) { out(std::string(reinterpret_cast<const char *>(s)) + "\n"); }
  void println() { out("\n"); }
  void println(int v) { out(std::to_string(v) + "\n"); }
  void println(unsigned int v) { out(std::to_string(v) + "\n"); }
  void println(unsigned long v) { out(std::to_string(v) + "\n"); }
};
extern MockSerial Serial;
