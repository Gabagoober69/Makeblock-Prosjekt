# Grafisk kanalmonitor for Windows

Viser kanalene bilen bruker mens RobotControl kjører på Orion.

## Start

1. Last opp `RobotControl/RobotControl.ino` til Orion (koble fra mottakeren under opplasting, koble den til igjen etterpå).
2. La USB-kabelen sitte i. Lukk Arduino Serial Monitor og andre programmer som bruker samme COM-port.
3. Dobbeltklikk `Start-Monitor.cmd`. Velg COM-port og trykk **Koble til**. Hastigheten er fast 115200 baud, 8N1.
4. Beveg spaker og brytere. De fire kanalene vises som stolper side om side:

| Kort | Kontroll | Visning |
| --- | --- | --- |
| CH2 GASS | Høyre spak opp/ned | Prosent og råverdi |
| CH4 STYRING | Venstre spak sideveis | Prosent og råverdi |
| CH5 SA | Hovedbryter | `NED = AKTIV` (191) eller `OPP = AV` (1792) |
| CH6 SF | Robotlyd | `PAA` eller `AV` |

Stolpene går fra -100 % nederst til +100 % øverst; midtlinjen er 0 %.

Windows PowerShell 5.1 og .NET Framework benyttes. Ingen Python eller ekstra pakker trengs. Startfilen bruker ExecutionPolicy Bypass kun for denne PowerShell-prosessen.

## Status

- **LIVE**: kanallinjer med OK kommer fra Orion.
- **INGEN GYLDIG RADIOINPUT**: Orion melder LOST (signalbrudd), eller ingen linje er mottatt siste sekund. Siste tall vises i grått.
- **DEMO**: simulerte verdier, merket i gult. Kobler fra COM-porten.
- **PORTFEIL**: lukk andre programmer på porten, sjekk port og USB-driver.

## Slik virker det

RobotControl sender to typer linjer over USB:

- Kanallinje 20 ganger/s, f.eks. `OK CH2=992 CH4=992 CH5=191 CH6=191`. Den leser monitoren.
- Statuslinje 5 ganger/s, f.eks. `ARMED T=500 S=0 L=500 R=500`. Den hopper monitoren over (nyttig i Serial Monitor).

Monitoren sender ingenting til Orion. Åpning av en USB-serieport kan likevel gi reset på enkelte kort/drivere.

Bryterterskler er de samme som i `RobotControl.ino` (`SWITCH_HIGH` og `SWITCH_LOW`): 1500 eller mer er høy, 484 eller mindre er lav, imellom er ukjent. SA ned = lav verdi (191). Endres dette i radioen, må `$SaDownIsHigh` øverst i `Monitor.ps1` endres tilsvarende.

## Sjekkliste før kjøring

- CH2 og CH4 ligger rundt 992 i midten. CH4 øker mot høyre, CH2 fremover.
- CH5 viser `NED = AKTIV` med SA ned.
- CH6 går til `PAA` når SF aktiveres.
- Slå av radioen: monitoren skal melde manglende radioinput.
- Løft beltene og kontroller motorretning, SA og ny start etter signalbrudd før gulvtest.
