#pragma once
#include <Arduino.h>

// Receiver UART Baud must be set to 115200 in ELRS WebUI.
// CRSF CRC covers type and payload, excluding address and length.
class CrsfRx {
  uint8_t buf[64], used = 0;
  uint32_t lastByte = 0;
  static uint8_t crc8(const uint8_t *p, uint8_t n) {
    uint8_t c = 0;
    while (n--) {
      c ^= *p++;
      for (uint8_t i=0; i<8; ++i) c = (c & 0x80) ? (c << 1) ^ 0xD5 : c << 1;
    }
    return c;
  }
public:
  uint16_t ch[16] = {};
  uint32_t lastChannels = 0, frames = 0;
  bool alive() const { return frames && uint32_t(millis()-lastChannels) < 250; }
  void poll() {
    while (Serial.available()) {
      uint32_t now = micros();
      if (uint32_t(now-lastByte) > 5000) used = 0;
      lastByte = now;
      uint8_t b = Serial.read();
      if (!used && b != 0xC8) continue;
      buf[used++] = b;
      if (used == 2 && (b < 2 || b > 62)) { used = 0; continue; }
      if (used >= 2 && used == buf[1]+2) {
        if (crc8(buf+2, buf[1]-1) == buf[used-1] &&
            buf[2] == 0x16 && (buf[1] == 24 || buf[1] == 25)) {
          uint32_t bits = 0;
          uint8_t count = 0, pos = 3;
          for (uint8_t c=0; c<16; ++c) {
            while (count < 11) { bits |= uint32_t(buf[pos++]) << count; count += 8; }
            ch[c] = bits & 0x7FF;
            bits >>= 11; count -= 11;
          }
          lastChannels = millis(); ++frames;
        }
        used = 0;
      }
    }
  }
};
