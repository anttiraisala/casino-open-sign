EEPROM-TYHJENNYSTYÖKALU - asettaa jokaisen Arduino Megan EEPROM-tavun arvoon 0xFF
=================================================================================

Tarkoitus
---------
Kyltin (casino_open_sign_v2) valikkoasetukset tallennetaan Megan EEPROMiin. Sketchin lataus USB:n kautta EI tyhjennä EEPROMia,
joten tallennetut asetukset säilyvät, kun lataat kyltin ohjelman uuden version (jos uuden version valikkorakenne on muuttunut,
kyltti huomaa sen rakennetunnisteesta ja aloittaa oletusarvoilla itse). Kun haluat aidosti tyhjän EEPROMin, lataa tämä työkalu
kerran. Lataa sen jälkeen kyltin ohjelma uudelleen: se ei löydä tallennettuja asetuksia ja aloittaa valikon oletusarvoilla.
(Kyltin valikon kohta "Restore defaults" tekee saman niille arvoille, jotka valikko tuntee; tämä työkalu tyhjentää jokaisen tavun.)

Mitä se tekee
-------------
Käynnistyksessä, kerran: laskee EEPROM-tavut, jotka eivät ole 0xFF, kirjoittaa 0xFF jokaiseen tavuun (EEPROM.update(), joten
vain eroavat tavut kirjoitetaan oikeasti ja EEPROM kuluu mahdollisimman vähän), lukee kaiken takaisin ja tarkistaa, että kaikki
tavut ovat 0xFF. Sen jälkeen se vain vilkuttaa LEDiä ja tulostaa tilarivin sekunnin välein. Se ei kirjoita EEPROMia uudelleen
ennen seuraavaa virrankytkentää tai resetiä, joten vaihda kyltin ohjelma takaisin kun olet valmis.

Johdotus
--------
Ei mitään. Työkalu ei käytä yhtään pinniä. Kytke USB-kaapeli tietokoneen ja Megan väliin (katso wiring-diagram.svg / .pdf).
Kyltin laitteisto saa jäädä kytketyksi.

Käyttö
------
  1. Avaa eeprom_clear/sketch.ino Arduino IDE:ssä, valitse Arduino Mega 2560 ja lataa se.
  2. Avaa Serial Monitor nopeudella 115200 baudia (nopeus on myös sketch.ino:n alussa ja tulostetaan käynnistyksessä).
  3. Näet kerran:
         EEPROM clear tool: every EEPROM byte is set to 0xFF once, now.
         Serial speed 115200 baud; a DBG line is printed every second.
         EEPROM size 4096 bytes, 37 of them were not 0xFF.
         EEPROM cleared: all bytes are 0xFF. Upload the sign program now.
     (4096 tavua on Megan EEPROMin koko; käytettyjen tavujen määrä riippuu siitä, mitä on tallennettu.) Jos avasit Serial Monitorin
     latauksen jälkeen, paina Megan reset-nappia nähdäksesi tekstin uudelleen. EEPROM tyhjennetään silloin uudelleen, mikä ei haittaa.
  4. Elonmerkki: pinnin 13 LED vilkkuu hitaasti (500 ms välein) kun EEPROM on tyhjä, NOPEASTI (100 ms välein) jos jotain tavua
     ei saatu tyhjennettyä. Kerran sekunnissa Serial Monitor saa rivin
         DBG up=<s> size=<tavua> before=<tavut, jotka eivät olleet 0xFF> bad=<tavut, jotka eivät vieläkään ole 0xFF> state=<OK|FAIL> beats=<n>
  5. Lataa kyltin ohjelma (casino_open_sign_v2/sketch.ino) uudelleen. Se aloittaa valikon oletusarvoilla.

Jos viesti on "EEPROM clear FAILED" ja LED vilkkuu nopeasti, jotain EEPROMin tavua ei voi enää kirjoittaa (kulunut tai viallinen
siru); yritä vielä kerran reset-napilla, ja jos vika pysyy, Megan EEPROM on rikki.

Tiedostot
---------
  sketch.ino            työkalu
  wiring-diagram.svg    USB-yhteys (ja .pdf, sama piirros)
  diagram.json          Wokwi-piiri (vain Mega; Wokwin EEPROM alkaa tyhjänä, joten se näyttää "0 of them were not 0xFF")
Generoi tools/generate_eeprom_clear.py; konetesti tests/eeprom_clear_harness.cpp ja mutaatiotesti tests/mutate_eeprom_clear.py
(52 mutanttia). Ei ajettu oikealla Megalla eikä Wokwissa.
