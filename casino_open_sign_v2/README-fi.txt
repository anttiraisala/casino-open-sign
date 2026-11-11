CASINO OPEN SIGN V2 - Arduino Mega 2560, kaksi 12 V WS2811 -nauhaa + 11 Grove RGB LED -tikkua
=============================================================================================

Kyltin ohjelman versio 2. Versio 1 (ylempi kansio, sketch.ino) pysyy koskemattomana.
V2:ssa on tasan KAKSI LED-nauhaa ja YHDESTOISTA tikkua:

  Nauha 1  D11  frame     12 V WS2811, 18 pikseliä (yksi pikseli = yksi piiri = 3 LEDiä = 10 cm), 60 x 30 cm levyn reunus
  Nauha 2  D12  letters   12 V WS2811, 10 pikseliä, yksi pikseli jokaisen kirjaimen takana
  Tikut    Grove Base Shield D2-D8, A1-A3: 10 nuolitikkua (V2-johdotus, katso alla), A0: CLOSED-valotikku
  D13 on edelleen tilaledi (elonmerkki).

Ei ajettu oikealla laitteistolla eikä Wokwissa. Tietokonetestit menevät läpi (katso Testit).

Tiedostot
---------
sketch.ino                 Ohjelma (asetukset alussa). Serial Monitor: 115200 baud.
diagram.json               Wokwi-piiri (liitä diagram.json-välilehdelle; 12 V nauhat simuloidaan Megan 5 V:sta).
libraries.txt              Kirjasto: Adafruit NeoPixel (Wire on valmiina).
wiring-diagram.svg/.pdf    Sähkökaavio, A4 pysty (kaksi virtalähdettä, sulakkeet, kondensaattorit, kaikki pinnit).
layout-diagram.svg/.pdf    Levyn sijoittelu, A4 vaaka: reunuksen pikselit 1-18, kirjainpikselit, CLOSED, nuoli (mittakaavassa).
layout-parts.svg/.pdf      Nauhat, pikselinumerot, kirjaintaulukko, tikkutaulukko, katkaisulista (A4 pysty).
parts-list.csv/.pdf        Ostoslista (partco.fi, tilanne 2026-11-02; hintoja/saatavuutta/linkkejä EI päivitetä).
README.txt / README-fi.txt Tämä tiedosto (englanti / suomi).
Nuoli (porausmalli, tikkupiirrokset, tikkujen johdotus) on ennallaan: katso ../arrow_sticks/.

Reunusnauha (D11): 18 pikseliä, edestä katsottuna
--------------------------------------------------
Pikseli 1 on ALAVASEMMASSA kulmassa; nauha kulkee vastapäivään:
    pikselit  1-6    alareuna vasemmalta oikealle (6 = oikea alakulma)
    pikselit  7-9    oikea reuna ylöspäin
    pikselit 10-15   yläreuna oikealta vasemmalle (10 = oikea yläkulma, 15 = vasen yläkulma)
    pikselit 16-18   vasen reuna alaspäin
Reunuksen valo (2 pikselin palkki valaisee, 2 pimeänä, toistuu; yksi askel 200 ms oletusnopeudella x1; valikko: Speed > Frame speed):
    nuoli osoittaa VASEMMALLE (asiakkaan vasemmalle): alkaa OIKEASTA YLÄKULMASTA (pikseli 10) ja kulkee kumpaakin suuntaa
                                              reunuksen ympäri - myötäpäivään (10, 9, 8, 7, 6, 5 ... 1) ja vastapäivään
                                              (10, 11 ... 15, 16, 17, 18, 1) - VASEMPAAN ALAKULMAAN (pikseli 1).
    nuoli osoittaa OIKEALLE (asiakkaan oikealle): alkaa VASEMMASTA YLÄKULMASTA (pikseli 15) ja kulkee kumpaakin suuntaa
                                              OIKEAAN ALAKULMAAN (pikseli 6).
Molemmat reitit ovat 9 askelta pitkiä ja liikkuvat täsmälleen yhtä aikaa (ei aikaeroa); ne kohtaavat loppukulmassa.

Kirjainnauha (D12): 10 pikseliä, yksi per kirjain
--------------------------------------------------
    pikselit 1-6    C A S I N O    (CASINO, vasemmalta oikealle)
    pikselit 7-10   N E P O        (OPEN on kytketty KÄÄNTEISESSÄ järjestyksessä: pikseli 7 = N ... pikseli 10 = O)
Ohjelma tekee kirjainten kartoituksen, joten jokainen kirjainkuvio (tilat 0-10) kulkee katsojalle luonnollisessa
lukujärjestyksessä: OPENin kirjaimet vasemmalta oikealle O P E N ovat pikselit 10, 9, 8, 7. Pikseli on 3 LEDiä samalla värillä (syvä punainen).

Mikä pysyy kuten versiossa 1
----------------------------
Nuolianimaatio, CLOSED-valo (viallinen loisteputki), kytkimet (D32 nuolen suunta, D30 kasino kiinni),
LCD (I2C, SDA D20, SCL D21), kirkkauskäyrät (LED-tyypeittäin, max 80, eksponentti 2.67; nyt valikon ominaisuuksia), käynnistyksen LED-testi (10 s; ikkuna nyt 2 pikseliä), elonmerkki (pinnin 13 LED vilkkuu; yksi Serial-rivi sekunnissa, 115200 baud).

Johdotus (tarkasti)
-------------------
    Arduino D11   -> 330 ohmin vastus R1 -> reunusnauhan DIN (pikseli 1, vasemman alakulman pää)
    Arduino D12   -> 330 ohmin vastus R2 -> kirjainnauhan DIN (pikseli 1, C:n takana)
    PSU2 12 V +   -> sulake F2 (5 A) -> KUMMANKIN nauhan +12 V (reunus syötetään alusta ja lopusta; kirjaimet alusta)
    PSU2 12 V -   -> molempien nauhojen GND JA Arduinon GND (yhteinen maa)
    Kondensaattori C2 1000 uF 25 V +12 V:n ja GND:n välille nauhoilla (plus +12 V:iin)
    PSU1 5 V +    -> sulake F1 (10 A) -> MOLEMMAT Megan pitkän pinnirivin 5V-pinnit (shield, 11 tikkua ja LCD saavat virtansa niistä)
    PSU1 5 V -    -> Megan GND-pinnit; kondensaattori C1 1000 uF 5 V -kiskon yli (plus +5 V:iin)
    Grove Base Shield Megan päällä (3V3/5V-kytkin asentoon 5 V): tikut D2 L-CAP, D6 L-WING-UP, D5 L-WING-DN, D7 RAIL-UP-L, D8 RAIL-UP-R,
                  D3 RAIL-DN-L, D4 RAIL-DN-R, A3 R-CAP, A1 R-WING-UP, A2 R-WING-DN, A0 CLOSED-valotikku (ei johtoa virtalähteestä tikkuun)
                  (vain V2: liittimet eroavat versiosta 1 ja kansiosta ../arrow_sticks/; tikkujen paikat levyllä eivät muutu)
    D32 -> liukukytkin SW1 yhteinen (kontakti 1 -> GND)    D30 -> liukukytkin SW5 yhteinen (kontakti 1 -> GND)
    D33 -> painike SW2 -> GND (ohittaa vain käynnistyksen LED-testin)
    D34-D37 -> SW6 ohjelmakytkimen pinnit 1, 2, 4, 8 (yhteinen C -> GND)
    D42-D45 -> SW3 valikkokytkimen pinnit 1, 2, 4, 8 (yhteinen C -> GND)
    D48 -> painike SW7 OK -> GND     D49 -> painike SW8 BACK -> GND     D50 -> painike SW9 SHIFT -> GND
    (jokainen painike: kontakti 1 Arduinon pinniin, kontakti 2 GND:hen; kaikki tulot käyttävät INPUT_PULLUP:ia, ei vastuksia)
    Vanhat nopeuskytkimet SW3/SW4 (D22-D29) on poistettu: nopeudet ovat valikon ominaisuuksia. Valikon pinnit ovat samat kuin ../menu_test/-kansiossa.
    D20 SDA, D21 SCL, 5V, GND -> LCD:n I2C-taustalevy
                  (käynnistyksessä sketch skannaa I2C-väylän ja tulostaa löydetyt laitteet sarjamonitoriin, 115200 baud;
                  sketch puhuu näytölle osoitteessa 0x27: jos skannaus näyttää esim. 0x3F, aseta LCD_I2C_ADDRESS sketch.inossa)
    D13 -> ei mitään (Mega-kortin LED)
  ÄLÄ KOSKAAN kytke +12 V:ta Arduinon pinniin tai 5 V:n osiin. USB vain kun PSU1 JA PSU2 on sammutettu.
  5 V:n datasignaali 12 V WS2811 -nauhalle toimii yleensä (kuten nauhatesteissä), mutta sitä ei ole varmistettu lopullisessa kyltissä.
  Molempien virtalähteiden verkkopuoli: kotelon sisällä, pätevän henkilön kytkemänä.
  Katso wiring-diagram.svg (sähkökaavio) ja layout-diagram.svg (sijoittelu).

Ostettavat osat
---------------
Katso parts-list.csv / parts-list.pdf (partco.fi, hinnat ja saatavuus ovat tilanne 2026-11-02 eikä niitä päivitetä: tarkista ennen ostoa).
Pääosat: Arduino Mega 2560, Grove Base Shield + 11 Grove RGB LED -tikkua, 5 V virtalähde Mean Well LRS-50-5 + 10 A sulake,
12 V virtalähde Mean Well LRS-50-12 + 5 A sulake, yksi rulla 12 V WS2811 -nauhaa (50 pikselistä käytetään 28), 2 x 330 ohmia, 2 x 1000 uF 25 V.

Ajaminen
--------
Oikea laitteisto: avaa sketch.ino Arduino IDE:ssä (kirjasto Adafruit NeoPixel), kortti Arduino Mega 2560, lataa PSU1 ja PSU2 sammutettuina,
kytke sitten virrat; Serial Monitor 115200 baud. Wokwi: uusi Arduino Mega -projekti, liitä sketch.ino ja diagram.json, lisää kirjasto
"Adafruit NeoPixel", paina Play (Wokwissa ei ole 12 V osaa: nauhat saavat siellä virran Megan 5 V:sta).
Jos värit ovat väärin, nauhan värijärjestys voi olla toinen: vaihda NEO_GRB + NEO_KHZ800 sketchin kahdella nauharivillä.
Käynnistyksen LED-testi (10 s) ajaa 2 pikselin ikkunan jokaisen nauhan läpi (kierros 1 valkoinen, kierros 2 punainen-vihreä-sininen); D33 ohittaa sen.

Asetusvalikko (valikkokytkin SW3 D42-D45, painikkeet OK D48, BACK D49, SHIFT D50)
--------------------------------------------------------------------------------
Tilasivu (näyttö käynnistyksen jälkeen): rivi 0 näyttää ohjelman (#0..#E, #F = satunnainen) ja kasinon tilan (Open/Closed); rivi 1 näyttää
nuolisymbolin ja asiakkaan puolen ("<- Customer right" = nuoli osoittaa asiakkaan oikealle) ja ohjelmassa F vuorottelee tekstin
"Change in M:SS" (aika seuraavaan satunnaisohjelmaan) kanssa. OK avaa valikon; käyttöliittymässä painikkeen nimi on aina OK.
Valikko: Controls (program, arrow, casino: vain luettavia kytkinasentoja; Arrow reverse / Casino reverse -liput), Speed (Frame speed, Letters speed,
Arrow speed: x0.125..x4, logaritminen, 30 askelta, kolme desimaalia, oletus x1), sitten yksi ryhmä per LED-tyyppi: Frame (reunuksen LED-nauha),
Letters (kirjainten LED-nauha) ja Arrow (nuolen tikut), kussakin oma nopeus sekä kirkkauden Min, Max, Exponent ja Mix (käyrän max on raja,
yleistä kirkkautta ei ole); Brightness (samat kirkkausarvot uudelleen ryhmiteltynä Frame / Letters / Arrow: yksi ominaisuus, kaksi paikkaa),
Random program (Change delay), Restore defaults (kysyy vahvistuksen).
Ominaisuusrivi näyttää "Nimi  arvo" (arvo oikeassa reunassa); jos ne eivät mahdu 16 merkkiin, rivi vierii yhtenä tekstinä "Nimi: arvo"
(tauko alussa, yksi merkki askeleessa, tauko viimeisen merkin ollessa oikeassa reunassa). OK vain luettavan arvon kohdalla avaa saman sivun kuin
muokkaus (nimi, sitten lukko ja arvo), mutta mitään ei voi muuttaa; BACK tai OK palaa.
Valikkokytkin SW3 on vapaasti pyörivä ja SUHTEELLINEN: koodin muutos on askel (1..7 eteen, 9..15 taakse, tasan 8 ohitetaan).
Tässä sketchissä käytetty valikon määrittely on tallennettu tähän tiedostoon menu-v2.json (tarkka kopio tiedostosta tools/menu-editor/menu-v2.json;
tools/generate_v2_menu.js kirjoittaa sen sketch.ino:n valikkolohkon kanssa; avaa se Menu Editorissa nähdäksesi tai muuttaaksesi valikkoa).
Valikkokytkimen käännös ensimmäisellä tai viimeisellä kohdalla ei tee mitään (pitkän rivin vieritys jatkuu); käännös pois ja heti takaisin
alle 300 ms sisällä (koskettimen pomppu) säilyttää myös sen rivin vierityksen.
SHIFT pohjassa kierrettäessä askel on isompi. OK menee alivalikkoon / aloittaa muokkauksen / hyväksyy, BACK menee tason ylös / peruu muokkauksen.
Pitkä rivinteksti vierii. Valikon juuressa, kun mitään ominaisuutta ei ole auki eikä muokata, 60 s ilman syötettä palauttaa näytön tilasivulle.
Ohjelmakytkin SW6 on ABSOLUUTTINEN ja sitä luetaan koko ajan (myös valikon ollessa auki).
Arvot tallennetaan EEPROMiin (asettelutunniste tarkistetaan käynnistyksessä; ei-täsmäävä asettelu antaa oletukset).
Testattu vain koneella (tests/casino_open_sign_v2_harness.cpp, valikkomoottorin testit), ei vielä laitteistolla eikä Wokwissa.
Tunnettu rajoitus: LCD yritetään uudelleen epäonnistuneen lähetyksen jälkeen, mutta sitä ei alusteta kokonaan uudelleen jaksoittain kuten hex_rotary_testissä.

Ohjelmakytkin SW6 (heksa-kiertokytkin, D34-D37, absoluuttinen, luetaan koko ajan)
-----------------------------------------------------------------
Asento 0..E = kirjainohjelma 0..14 ja vastaava reunuskuvio 0..14; F = satunnainen: satunnainen ohjelma 0..14 kahden minuutin välein,
ei koskaan samaa kahdesti peräkkäin.
  0 hengitys  1 aalto  2 kirjoituskone  3 syttyminen  4 sanojen vilkku  5 vuorottelu  6 skanneri  7 kipinä  8 jahti  9 pulssi
  10 (A) yhtyminen  11 (B) sanojen vaihto  12 (C) hedelmäpeli  13 (D) kaksoisjahti  14 (E) markiisi
Reunuskuviot ovat valkoisia ja seuraavat nuolen aloituskulmaa ohjelmissa 0, 1, 2, 6, 8, 13 ja 14. Molemmat ovat ensimmäisiä versioita,
joita arvioidaan livenä kyltissä (RDD-IDEAS.md IDEA-015, IDEA-016). Testattu vain koneella, ei vielä laitteistolla.

Asetusten muuttaminen
---------------------
sketch.ino:n alussa: FRAME_EDGE_LONG (6) ja FRAME_EDGE_SHORT (3) (reunuksen pikselit per reuna), PIN_FRAME (11), PIN_LETTERS (12),
FRAME_SPEED_MS (200), FRAME_BAR_LEDS (2), STARTUP_TEST_WINDOW_LEDS (2), kirkkauskäyrien oletukset sekä kirjain- ja nuoliajoitukset.
Pinnien tai reunuksen koon muuttamisen jälkeen generoi piirrokset ja Wokwi-tiedosto uudelleen:
    python3 tools/generate_v2_diagram.py && python3 tools/generate_v2_schematic.py && python3 tools/generate_v2_layout.py
    python3 tools/generate_v2_parts_list.py && python3 tools/export_pdf.py

Testit
------
tests/casino_open_sign_v2_harness.cpp tarkistaa ohjelman tietokoneella (odotetut arvot on kirjoitettu koodiin, ei lueta sketchistä),
tests/mutate_v2.py tarkistaa testit noin 170 sketchin mutantilla:
    g++ -std=c++17 -O1 -w -Itests -DSKETCH_PATH='"'$PWD'/casino_open_sign_v2/sketch.ino"' -o /tmp/v2 tests/casino_open_sign_v2_harness.cpp && /tmp/v2
    python3 tests/mutate_v2.py
(Ei ajettu oikealla laitteistolla eikä Wokwissa.)
