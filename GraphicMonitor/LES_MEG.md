# Grafisk kanalmonitor for Windows

## Start

1. Last ned eller klon repoet og pakk det ut om nødvendig. Ikke start programmet direkte inne i ZIP-filen.
2. Last opp `ChannelMonitor/ChannelMonitor.ino` til Orion. Behold `CrsfRx.h` i samme mappe. Motorutgangene er avslatt i dette programmet.
3. Bruk UART-koblingen beskrevet i ../docs/KOBLINGER.md. Mottaker-TX til Orion RX, felles GND og 5 V forsyning. Matek RX skal ikke kobles til Orion TX i denne losningen.
4. Lukk Arduino Serial Monitor og andre programmer som bruker samme COM-port.
5. Dobbeltklikk `Start-Monitor.cmd`. Velg COM-port og trykk **Koble til**. Hastigheten er fast 115200 baud, 8N1.
6. Beveg spaker og brytere. Alle 16 kanalene vises som fargede, vertikale stolper i to rader, med raverdier og prosent. Stolpene stiger fra -100 % nederst til +100 % overst; halv hoyde tilsvarer 0 %. CH4/CH2/CH5-navnene er forventet miks, ikke automatisk identifikasjon av spakene.

Windows PowerShell 5.1 og .NET Framework benyttes. Ingen Python, nettleser eller ekstra pakker trengs. Startfilen bruker ExecutionPolicy Bypass kun for denne PowerShell-prosessen; den endrer ikke maskinens permanente policy. Organisasjonspolicy kan fortsatt blokkere oppstart.

## Status og begrensninger

- **LIVE**: en komplett OK-linje er nylig mottatt fra Orion.
- **INGEN GYLDIG RADIOINPUT**: Orion melder LOST, eller ingen komplett linje er mottatt siste sekund. Sist mottatte tall beholdes i gratt.
- **DEMO**: simulerte kanaler, tydelig merket i gult. Demo kobler fra COM-porten og sender ingenting til roboten.
- **PORTFEIL**: lukk andre terminaler, sjekk riktig port og USB-driver, og forsok igjen.

Programmet viser tekstutdata fra ChannelMonitor, ikke den binaere CRSF-strommen direkte fra Matek. Det kan ikke brukes med RobotControl-sketsjen, siden den ikke sender kanaltekst. De to Arduino-programmene lastes opp hver for seg. Monitoren sender ingen serielle styrekommandoer, men apning av en USB-serieport kan likevel gi reset pa enkelte kort/drivere.

ChannelMonitor sender na omtrent 20 komplette kanallinjer per sekund. Last opp den oppdaterte ChannelMonitor-sketsjen for denne hastigheten. Stolpene animeres med en timer paa 16 ms (faktisk bildefrekvens avhenger av Windows og belastning), med lett tidsbasert utjevning. Bare stolpehoyden utjevnes; tall og prosent viser alltid siste mottatte maaling. Utjevningen har tidskonstant 45 ms og naar omtrent 95 % av et sprang etter 135 ms. Ved signalbrudd stopper animasjonen paa siste faktiske verdi og stolpene blir graa. Den gamle sketsjen fungerer fortsatt, men gir bare fire nye maalinger per sekund.

Fullt standardutslag tilsvarer omtrent 172 / 992 / 1811 i CRSF. Prosentvisningen klippes til +/-100 %, mens raverdien viser hele mottatte 11-bitsverdien. Visningen er ingen maaling av radiosignalstyrke eller LQ.

At 16 felt vises betyr ikke at alle er aktive over radio. Bruk kompatibel 16-kanals ELRS-modus og legg onskede innganger pa CH1-CH16 i radioens Mixes.

## Det som gjenstar pa roboten

- Bekreft vanlig, ikke-invertert CRSF og mottaker-UART 115200 (allerede satt).
- Avklar delt USB/UART pa Orion for det aktuelle kortet: to TX-utganger ma ikke drive D0 samtidig. En ekstern terminaladapter alene isolerer ikke USB-brikkens TX. Se ../README.md og ../docs/KOBLINGER.md for detaljer.
- Koble fra mottakerens TX under opplasting. Fjern Orion TX -> Matek RX for denne losningen.
- Kontroller miks: CH4 = venstre X (antatt etter at CH1 viste seg aa vaere hoyre X), CH2 = hoyre Y og CH5 = SA; SA skal bare gi hoy verdi i valgt armeringsstilling.
- Test kanalmonitoren forst. Senter skal ligge rundt 992. CH4 skal oke mot hoyre, CH2 fremover, og CH5 skal bli hoy bare ved armering.
- Sla av radioen: grafikken skal melde manglende radioinput.
- Last opp RobotControl nar kanalene er riktige. Loft beltene og kontroller motorretning, SA, ny armering etter signalbrudd for gulvtest. Avstandssperren er deaktivert i gjeldende kode.

Programmet er syntakskontrollert, kanalparseren er testet med gyldige/ugyldige tekstlinjer, og det nye stolpeelementet er kompilert med Windows PowerShell 5.1. Hele vinduet er rendret med simulerte kanalverdier og kontrollert visuelt. Den oppdaterte grafikken er ikke testet med fysisk COM-forbindelse her.
