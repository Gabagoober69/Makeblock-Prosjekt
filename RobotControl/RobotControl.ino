// RobotControl – styrer Orion-beltebilen med RC-radio.
// Faner: RobotControl (styring og motorer), Radio (leser mottakeren), Sound (robotlyd).
// Sikkerhet: bilen er bare aktiv når SA står NED og det er radiokontakt.
// Etter oppstart eller signalbrudd må SA opp og ned igjen, med spakene i midten.
// NB: koble fra mottakeren før opplasting (deler serieport med USB).

// ---------- Innstillinger ----------
const int CH_THROTTLE = 2;   // høyre spak opp/ned: frem/revers
const int CH_STEER    = 4;   // venstre spak sideveis: sving
const int CH_SA       = 5;   // bryter SA: hovedbryter (191 = ned)
const int CH_SOUND    = 6;   // bryter SF: robotlyd

const int SWITCH_HIGH = 1500;  // bryterverdi over dette = høy (1792)
const int SWITCH_LOW  = 484;   // bryterverdi under dette = lav (191)

const int DEADBAND_THROTTLE = 80;   // 8 %: spaken må forbi dette før bilen kjører
const int DEADBAND_STEER    = 30;   // 3 %
const int CENTER_TOLERANCE  = 100;  // spakene må være innenfor 10 % for å starte
const bool INVERT_STEER     = true; // speilvendt styring

const int LEFT_PWM = 6,  LEFT_DIR = 7;   // venstre motor (M1): fart- og retningspinne
const int RIGHT_PWM = 5, RIGHT_DIR = 4;  // høyre motor (M2)
const bool INVERT_LEFT = false, INVERT_RIGHT = false;  // true hvis et belte går feil vei

const int MAX_PWM   = 160;  // toppfart av 255 (9 V batteri på 6 V-motorer: ikke øk)
const int RAMP_STEP = 40;   // maks fartsendring per 10 ms → 0 til full fart på 0,25 s

// ---------- Tilstand ----------
bool armed = false;            // true = bilen får kjøre og lage lyd
bool saWasUp = false;          // har SA stått opp siden start / siste signalbrudd?
bool lastSoundSwitch = false;  // sto lyd-bryteren på sist vi sjekket?
int throttle = 0, steer = 0;           // spakene, -1000 til 1000
int leftSpeed = 0, rightSpeed = 0;     // farten motorene får nå, -1000 til 1000
unsigned long lastControl = 0, lastStatus = 0;  // tidspunkt (millis) for forrige kjøring

void setup() {
  pinMode(LEFT_PWM, OUTPUT);  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT); pinMode(RIGHT_DIR, OUTPUT);
  stopAll();
  Serial.begin(115200);   // inn: radio fra mottakeren, ut: status til PC
}

// loop() går tusenvis av ganger i sekundet og venter aldri.
void loop() {
  readRadio();     // les nye bytes fra mottakeren
  updateSound();   // spill videre på lyden hvis den er i gang
  if (millis() - lastControl >= 10) { lastControl = millis(); control(); }       // 100 ganger/s
  if (millis() - lastStatus >= 200) { lastStatus = millis(); printStatus(); }    // 5 ganger/s
}

// Leser radioen, avgjør om bilen er aktiv og styrer motorene.
void control() {
  throttle = stickValue(readChannel(CH_THROTTLE), DEADBAND_THROTTLE);
  steer    = stickValue(readChannel(CH_STEER), DEADBAND_STEER);
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

// Statuslinje til PC, f.eks. "ARMED T=500 S=0 L=500 R=500".
// NO LINK = signalbrudd. (SA up!) = SA må opp før start. T/S = gass/sving, L/R = motorfart.
void printStatus() {
  if (Serial.availableForWrite() < 48) return;   // ikke vent hvis bufferen er full
  Serial.print(!radioLink() ? "NO LINK" : armed ? "ARMED" : "DISARMED");
  if (!saWasUp) Serial.print(" (SA up!)");
  Serial.print(" T="); Serial.print(throttle);  Serial.print(" S="); Serial.print(steer);
  Serial.print(" L="); Serial.print(leftSpeed); Serial.print(" R="); Serial.println(rightSpeed);
}
