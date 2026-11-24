# Motorstyring – oversikt

Kort forklaring av hvordan koden er satt opp, hvordan delene henger sammen og hvorfor den er skrevet slik.

## Innhold

```
motor-styring-nikolai/
├── RobotControl/        Kjøreprogrammet (lastes opp til Orion for å kjøre)
│   ├── RobotControl.ino   innstillinger, sikkerhet, styring og motorer
│   ├── Radio.ino          leser kanalverdier fra mottakeren
│   └── Sound.ino          robotlyden
├── ChannelMonitor/      Testprogram: viser kanalene, motorene er av
│   ├── ChannelMonitor.ino
│   └── Radio.ino          kopi av Radio-fanen over
└── GraphicMonitor/      Windows-program som viser ChannelMonitor grafisk
```

`.ino`-filene i samme mappe åpnes som faner i Arduino IDE og blir satt sammen til ett program. Derfor kan RobotControl bruke `readChannel()` fra Radio-fanen uten noen `#include`.

## Signalveien

```
TX15 Max ──2,4 GHz ELRS──► Matek R24-D ──CRSF, 115200 baud──► Orion RX (pinne 0)
                                                                   │
                                          RobotControl: Radio → control() → motorer M1/M2 + buzzer
                                                                   │
                                                 Orion TX ──USB──► PC (statuslinjer)
```

- Orion har én serieport. **RX** tar imot radioen, **TX** går til PC-en via USB. Derfor kan begge brukes samtidig, men mottakeren må kobles fra under opplasting fordi USB da også bruker RX.
- Det er bare **ett program på Orion om gangen**: enten RobotControl (kjøre) eller ChannelMonitor (sjekke radioen).
- GraphicMonitor leser tekstlinjene fra ChannelMonitor (`OK CH2=992 CH4=992 CH5=191 CH6=191`). Den forstår ikke statuslinjene fra RobotControl. Bruk Arduinos Serial Monitor for dem.

## Hvordan RobotControl går rundt

```
loop()  (tusenvis av ganger/s, venter aldri)
 ├─ readRadio()     samler bytes → hel ramme → sjekker kontrollsum → 16 kanaler
 ├─ updateSound()   setter riktig tonehøyde hvis lyden spilles
 ├─ hvert 10. ms:   control()
 │     les spaker (stickValue) → sikkerhet (armed?) → lyd-bryter
 │     → ikke aktiv: stopAll()
 │     → aktiv: tanksstyring → ramp() → setMotor() × 2
 └─ hvert 200. ms:  printStatus()
```

## Kanaler

| Kanal | Kontroll | Bruk |
| --- | --- | --- |
| CH2 | Høyre spak opp/ned | Frem/revers |
| CH4 | Venstre spak sideveis | Sving |
| CH5 | SA | Hovedbryter: 191 = ned = aktiv |
| CH6 | SF | Robotlyd når den slås på |

CH1, CH3 og CH7–16 brukes ikke.

## Hvorfor koden er som den er

| Valg | Grunn |
| --- | --- |
| Ingen `delay()`, alt styres med `millis()` | Serieporten har bare plass til 64 bytes. Ved 115200 baud er den full på ca. 5,5 ms. Venter programmet, mister det radiorammer. |
| Bilen aktiv bare med SA ned + radiokontakt | Krav i oppgaven. SA ned gir 191 på vår radio (målt). |
| SA må opp før start (etter oppstart og signalbrudd) | Bilen skal aldri kjøre av seg selv når den slås på eller får kontakt igjen. |
| Spakene må stå i midten for å starte | Hindrer at bilen rykker av gårde hvis gassen står ute idet SA settes ned. |
| Stopp etter 250 ms uten gyldig ramme | Krav i oppgaven. Når forbindelsen er borte, kommer det ingen nye gyldige rammer. Hvor raskt mottakeren slutter å sende, må testes på bilen. |
| Kontrollsum (CRC) på hver ramme | En ødelagt byte kunne ellers gitt full gass. Ødelagte rammer kastes. |
| Dødbånd 8 % gass / 3 % sving | Bilen står stille når spaken er nær midten. Lettere å kontrollere. |
| Rampe: 0 → full fart på 0,25 s | Motorene er 6 V, men får 9 V batteri. Rampen demper strømtopper ved start og retningsbytte. |
| `MAX_PWM = 160` av 255 | Gir i snitt ca. 5,6 V til motorene (160/255 × 9 V). |
| Lyden spilles litt om gangen | Lyden varer 0,87 s. Med `delay()` ville bilen frosset like lenge. |
| Radio-fanen ligger i begge programmene | En Arduino-sketch kan bare bruke faner i sin egen mappe. Endres den ene, må den andre kopieres. |

## Det som må stemme mellom programmene

Endrer du én av disse, må de andre endres også:

| Hva | RobotControl.ino | ChannelMonitor.ino | GraphicMonitor/Monitor.ps1 |
| --- | --- | --- | --- |
| Kanalnumre | `CH_THROTTLE` osv. | `SHOWN_CHANNELS` | `$channels` |
| Bryterterskler | `SWITCH_HIGH`, `SWITCH_LOW` | – | `$SwitchHigh`, `$SwitchLow` |
| SA-retning (ned = lav) | `saDown = … <= SWITCH_LOW` | – | `$SaDownIsHigh = $false` |
| Linjeformat | – | `OK CH2=… CH4=…` | `ConvertFrom-ChannelLine` |

## Vanlige justeringer (alle øverst i RobotControl.ino)

- Et belte går feil vei → `INVERT_LEFT` / `INVERT_RIGHT`
- Svingen går feil vei → `INVERT_STEER`
- Mer/mindre slurr rundt midten → `DEADBAND_THROTTLE` / `DEADBAND_STEER`
- Mykere/raskere start → `RAMP_STEP`
- Toppfart → `MAX_PWM` (husk at motorene er 6 V)

## Teststatus

- Begge Arduino-programmene kompilerer for Orion (ATmega328): RobotControl bruker 18 % flash og 19 % RAM.
- Logikken er testet på PC med simulerte radiorammer: SA, oppstart, signalbrudd, dødbånd, rampe, lyd og støy på linja.
- Koden er sammenlignet med den tidligere, lengre versjonen over 40 minutter simulert kjøring og oppfører seg likt.
- **Ikke testet på bilen ennå.** Første test: løft beltene, sjekk motorretning, SA og stopp når radioen slås av.
