# SpaceWars – ESP32 med OLED og joystick

Et lille rumskydespil til ESP32 Dev Module med en sort-hvid 128×64 OLED-skærm
og et HW-504-joystick. Denne version er tilpasset fra
[VolosR/SpaceWars](https://github.com/VolosR/SpaceWars) og bruger PlatformIO
med Arduino-framework. Spillets tekst på skærmen er på engelsk.

## Hvad går spillet ud på?

Styr dit kamp-rumskib, undgå fjendens skud, og gennemfør alle fem baner.
Skibet skyder automatisk, så du kan koncentrere dig om at flyve og samle powerups.
Baggrunden har kun otte små stjerner, som bevæger sig én pixel hver 140 ms.
Stjerner skjules inden for tre pixels af fjendeskud og missiler (inklusive deres
udstødning), så farlige projektiler er lettere at se på den sort-hvide skærm.

Hver bane starter med en bølge af små fjender, der kommer ind fra højre og
flyver mod dig. De dukker op med tilfældige intervaller på 1–3 sekunder,
og der kan være op til tre på skærmen samtidig. Fjenderne sigter mod spilleren,
og de senere baner har hurtigere fjender og hyppigere skud.

Der er tre typer små fjender. Standard og scout har ét liv; gunner har tre:

| Type | Udseende og adfærd |
| --- | --- |
| Standard | Samme alien-sprite på alle baner, med bevægende antenner/fødder og blinkende øjne; ét målrettet skud ad gangen |
| Scout | Venstrevendt, missillignende alien med animerede bagfinner og blinkende øjne; 6 pixels pr. opdatering på alle fem baner, skyder ikke og styrer løbende direkte mod spilleren. Eksploderer ved kontakt og giver skade |
| Gunner | Tre liv, lille flyvende tallerken med blinkende pilotøjne og roterende kantlys; lav fart og to målrettede skud ad gangen |

Typerne blandes på alle baner. Gunnerens skudinterval er 25 % længere end
standardfjendens på samme bane. Antal fjender, spawn-intervaller og bosser er uændrede.
Standard og gunner flyver stadig vandret; scouten følger spillerens aktuelle position.
Standardfjendernes gennemsnitsfart er 1, 1,25, 1,5, 1,75 og 2 pixels pr.
bevægelsesopdatering på bane 1–5. Skudintervallet falder fra 3,75 til 3, 2,5,
2,25 og 2 sekunder. Standardfjender skyder altid kun én kugle ad gangen.
Gunnerens fart er uændret: i gennemsnit 0,5, 0,5, 1, 1 og 1,5 pixels pr.
bevægelsesopdatering på bane 1–5.
Gunners og bosser viser et kort, pulserende hit-flash, når de faktisk mister liv.
Bossens introduktion og aktive skjold blokerer fortsat skade og udløser ikke hit-flashet.
Scouts, der flyver ud af skærmen eller kolliderer med dig, tæller som afsluttet i bølgen,
men giver ingen kill-point eller powerups. Dit skjold beskytter også mod sammenstød.

Når bølgen er besejret, flyver banens boss ind fra højre. Under introduktionen
med teksten `BOSS BATTLE` kan hverken spilleren eller bossen skyde.
Efter en besejret boss vises en victory-skærm: klik på joysticket for at fortsætte.
Efter bane 5 har du vundet. Vælg `ENDLESS` eller `RESTART` med joystick op/ned,
og klik for at bekræfte. `RESTART` starter et nyt spil fra bane 1.
Menuskærmen viser en animeret fighter og din highscore. Victory-skærmen viser
et trofæ, den besejrede boss og din score (eller de to valg efter bane 5).
Game over viser separate `SCORE`- og `BEST`-felter; klik for at vende tilbage til menuen.
De markerede knapper og instruktioner bruger samme layout på alle tre skærme.

Du starter med tre liv og kan have op til fem. Mister du alle liv, er det game over.
Efter mistet liv er spilleren usårlig i 3 sekunder og blinker efter eksplosionen.
Din highscore gemmes i ESP32'ens hukommelse og bevares, når strømmen afbrydes.
Eksplosioner fra fjender, bosser og spilleren varer 0,96 sekunder; hele animationen
er strakt ud, inklusive glimt, fragmenter og gløder.

### Combo

Skyd fjender ned i træk uden at miste liv: efter 3 kills får du `x2`, efter 6 får
du `x3`. Multiplikatoren gælder pointet for det dræbende hit; almindelige hits på
bosser giver stadig ét point. Et lille `x2`/`x3` vises øverst til højre.
Comboen fortsætter mellem baner, men nulstilles, når du faktisk mister et liv.
Skade stoppet af skjold eller midlertidig usårlighed bryder ikke comboen.
Missiler giver fortsat to point og øger ikke comboen. Scout-sammenstød og
fjender, der forlader skærmen, tæller heller ikke som kills.

### Endless mode

Vælg `ENDLESS` efter bane 5 for at fortsætte med samme score, liv og combo.
Aktive powerups udløber ved overgangen som mellem normale baner.

- Bølge 1 har 14 små fjender. Hver bølge tilføjer to, op til 60 pr. bølge.
- De fem bosser gentages i rækkefølge. Bossens liv stiger med fire pr. endless-bølge,
  op til 200 ekstra liv oven i dens normale liv.
- Fjendernes skudinterval falder fra 2 sekunder mod et minimum på 0,9 sekunder
  (gunner skyder 25 % sjældnere; Dark Comet har stadig sit hurtigere angreb).
  Void Stalker har 3,2 sekunders cooldown plus 0,6 sekunders opladning i kampagnen,
  altså cirka 3,8 sekunder mellem missilaffyringer. I endless falder cooldown med
  0,16 sekunder pr. bølge, ned til mindst 1,44 sekunder; opladning er stadig 0,6 sekunder.
- Spawn-intervallet falder gradvist fra 1–3 sekunder til 0,5–1,5 sekunder.
- Standardfjender starter på 3 pixels pr. opdatering og stiger med én hver tredje
  bølge, op til 6. Gunner bevæger sig med halv standardfart.
- Scouts beholder altid **6 pixels pr. opdatering** og ét liv.
- Efter hver boss: klik for næste bølge. Spillet fortsætter, indtil du mister alle liv.

`W` i toppen viser endless-bølgen. Difficulty-grænserne forhindrer, at farten og
spawn-rytmen bliver uhåndterlige; der er ikke en normal afsluttende endless-boss.

| Bane | Små fjender | Boss | Bossens liv | Særlige angreb |
| --- | ---: | --- | ---: | --- |
| 1 | 4 | Dark Comet | 24 | Vifte af skud |
| 2 | 6 | Void Stalker | 36 | Varmesøgende missiler, som kan skydes ned |
| 3 | 8 | Nebula Queen | 36 | Rundt skjold; ram bossen, når skjoldet er åbent |
| 4 | 10 | Star Wraith | 44 | Laser med varsling, så du kan nå at undvige |
| 5 | 12 | Omega Prime | 54 | Vifteskud, missiler og laser; hurtigere angreb under halvt liv |

### Powerups

Flyv ind i et powerup-ikon for at samle det op. En lille flydende tekst viser,
hvad du har fået.

Mens en tidsbegrænset powerup er aktiv, vises en lille nedtællingsbjælke nederst:
`SH` = skjold, `SG` = shotgun og `L` = laser. Bjælken bliver kortere, indtil effekten
udløber. Flere timere kan vises samtidig; et nyt pickup af samme type fornyer varigheden.

| Powerup | Effekt |
| --- | --- |
| Hjerte / Extra Life | Giver ét ekstra liv, op til maks. fem |
| Skjold / Shield | Beskytter mod skade i 6 sekunder |
| Shotgun | Tre skud med et smalt spread i 8 sekunder |
| Lyn / Laser | En animeret laserstråle i 4 sekunder |

## Styring

- Bevæg joysticket for at flyve op, ned, til venstre og til højre.
- Skibet skyder automatisk; du skal ikke holde en knap nede.
- Tryk joysticket ned (`SW`) for at starte spillet, gå til næste bane eller genstarte.
- Lad joysticket stå i midten, når du tænder eller genstarter ESP32'en.
  Spillet kalibrerer joystickets midterposition ved opstart.

Styringen er roteret **90 grader mod venstre** i koden, så den passer til den
nuværende montering. Hvis du vender joystickmodulet anderledes efter genopbygning,
kan retningerne derfor føles forkerte. Behold samme orientering, eller tilpas
`updatePlayer()` i `src/main.ino`.

## Hardware

- ESP32 Dev Module.
- 1,3" I2C OLED, 128×64 pixels, med SH1106-driver og adresse `0x3C`.
- HW-504 analogt joystick med indbygget trykknap.
- Buzzer til lyde; koden sender tonesignalet på GPIO27.
- Breadboard, jumperledninger og USB-kabel til ESP32'en.

Den aktive kode bruger **U8g2**, ikke TFT_eSPI. En skærm med en anden driver
(for eksempel SSD1306) kræver en anden skærmdefinition i koden.

## Ledningsoversigt – sådan samler du det igen

**Tag USB og anden strøm fra, før du flytter ledninger.** Brug GPIO-numrene
på ESP32'en, ikke benenes rækkefølge. `G32`, `IO32` og `GPIO32` betyder samme pin.

### OLED-skærm

| Pin på OLED | Til ESP32 |
| --- | --- |
| VCC / VDD | 3V3, hvis dit OLED-modul understøtter 3,3 V forsyning |
| GND | GND |
| SDA | GPIO21 |
| SCL / SCK | GPIO22 |

Følg pin-navnene på dit modul: rækkefølgen af VCC, GND, SDA og SCL kan variere.
Dette er en **I2C-forbindelse**; `SCK` på nogle I2C-moduler er navnet for SCL.

### HW-504-joystick

| Pin på joystick | Til ESP32 |
| --- | --- |
| VCC / +5V | **3V3** |
| GND | GND |
| VRx | GPIO32 |
| VRy | GPIO33 |
| SW | GPIO25 |

Selvom joystickets forsyningspin kan være mærket `+5V`, skal den i denne opsætning
forbindes til **3V3**. Joystickets analoge udgange må ikke sende 5 V ind i ESP32'en.
`SW` bruger ESP32'ens interne pull-up, så der er ikke brug for en ekstern
pull-up-modstand til joystickknappen.

De tidligere fire separate retningsknapper bruges ikke længere.
GPIO18, GPIO19 og GPIO23 skal derfor ikke forbindes til knapper for dette spil.
GPIO25 bruges nu til joystickets `SW`.

### Buzzer

For en lille, direkte GPIO-egnet **passiv piezo-buzzer med to ben**:

| Pin på buzzer | Til ESP32 |
| --- | --- |
| + / signal | GPIO27 |
| − | GND |

Har din buzzer tre ben (`S`, `+`, `−`), eller kræver den mere strøm end en GPIO
kan levere, skal forbindelsen tilpasses modulet. Signalbenet bruger GPIO27,
men forsyning og eventuel transistor afhænger af den konkrete buzzer.
Forbind ikke en almindelig højttaler direkte til GPIO27.

### Fælles strøm på breadboardet

Forbind ESP32 **3V3** til breadboardets plus-skinne og ESP32 **GND** til minus-skinnen.
Joystick og en 3,3 V-egnet OLED kan derefter dele disse skinner.
Alle komponenter skal have fælles GND.

Nogle breadboards har strøm-skinner, der er afbrudt i midten. Hvis du bruger begge
halvdele, skal de forbindes med jumperledninger. Kontrollér også, at signalben
ikke deler den samme forbundne række ved en fejl.

## Byg og upload med PlatformIO

1. Åbn projektmappen, som indeholder `platformio.ini`, i VS Code.
2. Tilslut ESP32'en via USB, og lad joysticket stå i midten.
3. Klik **Build** for at kompilere. PlatformIO installerer U8g2 og Tone32 via `lib_deps`.
4. Klik **Upload** for at overføre spillet til ESP32'en.
5. Klik på joysticket for at starte spillet.

Projektet er sat op med `board = esp32dev` og `framework = arduino`.
Hvis upload sidder fast ved `Connecting...`, kan det være nødvendigt at holde
ESP32'ens **BOOT**-knap nede, mens forbindelsen oprettes.

Den kode, der bliver bygget, ligger i **`src/main.ino`**. Den oprindelige
`ttgoGameConsole.ino` og de gamle billed-headere i projektroden er ikke den
aktive version af spillet.

## Hvis noget ikke virker efter genopbygning

- **Sort skærm:** Kontrollér strøm, fælles GND, SDA → GPIO21 og SCL → GPIO22.
  Koden forventer en SH1106-skærm med I2C-adresse `0x3C`.
- **Skibet bevæger sig af sig selv:** Lad joysticket være i midten og tryk RESET.
  Kontrollér også VRx, VRy og GND.
- **Forkerte retninger:** Kontrollér, at VRx og VRy ikke er byttet, og at joystickets
  orientering svarer til den roterede styring.
- **Klik starter ikke spillet:** Kontrollér SW → GPIO25 og joystickets GND.
- **Ingen lyd:** Kontrollér GPIO27, fælles GND og at buzzerens type passer til tilslutningen.
