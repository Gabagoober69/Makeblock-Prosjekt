# Koblinger og begrensninger

## ELRS i port 5

| Fra Matek R24-D | Til Orion |
| --- | --- |
| Forsyning | 5 V |
| GND | GND |
| TX | RX / D0 |
| RX | Ikke tilkoblet |

Orion TX til Matek RX er klippet og isolert i dette prosjektet. Programmene bruker énveis mottak og sender terminaltekst ut fra Orion; de sender ikke CRSF-telemetri til mottakeren.

Port 5 deler UART med USB-seriebrikken CH340G. Koble fra mottakerens TX før opplasting, enklest ved å ta ut port 5-pluggen. Under opplasting kan mottakeren ellers forstyrre datastrømmen til oppstartslasteren. USB-kanalovervåking med ELRS tilkoblet har fungert på eierens kort. Dette fastslår ikke verdien på eventuelle seriemotstander; ytterligere klipping eller kortmodifikasjon er ikke en forutsetning som er verifisert her.

## Motorer og sensorer

| Del | Tilkobling | Status |
| --- | --- | --- |
| Venstre motor | M1; PWM D6 og retning D7 | Motorfortegn -1 |
| Høyre motor | M2; PWM D5 og retning D4 | Motorfortegn +1 |
| Ultralyd | Port 3, signal D13 | Avlesning og sperre deaktivert |
| IR-enhet | Port 7 | Modell ikke fastslått, ingen kodefunksjon |
| Buzzer | SCL/A5 på kortet | Robotlyd fra CH6; fysisk bryter ON |

Motorfortegn og plassering må bekreftes med beltene løftet. SCL kan ikke brukes til vanlig I²C samtidig med buzzerfunksjonen. Lyd bruker `tone()` på Timer2, mens motor-PWM på D5/D6 bruker Timer0.

Fartsgrensen er 160/255 med seks alkaliske batterier, 9 V nominelt. Motorene er merket 6 V. PWM-grensen er ingen spenningsregulator eller dokumentert beskyttelse mot motoroverbelastning. Strømgrenser og temperatur under tung belastning er ikke målt.

Når ultralydsperren senere aktiveres med `ENABLE_OBSTACLE_STOP=true`, er ønsket oppførsel stopp ved <=5 cm og bare rett revers tillatt, med frigivelse ved >=7 cm. Manglende ekko utløser også sperren. Dette ga tidligere problemer i robottesten og er ikke løst eller validert som kollisjonsbeskyttelse.

## Kilder

- [Matek R24-D](https://www.mateksys.com/?portfolio=elrs-r24)
- [Makeblock Orion pinnekart](https://github.com/Makeblock-official/Makeblock-Libraries/blob/master/src/MeOrion.h)
- [Makeblock ultralydsensor](https://support.makeblock.com/hc/en-us/articles/16579980385047-About-Me-Ultrasonic-Sensor)
- [ExpressLRS kanalmoduser](https://www.expresslrs.org/software/switch-config/)
