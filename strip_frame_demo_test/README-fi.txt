STRIP FRAME DEMO TEST - kolme värivaloa 18 pikselin kehysnauhalla, sykkivä yksittäinen pikseli ja väripalkki
================================================================================================

(Omistaja kutsui sitä nimellä "Street Frame Demo Test"; tulkittu "Strip Frame Demo Test", kansio strip_frame_demo_test.)

Tarkoitus
---------
Demo kyltin kehyksen ledinauhalle. Käytössä sama 12 V:n WS2811-nauha kuin strip_knight_rider_testissä (yksi ohjainpiiri
= yksi "pikseli" = 3 LEDiä = 10 cm).

  Pinni D9:  KEHYSNAUHA, 18 pikseliä (1,8 m, 54 LEDiä) koko kyltin ympäri (60 x 30 cm). Nauha alkaa vasemmasta alanurkasta
             ja kiertää vastapäivään:
               pikselit  1.. 6  alareuna, vasemmalta oikealle
               pikselit  7.. 9  oikea reuna, ylöspäin
               pikselit 10..15  yläreuna, oikealta vasemmalle
               pikselit 16..18  vasen reuna, alaspäin (18 on heti pikselin 1 yläpuolella)
             Samalla nauhalla kulkee kolme Knight Rider -valoa. Niiden reitti alkaa VASEMMASTA YLÄKULMASTA (pikseli 15) ja
             kiertää MYÖTÄPÄIVÄÄN: yläreuna oikealle (15..10), oikea reuna alas (9..7), alareuna vasemmalle (6..1), vasen
             reuna ylös (18..16). Pikselissä 16, reitin päässä, valo pomppaa takaisin ja kulkee samaa reittiä pikseliin 15
             asti, ja niin edelleen:
               PUNAINEN  2 pikseliä sekunnissa      (500 ms per askel, kierros 17 s)
               VIHREÄ    1,3 x punaista nopeammin   (385 ms per askel, kierros 13,09 s)
               SININEN   1,5 x punaista nopeammin   (333 ms per askel, kierros 11,32 s)
             Ne ajautuvat eri vaiheeseen ja värit SUMMAUTUVAT, kun valot ovat samalla pikselillä: punainen + vihreä =
             keltainen, vihreä + sininen = syaani, punainen + sininen = magenta, kaikki kolme = valkoinen.
             Päätypikselit näytetään kerran per käännös (kierros on 34 askelta).
  Pinni D10: YKSITTÄINEN pikseli (yksi piiri, 3 LEDiä). Pulssi pimeä -> kirkas -> pimeä 3 sekunnin välein; väri vaihtuu
             joka pulssilla: punainen, oranssi, keltainen, vihreä, syaani, sininen, violetti, valkoinen, sitten taas alusta.

  Pinni D11: VÄRIPALKKI, 6 pikseliä (60 cm, 18 LEDiä). Sarja punainen, vihreä, sininen, syaani, violetti, valkoinen näkyy
             yksi väri per pikseli ja siirtyy yhden pikselin eteenpäin sekunnin välein kohti pikseliä 6; pikseliltä 6
             poistuva väri tulee takaisin pikselille 1.

Kirkkauden huippu on TEST_LEVEL 80 / 255 (sama kuin muissa nauhatesteissä).
Ei kuulu kyltin varsinaiseen ohjelmaan; kehysnauhan suunnalla ja paikalla on merkitystä vain tuloksen lukemisessa.

Tiedostot
---------
sketch.ino             Ohjelma (asetukset alussa). Baudinopeus 115200.
diagram.json           Wokwi-kytkentä (18 pikselin nauha D9:ssä, 1 pikselin nauha D10:ssä, 6 pikselin nauha D11:ssä, 330 ohmin vastukset; liitä diagram.json-välilehdelle).
libraries.txt          Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Sähkökaavio (A4 pysty).
layout-diagram.svg/.pdf  Kehyksen reitti ja mitä pitäisi nähdä (A4 vaaka).
README.txt / README-fi.txt  Tämä tiedosto (englanti / suomi).

Kytkentä (tarkasti)
-------------------
    Arduino D9   -> 330 ohmin vastus R1 -> kehysnauhan DIN, 18 pikseliä (pää, josta painetut nuolet osoittavat poispäin)
    Arduino D10  -> 330 ohmin vastus R2 -> yksittäisen pikselin pätkän DIN (yksi piiri = 3 LEDiä, leikattu samalta rullalta)
    Arduino D11  -> 330 ohmin vastus R3 -> väripalkin DIN, 6 pikseliä (60 cm:n pätkä samalta rullalta)
    12 V:n virtalähde +  -> kaikkien kolmen nauhan +12 V (vain virtalähteestä, ei koskaan Arduinon pinniin)
    12 V:n virtalähde -  -> kaikkien kolmen nauhan GND JA Arduinon GND (yhteinen maa)
    Arduino              -> USB-kaapeli tietokoneeseen (virta + Serial Monitor)
    Valinnainen: 1000 uF kondensaattori, väh. 25 V, +12 V ja GND välille nauhan alussa (plus +12 V:iin).
  Ei 12 V:a mihinkään Arduinon pinniin, eikä nauhan +12 V koskaan mene Arduinon 5V-pinniin.
  Datapinni antaa 5 V:a 12 V:n nauhalle: tämä yleensä toimii, mutta sitä ei ole varmistettu.
  Teho: enintään noin kymmenen palavaa pikseliä tasolla TEST_LEVEL 80 / 255 vie hyvin vähän virtaa; 12 V 1 A riittää reilusti.
  Katso kuva: wiring-diagram.svg. Voit testata myös vain osalla nauhoista.

Ostoslista (partco.fi; hinnat ja saatavuus ovat tilanne 2026-11-02, tarkista ennen ostoa)
  - 12 V osoitettava nauha WS2811, 5 m (SKU LED-N-RGB-W2812X150-5M): https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
    (leikkaa leikkausviivoilta: 18 pikseliä D9:lle, 1 pikseli D10:lle ja 6 pikseliä D11:lle (yhteensä 25 pikseliä); loput jää muihin testeihin)
  - 12 V DC -virtalähde, väh. 1 A (pistorasiasovitin) ja ruuviliitosjatkoliitin
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 3 kpl 330 ohmin vastuksia, 1000 uF 25 V kondensaattori (valinnainen), hyppylankoja.
  (Ei kuulu kyltin osalistaan; katso ../parts-list.*. Jos sinulla on jo strip_knight_rider_testin osat, mitään uutta ei tarvita.)

Miten osoitus toimii (lyhyesti)
-------------------------------
Piireillä ei ole osoitetta. Jokainen piiri ottaa DIN:iin tulevan datan ensimmäiset 24 bittiä (yksi väri), pitää
ne itsellään ja välittää loput DOUT:ista seuraavalle. Siis DIN:in vieressä oleva piiri saa ensimmäisen värin
(pikseli 1), toinen piiri toisen jne. Datan jälkeen yli 50 mikrosekunnin tauko lukitsee kaikki värit.
Liian monelle pikselille lähetetyt värit katoavat, joten FRAME_PIXELS:n ja SINGLE_PIXELS:n pitää vastata todellisia
määriä. Syöttö DOUT-päästä ei toimi (data kulkee yhteen suuntaan).

Ajaminen
--------
Oikea laite: avaa sketch.ino Arduino IDE:ssä (kirjasto Adafruit NeoPixel), kortti Arduino Mega 2560, lataa ja avaa
Serial Monitor nopeudella 115200 baud. Wokwi: uusi Arduino Mega -projekti, liitä sketch.ino ja diagram.json, lisää
kirjasto "Adafruit NeoPixel", paina Play.
Elonmerkki: sisäänrakennettu LED (pinni 13) vilkkuu ohjelman ajon aikana (hetkeksi käännetty, kun kehysvalo kääntyy päässä
ja jokaisen uuden pulssivärin alussa; kaksi kertaa nopeammin, kun muutoksia tulee paljon). Serial Monitor tulostaa
yhden rivin sekunnissa:
    DBG up=<s> red=<pikseli> green=<pikseli> blue=<pikseli> d10=<väri 0-7>:<taso 0-80> d11=<siirtymä 0-5> events=<n> beats=<n>
(nauhan pikselit 1..18; värinumerot 0 punainen, 1 oranssi, 2 keltainen, 3 vihreä, 4 syaani, 5 sininen, 6 violetti, 7 valkoinen.)

Asetusten muuttaminen
---------------------
sketch.ino:n alussa: FRAME_PIXELS (18), ROUTE_START_PIXEL (15), SINGLE_PIXELS (1), COLOR_ORDER (NEO_GRB + NEO_KHZ800; kokeile NEO_RGB + NEO_KHZ800,
jos punainen ei ole punainen), TEST_LEVEL (80), STEP_RED_MS (500), STEP_GREEN_MS (385), STEP_BLUE_MS (333), PULSE_TICKS (150,
yhdessä PULSE_UPDATE_MS 20:n kanssa 3 s per pulssi), BAR_PIXELS (6), BAR_STEP_MS (1000), PIN_FRAME (9), PIN_SINGLE (10), PIN_BAR (11).

Testit
------
tests/strip_frame_demo_test_harness.cpp tarkistaa ohjelman tietokoneella, tests/mutate_strip_frame_demo.py tarkistaa
testit mutanteilla:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/sf tests/strip_frame_demo_test_harness.cpp && /tmp/sf
    python3 tests/mutate_strip_frame_demo.py
(Ei ajettu oikealla laitteella eikä Wokwissa.)
