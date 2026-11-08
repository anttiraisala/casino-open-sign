11 STICKS - nuolen 10 tikkua ja CLOSED-valotikku samalla Grove Base Shieldillä
==============================================================================

Tarkoitus
---------
Pidetään kaikki 11 Grove RGB LED -tikkua päällä kuten kyltissä ja katsotaan, kuumeneeko mikään: Grove Base Shield,
Grove-liittimet, tikut, johdot, virtalähde. Nuolen 10 tikkua kytketään liittimiin täsmälleen kuten kyltissä; 11. tikku,
"CLOSED"-valo (kyltin open/closed-merkki), menee ainoaan jäljellä olevaan liittimeen: A3. Lataus Arduino IDE:llä.
Testillä ei ole mitään tekemistä kyltin oman ohjelman kanssa.
(Samankaltainen kuin ../stick_test_direct, mutta kyltin tikkujärjestyksellä, tasaisella CLOSED-valolla ja kyltin kirkkaudella.)

Mitä näet
---------
  - Tikku 11 (A3, CLOSED-valo): kaikki 10 LEDiä tasaisen valkoisena tasolla 80 / 255 koko ajan.
  - Tikut 1-10 (nuoli): valkoinen aalto virtaa kutakin tikkua pitkin (kirkkaus 0..80, keskiarvo noin puolet), joten
    nuolen kuorma on kuin kyltin virtaavassa nuolessa.
  - Painike (pinni 33 -> GND, valinnainen): painallus vaihtaa tilaa FLOW (oletus) <-> ALL ON = kaikki 110 LEDiä tasaisen
    valkoisena tasolla 80, pahin tapaus. Kokeile ALL ONia myös hetki (arvio 2,1 A, arvaus).
  - Kaikki alkaa heti kytkettäessä virta (ei pimeää taukoa).

Tikku, liitin ja nimi (kuten kyltissä; katso ../arrow_sticks/)
  Tikku  1 L-CAP D2        Tikku  7 RAIL-DN-R D8
  Tikku  2 L-WING-UP D3    Tikku  8 R-CAP A0
  Tikku  3 L-WING-DN D4    Tikku  9 R-WING-UP A1
  Tikku  4 RAIL-UP-L D5    Tikku 10 R-WING-DN A2
  Tikku  5 RAIL-UP-R D6    Tikku 11 CLOSED-valo A3
  Tikku  6 RAIL-DN-L D7

Tiedostot
---------
sketch.ino             Testiohjelma. Asetukset alussa (ARROW_LEVEL, CLOSED_LEVEL, START_ALL_ON, ...).
diagram.json           Wokwi-kytkentä (11 nauhaa à 10 pikseliä, simulaatiossa 5V:sta; liitä diagram.json-välilehdelle).
libraries.txt          Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Kytkentä sähkökaaviona (A4 pysty).
layout-diagram.svg/.pdf  Mikä tikku menee mihinkin liittimeen ja mistä lämpöä tunnustellaan (A4 vaaka).
README.txt / README-fi.txt  Tämä tiedosto (englanti / suomi).

Elonmerkki (koko projektin sääntö)
----------------------------------
  - Pinnin 13 LED vilkkuu koko ajan ohjelman ajon aikana (250 ms välein); se käännetään hetkeksi, kun tila vaihtuu,
    ja se vilkkuu kaksi kertaa nopeammin, jos tila vaihtuu usein. Jos se lakkaa vilkkumasta, ohjelma on jumissa tai resetoitu.
  - Serial Monitorin nopeus: 115200 baud (sketch.ino:n alussa ja tulostetaan käynnistyessä). Kerran sekunnissa:
      DBG up=<s> mode=flow|all fps=50 lit=<LEDejä / 110> ma=<arvioitu mA> events=<n> beats=<n>
    Arvioitu virta on arvaus (20 mA per värikanava arvolla 255), ei mittaus.
    Serial Monitor tarvitsee USB-kaapelin, jota EI kytketä, kun 5 V:n virtalähde on päällä (katso alla).

Osat (partco.fi; hinnat ja saatavuus ovat tilanne 2026-11-02, tarkista ennen ostoa)
-----------------------------------------------------------------------------------
  - 11 x Grove RGB LED Stick (10x WS2813 mini) Grove-kaapeleineen (haku "Grove RGB LED Stick"), Arduino Mega 2560 + Grove Base Shield.
  - 1 x Mean Well LRS-50-5, 5 V 10 A, 23,79 EUR  https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x lasisulake/autosulake 10 A (SUA-10A) 0,31 EUR + pidike  https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x kondensaattori 1000 uF 25 V (ELKO105-1000U-25V) 1,73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - Johto 1,5 mm2 syöttöön, valinnainen painike.
  Virtalähteen 230 V:n puoli kuuluu koteloon ja sen saa kytkeä vain osaava henkilö.

Kytkentä (tarkasti; katso wiring-diagram.pdf)
---------------------------------------------
  Tikku n omalla Grove-kaapelillaan yllä olevan taulukon liittimeen (liitin = signaalipinni).
  Shieldin 3V3/5V-kytkin: 5 V.
  Virtalähde V+ -> 10 A sulake -> Megan 5V-pinnit; V- -> Megan GND-pinnit. Käytä pitkän kaksoisriviliittimen päässä
  olevia 5V- ja GND-pinnejä (shield ei peitä niitä), molemmat 5V- ja molemmat GND-pinnit, 1,5 mm2 johto.
  1000 uF kondensaattori syöttöjohtojen väliin, plus +5 V:iin.
  Valinnainen painike D33:n ja GND:n välillä.
  Tikut saavat virtansa vain shieldin ja Grove-liittimien kautta; ei johtoa virtalähteestä tikulle.
  USB-kaapelia EI kytketä, kun virtalähde on päällä.

Päälle kytkentä
---------------
  1. Arduino IDE: kirjasto "Adafruit NeoPixel", kortti Arduino Mega 2560, avaa sketch.ino ja lataa USB:llä (virta pois).
  2. Irrota USB. Kytke tikut ja syöttö. 3. Kytke virta: valot alkavat heti.
  (Kaksi 5 V:n lähdettä rinnakkain voi työntää virtaa USB-porttiin, siksi ei USB:tä virran ollessa päällä.
  Serial Monitoria varten käytä vain USB:tä, virtalähde pois ja vain muutama tikku, tai luota pinnin 13 LEDiin.)

Mitä tarkistetaan / mitä kerrot
-------------------------------
  - Anna pyöriä FLOW-tilassa 30-60 minuuttia. Tunnustele (varovasti) shield, Grove-liittimet, kaapelit, tikkujen
    selät, CLOSED-valotikku (palaa koko ajan) ja virtalähde. Onko mikään lämmin tai kuuma?
  - Sitten ALL ON 5-10 minuuttia (painike). Sama tarkistus. Katkaise virta heti, jos jokin on kuuma tai haisee.
  - Välkkyminen, himmeys, keltainen sävy, viimeinen tikku heikompi tai Arduinon uudelleenkäynnistys (aalto alkaa alusta)?
  - Jos on yleismittari: jännite viimeisellä tikulla (väh. 4,5 V) ja virtalähteen virta.
  - Jos jokin lämpenee tai himmenee: pienennä ARROW_LEVEL / CLOSED_LEVEL (esim. 40).

Testit
------
tests/sticks_11_test_harness.cpp tarkistaa ohjelman logiikan tietokoneella; tests/mutate_sticks_11.py tarkistaa testit mutanteilla:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/s11 tests/sticks_11_test_harness.cpp && /tmp/s11
(Ei ajettu oikealla laitteella eikä Wokwissa; virta-arvot ovat arvioita.)
