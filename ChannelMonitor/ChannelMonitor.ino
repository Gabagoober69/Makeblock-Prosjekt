#include "CrsfRx.h" // En header fil jeg fant på nettet, lar ATmega328P lese datapakken som kommer inn fra MATEK, jeg har en grov ide om hvordan den funker. 
CrsfRx radio; // Oppretter et objekt kalt radio av klassen CrsfRx fra headerfilen.
uint32_t printed = 0; //definerer variabler i RAM
uint8_t nextChannel = 0; //definerer variabler i RAM  
bool printing = false; //definerer variabler i RAM
uint16_t snapshot[16]; //definerer variabel tabell i RAM
bool snapshotAlive = false; //definerer variabler i RAM

void setup() {
  // Siden jeg tester programmet med motorene tilkoblet, forsikrer jeg meg om at de er skrudd av
  pinMode(6, OUTPUT); analogWrite(6, 0);
  pinMode(5, OUTPUT); analogWrite(5, 0);
  Serial.begin(115200); // Matek leverer på 115200 baud, ATmega328P leverer på 115200 baud, og PC er klar for å motta på 115200 baud
}

void loop() {
  radio.poll(); //Klasse funksjon som gir ino programmet tilgang til veridene fra crsf headeren
  if (!printing && millis()-printed >= 50) { // Starter et nytt snapshot når forrige utskrift er ferdig og minst 50 millisekunder har gått. Det gir opptil 20 snapshots per sekund.
    // for løkke som setter de 16 kanalverdiene fra CRSFrx inn i min variabel tabell
    for (uint8_t i=0; i<16; ++i) snapshot[i] = radio.ch[i];
    snapshotAlive = radio.alive(); //Klasse funksjon som skjekker at CRSF faktisk har mottat et gyldig radio signal med kanalverider. Dersom den har det, blir snapshotAlive = true
    printed = millis(); nextChannel = 0; printing = true; // Programmet gjør seg klar for neste del. printed begynner å ta tiden fra sist utskrift. nextchannel starter fra bunnen av listen/tabellen. 
    // Printing blir true for å kjøre neste par linjer med kode, nå som vi har fylt inn snapshot tabellen.  
  }
  // Nå har vi endret verdien til printing, og TX bufferen på orion er ledig til å sende 24 bytes.  
  if (printing && Serial.availableForWrite() >= 24) {
    if (nextChannel == 0) Serial.print(snapshotAlive ? F("OK ") : F("LOST ")); //Dersom serial Alive er sann, er det OK, ellers false. F er lagret i flash minnet. En del av kompilert kode. 
    Serial.print(F("CH")); Serial.print(nextChannel+1); // Tabellindeksen starter på 0. Legger til 1 for å vise kanalnavnene CH1 til CH16.
    Serial.print('='); Serial.print(snapshot[nextChannel]); Serial.print(' '); // snapshot[0] inneholder CH1, snapshot[1] inneholder CH2 osv. Skriver verdien og et mellomrom.
    // Ved hvert kall til loop() skrives neste kanal når printing er sann og TX-bufferen har plass. Etter alle 16 kanalene avsluttes linjen og printing settes til false.
    if (++nextChannel == 16) { Serial.println(); printing = false; }
  }
}
