// MotorTest – sjekker motorene uten radio.
// LØFT BELTENE FØR DU LASTER OPP. Batteripakken må være på.
// Kjører i ring: venstre frem, venstre bak, høyre frem, høyre bak, 2 sekunder hver,
// med pause imellom. Skriver hva den gjør til Serial Monitor (115200).

const int LEFT_PWM = 6,  LEFT_DIR = 7;   // M1
const int RIGHT_PWM = 5, RIGHT_DIR = 4;  // M2
const int TEST_PWM = 120;                // moderat fart (av 255)

void setup() {
  pinMode(LEFT_PWM, OUTPUT);  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT); pinMode(RIGHT_DIR, OUTPUT);
  Serial.begin(115200);
}

void run(const char *text, int pwmPin, int dirPin, int dir) {
  Serial.println(text);
  digitalWrite(dirPin, dir);
  analogWrite(pwmPin, TEST_PWM);
  delay(2000);
  analogWrite(pwmPin, 0);
  Serial.println("pause");
  delay(1000);
}

void loop() {
  run("VENSTRE (M1) retning HIGH", LEFT_PWM, LEFT_DIR, HIGH);
  run("VENSTRE (M1) retning LOW",  LEFT_PWM, LEFT_DIR, LOW);
  run("HOYRE (M2) retning HIGH",   RIGHT_PWM, RIGHT_DIR, HIGH);
  run("HOYRE (M2) retning LOW",    RIGHT_PWM, RIGHT_DIR, LOW);
}
