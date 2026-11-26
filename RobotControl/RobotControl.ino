// =====================================================================
//  RobotControl – styrer Orion med RC-radio
// =====================================================================
//  Gjør tre ting samtidig:
//    1. Leser radioen og kjører motorene.
//    2. Spiller av lyd når SF-spaken slås på.
//    3. Sender kanalverdier og status til PC-en via USB, slik at
//       GraphicMonitor (eller Serial Monitor) kan vise dem mens bilen kjører.
//
//  Sikkerhet: bilen er bare aktiv når SA står NED og det er radiokontakt.
//  Etter oppstart eller signalbrudd må SA opp og ned igjen, med spakene i midten.
//  NB: koble fra mottakeren før opplasting (deler serieport med USB).
// =====================================================================


// ============================ INNSTILLINGER ============================

const int CH_THROTTLE = 1;   // venstre spak opp/ned: frem/revers
const int CH_STEER    = 3;   // høyre spak sideveis: sving
const int CH_SA       = 5;   // bryter SA: hovedbryter (191 = ned)
const int CH_SOUND    = 6;   // bryter SF: robotlyd

const int SWITCH_HIGH = 1500;  // bryterverdi over dette = høy (1792)
const int SWITCH_LOW  = 484;   // bryterverdi under dette = lav (191)

const int DEADBAND_THROTTLE = 80;   // 8 %: spaken må forbi dette før bilen kjører
const int DEADBAND_STEER    = 30;   // 3 %
const int CENTER_TOLERANCE  = 100;  // spakene må være innenfor 10 % for å starte
const bool INVERT_THROTTLE  = false; // true hvis bilen kjører bakover når spaken skyves frem
const bool INVERT_STEER     = true;  // speilvendt styring

const int LEFT_PWM = 6,  LEFT_DIR = 7;   // venstre motor (M1): fart- og retningspinne
const int RIGHT_PWM = 5, RIGHT_DIR = 4;  // høyre motor (M2)
const bool INVERT_LEFT = false, INVERT_RIGHT = false;  // true hvis et belte går feil vei

const int MAX_PWM   = 160;  // toppfart av 255 (9 V batteri på 6 V-motorer: ikke øk)
const int RAMP_STEP = 40;   // maks fartsendring per 10 ms → 0 til full fart på 0,25 s (kontrollerer akselerasjon

const int BUZZER_PIN = A5;  // buzzeren på Orion (BUZZER-bryteren må stå på ON)

const bool SHOW_CHANNELS = true;  // kanallinje til GraphicMonitor 20 ganger/s
const bool SHOW_STATUS   = true;  // statuslinje til Serial Monitor 5 ganger/s


// ============================== TILSTAND ==============================

bool armed = false;            // true = bilen får kjøre og lage lyd
bool saWasUp = false;          // har SA stått opp siden start / siste signalbrudd?
bool lastSoundSwitch = false;  // sto lyd-bryteren på sist vi sjekket?
int throttle = 0, steer = 0;           // spakene, -1000 til 1000
int leftSpeed = 0, rightSpeed = 0;     // farten motorene får nå, -1000 til 1000
unsigned long lastControl = 0, lastChannels = 0, lastStatus = 0;  // tidspunkt (millis)


// =========================== OPPSTART OG LØKKE ===========================

void setup() {
  pinMode(LEFT_PWM, OUTPUT);  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT); pinMode(RIGHT_DIR, OUTPUT);
  stopAll();
  Serial.begin(115200);   // inn: radio fra mottakeren, ut: tekst til PC
}

// loop() går tusenvis av ganger i sekundet og venter aldri.
void loop() {
  readRadio();     // les nye bytes fra mottakeren
  updateSound();   // spill videre på lyden hvis den er i gang

  if (millis() - lastControl >= 10) {   // styring 100 ganger/s
    lastControl = millis();
    control();
  }

  // Utskrift: bare når hele linjen får plass i utbufferen (48 tegn),
  // så programmet aldri står og venter på USB.
  if (SHOW_CHANNELS && millis() - lastChannels >= 50 && Serial.availableForWrite() >= 48) {
    lastChannels = millis();
    printChannels();
  }
  if (SHOW_STATUS && millis() - lastStatus >= 200 && Serial.availableForWrite() >= 48) {
    lastStatus = millis();
    printStatus();
  }
}


// ============================== STYRING ==============================

// Leser radioen, avgjør om bilen er aktiv og styrer motorene.
void control() {
  throttle = stickValue(readChannel(CH_THROTTLE), DEADBAND_THROTTLE);
  steer    = stickValue(readChannel(CH_STEER), DEADBAND_STEER);
  if (INVERT_THROTTLE) throttle = -throttle;
  if (INVERT_STEER) steer = -steer;

  bool saDown = readChannel(CH_SA) <= SWITCH_LOW;
  bool centered = abs(throttle) <= CENTER_TOLERANCE && abs(steer) <= CENTER_TOLERANCE;

  // Sikkerhet
  if (!radioLink())                         { armed = false; saWasUp = false; }  // signalbrudd
  else if (!saDown)                         { armed = false; saWasUp = true; }   // SA opp
  else if (!armed && saWasUp && centered)   { armed = true; }                    // start

  // Lyd: reager bare i øyeblikket bryteren slås PÅ
  bool soundSwitch = readChannel(CH_SOUND) >= SWITCH_HIGH;
  bool soundOn = soundSwitch && !lastSoundSwitch;
  lastSoundSwitch = soundSwitch;

  if (!armed) { stopAll(); return; }   // stopp med en gang, uten rampe
  if (soundOn) playSound();

  // Tanksstyring: sving til høyre = venstre belte raskere.
  int left = throttle + steer;
  int right = throttle - steer;
  int largest = max(abs(left), abs(right));
  if (largest > 1000) {   // skaler ned begge likt så svingen beholdes
    left  = (long)left * 1000 / largest;
    right = (long)right * 1000 / largest;
  }
  leftSpeed  = ramp(leftSpeed, left);
  rightSpeed = ramp(rightSpeed, right);
  setMotor(LEFT_PWM, LEFT_DIR, INVERT_LEFT, leftSpeed);
  setMotor(RIGHT_PWM, RIGHT_DIR, INVERT_RIGHT, rightSpeed);
}

// Radioverdi (172–1811) → -1000…1000. Innenfor dødbåndet blir det 0, og
// utenfor starter verdien på 0 ved kanten (ingen hopp).
int stickValue(int raw, int deadband) {
  int value = constrain(map(raw, 172, 1811, -1000, 1000), -1000, 1000);
  if (abs(value) <= deadband) return 0;
  if (value > 0) return map(value, deadband, 1000, 0, 1000);
  return map(value, -1000, -deadband, -1000, 0);
}

// Flytter farten maks RAMP_STEP mot målet. Hindrer rykk og strømtopper.
int ramp(int current, int target) {
  return constrain(target, current - RAMP_STEP, current + RAMP_STEP);
}


// ============================== MOTORER ==============================

// Setter én motor. speed: -1000 (full revers) … 0 (stopp) … 1000 (full frem).
void setMotor(int pwmPin, int dirPin, bool invert, int speed) {
  bool forward = speed > 0;
  if (invert) forward = !forward;
  digitalWrite(dirPin, forward);                             // true = HIGH, false = LOW
  analogWrite(pwmPin, map(abs(speed), 0, 1000, 0, MAX_PWM)); // fart → PWM 0–MAX_PWM
}

// Stopper motorene og lyden med en gang.
void stopAll() {
  leftSpeed = 0; rightSpeed = 0;
  analogWrite(LEFT_PWM, 0); analogWrite(RIGHT_PWM, 0);
  stopSound();
}


// =============================== RADIO ===============================
// Mottakeren sender en melding ("ramme") på 26 bytes mange ganger i sekundet:
//   byte 0      0xC8   start på rammen
//   byte 1      24     lengde (antall bytes som kommer etter denne)
//   byte 2      0x16   type: kanalverdier
//   byte 3–24          16 kanaler, 11 bit hver, lagt tett etter hverandre
//   byte 25            kontrollsum (sjekker at ingenting er ødelagt)
// Mottakeren sender også andre typer meldinger. Dem hopper vi over.

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

    if (byteCount == 0 && b != 0xC8) continue;                   // vent på startbyten
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


// ================================ LYD ================================
// Hver tone glir fra startHz til endHz på durationMs millisekunder. 0 Hz = pause.
// Lyden spilles litt om gangen fra loop(), så bilen kan kjøre samtidig.
// Midlertidig lyd: endre tallene for å lage en annen.

const int NUM_TONES = 7;
int startHz[NUM_TONES]    = {1200,  0, 1600,  0,  600,  0, 1800};
int endHz[NUM_TONES]      = {1200,  0, 1600,  0, 1800,  0,  500};
int durationMs[NUM_TONES] = {  80, 40,   80, 60,  250, 60,  300};

int toneIndex = -1;           // tonen som spilles nå (-1 = ingen lyd)
unsigned long toneStart = 0;  // når tonen startet (millis)
int lastFreq = 0;             // frekvensen buzzeren har nå

void playSound() { toneIndex = 0; toneStart = millis(); lastFreq = 0; }

void stopSound() {
  if (toneIndex < 0) return;
  toneIndex = -1; lastFreq = 0;
  noTone(BUZZER_PIN);
}

// Kalles fra loop(): setter riktig frekvens akkurat nå.
void updateSound() {
  if (toneIndex < 0) return;
  long elapsed = millis() - toneStart;
  if (elapsed >= durationMs[toneIndex]) {          // tonen er ferdig: neste
    toneStart += durationMs[toneIndex];
    if (++toneIndex >= NUM_TONES) { stopSound(); return; }
    elapsed = millis() - toneStart;
  }
  int freq = map(elapsed, 0, durationMs[toneIndex], startHz[toneIndex], endHz[toneIndex]);
  if (freq == lastFreq) return;                    // uendret: ikke start buzzeren på nytt
  lastFreq = freq;
  if (freq < 31) noTone(BUZZER_PIN);               // under 31 Hz klarer ikke tone(): stille
  else tone(BUZZER_PIN, freq);
}


// ============================= UTSKRIFT TIL PC =============================

// Kanallinje til GraphicMonitor, f.eks. "OK CH3=992 CH1=992 CH5=191 CH6=191".
// OK = radiokontakt, LOST = signalbrudd.
void printChannels() {
  Serial.print(radioLink() ? "OK" : "LOST");
  Serial.print(" CH"); Serial.print(CH_THROTTLE); Serial.print("="); Serial.print(readChannel(CH_THROTTLE));
  Serial.print(" CH"); Serial.print(CH_STEER);    Serial.print("="); Serial.print(readChannel(CH_STEER));
  Serial.print(" CH"); Serial.print(CH_SA);       Serial.print("="); Serial.print(readChannel(CH_SA));
  Serial.print(" CH"); Serial.print(CH_SOUND);    Serial.print("="); Serial.println(readChannel(CH_SOUND));
}

// Statuslinje til Serial Monitor, f.eks. "ARMED T=500 S=0 L=500 R=500".
// NO LINK = signalbrudd. (SA up!) = SA må opp før start. T/S = gass/sving, L/R = motorfart.
// GraphicMonitor hopper over disse linjene.
void printStatus() {
  Serial.print(!radioLink() ? "NO LINK" : armed ? "ARMED" : "DISARMED");
  if (!saWasUp) Serial.print(" (SA up!)");
  Serial.print(" T="); Serial.print(throttle);  Serial.print(" S="); Serial.print(steer);
  Serial.print(" L="); Serial.print(leftSpeed); Serial.print(" R="); Serial.println(rightSpeed);
}
