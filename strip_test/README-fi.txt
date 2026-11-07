LED-NAUHAN TESTI - selvitä, millainen nauha on
===============================================

Tämä on README.txt:n suomennos. Jos tiedostot poikkeavat toisistaan, englanninkielinen README.txt on pääversio.

Tarkoitus
---------
Kyltti on suunniteltu 12 V:n osoitteelliselle nauhalle "RGB-LEDNAUHA 30kpl/m WS2811 5m" (partco.fi,
tuotekoodi LED-N-RGB-W2812X150-5M): https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103
Tuotesivu ei kerro kaikkea, mitä tarvitsemme tietää. Leikkaa nauhasta lyhyt pätkä, aja tämä testi ja kerro,
mitä näet. Sen jälkeen pääohjelma voidaan tehdä juuri tälle nauhalle.

Mitä halutaan selvittää
  1. Värijärjestys (RGB, GRB, BRG, ...) ja bittinopeus (800 kHz vai 400 kHz), jota nauha tarvitsee.
  2. Ohjaako yksi ohjainpiiri 1 LEDiä vai 3 LEDiä? (Todennäköisesti 3: silloin "pikseli" on 10 cm pitkä.)
  3. Toimiiko nauha Arduinon 5 V:n datasignaalilla ilman tasonmuunninta?
  4. Muut poikkeavuudet: välkkyminen, rikkinäiset pikselit, nauhan oikea leveys, nauhaan painettu teksti.

Tiedostot
---------
sketch.ino         Testiohjelma. Asetukset ovat tiedoston alussa.
diagram.json       Wokwi-kytkentä ohjelman kokeilemiseen simulaattorissa (liitä diagram.json-välilehdelle).
libraries.txt      Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf   Kytkentä sähköisenä kaaviona (A4).
README.txt         Englanninkielinen versio.   README-fi.txt   Tämä tiedosto (suomi).

Mitä tarvitset testiin (oikea nauha)
------------------------------------
  - Lyhyt pätkä nauhaa: 3-6 kappaletta 10 cm:n paloja (30-60 cm). Leikkaa VAIN merkityistä leikkauskohdista.
    Juota johdot DIN-pään liitospaikkoihin (+12 V, DIN, GND) tai käytä krokotiilijohtoja.
  - 12 V:n tasavirtalähde, vähintään 1 A (pistorasiaan kytkettävä adapteri on turvallisin: ei verkkojännitteen johdotusta).
    Lyhyelle pätkälle (30-60 cm) kirkkaudella 60 riittää jopa USB:stä 12 V:ksi nostava muunnin ADA2778 (500 mA):
    https://www.partco.fi/fi/teholaehteet/dcdc-jaennitemuuntimet/step-up-dcdc-jaennitettae-nostava/23817-ada2778.html
    Partcolla on ruuviliittimellinen DC-pistokkeen jatkoliitin: DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA
    (tuotekoodi DCJ21RUU) https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875
  - 330 ohmin vastus (R1) datajohtoon.
  - Valinnaiset: painike (pinni 3 -> GND) ja vähintään 25 V:n 1000 uF kondensaattori nauhan virran yli.
  - Kytkentäjohtoja. Arduino Mega saa virtansa USB:stä.
  (Nämä osat eivät kuulu kyltin osalistaan, katso ../parts-list.*.)

Kytkentä (tarkasti)
-------------------
    Arduinon pinni D6 -> 330 ohmin vastus -> nauhan DIN (pää, josta nauhaan painetut nuolet osoittavat poispäin)
    12 V:n lähde +    -> nauhan +12 V
    12 V:n lähde -    -> nauhan GND JA Arduinon GND (yhteinen maa)
    Arduinon D3       -> valinnainen painike -> GND (seuraava asetus)
  Nauhan +12 V ei saa mennä Arduinon 5V-pinniin, eikä 12 V saa mennä mihinkään Arduinon pinniin.
  Katso kuva wiring-diagram.svg.

Ohjelman ajaminen
-----------------
Oikea laite: avaa sketch.ino Arduino IDE:ssä (kirjasto Adafruit NeoPixel), valitse Arduino Mega 2560, lataa ohjelma
ja avaa sarjamonitori nopeudella 115200. Wokwi: luo uusi Arduino Mega -projekti, liitä sketch.ino ja diagram.json,
lisää kirjasto "Adafruit NeoPixel" (libraries.txt) ja paina Play. Simulaattori näyttää ihanteellisen nauhan (jokainen
pikseli erikseen), joten se osoittaa vain, että ohjelma toimii: oikea nauha on se, josta haluamme tietoa.

Mitä testi tekee
----------------
Se käy läpi 12 asetusta (6 värijärjestystä x 2 bittinopeutta, 12 sekuntia kutakin; D3:n painike hyppää heti seuraavaan):
  asetus 1: GRB, 800 kHz     asetus 7: GRB, 400 kHz
  asetus 2: RGB, 800 kHz     asetus 8: RGB, 400 kHz
  asetus 3: BRG, 800 kHz     asetus 9: BRG, 400 kHz
  asetus 4: RBG, 800 kHz     asetus 10: RBG, 400 kHz
  asetus 5: GBR, 800 kHz     asetus 11: GBR, 400 kHz
  asetus 6: BGR, 800 kHz     asetus 12: BGR, 400 kHz
Jokaisessa asetuksessa:
  - Ensimmäiset 3 sekuntia: pikselin 1 pitäisi olla PUNAINEN, pikselin 2 VIHREÄ ja pikselin 3 SININEN.
  - Sen jälkeen yksi VALKOINEN pikseli kulkee nauhaa pitkin, yksi askel puolen sekunnin välein.
  - Piirilevyn oma LED (pinni 13) vilkuttaa asetuksen numeron alussa; sarjamonitori näyttää numeron ja värijärjestyksen.
Väärän värijärjestyksen asetukset näyttävät väärät värit (esimerkiksi vihreä, punainen, sininen). Väärä nopeus
näyttää yleensä ei mitään, valkoista tai satunnaisia värejä tai välkkymistä.

Mitä kerrot minulle (kopioi tämä ja täytä)
------------------------------------------
  1. Asetuksen numero (numerot), joissa pikselit 1-3 ovat punainen, vihreä, sininen tässä järjestyksessä: ______
  2. Kun valkoinen pikseli kulkee: montako LEDiä palaa yhtä aikaa (1 vai 3)? ______
  3. Toimivatko kaikki pätkän pikselit (myös viimeinen)? Välkkyykö jokin? ______
  4. Nauhan leveys ja siihen tai kelaan painettu teksti (merkki, piiri, jännite): ______
  5. Minkä pään kytkit DIN-päähän (täsmääkö se painettuihin nuoliin)? ______
  6. Mitä muuta huomasit (värit liian himmeät, pikseli jää päälle, mitään ei tapahdu): ______

Jos mikään ei syty: tarkista DIN-pää (nuolet), yhteinen maa ja 12 V:n lähde. Kokeile tasonmuunninta (74HCT125)
vasta, jos kaikki muu on kunnossa.

Asetusten muuttaminen
---------------------
sketch.ino:n alussa: PIN_STRIP_DATA (6), PIN_BUTTON (3), TEST_PIXELS (12), BRIGHTNESS (60), PHASE_MS (12000),
AUTO_ADVANCE (false = vaihda asetusta vain painikkeella), COLOUR_TEST_MS, WALK_STEP_MS.

Testit
------
Tiedosto tests/strip_test_harness.cpp (projektin tests-kansiossa) tarkistaa ohjelman tietokoneella. Sitä ei ole ajettu
oikealla laitteella eikä Wokwissa.
