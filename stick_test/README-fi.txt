TIKKUTESTI - riittääkö Arduino + erillinen 5 V:n virtalähde 10 Grove RGB LED -tikulle?
======================================================================================

Tarkoitus
---------
Erillinen testi, joka ei riipu kasinokyltin pääohjelmasta. Se ajaa 10 Grove RGB LED Stick -tikkua
(10x WS2813 mini, 10 lediä per tikku = 100 lediä) Arduino Mega 2560:n datanastoilla, kun tikut saavat
virtansa ERILLISESTÄ 5 V:n virtalähteestä. Testi näyttää toimiiko kokonaisuus (johdotus, virta, maa, data)
ja paljonko tikut kuluttavat kirkkauden noustessa. Lataus Arduino IDE:llä.

Tiedostot
---------
sketch.ino          Testiohjelma. Asetukset ovat alussa (ACTIVE_STICKS, MAX_BRIGHTNESS, nastat).
diagram.json        Wokwi-piiri simulaattoriin (10 nauhaa à 10 pikseliä; simulaatio ottaa virran 5V:stä).
libraries.txt       Kirjasto: Adafruit NeoPixel.
wiring-diagram.svg/.pdf   Johdotus sähkökaaviona (A4).
layout-diagram.svg/.pdf   10 tikun nuoli (malli B), tikkujen numerot ja nastat (A4 vaaka).
README.txt          Englanninkielinen ohje.   README-fi.txt   Tämä suomennos.

Tarvittavat osat (partco.fi; hinnat ja saatavuus ovat tilannekuva 2.11.2026, tarkista ennen ostoa)
-----------------------------------------------------------------------------------------------
  - 10 x Grove RGB LED Stick (10x WS2813 mini), 80 x 10 x 10 mm, 3,3/5 V (sinulla on jo 9;
    puuttuvan voi ostaa partco.fi:stä, haku "Grove RGB LED Stick").
  - 1 x Mean Well LRS-50-5, 5 V 10 A, 23,79 EUR
    https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
  - 1 x autosulake 10 A (SUA-10A), 0,31 EUR, sekä sulakepidike
    https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  - 1 x kondensaattori 1000 uF 25 V (ELKO105-1000U-25V), 1,73 EUR
    https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238
  - 10 x vastus 330 ohm (VASTUS-2W-330R), 0,45 EUR/kpl (suositeltu, datalinjoissa)
    https://partco.fi/tuote/metallioksidikalvovastus-2w-330ohm-9515
  - Arduino Mega 2560 ja halutessa Grove Base Shield, USB-kaapeli, johtoa (1,5 mm2 +5 V:n ja 0 V:n kiskoihin),
    valinnainen painonappi.
  Virtalähteen 230 V:n puoli kuuluu koteloon ja sen saa kytkeä vain sen osaava henkilö.

Tikkujen virransyöttö (tärkeää)
-------------------------------
Täysi valkoinen 100 ledillä voi vetää noin 5 A (noin 0,5 A per tikku). Arduino ja Grove-shield eivät sitä
anna, joten tikut ottavat +5 V:n ja 0 V:n virtalähteestä. Arduino antaa VAIN datan (keltainen johto) ja
yhteisen maan. Ohjelma rajoittaa kirkkauden (MAX_BRIGHTNESS 128, noin puolet), joten testi ei pyydä täyttä
5 A:a; nosta rajaa vasta kun ensimmäinen ajo meni hyvin.

Grove-kaapelin muutos: vedä jokaisen tikun kaapelin shieldin puoleisesta Grove-liittimestä punainen (VCC) ja
musta (GND) napa ulos muovikotelosta (nosta pieni salpa neulalla) ja eristä ne, jotta shieldin 5 V ja tikkujen
syöttö eivät yhdisty kaapelien kautta. Keltainen (SIG) jää. Tee sama kaikille 10 kaapelille.
Punaisen ja mustan johdon tikun puoleiset päät menevät virtalähteen kiskoihin.

Johdotus (tarkasti; katso wiring-diagram.pdf)
---------------------------------------------
  Tikku  Datanasta (330 ohmin kautta)   Tikku  Datanasta (330 ohmin kautta)
   1     D2                                6     D7
   2     D3                                7     D8
   3     D4                                8     A0
   4     D5                                9     A1
   5     D6                               10     A2
  Tikun n keltainen johto (DIN) -> 330 ohmin vastus -> yllä oleva nasta.
  Jokaisen tikun punainen johto -> +5 V:n kisko (virtalähteen V+ 10 A:n sulakkeen kautta).
  Jokaisen tikun musta johto    -> 0 V:n kisko (virtalähteen V-).
  1000 uF kondensaattori kiskojen yli, plus +5 V:ään.
  Arduinon GND -> 0 V:n kisko YHDELLÄ johdolla (ainoa maayhteys). Älä yhdistä Arduinon 5V-nastaa kiskoihin.
  Valinnainen painonappi D22:n ja GND:n välissä: ohittaa vaiheen.
  Shieldin 3V3/5V-kytkimen (jos käytössä) tulee olla asennossa 5 V.

Päällekytkentä (järjestys)
--------------------------
  1. Tarkista johdotus, +5 V ja 0 V välillä ei saa olla oikosulkua.
  2. Kytke 5 V:n virtalähde päälle ensin (tikut pysyvät pimeinä).
  3. Kytke USB-kaapeli (Arduino käynnistää testin). Sammuta päinvastaisessa järjestyksessä.
  4. Arduino IDE: asenna kirjasto "Adafruit NeoPixel", kortti Arduino Mega tai Mega 2560, avaa sketch.ino,
     Upload. Serial Monitor 115200 baudia näyttää vaiheet.

Mitä testi tekee (toistuu)
--------------------------
  - IDENTIFY: jokainen tikku syttyy valkoisena vuorollaan, 1,2 s kukin (tikku 1..10). Tarkista numerointi.
  - COLOURS: kaikki tikut punaisella, sitten vihreällä, sitten sinisellä. Tarkista värijärjestys.
  - WALK: yksi valkoinen led kulkee kaikkien 100 ledin läpi. Tarkista että jokainen led toimii.
  - RAMP: kaikki ledit valkoisina, kirkkaus nousee portaittain (16, 32, 64, 96, 128), 5 s kukin. Katso
    himmenemistä, värin siirtymistä punaiseen/keltaiseen kaukaisimmissa tikuissa, välkettä tai resettejä.
    Serial Monitor näyttää arvioidun virran. Pinnin 13 led palaa RAMP- ja SOAK-vaiheessa.
  - SOAK: 30 s suurimmalla kirkkaudella; sitten 3 s tauko ja kierros alkaa alusta.
  Painike (D22) ohittaa nykyisen vaiheen.

Mitä raportoida takaisin
------------------------
  - Syttyvätkö kaikki tikut oikeassa järjestyksessä ja väreillä?  - Välkettä, resettejä, himmenemistä, keltaista sävyä?
  - Pysyykö virtalähde viileänä ja jännite lähellä 5 V:a täydellä kirkkaudella (yleismittari viimeisellä tikulla)?
  - Toimivatko tikut 330 ohmin vastuksilla? Käytitkö ACTIVE_STICKS 9 vai 10?
  Huom: "arvioitu virta" on laskelma (20 mA per kanava), ei mittaus.

Asetukset
---------
  ACTIVE_STICKS = 10 (aseta 9, jos tikkuja on vain yhdeksän; nastalista pysyy samana).
  MAX_BRIGHTNESS = 128 (0..255). IDENTIFY-, COLOUR- ja WALK-kirkkaudet ovat erikseen ja matalia.

Huomioita
---------
  - Grove-tikkujen kuumavaihtoa ei tueta: katkaise kaikki virrat ennen kaapelien vaihtoa.
  - Tämä testi on erillinen kasinokyltistä eikä muuta siinä mitään.
  - Hosttesti tests/stick_test_harness.cpp tarkistaa ohjelman logiikan mock-kirjastolla (ei oikealla laitteella).
