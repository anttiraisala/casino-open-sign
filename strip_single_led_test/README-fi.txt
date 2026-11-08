LED STRIP SINGLE LED - yksi pikseli kulkee koko nauhan läpi (kolme testiä yhtä aikaa)
======================================================================================

Tarkoitus
---------
Selvittää, montako pikseliä (ohjainpiiriä) 12 V:n WS2811-nauhassa oikeasti on, ja tarkistaa värijärjestys.
Nauhassa (5 m, 3 LEDiä / 10 cm = 150 LEDiä) on todennäköisesti vain 50 pikseliä: yksi WS2811-piiri ohjaa
3 LEDiä. Testi 1 näyttää sen: palava kohta on 10 cm pitkä ja askeleen 50 jälkeen nauha pysyy pimeänä.
Jos pikseleitä on oikeasti 150, kulkeva valo on 3 cm pitkä ja etenee loppuun asti. Serial Monitor kertoo
molemmissa tapauksissa askeleen.

Tiedostot
---------
sketch.ino             Ohjelma (asetukset alussa). Baudinopeus 115200.
diagram.json           Wokwi-kytkentä (3 nauhaa à 150 pikseliä, 330 ohmin vastukset; liitä diagram.json-välilehdelle).
libraries.txt          Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Sähkökaavio (A4 pysty).
layout-diagram.svg/.pdf  Mitä kussakin kolmessa testissä katsotaan (A4 vaaka).
README.txt / README-fi.txt  Tämä tiedosto (englanti / suomi).

Kolme testiä (ne pyörivät yhtä aikaa, yksi per datapinni)
---------------------------------------------------------
  Testi 1, D9:  YKSI valkoinen pikseli kulkee pikselistä 1 pikseliin 150 (150 ms / askel, 22,5 s), sitten alusta.
  Testi 2, D10: yksi pikseli kulkee (60 ms / askel). Väri vaihtuu joka kierroksella: punainen, vihreä, sininen,
                valkoinen. Kierroksen 1 pitää näyttää PUNAISELTA. Jos se on vihreä tai sininen, vaihda
                COLOR_ORDER (katso alla).
  Testi 3, D11: vilkkuva viivain (500 ms päällä, 500 ms pois): joka 10. pikseli sininen, joka 50. valkoinen
                (pikselit 1, 51, 101 valkoisia). Laske valkoiset: se kertoo 50 pikselin lohkojen määrän.
Voit kytkeä vain yhden nauhan: testille 1 D9, testille 2 D10, testille 3 D11. Käyttämättömät pinnit eivät haittaa.

Kytkentä (tarkasti)
-------------------
    Arduino D9   -> 330 ohmin vastus R1 -> nauhan 1 DIN (pää, josta painetut nuolet osoittavat poispäin)
    Arduino D10  -> 330 ohmin vastus R2 -> nauhan 2 DIN
    Arduino D11  -> 330 ohmin vastus R3 -> nauhan 3 DIN
    12 V:n virtalähde +  -> kunkin nauhan +12 V (vain virtalähteestä, ei koskaan Arduinon pinniin)
    12 V:n virtalähde -  -> kunkin nauhan GND JA Arduinon GND (yhteinen maa)
    Arduino              -> USB-kaapeli tietokoneeseen (virta + Serial Monitor)
    Valinnainen: 1000 uF kondensaattori, väh. 25 V, +12 V ja GND välille nauhan alussa (plus +12 V:iin).
  Ei 12 V:a mihinkään Arduinon pinniin, eikä nauhan +12 V koskaan mene Arduinon 5V-pinniin.
  Datapinni antaa 5 V:a 12 V:n nauhalle: tämä yleensä toimii, mutta sitä ei ole varmistettu; testi näyttää sen.
  Teho: nauha on 7 W/m täydellä valkoisella; ohjelma sytyttää YHDEN pikselin tasolla TEST_LEVEL 80 / 255,
  joten virta on pieni. 12 V 1 A virtalähde riittää kaikille kolmelle nauhalle.
  Katso kuva: wiring-diagram.svg.

Ostoslista (partco.fi; hinnat ja saatavuus ovat tilanne 2026-11-02, tarkista ennen ostoa)
  - 12 V osoitettava nauha WS2811, 5 m (sama nauha, joka sinulla jo on; SKU LED-N-RGB-W2812X150-5M):
    https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
  - 12 V DC -virtalähde, väh. 1 A (pistorasiasovitin) ja ruuviliitosjatkoliitin
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 3 kpl 330 ohmin vastuksia, 1000 uF 25 V kondensaattori (valinnainen), hyppylankoja.
  (Nämä osat eivät kuulu kyltin osalistaan; katso ../parts-list.*.)

Ajaminen
--------
Oikea laite: avaa sketch.ino Arduino IDE:ssä (kirjasto Adafruit NeoPixel), kortti Arduino Mega 2560,
lataa ja avaa Serial Monitor nopeudella 115200 baud.
Wokwi: uusi Arduino Mega -projekti, liitä sketch.ino ja diagram.json, lisää kirjasto "Adafruit NeoPixel",
paina Play. Simulaattori näyttää ihanteelliset pikselit, joten se vain osoittaa ohjelman toimivan.
Elonmerkki: sisäänrakennettu LED (pinni 13) vilkkuu ohjelman ajon aikana (hetkeksi käännetty kierroksen
päättyessä, nopeampi kun muutoksia on paljon). Serial Monitor tulostaa yhden rivin sekunnissa:
    DBG up=<s> t1=<askel>/150 t2=<askel>/150 pass=<0..3> t3=on|off events=<n> beats=<n>

Tulosten lukeminen
------------------
  - Testi 1: merkitse askeleen numero (t1=...), kun nauha pimenee. Se on oikea pikselimäärä
    (noin 50, jos yksi piiri ohjaa 3 LEDiä). Merkitse montako LEDiä palaa kerrallaan (1 vai 3).
  - Testi 2: kierroksen 1 väri (odotus punainen). Väärät värit = väärä värijärjestys.
  - Testi 3: montako valkoista merkkiä nauhalla näkyy.
  - Mikään ei pala: tarkista DIN-pää (nuolet), yhteinen GND, 12 V:n syöttö; kokeile tasonmuunninta (74HCT125) viimeisenä.

Kerro minulle (täytä)
  1. Askel, jossa testi 1 pimenee: ______   Kerrallaan palavat LEDit: ______
  2. Testin 2 kierrosten 1 / 2 / 3 värit: ______ / ______ / ______
  3. Testissä 3 näkyvät valkoiset merkit: ______
  4. Välkkyminen, kuollut pikseli, muuta: ______

Asetusten muuttaminen
---------------------
sketch.ino:n alussa: STRIP_PIXELS (150), COLOR_ORDER (NEO_GRB + NEO_KHZ800; kokeile NEO_RGB + NEO_KHZ800),
TEST_LEVEL (80), STEP1_MS, STEP2_MS, RULER_MS, RULER_STEP (10), RULER_BIG_STEP (50), PIN_STRIP_1..3 (9, 10, 11).

Testit
------
tests/strip_single_led_test_harness.cpp tarkistaa ohjelman tietokoneella, tests/mutate_strip_single_led.py
tarkistaa testit mutanteilla:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/ssl tests/strip_single_led_test_harness.cpp && /tmp/ssl
(Ei ajettu oikealla laitteella eikä Wokwissa.)
