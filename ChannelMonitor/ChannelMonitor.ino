// =====================================================================
//  ChannelMonitor – viser kanalene bilen bruker, 20 ganger i sekundet
// =====================================================================
//  Brukes til å sjekke radioen før kjøring, sammen med GraphicMonitor på
//  PC-en (eller Serial Monitor). Motorene holdes av hele tiden.
//  Lastes opp i stedet for RobotControl.
//
//  Eksempel på linje:  OK CH2=992 CH4=992 CH5=191 CH6=191
//    OK / LOST = radiokontakt / ingen radiokontakt
//    CH2 frem/revers   CH4 sving   CH5 SA (191 = ned)   CH6 SF lyd
//
//  Fanen Radio er en kopi av den i RobotControl.

const int SHOWN_CHANNELS[4] = {2, 4, 5, 6};  // kanalene som skrives ut
unsigned long lastPrint = 0;                 // når forrige linje ble skrevet (millis)

void setup() {
  pinMode(6, OUTPUT);  analogWrite(6, 0);   // venstre motor (M1) av
  pinMode(5, OUTPUT);  analogWrite(5, 0);   // høyre motor (M2) av
  Serial.begin(115200);
}

void loop() {
  readRadio();

  // Skriv bare når hele linjen får plass i utbufferen, så vi aldri venter
  // og går glipp av radiobytes.
  if (millis() - lastPrint >= 50 && Serial.availableForWrite() >= 48) {
    lastPrint = millis();

    if (radioLink()) {
      Serial.print("OK");
    } else {
      Serial.print("LOST");
    }
    for (int i = 0; i < 4; i++) {
      Serial.print(" CH");
      Serial.print(SHOWN_CHANNELS[i]);
      Serial.print("=");
      Serial.print(readChannel(SHOWN_CHANNELS[i]));
    }
    Serial.println();
  }
}
