# Motorstyring – delrapport (Nikolai)

## Ansvarsområde

Kodebasen som gjør radiosignalene om til motorbevegelser på Makeblock Orion-beltebilen. Signalveien er RadioMaster TX15 Max → ELRS-mottaker (Matek R24-D) → CRSF-signal → Orion → to DC-motorer og buzzer.

## Innhold

| Fil | Funksjon |
| --- | --- |
| `RobotControl/RobotControl.ino` | Kjører programmet, leser radioen, styrer motorene, spiller lyd og sender kanalverdier og status til PC. |
| `MotorTest/MotorTest.ino` | Testprogram for feilsøking |

## Løsningen

- **Styring:** venstre spak opp/ned (CH3) gir frem/revers, høyre spak sideveis (CH1) gir sving. Svingingen lages ved å gi beltene ulik fart (tanksstyring).
- **Sikkerhet:**
  - Bilen er bare aktiv når SA-spaken står nede og det er radiokontakt.
  - Ved signalbrudd stopper motorer og lyd etter 250 ms.
  - Etter oppstart og etter signalbrudd må SA settes opp og ned igjen med spakene i midten før bilen kjører.
- **Kjøreegenskaper:**
  - Slurrerom på 8 % for gass og 3 % for sving.
  - Fartsrampe på 0,25 s fra stillstand til full fart, som demper strømtopper.
  - Toppfarten er begrenset til PWM 160 av 255, fordi 9 V-batteriet driver motorer merket 6 V.
- **Radiolesing:** hver CRSF-ramme (26 bytes) kontrolleres med CRC-8, og ødelagte rammer forkastes.
- **Programstruktur:** programmet bruker aldri `delay()`, slik at ingen radiorammer går tapt. Hele programmet ligger i én fil med navngitte seksjoner og kommentarer.

## Utviklingsforløp

1. Første versjon: modulær kode med kanalmonitor og grafisk Windows-monitor.
2. Krav lagt til: SA ned som eneste aktive stilling, full stopp ved signalbrudd og dødbånd.
3. Forenkling: koden ble skrevet om til enklere C++. Den gikk fra 466 til ca. 230 linjer med samme funksjon, og ble til slutt samlet i én fil.

Hver versjon ble kompilert for ATmega328 og testet på PC med simulerte radiorammer. Den forenklede koden ble sammenlignet med originalen over 40 minutter simulert kjøring med tilfeldige spakbevegelser, signalbrudd og støy. Funksjonen var lik, bortsett fra avrunding på ±1 av 255 i PWM.

## Feilsøking

| Problem | Årsak | Løsning |
| --- | --- | --- |
| SA-armering fungerte motsatt av antatt | Radioen gir 191 (lav verdi) med SA nede | Målt med kanalmonitoren, deretter tilpasset i koden |
| Grafisk monitor avviste nye data | Den godtok bare linjer med nøyaktig 16 kanaler | Tolkingen ble endret til å godta de kanalene som faktisk brukes |
| Kanalmonitor og kjøreprogram kunne ikke brukes samtidig | Orion kjører bare ett program om gangen | Programmene ble slått sammen. Kjøreprogrammet sender nå selv kanalverdier til PC |

## Status og begrensninger

- Bilen er testet og fungerer: radio, sikkerhetsregler og motorer.
- Det nye spakoppsettet (CH3/CH1) er testet og fungerer som forventet.
- Venstre spak fjærer ikke tilbake til midten (vanlig for gass-spaken på radioer), stopper ikke bilen når spaken slippes. Bruk SA-spaken for å stoppe.
- Begrensninger som gjenstår: ingen batterivarsel, ingen strømmåling, ingen automatisk retningskorrigering.

Delrapporten finnes også som Word-fil: `Delrapport_Motorstyring_Nikolai.docx`.
