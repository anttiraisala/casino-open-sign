SUORA TIKKUTESTI - enintään 11 Grove RGB LED -tikkua kytkettynä MUUTTAMATTOMINA Grove Base Shieldiin
===================================================================================================

Tarkoitus
---------
Todiste siitä, että ledi-tikut toimivat ja että ne voivat kaikki käydä yhtä aikaa. Jokainen tikku kytketään
tavallisella Grove-kaapelillaan Arduino Mega 2560:n Grove Base Shieldin liittimeen; mitään ei siirrellä eikä
muokata. Koko Arduino + shield saa virtansa ulkoisesta 5 V:n virtalähteestä. Kysymys: riittääkö virta kaikille
tikuille shieldin ja Grove-liittimien kautta? Lataus Arduino IDE:llä. Testillä ei ole mitään tekemistä itse
kasinokyltin kanssa.
(Toinen testi, ../stick_test, syöttää tikut suoraan virtalähteestä muokatuilla Grove-kaapeleilla.)

Mitä näet
---------
Jokaisessa tikussa pyörii liikkuva kuvio KOKO AJAN heti kun ohjelma käynnistyy (ei pimeää taukoa, ei testivaiheita).
  - 11 kuviota: pyyhkäisijä ("Knight Rider"), jahti, komeetta, sateenkaarikomeetta, teatterijahti, kaksi pomppivaa
    pistettä, täyttö ja tyhjennys, parittomat/parilliset vuorotellen, kimallus, poliisivalot, taaksepäin juoksija.
    Jokainen kuvio sytyttää vain 1-6 lediä kerrallaan.
  - VALOKEILA: yksi tikku kerrallaan (1,2 s) sytyttää kaikki 10 lediään valkoisena; valokeila kiertää tikut 1, 2, ... 11.
  - KUVIOIDEN SIIRTO: 9,6 s välein kuviot siirtyvät yhden tikun eteenpäin, joten jokainen tikku näyttää vuorollaan
    jokaisen kuvion.
  - KUORMATESTI: pidä painiketta (D22 -> GND) pohjassa: kaikkien tikkujen kaikki ledit palavat valkoisina
    MAX_BRIGHTNESS-kirkkaudella (pahin tapaus, arviolta noin 3,3 A 11 tikulla kirkkaudella 128) kunnes päästät irti.
    Pidä sitä minuutti ja tunnustele, lämpeneekö shield, liittimet tai johdot.
Megan Grove Base Shieldissä on 11 liitintä, joilla on datanasta (D2..D8 ja A0..A3), joten tikkuja voi olla
enintään 11. Aseta sketch.ino:ssa ACTIVE_STICKS tikkujesi määräksi (oletus 11; käyttämättömät liittimet eivät haittaa).

Tiedostot
---------
sketch.ino          Testiohjelma. Asetukset ovat alussa (ACTIVE_STICKS, MAX_BRIGHTNESS, ajastukset).
diagram.json        Wokwi-piiri simulaattoriin (11 nauhaa à 10 pikseliä; simulaatio ottaa virran 5V:stä).
libraries.txt       Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf   Johdotus sähkökaaviona (A4).
layout-diagram.svg/.pdf   10 tikun nuoli (malli B) ja kunkin tikun nasta (A4 vaaka).
README.txt          Englanninkielinen ohje.   README-fi.txt   Tämä suomennos.

Sign of life -elonmerkki (sääntö koko projektille)
--------------------------------------------------
  - Pinnin 13 ledi (kortilla merkintä L) vilkkuu koko ajan kun ohjelma käy (250 ms välein).
    Se kertoo kuormasta: se kääntyy hetkeksi päinvastaiseksi kun jokin muuttuu (valokeila siirtyy tai kuviot
    siirtyvät), ja se vilkkuu kaksi kertaa nopeammin kun muutoksia tulee paljon tai kuormatesti on päällä.
    Jos se lakkaa vilkkumasta, ohjelma on jumissa tai Arduino on resetoitunut.
  - Serial Monitorin nopeus: 115200 baudia (kerrotaan sketch.ino:n alussa ja tulostetaan käynnistyksessä).
    Kerran sekunnissa tulostuu rivi, esimerkiksi:
      DBG up=12s fps=50 spot=11 set=1 events=10 beats=47 load=0
    up = sekunnit käynnistyksestä, fps = kuvaa sekunnissa, spot = tikku, jolla on valokeila, set = kuviosarja,
    events = tilamuutokset tähän mennessä, beats = ledin vaihdot, load = 1 kun painike on pohjassa. Arvot muuttuvat
    rivistä toiseen, joten näet että ohjelma elää.
    Serial Monitor vaatii USB-kaapelin, jota EI kytketä kun virtalähde on päällä (katso alla).

Osat (partco.fi; hinnat ja saatavuus ovat tilannekuva 2.11.2026, tarkista ennen ostoa)
--------------------------------------------------------------------------------------
  - Enintään 11 x Grove RGB LED Stick (10x WS2813 mini) Grove-kaapeleineen (haku "Grove RGB LED Stick").
  - 1 x Mean Well LRS-50-5, 5 V 10 A, 23,79 EUR  https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x autosulake 10 A (SUA-10A) 0,31 EUR + sulakepidike  https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x kondensaattori 1000 uF 25 V (ELKO105-1000U-25V) 1,73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - Johtoa 1,5 mm2 syöttöön, Arduino Mega 2560 + Grove Base Shield, valinnainen painonappi.
  Virtalähteen 230 V:n puoli kuuluu koteloon ja sen saa kytkeä vain sen osaava henkilö.

Johdotus (tarkasti; katso wiring-diagram.pdf)
---------------------------------------------
  Tikku  Grove-liitin (signaalinasta)     Tikku  Grove-liitin (signaalinasta)
   1     D2                                7     D8
   2     D3                                8     A0
   3     D4                                9     A1
   4     D5                               10     A2
   5     D6                               11     A3
   6     D7
  Shieldin 3V3/5V-kytkin: 5 V.
  Virtalähteen V+ -> 10 A sulake -> Megan 5V-navat; virtalähteen V- -> Megan GND-navat. Käytä pitkän kaksoisriviliittimen
  päässä olevia 5V- ja GND-napoja (shield ei peitä niitä), molempia 5V- ja molempia GND-napoja, 1,5 mm2 johto.
  1000 uF kondensaattori syöttöjohtojen yli, plus +5 V:ään.
  Valinnainen painonappi D22:n ja GND:n välissä (kuormatesti kun pidetään pohjassa).
  USB-kaapelia EI kytketä kun virtalähde on päällä (katso alla).

Päällekytkentä
--------------
  1. Arduino IDE: asenna kirjasto "Adafruit NeoPixel", kortti Arduino Mega tai Mega 2560, avaa sketch.ino ja
     Upload USB:llä (virtalähde pois päältä).
  2. Irrota USB-kaapeli. Kytke tikut liittimiin ja virtalähde.
  3. Kytke virtalähde päälle. Kuviot alkavat heti ja pinnin 13 ledi vilkkuu.
  Miksi ei USB:tä samaan aikaan: virtalähde syöttää 5V-napaa suoraan, ja kaksi 5 V:n lähdettä rinnakkain
  voi työntää virtaa USB-porttiin. Ilman USB:tä ei ole Serial Monitoria; pinnin 13 ledi kertoo että ohjelma käy.
  (Serial Monitoria varten irrota virtalähde, kytke USB ja aja testi vain ACTIVE_STICKS = 1 tai 2.)

Mitä tarkistaa / raportoida
---------------------------
  - Näyttääkö jokainen tikku kuvionsa ja kiertääkö valokeila kaikki tikut?
  - Välkettä, vääriä värejä, himmenemistä, keltaista sävyä tai Arduinon uudelleenkäynnistymistä (valokeila alkaa
    uudestaan tikusta 1)?
  - Pidä painiketta minuutti pohjassa: lämpeneekö jokin (shield, Grove-liittimet, syöttöjohdot, 5V-navan alue)?
    Täysi valkoinen on noin 0,5 A per tikku; shieldin johtimet ja Grove-liittimet on tehty pienille virroille, joten
    juuri tätä testi tutkii. Jännite viimeisellä tikulla yleismittarilla (4,5 V tai enemmän)?
  - Jos jokin lämpenee tai himmenee: pienennä MAX_BRIGHTNESS (esim. 64) tai käytä ../stick_test-testiä.

Huomioita
---------
  - Grove-tikkujen kuumavaihtoa ei tueta: katkaise kaikki virrat ennen kaapelien vaihtoa.
  - Hosttesti tests/stick_test_direct_harness.cpp tarkistaa ohjelman logiikan mock-kirjastolla (ei oikealla laitteella).
  - Tämä testi on erillinen kasinokyltistä eikä muuta siinä mitään.
