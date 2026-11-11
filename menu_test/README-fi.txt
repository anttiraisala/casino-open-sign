VALIKKOTESTI - Casino Open Sign V2:n asetusvalikko oikeilla ohjaimilla ja I2C-LCD-näytöllä
==========================================================================================

Tarkoitus
---------
Kokeile suunniteltua asetusvalikkoa (ks. ../RDD-REQUIREMENTS.md REQ-012..017) oikealla laitteistolla ennen kuin se viedään
kylttiin: kaksi heksakiertokytkintä, OK / BACK / SHIFT -painikkeet, kaksi liukukytkintä ja 2x16 LCD. LED-nauhoja tai
tikkuja ei kytketä; Mega saa virran USB:stä. Valikon sisältö tulee tiedostosta ../tools/menu-editor/menu-v2.json
(muokkaa Menu Editorilla); sketch.ino generoidaan siitä (node tools/generate_menu_test.js). Tämä testi EI muuta kyltin
ohjelmaa (casino_open_sign_v2/): valikkoa ei ole vielä kyltissä.

Ohjaimet
--------
  Ohjelmakytkin        jalat 1, 2, 4, 8 -> D34-D37, yksi C-jalka GND:hen. Absoluuttinen asento 0..F = ohjelman numero
                       tilasivulla. Luetaan koko ajan; sen kääntäminen valikon ollessa auki sulkee valikon.
  Valikkokytkin        jalat 1, 2, 4, 8 -> D42-D45, yksi C-jalka GND:hen. Vapaasti pyörivä: kääntäminen siirtää valintaa
                       tai muuttaa arvoa (suunta ja askeleet asennon muutoksesta, kuten hex_rotary_testissä).
  OK D48, BACK D49, SHIFT D50   painikkeet GND:hen. SHIFT pohjassa käännettäessä askel on kymmenkertainen.
  Nuolikytkin          D32 (keskikosketin), sivukosketin GND:hen. Kahva GND-puolella = nuoli osoittaa vasemmalle.
  Casino-kytkin        D30 (keskikosketin), sivukosketin GND:hen. Kahva GND-puolella = casino kiinni (Closed).
  LCD                  I2C-moduuli 0x27: GND, VCC -> 5V, SDA D20, SCL D21.
Tarkka kytkentä: wiring-diagram.svg/.pdf (kytkentäkaavio) ja layout-diagram.svg/.pdf (Megan pinnit ja taulukko).

Mitä näet ja teet
-----------------
  Tilasivu      rivi 0 "#<ohjelma> Casino Open|Closed", rivi 1 nuoli + "Customer right|left". Ohjelmassa F rivi 1 vaihtuu
                3 sekunnin välein tekstiksi "Change in M:SS" (lähtölaskenta on tässä vain simuloitu "Change delay" -arvosta).
  OK            avaa valikon juuresta (otsikko "Menu" ja laskuri "NNs"). Listassa OK avaa alivalikon tai aloittaa arvon
                muokkauksen. BACK menee tason ylös tai poistuu valikosta juuressa.
  Valikkokytkin siirtää valintaa (lista pysähtyy päihin) tai muuttaa arvoa muokattaessa (heti; sarjamonitori tulostaa
                "CB ..." -rivin jokaisesta callbackista). OK tallentaa arvon EEPROMiin, BACK peruu (vanha arvo).
  Restore defaults  kysyy "OK=yes BACK=no" (kysymys "Restore defaults?" on 17 merkkiä ja vierii 16 merkin rivillä).
  Aikakatkaisu  valikko sulkeutuu 60 s ilman syötettä, vain juurilistassa (jäljellä olevat sekunnit näkyvät).
  Kytkimet      nuoli-, casino- tai ohjelmakytkimen muutos valikon ollessa auki sulkee valikon / peruu muokkauksen.
                "Arrow reverse" ja "Casino reverse" kääntävät liukukytkinten merkityksen heti.
  Pitkät tekstit  rivi, joka ei mahdu, odottaa 1 s ja vierii sitten yhden merkin 400 ms välein.
Rivin 1 nuolet ovat LCD:n ROM-merkit 0x7E / 0x7F. Jos näytössä on muita symboleja, kerro AI:lle
(valikko-JSONiin tulee silloin "arrowAscii": true ja tilasivu käyttää merkkejä "<-" / "->").

Asetukset sketch.ino:ssa (CONFIGURATION alussa)
-----------------------------------------------
  HEX_BIT_IS_LOW (true: C:hen kytketty jalka on asetettu bitti), MENU_SWITCH_REVERSED (kääntösuunta),
  ARROW_POINTS_LEFT_WHEN_LOW, CASINO_CLOSED_WHEN_LOW, pinnit, värinänestoajat, LCD_I2C_ADDRESS, MENU_EEPROM_BASE.
Väärä kääntösuunta: aseta MENU_SWITCH_REVERSED arvoon true. Asento 0 näkyy F:nä: aseta HEX_BIT_IS_LOW arvoon false.

Mitä kokeilla ja kirjata ylös
-----------------------------
  1. Tilasivu: seuraavatko ohjelman numero, Casino ja nuoli kolmea kytkintä (ja Reverse-asetuksia)?
  2. OK avaa valikon; liikkuuko valinta yhden kohdan per napsahdus oikeaan suuntaan ja pysähtyykö se päihin?
  3. Muokkaa arvoa (esim. Brightness > Strip > Max): muuttuuko heti, tekeekö SHIFT kymmenen askelta, tallentaako OK,
     peruuttaako BACK? Katkaise Megan virta ja kytke uudelleen: onko tallennettu arvo tallella (sarjamonitori "EEPROM: saved ...")?
  4. Restore defaults: vahvistussivu, OK = kyllä, BACK = ei.
  5. Näyttävätkö rivin 1 nuolet nuolilta? (Muuten "arrowAscii".)
  6. Onko vierivä pitkä teksti luettavissa (esim. "Restore defaults?")?
  7. Käännä ohjelmakytkin asentoon F: rivi 1 vaihtelee tekstin "Change in M:SS" kanssa.

Tiedostot
---------
sketch.ino              Testiohjelma (generoitu; moottori ../tools/menu_engine/engine.inc, kuori test_shell.ino.in).
diagram.json            Wokwi-kytkentä (DIP-kytkinpankit korvaavat kiertokytkimet).
wiring-diagram.svg/.pdf Kytkentäkaavio (A4).   layout-diagram.svg/.pdf  Pinnit ja kytkentätaulukko (A4 vaaka).
README.txt              Englanninkielinen versio.   README-fi.txt  Tämä tiedosto.
Ei lisäkirjastoja: vain Wire ja EEPROM, jotka kuuluvat Arduino IDE:hen.
Koneella ajettavat testit (projektin juuressa): tests/menu_diff_test.py (JS-moottori vs C++-moottori, satunnaiset tapahtumat),
tests/menu_engine_harness.cpp + tests/menu_engine_checks.inc (EEPROM-kuva, tekstit, pinnit, värinänesto, elonmerkki) ja
tests/mutate_menu_engine.py.

Elonmerkki (koko projektin sääntö)
----------------------------------
  - Pinnin 13 LED vilkkuu koko ajan (500 ms), kääntyy hetkeksi (80 ms) käänteiseksi jokaisesta näppäin- tai kytkintapahtumasta
    ja vilkkuu nelinkertaisella nopeudella (125 ms), kun tapahtumia tulee paljon.
  - Sarjamonitorin nopeus: 115200 baud (SERIAL_BAUD sketch.inon alussa, tulostetaan käynnistyksessä). Kerran sekunnissa:
      DBG up=12 mode=S prog=F arrow=R casino=O menu=3 keys=5 cb=2 eeprom=saved lcd=1
    mode S tila / M valikko / E muokkaus / C vahvistus, prog heksa, arrow L/R, casino C/O, menu = valikkokytkimen askeleet,
    keys = OK/BACK-painallukset, cb = callbackit, eeprom saved tai defaults, lcd = 1 kun näyttö vastaa.
  - Käynnistyksessä I2C-skannaus listaa laitteet ja varoittaa, jos LCD-osoitteessa ei vastaa mikään.
  - Näyttöä tarkkaillaan koko ajan: yritys uudelleen sekunnin välein jos katoaa ja täysi alustus 5 s välein (LCD_REINIT_MS).
    Jos LCD:ssä on valo mutta ei tekstiä, käännä moduulin kontrastitrimmeriä ja tarkista että VCC on vähintään 4,75 V.
