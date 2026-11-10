HEKSAKIERTOKYTKIMEN TESTI - heksakiertokytkin (0-F) muuttaa lukua A, joka näkyy I2C-LCD-näytöllä
================================================================================================

Tarkoitus
---------
Selvittää, miten heksakiertokytkin käyttäytyy, ennen kuin sitä käytetään suunnitellun asetusvalikon säätimenä
(katso ../RDD-REQUIREMENTS.md, REQ-005). Kytkin on vapaasti pyörivä 16-asentoinen kytkin, jonka asento on absoluuttinen
4-bittinen koodi (jalat 1, 2, 4, 8 ja yhteinen jalka C). Se ei ole inkrementaalinen enkooderi. Testi lukee kaikki neljä
jalkaa ja päättelee suunnan ja askelmäärän asennon muutoksesta. Testillä ei ole mitään tekemistä kyltin LED-nauhojen
tai -tikkujen kanssa, eikä ledejä kytketä.

Mitä näet
---------
Ohjelma pitää muistissa yhtä lukua A.
  - A kulkee välillä 2-51 eikä kierrä ympäri (wrap): se pysähtyy rajaan. Yksi kytkimen askel muuttaa A:ta yhdellä.
  - Myötäpäivään kääntäminen kasvattaa A:ta, vastapäivään kääntäminen pienentää sitä.
  - A on käynnistyksessä 26 (keskellä, jotta molempia suuntia voi kokeilla heti).
LCD (2x16) näyttää koko ajan:
    rivi 0   A=27  (2..51)
    rivi 1   Pos:7 CW  n:123     kytkimen raaka asento 0-F, viimeisin suunta (CW / CCW, "--" ennen ensimmäistä liikettä),
                                 lasketut kytkimen askeleet (näyttö pysähtyy lukuun 9999)
Suunnan päättely: delta = (uusi asento - vanha asento) mod 16.
  1..7 = niin monta askelta eteenpäin, 9..15 = (16 - delta) askelta taaksepäin, 8 = puoli kierrosta kerralla: suuntaa
  ei voi tietää, joten se jätetään huomiotta (asento LCD:llä silti seuraa). Nopea käännös, joka ohittaa asentoja,
  lasketaan oikein enintään 7 askeleeseen asti. Uuden asennon pitää pysyä muuttumattomana 10 ms (kontaktipompun
  vaikutus ohitetaan).

Kaksi asetusta sketch.ino:ssa (CONFIGURATION-osan ensimmäiset rivit)
--------------------------------------------------------------------
  reverseDirection   yksi bitti muistissa. false = nouseva asentokoodi on myötäpäivää. Jos A menee väärään suuntaan,
                     kun käännät kytkintä, vaihda false arvoon true (johtoja ei tarvitse siirtää).
  HEX_BIT_IS_LOW     true = jalka, joka on kytketty C:hen, lukee LOW ja on asetettu bitti (normaali). Jos asento 0 näkyy
                     LCD:llä F:nä, kytkin on käänteiskoodattu: aseta tämä arvoon false.

Mitä kokeilla ja kirjata ylös
-----------------------------
  1. Käännä myötäpäivään yksi napsahdus kerrallaan: kasvaako A yhdellä ja lukeeko rivillä 1 CW? (Jos ei: reverseDirection.)
  2. Käännä kytkin ensimmäiseen napsahdukseen: onko rivin 1 asento 0? (Jos se on F: HEX_BIT_IS_LOW.)
  3. Näkyvätkö kaikki 16 asentoa 0-F järjestyksessä ilman toistuvia tai ohitettuja numeroita? (Väärä järjestys tarkoittaa,
     että kaksi jalkaa on vaihtunut keskenään.)
  4. Käännä nopeasti monta napsahdusta: A:n pitää muuttua yhtä monta askelta.
  5. Jatka kääntämistä samaan suuntaan: A:n pitää pysähtyä lukuun 51 (tai 2) ja pysyä siinä.
Sarjamonitori tulostaa samat arvot, joten voit lukea ne myös sieltä.

Tiedostot
---------
sketch.ino          Testiohjelma. Asetukset ovat alussa (pinnit, rajat, reverseDirection, LCD:n osoite, ajoitukset).
diagram.json        Wokwi-kytkentä simulaattoriin (DIP-kytkinrivi 1-4 korvaa kiertokytkimen).
wiring-diagram.svg/.pdf   Johdotus sähkökaaviona (A4).
layout-diagram.svg/.pdf   Mihin johdot menevät Megan pinnirivistöllä ja kytkimen jalkataulukko (A4 vaaka).
README.txt          Englanninkielinen ohje.   README-fi.txt   Tämä tiedosto (suomi).
Ylimääräistä kirjastoa ei tarvita: ohjelma käyttää vain Wire-kirjastoa, joka kuuluu Arduino IDE:hen.
Koneella ajettava testi: tests/hex_rotary_test_harness.cpp, mutaatiotesti: tests/mutate_hex_rotary.py (projektin juuressa).

Elonmerkki (koko projektin sääntö)
----------------------------------
  - Pinnin 13 LED (merkitty L) vilkkuu koko ajan, kun ohjelma pyörii (500 ms välein). Se kääntyy hetkeksi (80 ms)
    käänteiseksi jokaisella asennon muutoksella ja vilkkuu nelinkertaisella nopeudella (125 ms), kun kytkintä käännetään
    paljon (kolme tai useampi asennon muutos noin sekunnin sisällä). Jos se lakkaa vilkkumasta, ohjelma on jumissa tai
    nollautunut.
  - Sarjamonitorin nopeus: 115200 baud (kerrotaan sketch.ino:n alussa ja tulostetaan käynnistyksessä). Kerran sekunnissa
    tulostuu rivi, esimerkiksi:
      DBG up=12 pos=7 A=27 dir=CW steps=123 chg=40 raw=7 beats=24 lcd=1
    up = sekunnit käynnistyksestä, pos = hyväksytty kytkimen asento (heksa), A, dir = viimeisin suunta, steps = lasketut
    askeleet, chg = asennon muutokset, raw = neljän jalan bitit juuri nyt luettuna (heksa), beats = LEDin vaihdot,
    lcd = 1 kun näyttö vastaa. up ja beats muuttuvat riviltä riville, joten näet, että ohjelma elää.
  - Käynnistyksessä I2C-skannaus luettelee kaikki löydetyt laitteet ("found 0x27 (LCD address in this sketch)") ja
    varoittaa, jos LCD:n osoitteessa ei vastaa mikään.

LCD tässä testissä
------------------
Näyttöä tarkkaillaan koko ajan. Jos se ei vastaa käynnistyksessä tai lakkaa vastaamasta, ohjelma yrittää käynnistää sen
uudelleen kerran sekunnissa. Viiden sekunnin välein näyttö myös alustetaan alusta ja piirretään uudelleen (lyhyt
välähdys). Tämä palauttaa näytön, jonka ohjain on nollautunut jännitekuopan takia taustavalon palaessa, mikä näyttää
"valo mutta ei tekstiä". (LCD_REINIT_MS arvoon 0 poistaa jaksottaisen uudelleenkäynnistyksen.)
Jos LCD:llä ei näy tekstiä mutta valo palaa:
  - käännä I2C-moduulin sinistä kontrastisäätöä pienellä ruuvimeisselillä;
  - mittaa LCD:n VCC- ja GND-nastojen väli: vähintään 4,75 V tarvitaan (Megan oma säädin tarvitsee pyöreässä
    virtaliittimessä vähintään 7 V; tässä testissä USB riittää);
  - lue sarjamonitoria: skannauksen pitää listata näyttö (yleensä 0x27, joskus 0x3F: aseta LCD_I2C_ADDRESS).

Osat
----
  - Arduino Mega 2560 (USB-kaapeli virtaa, ohjelman lataamista ja sarjamonitoria varten).
  - 1 x heksakiertokytkin 0-F, jalat 1, 2, 4, 8 ja kaksi C (omistajan partco-kytkin "hex-enkooderi kiertokytkin tapilla 0-F").
  - 1 x 2x16-merkkinen LCD I2C-moduulilla (PCF8574).
  - 9 hyppylankaa (5 kytkimelle: neljä jalkaa ja C, 4 LCD:lle), naaras-naaras tai osien mukaan.

Johdotus (tarkasti; katso wiring-diagram.pdf ja layout-diagram.pdf)
-------------------------------------------------------------------
  Heksakiertokytkimen jalka   Arduino Megan pinni
    1                         D34
    2                         D35
    4                         D36
    8                         D37
    C (toinen kahdesta)       GND   (mikä tahansa GND-pinni; toinen C-jalka jää vapaaksi)
  Ei vastuksia: Megan sisäisiä vetovastuksia käytetään (INPUT_PULLUP). C:hen kytketty jalka lukee LOW = bitti asetettu.
  I2C-LCD                     Arduino Megan pinni
    GND                       GND
    VCC                       5V
    SDA                       D20 (SDA)
    SCL                       D21 (SCL)
  Pinnin 13 oma LED on elonmerkkiä varten.

Käynnistys
----------
  1. Arduino IDE: kortti Arduino Mega tai Mega 2560, avaa sketch.ino ja Lataa (Upload) USB:llä.
  2. Avaa sarjamonitori nopeudella 115200 baud: käynnistysteksti, I2C-skannaus ja sen jälkeen yksi DBG-rivi sekunnissa.
  3. Käännä kytkintä: A muuttuu LCD:llä ja DBG-rivillä.
  Simulaattori: avaa diagram.json ja sketch.ino Wokwissa. Wokwissa ei ole heksakiertokytkintä, joten DIP-kytkinrivi
  korvaa sen: kytkin 1 = jalka 1 (D34), 2 = jalka 2 (D35), 3 = jalka 4 (D36), 4 = jalka 8 (D37); ON = jalka kytketty
  C:hen = bitti asetettu. Kun neljä kytkintä asetetaan seuraavan asennon binääriluvuksi, A liikkuu yhden askeleen.
