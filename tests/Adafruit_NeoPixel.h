// Host-side mock of the Adafruit_NeoPixel API subset used by the sketch.
#pragma once
#include "Arduino.h"
#include <vector>
// The values of the real library (colour order and bit speed flags).
#define NEO_RGB 0x06
#define NEO_RBG 0x09
#define NEO_GRB 0x52
#define NEO_GBR 0xA1
#define NEO_BRG 0x58
#define NEO_BGR 0xA4
#define NEO_KHZ800 0x0000
#define NEO_KHZ400 0x0100
class Adafruit_NeoPixel;
extern std::vector<Adafruit_NeoPixel*> g_strips;
class Adafruit_NeoPixel {
 public:
  Adafruit_NeoPixel(uint16_t n, int16_t pin, uint16_t type = 0) : pin_(pin), buf_(n, 0), shown_(n, 0), type_(type) { g_strips.push_back(this); }
  void begin() {}
  void updateType(uint16_t t) { type_ = t; typeChanges++; }
  void updateLength(uint16_t n) { buf_.assign(n, 0); shown_.assign(n, 0); }
  void setBrightness(uint8_t b) { brightness_ = b; }
  void clear() { for (auto &c : buf_) c = 0; }
  uint16_t numPixels() const { return buf_.size(); }
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }
  void setPixelColor(uint16_t i, uint32_t c) { if (i < buf_.size()) buf_[i] = c; else overflow++; }
  void show() { shown_ = buf_; shownBrightness_ = brightness_; shows++; }
  int16_t pin_; std::vector<uint32_t> buf_, shown_; long shows = 0; long overflow = 0; uint16_t type_ = 0; long typeChanges = 0; uint8_t brightness_ = 255; uint8_t shownBrightness_ = 255;
};
