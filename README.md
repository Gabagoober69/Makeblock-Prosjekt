# Makeblock Orion med ExpressLRS

Fjernstyrt Makeblock Starter Kit-belterobot med Orion, Matek R24-D og RadioMaster TX15 Max.

**Se [OVERSIKT.md](OVERSIKT.md)** for hvordan koden er satt opp, hvordan delene henger sammen og hvorfor.

## Innhold

| Mappe | Innhold |
| --- | --- |
| `RobotControl/` | Arduino-programmet: radio inn, motorer og lyd ut, kanaler og status til PC |
| `GraphicMonitor/` | Windows-program som viser kanalene mens bilen kjører (se `LES_MEG.md`) |

## Kort om bruk

1. Åpne `RobotControl/RobotControl.ino` i Arduino IDE, velg **Arduino Uno**.
2. Koble fra mottakeren, last opp, koble mottakeren til igjen.
3. Sett SA opp og deretter ned med spakene i midten for å starte. SA opp stopper bilen.
4. Valgfritt: start `GraphicMonitor/Start-Monitor.cmd` for å se kanalene.

## Samarbeid

Gjør endringer på en egen gren og åpne en pull request. Oppgi hvilke filer som er endret, og hva som faktisk er testet. Ikke legg inn ELRS-bindingsfrase, passord eller tilgangstokens i repoet.
