# Motorstyring – delrapport (Nikolai)

## Ansvarsområde

Kodebasen som gjør radiosignalene om til motorbevegelser på Makeblock Orion-beltebilen. Signalveien er RadioMaster TX15 Max → ELRS-mottaker (Matek R24-D) → CRSF-signal → Orion → to DC-motorer og buzzer.

## Innhold

| Fil | Funksjon |
| --- | --- |
| `RobotControl/RobotControl.ino` | Kjøreprogrammet. Leser radioen, styrer motorene, spiller lyd og sender kanalverdier og status til PC. |
| `MotorTest/MotorTest.ino` | Testprogram som kjører hvert belte frem og tilbake uten radio. |

## Løsningen

- **Styring:** venstre spak opp/ned (CH3) gir frem/revers, høyre spak sideveis (CH1) gir sving. Svingingen lages ved å gi beltene ulik fart (tanksstyring).
- **Sikkerhet:**
  - Bilen er bare aktiv når SA står nede og det er radiokontakt.
  - Ved signalbrudd stopper motorer og lyd etter 250 ms.
  - Etter oppstart og etter signalbrudd må SA settes opp og ned igjen med spakene i midten før bilen kjører.
- **Kjøreegenskaper:**
  - Dødbånd på 8 % for gass og 3 % for sving.
  - Fartsrampe på 0,25 s fra stillstand til full fart, som demper strømtopper.
  - Toppfarten er begrenset til PWM 160 av 255, fordi 9 V-batteriet driver motorer merket 6 V.
- **Radiolesing:** hver CRSF-ramme (26 bytes) kontrolleres med CRC-8, og ødelagte rammer forkastes.
- **Programstruktur:** programmet bruker aldri `delay()`, slik at ingen radiorammer går tapt. Hele programmet ligger i én fil med navngitte seksjoner og norske kommentarer.

## Utviklingsforløp

1. **Første versjon:** modulær kode med kanalmonitor og grafisk Windows-monitor.
2. **Krav lagt til:** SA ned som eneste aktive stilling, full stopp ved signalbrudd og dødbånd.
3. **Forenkling:** koden ble skrevet om til enklere C++. Den gikk fra 466 til ca. 230 linjer med samme oppførsel, og ble til slutt samlet i én fil.
4. **Spakoppsett:** endret til venstre spak for gass og høyre spak for sving.

Hver versjon ble kompilert for ATmega328 og testet på PC med simulerte radiorammer. Den forenklede koden ble sammenlignet med originalen over 40 minutter simulert kjøring med tilfeldige spakbevegelser, signalbrudd og støy. Oppførselen var lik, bortsett fra avrunding på ±1 av 255 i PWM.

## Feilsøking

| Problem | Årsak | Løsning |
| --- | --- | --- |
| SA-armering fungerte motsatt av antatt | Radioen gir 191 (lav verdi) med SA nede | Målt med kanalmonitoren, deretter tilpasset i koden |
| Grafisk monitor avviste nye data | Den godtok bare linjer med nøyaktig 16 kanaler | Tolkingen ble endret til å godta de kanalene som faktisk brukes |
| Kanalmonitor og kjøreprogram kunne ikke brukes samtidig | Orion kjører bare ett program om gangen | Programmene ble slått sammen. Kjøreprogrammet sender nå selv kanalverdier til PC |
| Opplasting feilet («can't open device COM4») | Orion var tildelt COM5 | Riktig port valgt under Tools → Port |
| Uleselig tekst i Serial Monitor | Monitoren sto på 9600 baud, programmet bruker 115200 | Baudrate satt til 115200 |
| `LOST` / `NO LINK` på alle kanaler | Kjøreprogrammet var ikke lastet opp riktig. Koden lå i en gammel fane, og opplastingen hadde feilet | Testprogrammet RadioTest viste ca. 240 gyldige rammer/s, altså at ledninger og mottaker var i orden. Ny opplasting fra en ren sketch-mappe løste det |
| Riktige verdier på PC, men motorene sto stille | Feilen lå utenfor radiodelen | MotorTest testet motorene isolert. Deretter fungerte bilen som forventet |

## Status og begrensninger

- Bilen er testet og fungerer: radio, sikkerhetsregler og motorer.
- Det nye spakoppsettet (CH3/CH1) er testet på PC, men må bekreftes på bilen. Går gass eller sving feil vei, endres `INVERT_THROTTLE` eller `INVERT_STEER`.
- Hvis venstre spak ikke fjærer tilbake til midten (vanlig for gass-spaken på radioer), stopper ikke bilen når spaken slippes. Bruk da SA for å stoppe, eller juster spaken til å fjære tilbake.
- Begrensninger som gjenstår: ingen batterivarsel, ingen strømmåling, ingen automatisk retningskorrigering, og hindringsstoppen er ikke aktiv. Signalbruddsstoppen er programstyrt og ikke en uavhengig nødstopp.
