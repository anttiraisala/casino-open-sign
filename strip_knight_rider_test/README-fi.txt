LED STRIP KNIGHT RIDER - yksi valo kulkee edestakaisin kolmella nauhalla
========================================================================

Tarkoitus
---------
Demo kolmelle 12 V:n WS2811-nauhalle, joissa kussakin on 50 pikseliä (5 m rulla: yksi ohjainpiiri 3 LEDille, eli
50 piiriä, 150 LEDiä). Yksi pikseli palaa kerrallaan ja pomppii päästä päähän kuin Knight Riderin valo.
Demo näyttää myös, miten osoitus toimii: pikseli 1 on DIN-päässä oleva piiri, ja palava pikseli valitaan
pelkästään sen paikalla Arduinon lähettämässä datassa. Katso "Miten osoitus toimii" alta.

  Pinni D9:  PUNAINEN valo, 1 pikseli sekunnissa   (yksi kierros päästä päähän ja takaisin = 98 s)
  Pinni D10: SININEN valo, 4 pikseliä sekunnissa   (yksi kierros = 24,5 s)
  Pinni D11: VIHREÄ valo, 10 pikseliä sekunnissa   (yksi kierros = 9,8 s)
Palava pikseli on 10 cm pitkä (3 LEDiä), jos nauha on rakennettu kuten yllä.
Kierros on 98 askelta: pikseli 1, 2, ... 50, 49, ... 2, sitten taas 1 (päätypikselit näytetään kerran per käännös).

Tiedostot
---------
sketch.ino             Ohjelma (asetukset alussa). Baudinopeus 115200.
diagram.json           Wokwi-kytkentä (3 nauhaa à 50 pikseliä, 330 ohmin vastukset; liitä diagram.json-välilehdelle).
libraries.txt          Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf  Sähkökaavio (A4 pysty).
layout-diagram.svg/.pdf  Mitä pitäisi nähdä (A4 vaaka).
README.txt / README-fi.txt  Tämä tiedosto (englanti / suomi).

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
  Datapinni antaa 5 V:a 12 V:n nauhalle: tämä yleensä toimii, mutta sitä ei ole varmistettu.
  Teho: yksi palava pikseli per nauha tasolla TEST_LEVEL 80 / 255 vie hyvin vähän virtaa; 12 V 1 A riittää reilusti.
  Voit kytkeä vain yhden tai kaksi nauhaa; käyttämättömät pinnit eivät haittaa. Katso kuva: wiring-diagram.svg.

Ostoslista (partco.fi; hinnat ja saatavuus ovat tilanne 2026-11-02, tarkista ennen ostoa)
  - 12 V osoitettava nauha WS2811, 5 m (SKU LED-N-RGB-W2812X150-5M): https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
  - 12 V DC -virtalähde, väh. 1 A (pistorasiasovitin) ja ruuviliitosjatkoliitin
    DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA (SKU DCJ21RUU): https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 3 kpl 330 ohmin vastuksia, 1000 uF 25 V kondensaattori (valinnainen), hyppylankoja.
  (Ei kuulu kyltin osalistaan; katso ../parts-list.*.)

Miten osoitus toimii (lyhyesti)
-------------------------------
Piireillä ei ole osoitetta. Jokainen piiri ottaa DIN:iin tulevan datan ensimmäiset 24 bittiä (yksi väri), pitää
ne itsellään ja välittää loput DOUT:ista seuraavalle. Siis DIN:in vieressä oleva piiri saa ensimmäisen värin
(pikseli 1), toinen piiri toisen jne. Datan jälkeen yli 50 mikrosekunnin tauko lukitsee kaikki värit.
Jos katkaiset nauhan leikkausviivalta, pätkä on täysi nauha: syötä DIN-päähän, ja sen vieressä oleva piiri on
pikseli 1. Viimeinen piiri ei "tiedä" olevansa viimeinen, sen DOUT ei vain johda mihinkään. Liian monelle pikselille
lähetetyt värit katoavat, joten STRIP_PIXELS:n pitää vastata todellista määrää. Syöttö DOUT-päästä ei toimi (data
kulkee yhteen suuntaan).

Ajaminen
--------
Oikea laite: avaa sketch.ino Arduino IDE:ssä (kirjasto Adafruit NeoPixel), kortti Arduino Mega 2560, lataa ja avaa
Serial Monitor nopeudella 115200 baud. Wokwi: uusi Arduino Mega -projekti, liitä sketch.ino ja diagram.json, lisää
kirjasto "Adafruit NeoPixel", paina Play.
Elonmerkki: sisäänrakennettu LED (pinni 13) vilkkuu ohjelman ajon aikana (hetkeksi käännetty, kun valo kääntyy päässä).
Serial Monitor tulostaa yhden rivin sekunnissa:
    DBG up=<s> d9=<pikseli>/50 d10=<pikseli>/50 d11=<pikseli>/50 events=<n> beats=<n>

Asetusten muuttaminen
---------------------
sketch.ino:n alussa: STRIP_PIXELS (50), COLOR_ORDER (NEO_GRB + NEO_KHZ800; kokeile NEO_RGB + NEO_KHZ800, jos punainen
ei ole punainen), TEST_LEVEL (80), STEP1_MS (1000), STEP2_MS (250), STEP3_MS (100), PIN_STRIP_1..3 (9, 10, 11).

Testit
------
tests/strip_knight_rider_test_harness.cpp tarkistaa ohjelman tietokoneella, tests/mutate_strip_knight_rider.py
tarkistaa testit mutanteilla:
    g++ -std=c++17 -O1 -w -Itests -o /tmp/kr tests/strip_knight_rider_test_harness.cpp && /tmp/kr
(Ei ajettu oikealla laitteella eikä Wokwissa.)
