# Grafisk kanalmonitor for Windows

## Start

1. Last ned eller klon repoet og pakk det ut om nødvendig. Ikke start programmet direkte inne i ZIP-filen.
2. Last opp `ChannelMonitor/ChannelMonitor.ino` til Orion. Behold `Radio.ino` i samme mappe (den åpnes som egen fane). Motorutgangene er avslått i dette programmet.
3. Mottaker-TX til Orion RX, felles GND og 5 V forsyning. Matek RX skal ikke kobles til Orion TX i denne løsningen.
4. Lukk Arduino Serial Monitor og andre programmer som bruker samme COM-port.
5. Dobbeltklikk `Start-Monitor.cmd`. Velg COM-port og trykk **Koble til**. Hastigheten er fast 115200 baud, 8N1.
6. Beveg spaker og brytere. De fire kanalene roboten bruker vises som stolper side om side:

| Kort | Kontroll | Visning |
| --- | --- | --- |
| CH2 GASS | Høyre spak opp/ned | Prosent og råverdi |
| CH4 STYRING | Venstre spak sideveis | Prosent og råverdi |
| CH5 SA | Hovedbryter | `NED = AKTIV` (191) eller `OPP = AV` (1792) |
| CH6 SF | Robotlyd | `PAA` eller `AV` |

Stolpene stiger fra -100 % nederst til +100 % øverst; halv høyde tilsvarer 0 %. Navnene er forventet miks i radioen, ikke automatisk identifikasjon av spakene.

Windows PowerShell 5.1 og .NET Framework benyttes. Ingen Python, nettleser eller ekstra pakker trengs. Startfilen bruker ExecutionPolicy Bypass kun for denne PowerShell-prosessen; den endrer ikke maskinens permanente policy. Organisasjonspolicy kan fortsatt blokkere oppstart.

## Status og begrensninger

- **LIVE**: en komplett OK-linje er nylig mottatt fra Orion.
- **INGEN GYLDIG RADIOINPUT**: Orion melder LOST, eller ingen komplett linje er mottatt siste sekund. Sist mottatte tall beholdes i grått.
- **DEMO**: simulerte kanaler, tydelig merket i gult. Demo kobler fra COM-porten og sender ingenting til roboten.
- **PORTFEIL**: lukk andre terminaler, sjekk riktig port og USB-driver, og forsøk igjen.

Programmet viser tekstutdata fra ChannelMonitor, ikke den binære CRSF-strømmen direkte fra Matek. Det kan ikke brukes med RobotControl, som sender egne statuslinjer. De to Arduino-programmene lastes opp hver for seg. Monitoren sender ingen serielle styrekommandoer, men åpning av en USB-serieport kan likevel gi reset på enkelte kort/drivere.

ChannelMonitor sender omtrent 20 linjer per sekund, for eksempel `OK CH2=992 CH4=992 CH5=191 CH6=191`. Monitoren godtar også den gamle 16-kanalslinjen og viser da bare de fire kanalene. Stolpene animeres med en timer på 16 ms, med lett tidsbasert utjevning (tidskonstant 45 ms, ca. 95 % av et sprang etter 135 ms). Bare stolpehøyden utjevnes; tall og tekst viser alltid siste mottatte måling. Ved signalbrudd stopper animasjonen på siste faktiske verdi og stolpene blir grå.

Fullt standardutslag tilsvarer omtrent 172 / 992 / 1811 i CRSF. Prosentvisningen klippes til +/-100 %, mens råverdien viser hele den mottatte 11-bitsverdien. Visningen er ingen måling av radiosignalstyrke eller LQ.

Bryterterskler er de samme som i `RobotControl/RobotControl.ino` (`SWITCH_HIGH` og `SWITCH_LOW`): råverdi 1500 eller mer er høy, 484 eller mindre er lav, og verdier imellom regnes som ukjent. SA ned = lav verdi (191). Endres dette i radioen, må `$SaDownIsHigh` øverst i `Monitor.ps1` endres tilsvarende.

## Sjekkliste før kjøring

- Senter på CH2 og CH4 skal ligge rundt 992. CH4 skal øke mot høyre, CH2 fremover.
- CH5 skal vise `NED = AKTIV` med SA ned.
- CH6 skal gå til `PAA` når SF aktiveres.
- Slå av radioen: grafikken skal melde manglende radioinput.
- Last opp RobotControl når kanalene er riktige. Løft beltene og kontroller motorretning, SA og ny armering etter signalbrudd før gulvtest.
