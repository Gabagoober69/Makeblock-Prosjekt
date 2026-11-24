// Sound – robotlyden, spilt litt om gangen så bilen kan kjøre samtidig.
// Hver tone glir fra startHz til endHz på durationMs millisekunder. 0 Hz = pause.
// Midlertidig lyd: endre tallene for å lage en annen.

const int BUZZER_PIN = A5;   // buzzeren på Orion (BUZZER-bryteren må stå på ON)
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
