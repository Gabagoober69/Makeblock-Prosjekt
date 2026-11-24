# Motorstyring – oversikt

Kort forklaring av hvordan koden er satt opp, hvordan delene henger sammen og hvorfor den er skrevet slik.

## Innhold

```
motor-styring-nikolai/
├── RobotControl/
│   └── RobotControl.ino   hele Arduino-programmet (lastes opp til Orion)
└── GraphicMonitor/        Windows-program som viser kanalene mens bilen kjører
```

Det er bare **ett Arduino-program**. Det kjører bilen, spiller lyd og sender kanalverdier og status til PC-en, alt samtidig.

## Signalveien

```
TX15 Max ──2,4 GHz ELRS──► Matek R24-D ──CRSF, 115200 baud──► Orion RX (pinne 0)
                                                                   │
                                     RobotControl: radio → control() → motorer M1/M2 + buzzer
                                                                   │
                                     Orion TX ──USB──► PC: GraphicMonitor eller Serial Monitor
```

- Orion har én serieport. **RX** tar imot radioen, **TX** sender tekst til PC-en. Begge brukes samtidig.
- Mottakeren må kobles fra under opplasting, fordi USB da også bruker RX.
- Til PC-en går to typer linjer:
  - kanallinje 20 ganger/s: `OK CH2=992 CH4=992 CH5=191 CH6=191` (leses av GraphicMonitor)
  - statuslinje 5 ganger/s: `ARMED T=500 S=0 L=500 R=500` (for Serial Monitor, GraphicMonitor hopper over den)

## Delene i RobotControl.ino

Filen er delt i seksjoner med overskrift:

| Seksjon | Gjør |
| --- | --- |
| INNSTILLINGER | Alle tall du kan justere: kanaler, dødbånd, pinner, fart, rampe |
| TILSTAND | Variabler som husker ting mellom rundene (`armed`, `saWasUp`, fart) |
| OPPSTART OG LØKKE | `setup()` én gang, `loop()` om og om igjen |
| STYRING | `control()`: sikkerhet og tanksstyring. `stickValue()`, `ramp()` |
| MOTORER | `setMotor()`, `stopAll()` |
| RADIO | Samler 26 bytes til en ramme, sjekker kontrollsum, pakker ut 16 kanaler |
| LYD | Robotlyden, spilt litt om gangen |
| UTSKRIFT TIL PC | `printChannels()` og `printStatus()` |

## Hvordan programmet går rundt

```
loop()  (tusenvis av ganger/s, venter aldri)
 ├─ readRadio()        nye bytes → hel ramme → kontrollsum → 16 kanaler
 ├─ updateSound()      riktig tonehøyde hvis lyden spilles
 ├─ hvert 10. ms:      control()
 │     les spaker → sikkerhet (armed?) → lyd-bryter
 │     → ikke aktiv: stopAll()
 │     → aktiv: tanksstyring → ramp() → setMotor() × 2
 ├─ hvert 50. ms:      printChannels()   (hvis det er plass i utbufferen)
 └─ hvert 200. ms:     printStatus()     (hvis det er plass i utbufferen)
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
| Alt i ett program | Orion kan bare kjøre ett program om gangen. Da kan bilen kjøre mens monitoren viser kanalene. |
| Ingen `delay()`, alt styres med `millis()` | Serieporten har bare plass til 64 bytes. Ved 115200 baud er den full på ca. 5,5 ms. Venter programmet, mister det radiorammer. |
| Utskrift bare når den får plass | Hvis utbufferen er full, ville `Serial.print` vente. Da hoppes linjen heller over til neste runde. |
| Bilen aktiv bare med SA ned + radiokontakt | Krav i oppgaven. SA ned gir 191 på vår radio (målt). |
| SA må opp før start (etter oppstart og signalbrudd) | Bilen skal aldri kjøre av seg selv når den slås på eller får kontakt igjen. |
| Spakene må stå i midten for å starte | Hindrer at bilen rykker av gårde hvis gassen står ute idet SA settes ned. |
| Stopp etter 250 ms uten gyldig ramme | Krav i oppgaven. Når forbindelsen er borte, kommer det ingen nye gyldige rammer. Hvor raskt mottakeren slutter å sende, må testes på bilen. |
| Kontrollsum (CRC) på hver ramme | En ødelagt byte kunne ellers gitt full gass. Ødelagte rammer kastes. |
| Dødbånd 8 % gass / 3 % sving | Bilen står stille når spaken er nær midten. Lettere å kontrollere. |
| Rampe: 0 → full fart på 0,25 s | Motorene er 6 V, men får 9 V batteri. Rampen demper strømtopper ved start og retningsbytte. |
| `MAX_PWM = 160` av 255 | Gir i snitt ca. 5,6 V til motorene (160/255 × 9 V). |
| Lyden spilles litt om gangen | Lyden varer 0,87 s. Med `delay()` ville bilen frosset like lenge. |

## Det som må stemme mellom RobotControl og GraphicMonitor

| Hva | RobotControl.ino | GraphicMonitor/Monitor.ps1 |
| --- | --- | --- |
| Kanalnumre | `CH_THROTTLE` osv. og `printChannels()` | `$channels` |
| Bryterterskler | `SWITCH_HIGH`, `SWITCH_LOW` | `$SwitchHigh`, `$SwitchLow` |
| SA-retning (ned = lav) | `saDown = … <= SWITCH_LOW` | `$SaDownIsHigh = $false` |
| Linjeformat | `printChannels()`: `OK CH2=…` | `ConvertFrom-ChannelLine` |

## Vanlige justeringer (alle under INNSTILLINGER)

- Et belte går feil vei → `INVERT_LEFT` / `INVERT_RIGHT`
- Svingen går feil vei → `INVERT_STEER`
- Mer/mindre slurr rundt midten → `DEADBAND_THROTTLE` / `DEADBAND_STEER`
- Mykere/raskere start → `RAMP_STEP`
- Toppfart → `MAX_PWM` (husk at motorene er 6 V)
- Skru av utskrift til PC → `SHOW_CHANNELS` / `SHOW_STATUS`

## Teststatus

- Kompilerer for Orion (ATmega328): 19 % flash og 21 % RAM.
- Testet på PC med simulerte radiorammer: SA, oppstart, signalbrudd, dødbånd, rampe, lyd, støy på linja og utskriften til GraphicMonitor.
- Sammenlignet med de tidligere versjonene over 40 minutter simulert kjøring: oppfører seg likt.
- **Ikke testet på bilen ennå.** Første test: løft beltene, sjekk motorretning, SA og stopp når radioen slås av.
