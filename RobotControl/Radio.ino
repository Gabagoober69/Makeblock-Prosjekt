// Radio – leser kanalverdiene fra ELRS-mottakeren.
//
// Mottakeren sender en melding ("ramme") på 26 bytes mange ganger i sekundet:
//   byte 0      0xC8   start på rammen
//   byte 1      24     lengde (antall bytes som kommer etter denne)
//   byte 2      0x16   type: kanalverdier
//   byte 3–24          16 kanaler, 11 bit hver, lagt tett etter hverandre
//   byte 25            kontrollsum (sjekker at ingenting er ødelagt)
// Mottakeren sender også andre typer meldinger. Dem hopper vi over.
//
// Fanen er lik i RobotControl og ChannelMonitor.

const int FRAME_SIZE = 26;
byte frame[FRAME_SIZE];            // rammen vi holder på å samle
int byteCount = 0;                 // hvor mange bytes vi har samlet (0–26)

int channels[17];                  // channels[2] = CH2 osv. (plass 0 brukes ikke)
unsigned long lastValidFrame = 0;  // når siste gode ramme kom (millis)
bool gotFrame = false;             // har vi fått minst én god ramme?


// Gir verdien til en kanal, f.eks. readChannel(2) for CH2.
int readChannel(int number) {
  return channels[number];
}

// true = god ramme mottatt de siste 250 ms. false = signalbrudd.
bool radioLink() {
  return gotFrame && millis() - lastValidFrame < 250;
}


// Kalles fra loop(). Tar én og én byte og samler dem til en hel ramme.
void readRadio() {
  while (Serial.available()) {
    byte b = Serial.read();

    if (byteCount == 0 && b != 0xC8) continue;               // vent på startbyten
    if (byteCount == 1 && b != 24) { byteCount = 0; continue; }  // ikke en kanalramme: start på nytt

    frame[byteCount] = b;
    byteCount++;

    if (byteCount == FRAME_SIZE) {                           // alle 26 bytes er her
      byteCount = 0;
      if (frame[2] == 0x16 && checksum() == frame[25]) {     // riktig type og ikke ødelagt
        unpackChannels();
      }
    }
  }
}


// Henter de 16 kanalene ut av byte 3–24.
// Hver kanal er 11 bit, så de passer ikke pent inn i bytes (8 bit).
// Kanal 1 starter på bit 0, kanal 2 på bit 11, kanal 3 på bit 22 osv.
// For hver kanal: finn byten den starter i, hent tre bytes (24 bit, nok til
// å dekke kanalen), skyv bort bitene foran den og behold de 11 neste.
void unpackChannels() {
  for (int ch = 1; ch <= 16; ch++) {
    int startBit = (ch - 1) * 11;   // hvor kanalen starter, talt i bit
    int i = 3 + startBit / 8;       // byten den starter i
    int skip = startBit % 8;        // antall bit i den byten som tilhører kanalen før

    long threeBytes = frame[i] | ((long)frame[i + 1] << 8) | ((long)frame[i + 2] << 16);
    channels[ch] = (threeBytes >> skip) & 2047;   // 2047 = 11 enere: behold 11 bit
  }
  lastValidFrame = millis();
  gotFrame = true;
}


// Kontrollsum (CRC-8): et tall regnet ut fra byte 2–24 med en fast oppskrift.
// Mottakeren regnet ut det samme før sending og la det i byte 25.
// Like tall = rammen kom frem uten feil. Fungerer som kontrollsifferet i et personnummer.
byte checksum() {
  byte crc = 0;
  for (int i = 2; i < 25; i++) {
    crc = crc ^ frame[i];
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 128) crc = (crc << 1) ^ 0xD5;
      else crc = crc << 1;
    }
  }
  return crc;
}
