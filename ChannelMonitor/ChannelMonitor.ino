#include "CrsfRx.h"
CrsfRx radio;
uint32_t printed = 0;
uint8_t nextChannel = 0;
bool printing = false;
uint16_t snapshot[16];
bool snapshotAlive = false;

void setup() {
  // Orion M1/M2 PWM outputs: disable motors even in this diagnostic sketch.
  pinMode(6, OUTPUT); analogWrite(6, 0);
  pinMode(5, OUTPUT); analogWrite(5, 0);
  Serial.begin(115200);
}

void loop() {
  radio.poll();
  if (!printing && millis()-printed >= 50) { // 20 snapshots per second.
    // A complete output line represents one snapshot of all channels.
    for (uint8_t i=0; i<16; ++i) snapshot[i] = radio.ch[i];
    snapshotAlive = radio.alive();
    printed = millis(); nextChannel = 0; printing = true;
  }
  // Small writes keep the main loop servicing incoming CRSF bytes.
  if (printing && Serial.availableForWrite() >= 24) {
    if (nextChannel == 0) Serial.print(snapshotAlive ? F("OK ") : F("LOST "));
    Serial.print(F("CH")); Serial.print(nextChannel+1);
    Serial.print('='); Serial.print(snapshot[nextChannel]); Serial.print(' ');
    if (++nextChannel == 16) { Serial.println(); printing = false; }
  }
}
