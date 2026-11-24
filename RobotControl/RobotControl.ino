#include "CrsfRx.h"

// Channels are numbered 1..16. Configure these explicitly in EdgeTX Mixes.
const uint8_t STEER_CH=4, DRIVE_CH=2, ARM_CH=5, HORN_CH=6;
const int STEER_SIGN=-1, DRIVE_SIGN=1; // Steering mirrored; forward/reverse unchanged.
// Assumption: left motor M1, right motor M2. Check with tracks lifted!
const int LEFT_SIGN=-1, RIGHT_SIGN=1; // Both directions reversed after robot test.
const int MAX_PWM=160; // Increased drive limit; PWM is not voltage regulation.
const bool ENABLE_OBSTACLE_STOP=false; // TEMPORARY: disabled for drive/steering test.
const uint8_t SONAR_PIN=13; // Orion PORT_3 signal S2
const uint8_t BUZZER_PIN=A5; // Orion onboard buzzer uses SCL; physical switch must be ON.
CrsfRx radio;
bool armed=false, sawDisarmed=false, previousArm=false;
bool obstacle=true;
int distanceCm=-1; // -1 = missing/invalid echo, never interpreted as clear space.
uint32_t neutralSince=0;

// SF must be mixed to CH6 on the transmitter. Play once on each OFF -> ON edge.
// Confused robot: beeps, rising whistle, falling "waaah", then three low hiccups.
struct SoundPart { uint16_t startHz, endHz, durationMs; };
const SoundPart funnySound[]={
  {800,800,90},{0,0,60},{1200,1200,120},{0,0,70},
  {1000,2200,260},{0,0,90},{1100,180,550},{0,0,100},
  {220,160,100},{0,0,70},{180,120,100},{0,0,70},{140,90,160}
};
const uint8_t SOUND_PARTS=sizeof(funnySound)/sizeof(funnySound[0]);
bool previousHorn=true, soundPlaying=false;
uint32_t soundStarted=0, lastPitchUpdate=0;
void funnySoundTick() {
  if (!radio.alive()) {
    if (soundPlaying) noTone(BUZZER_PIN);
    soundPlaying=false; previousHorn=true; return;
  }
  bool horn=radio.ch[HORN_CH-1]>1500;
  if (!horn) {
    if (soundPlaying) noTone(BUZZER_PIN);
    soundPlaying=false;
  } else if (!previousHorn) {
    soundPlaying=true; soundStarted=millis(); lastPitchUpdate=soundStarted-10;
  }
  previousHorn=horn;
  if (!soundPlaying) return;
  uint32_t now=millis(), elapsed=now-soundStarted;
  uint8_t part=0;
  while (part<SOUND_PARTS && elapsed>=funnySound[part].durationMs) {
    elapsed-=funnySound[part].durationMs; ++part;
  }
  if (part>=SOUND_PARTS) { noTone(BUZZER_PIN); soundPlaying=false; return; }
  if (uint32_t(now-lastPitchUpdate)>=10) {
    lastPitchUpdate=now;
    const SoundPart &p=funnySound[part];
    if (p.startHz==0) { noTone(BUZZER_PIN); return; }
    uint16_t hz=int32_t(p.startHz)+(int32_t(p.endHz)-p.startHz)*int32_t(elapsed)/p.durationMs;
    // Timer2 sound leaves motor PWM on Timer0 alone. Short duration avoids stuck sound.
    tone(BUZZER_PIN,hz,30);
  }
}

// Nonblocking single-wire ultrasonic measurement; no pulseIn() UART stalls.
uint8_t sonarState=0;
uint32_t sonarStarted=0, echoStarted=0, lastPing=0;
void sonarTick() {
  uint32_t now=micros();
  if (!sonarState && uint32_t(now-lastPing)>=60000) {
    lastPing=now;
    pinMode(SONAR_PIN, OUTPUT); digitalWrite(SONAR_PIN, LOW);
    delayMicroseconds(2); digitalWrite(SONAR_PIN,HIGH);
    delayMicroseconds(10); digitalWrite(SONAR_PIN,LOW);
    pinMode(SONAR_PIN,INPUT);
    sonarStarted=micros(); sonarState=1;
  } else if (sonarState==1 && !digitalRead(SONAR_PIN)) {
    // Like pulseIn: discard any HIGH left over from the trigger first.
    sonarState=2;
  } else if (sonarState==2 && digitalRead(SONAR_PIN)) {
    echoStarted=now; sonarState=3;
  } else if (sonarState==3 && !digitalRead(SONAR_PIN)) {
    uint32_t width=now-echoStarted;
    float cm=width/58.0f;
    distanceCm=width<100 ? -1 : int(cm);
    if (width<100 || cm<=5.0f) obstacle=true;
    else if (cm>=7.0f) obstacle=false; // Hysteresis avoids chattering.
    sonarState=0;
  }
  if (sonarState && uint32_t(now-sonarStarted)>25000) {
    distanceCm=-1; obstacle=true; sonarState=0; // Missing echo: allow reverse only.
  }
}

int stick(uint8_t channel, int sign) {
  int v=constrain(int(radio.ch[channel-1])-992,-820,820)*sign;
  if (abs(v)<=50) return 0;
  return (v>0 ? v-50 : v+50)*100L/770;
}
void motor(uint8_t pwm, uint8_t dir, int speed) {
  digitalWrite(dir,speed>=0 ? HIGH : LOW);
  analogWrite(pwm,constrain(abs(speed),0,255));
}
void drive(int left, int right) {
  motor(6,7,left*LEFT_SIGN); motor(5,4,right*RIGHT_SIGN);
}
void setup() {
  digitalWrite(BUZZER_PIN,LOW); pinMode(BUZZER_PIN,OUTPUT);
  pinMode(6,OUTPUT); pinMode(7,OUTPUT);
  pinMode(5,OUTPUT); pinMode(4,OUTPUT);
  drive(0,0);
  Serial.begin(115200);
}
void loop() {
  radio.poll();
  funnySoundTick(); // Nonblocking sound; drive and failsafe continue during playback.
  if (ENABLE_OBSTACLE_STOP) sonarTick();
  // Short, nonblocking diagnostics. Use Serial Monitor at 115200, not the GUI.
  static uint32_t lastLog=0;
  if (millis()-lastLog>=250) {
    char line[64];
    int n=snprintf(line,sizeof(line),"R=%u A=%u S4=%u T2=%u H6=%u D=%d B=%u\r\n",
      unsigned(radio.alive()),unsigned(armed),unsigned(radio.ch[STEER_CH-1]),
      unsigned(radio.ch[DRIVE_CH-1]),unsigned(radio.ch[HORN_CH-1]),
      distanceCm,unsigned(ENABLE_OBSTACLE_STOP && obstacle));
    if (n>0 && n<int(sizeof(line)) && Serial.availableForWrite()>=n) {
      Serial.write((const uint8_t*)line,n); lastLog=millis();
    }
  }
  if (!radio.alive()) {
    armed=false; sawDisarmed=false; previousArm=true; neutralSince=0;
    drive(0,0); return;
  }
  int turn=stick(STEER_CH,STEER_SIGN), throttle=stick(DRIVE_CH,DRIVE_SIGN);
  bool arm=radio.ch[ARM_CH-1]>1500;
  bool neutral=turn==0 && throttle==0;
  if (neutral) { if (!neutralSince) neutralSince=millis(); }
  else neutralSince=0;
  if (!arm) { armed=false; sawDisarmed=true; }
  if (arm && !previousArm && sawDisarmed && neutralSince && millis()-neutralSince>=500)
    armed=true;
  previousArm=arm;
  if (!armed) { drive(0,0); return; }
  if (ENABLE_OBSTACLE_STOP && obstacle) {
    if (throttle>=0) drive(0,0);
    else { int p=throttle*MAX_PWM/100; drive(p,p); }
    return;
  }
  // At zero throttle, opposite track directions turn the robot in place.
  // Steering follows stick direction in both forward and reverse.
  int left=throttle+turn, right=throttle-turn;
  int scale=max(100,max(abs(left),abs(right)));
  drive(long(left)*MAX_PWM/scale,long(right)*MAX_PWM/scale);
}
