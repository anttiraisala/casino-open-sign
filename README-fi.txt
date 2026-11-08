KASINOKYLTTI "CASINO OPEN" - Arduino Mega 2560 + 10 WS2812B-LED-nauhaa (Wokwi-projekti)
=====================================================================================

PROJEKTIN TILA (6.11.2026)
--------------------------
- VAIHE 1 VALMIS (7.11.2026): kyltin ohjelma (sketch.ino) ja diagram.json käyttävät nyt tikkunuolta: 10 Grove RGB
  LED -tikkua Grove Base Shieldin liittimissä D2-D8 ja A0-A2, yksinkertainen virtaava nuolianimaatio. Kehys, kirjaimet,
  kytkin ja painike siirrettiin vapaisiin pinneihin (kehys D9/D10, kirjaimet D11, nuolikytkin D32,
  painike D33). Vanha kuudesta ledinauhasta tehty nuoli on varmuuskopiona kansiossa backup/old_arrow_strips/
  (ei enää kytkettynä). Ei vielä käännetty oikealle Arduinolle eikä ajettu Wokwissa minun toimestani: aja se
  Wokwissa ja kerro mitä näet.
  Suunniteltu mutta EI toteutettu: 12 V:n WS2811-nauhat, 60 x 30 cm levy (alla olevat osiot kuvaavat vielä
  kehyksen ja kirjainten 5 V:n nauhasuunnitelman).
- Käyttäjän oikealla laitteella testaama: stick_test_direct. Kaikki 11 Grove RGB LED -tikkua muuttamattomina
  Grove Base Shieldissä (liittimet D2..D8, A0..A3), Arduino + shield ulkoisesta 5 V:n virtalähteestä, kaikissa tikuissa
  pyörivät liikkuvat kuviot. Virta ja data riittävät siis 11 tikulle (stick_test, muokatuilla kaapeleilla,
  ei siksi ollut tarpeen).
- Ei vielä testattu oikealla laitteella: strip_test (WS2812B-nauha) ja itse kyltin ohjelma.
- Sign of life -sääntö (pin 13 -ledi + yksi Serial-rivi sekunnissa, 115200 baudia) on ohjelmassa sketch.ino ja
  stick_test_direct; strip_test ja stick_test saavat sen seuraavassa päivityksessä (vaihe 2, katso AI_CONTEXT.md).
- Osalistat ovat tilannekuva 2.11.2026 (hintoja, saatavuutta ja linkkejä ei päivitetä itsestään).

Tämä on README.txt:n suomennos. Jos tiedostot poikkeavat toisistaan, englanninkielinen
README.txt on pääversio.

Tiedostot
---------
DESCRIPTION.md             Mikä projekti on, mihin sitä käytetään ja mitä on suunnitteilla (englanniksi).
AI_CONTEXT.md              Tarkka tekninen kuvaus uutta tekoälykeskustelua varten (englanniksi).
sketch.ino                 Arduino-ohjelma. Kaikki asetukset ovat tiedoston alussa.
diagram.json               Wokwi-kytkentä: Arduino Mega, kyltin muotoon asetetut
                           LED-nauhaosat, liukukytkin, painike ja muutama tekstikyltti.
libraries.txt              Tarvittava kirjasto: Adafruit NeoPixel.
tools/generate_diagram.py  Luo diagram.json:n uudelleen sketch.ino:n asetuksista.
tools/size_calculator.py   Ehdottaa LED-määrät annetulle kyltin koolle.
wiring-diagram.svg/.pdf    Sähköinen kytkentäkaavio, yksi pysty-A4 (tulosta 100 %:lla).
layout-diagram.svg/.pdf    Mihin kukin LED-nauha tulee levyllä: mittakaavassa, nimillä,
                           pinneillä, LED-määrillä, LED-indeksialueilla ja mitoilla (cm).
                           Yksi vaaka-A4 (levy on piirretty noin mittakaavassa 1:2,4).
layout-parts.svg/.pdf      Osaluettelosivu (pysty-A4): LED-määrät, leikkuulista sekä
                           jokaisen kirjaimen LEDit ja indeksialue.
tests/                     Koneella ajettava testiohjelma (katso AI_CONTEXT.md, luku 8).
parts-list.pdf/.csv/.xlsx  Täydellinen ostoslista: jokainen osa määrineen, hintoineen, varastotilanteineen ja
                           linkkeineen partcolle (PDF tulostettavaksi, CSV mihin tahansa taulukko-ohjelmaan,
                           XLSX kaavoilla laskettuine rivisummineen ja summineen). Osalistan pääversio.
tools/parts_data.py        Osalistan tiedot (muokkaa osia, määriä ja hintoja täällä).
tools/generate_parts_list.py  Tekee kolme osalistatiedostoa.
strip_test/                Erillinen pieni testiprojekti LED-nauhalle: leikkaa nauhasta lyhyt pätkä, aja
                           strip_test/sketch.ino (Arduino tai Wokwi tiedostolla strip_test/diagram.json) ja kerro, mitä
                           näet (värijärjestys, bittinopeus, LEDejä per pikseli). Katso strip_test/README-fi.txt.
strip_single_led_test/     Testi: kolme 12 V WS2811 -nauhaa pinneissä D9/D10/D11; yksi pikseli kulkee nauhaa pitkin, jotta
                           oikea pikselimäärä selviää (150 LEDiä = luultavasti 50 pikseliä) ja värijärjestys tarkistuu. Katso strip_single_led_test/README-fi.txt.
strip_knight_rider_test/    Demo: kolme 12 V WS2811 -nauhaa à 50 pikseliä; yksi valo pomppii edestakaisin (D9 punainen 1 pikseli/s,
                           D10 sininen 4/s, D11 vihreä 10/s). Katso strip_knight_rider_test/README-fi.txt.
sticks_11_test/             Testi "11 sticks": nuolen 10 tikkua + CLOSED-valotikku (A3) Grove-shieldissä, pidetään päällä, jotta
                           nähdään lämpeneekö mikään. Katso sticks_11_test/README-fi.txt.
stick_test/                Erillinen testi 10 Grove RGB LED -tikulle (suunniteltu tikkunuoli) erillisellä 5 V:n virtalähteellä:
                           lataa stick_test/sketch.ino Arduino IDE:llä. Katso stick_test/README-fi.txt.
stick_test_direct/        Toinen tikkutesti: enintään 11 tikkua muuttamattomina Grove-shieldiin, jatkuvasti liikkuvat kuviot,
                           ulkoinen 5 V Megan 5V/GND-napoihin.
arrow_sticks/              Tikkunuoli: porausmalli 1:1 (PDF), asettelu ja johdotus. Katso arrow_sticks/README-fi.txt.
                           Katso stick_test_direct/README-fi.txt.
backup/old_arrow_strips/   Edellinen kyltti (nuoli kuudesta ledinauhasta nastoissa D4, D5, D9-D12): ohjelma, diagram.json ja
                           poistettujen osien lista. Varmuuskopio, ei enää kytkettynä.
tools/export_pdf.py        Tekee kolme PDF:ää kolmesta SVG:stä (tarvitsee Playwrightin).
tools/generate_schematic.py  Piirtää wiring-diagram.svg:n uudelleen sketch.ino:n asetuksista.
tools/generate_layout.py   Piirtää layout-diagram.svg:n ja layout-parts.svg:n uudelleen sketch.ino:n asetuksista.
tools/sign_data.py         Kahden piirtoskriptin yhteinen nauhadata.
README.txt                 Tämän tiedoston englanninkielinen versio.
README-fi.txt              Tämä tiedosto (suomi).

Sisältö
-------
 1. Projektin avaaminen Wokwissa
 2. Kyltti Wokwissa
 3. Kyltin koko ja LED-määrät
 4. Kytkennät: mikä menee mihinkin pinniin
 5. Ostettavat osat (partco.fi)
 6. Nauhat, LEDien järjestys ja leikkuulista
 7. Koon tai tekstin muuttaminen
 8. Animaatiot ja syötteet
 Kirkkauskäyrät (sketch.ino, "Brightness curves")
-------------------------------------------------
Värit lasketaan kuten ennenkin. Kunkin värin kirkkaus (suurin kanava) kuljetetaan sitten 256 arvon hakutaulukon
läpi, yksi taulukko per ledityyppi: LED_TYPE_STRIP (kehys, kirjaimet) ja LED_TYPE_STICK (nuoli, CLOSED-valo).
Kaikki kolme kanavaa skaalataan samalla kertoimella, joten sävy säilyy.
  ulos = min + (max - min) * ((1 - mix) * t + mix * t^eksponentti),  t = sisään / 255,  pyöristetty 0..255
Per tyyppi: min (0..255), max (0..255), eksponentti (0,1..20, 1 = lineaarinen), mix (0..1, 0 = lineaarinen,
1 = täysi potenssikäyrä) ja keepZero (syöte 0 pysyy nollana). Oletukset (CURVE_DEFAULT_* sketchin alussa),
samat molemmille tyypeille: min 0, max 80, eksponentti 2,67, mix 1,0, keepZero true. Taulukot rakennetaan
käynnistyksessä; setBrightnessCurve(tyyppi, min, max, eksponentti, mix, keepZero) rakentaa yhden taulukon heti
uudelleen (256 powf-kutsua, vain kun arvo muuttuu). Uusi ledityyppi tarvitsee uuden LED_TYPE_*-vakion,
suuremman LED_TYPE_COUNTin ja yhden arvon lisää jokaiseen oletustaulukkoon.
Globaalia kirkkautta ei enää ole: kirjaston kirkkaus pysyy arvossa 255, ja kunkin käyrän max on ledityypin
kirkkausraja (oletus 80/255, sama raja kuin vanhalla globaalilla asetuksella; nosta sitä vasta kun olet
tarkistanut virtalähteen, katso Virta-kappale luvussa 2). Käyrän arvoille ei ole vielä käyttöliittymää (suunnitteilla: valikko ja enkooderit).

9. Huomioita oikeasta laitteistosta


1. Projektin avaaminen Wokwissa
-------------------------------
1. Mene osoitteeseen https://wokwi.com/projects/new/arduino-mega
2. Jokainen tiedosto menee samannimiselle välilehdelle:
     sketch.ino   -> sketch.ino-välilehti (C++-koodia, alkaa /* -kommentilla)
     diagram.json -> diagram.json-välilehti (JSONia, alkaa merkeillä { ja "version": 1)
   Valitse välilehden koko sisältö (Ctrl+A) ja liitä koko tiedosto sen päälle. Jos
   sketch päätyy diagram.json-välilehdelle, Play-nappi katoaa, koska kaavio ei ole
   kelvollista JSONia.
3. Lisää kirjasto: joko laita libraries.txt libraries.txt-välilehdelle (luo välilehti
   "+"-napista, jos sitä ei ole) tai lisää "Adafruit NeoPixel" Library Managerista.
4. Paina vihreää Play-nappia. Zoomaa kaaviota (+ / - / F -näppäimet), niin näet koko
   kyltin.


2. Kyltti Wokwissa
------------------
Wokwissa ei ole piirtämiseen tai taustakuvaan sopivaa osaa, joten kyltti on rakennettu
itse LED-nauhoista. Jokainen nauhaosa on siinä kohdassa, jossa se olisi oikeassa
kyltissä:
  - kehys on neljän reunan rengas,
  - "CASINO" on "OPEN":in yläpuolella; jokaisen kirjaimen takana on ryhmä yksittäisiä
    LEDejä, jotka on piirretty ympyräksi, ja kirjain on kirjoitettu keskelle (Wokwin
    tekstikyltti; sen kokoa ei voi säätää, joten se ei välttämättä mahdu ympyrään
    täsmälleen),
  - OPEN-rivin alla on valkoinen 10 LEDin nauhaosa (CLOSED-valo = 11. Grove-tikku liittimessä A3) tekstikyltin
    kanssa; oikeassa kyltissä se on levyn takana reikätekstin "CLOSED" alla,
  - 10 Grove-tikusta tehty nuoli on piirretty kehyksen ALLE (levyalueen ulkopuolelle) kymmenenä
    16 pikseliä/LED -nauhaosana, joissa on tikkujen nimet (L-CAP, RAIL-UP-L ...). Wokwissa ei ole Grove-
    tikkua, joten se on piirretty Wokwin LED-koolla (16 px/LED); oikea nuoli on levyllä 349,76 x 102,32 mm
    (katso layout-diagram.svg ja arrow_sticks/),
  - syötteet (kytkimet, painike, DIP-rivi) ovat Arduinon yläpuolella selitteet niiden yläpuolella,
    ja I2C-LCD on Arduinon alapuolella.
Wokwissa 60 LEDiä/m -nauha on piirretty 16 pikselillä per LED, eli 16 pikseliä
kaaviossa on oikeassa kyltissä 16,7 mm, ja koko kehys on 480 x 432 pikseliä eli
50 x 45 cm. SW5 (kasino suljettu) alkaa Wokwissa auki-asennossa. Levyyn leikattua
CLOSED-reikätekstiä Wokwi ei pysty näyttämään, joten sen tilalla on tekstikyltti.

45 cm:n levyn pystyjako ylhäältä: kehyksen reuna, kaksi tekstiriviä (CASINO, OPEN),
CLOSED-reikäteksti (noin 2,5 cm korkea ja 10 cm leveä), nuoli (16,7 cm korkea) ja
kehyksen reuna. Nuolen alle jää noin 5 cm vapaata tilaa.
Vierekkäiset erilliset nauhat on piirretty yhden LEDin päähän toisistaan, jotta jokaisen
nauhan näkee erikseen. Oikeassa kyltissä nauhat ovat kiinni toisissaan. Osa kaavion
osista on vain yhden taivutetun nauhan paloja (esimerkiksi kirjaimen yksittäiset LEDit
tai kärjen kaksi siipeä), ja ne on ketjutettu Wokwin sisällä. Oikealla olevat
tekstikyltit kertovat, mikä Arduinon pinni ohjaa mitäkin nauhaa. DIP-kytkinrivi korvaa
kaksi nopeuskytkintä (katso luku 8).

Jos osat menevät Wokwissa vähän päällekkäin tai väliin jää rakoja, raahaa osia tai säädä
vakioita PITCH, THICKNESS ja LENGTH_PADDING tiedoston tools/generate_diagram.py alussa
ja aja se uudestaan.


3. Kyltin koko ja LED-määrät
----------------------------
Lähtökohta (voi vielä muuttua):
  LED-nauha        WS2812B, 5 V, 60 LEDiä metrillä (LEDien väli 16,7 mm)
  kyltti (levy)    leveys 50 cm, korkeus 40 cm
  nuoli            10 Grove RGB LED -tikkua, 349,76 x 102,32 mm (arrow_sticks/); alla olevat vanhan
                   nauhanuolen luvut ovat vain varmuuskopiota varten

Tuloksena LED-määrät (pyöristetty kokonaisiin LEDeihin):
  kehys            2 x 53 = 106 LEDiä   (ylä- ja alareuna 28 LEDiä, vasen ja oikea reuna 25)
                   LED-renkaan uloin reuna: 50,0 x 45,0 cm
  kirjaimet        58 LEDiä             (6 per kirjain, kapeassa "I":ssä 4)
  nuoli            68 LEDiä             (vartalon reunat 2 x 10, päädyt 2 x 4,
                                         kärjet 2 x 2 x (2 + 8))
                   koko: 39,1 x 16,7 cm (täsmälleen 15 cm korkeaa nuolta ei saa kokonaisilla
                   LEDeillä ja parillisella päädyllä; toinen vaihtoehto olisi 13,3 cm)
  CLOSED-valo     10 LEDiä             (11. Grove-tikku liittimessä A3, valkoinen; palaa vain kasinon ollessa suljettu)
  yhteensä         238 LEDiä = 3,97 m nauhaa, joten yksi 5 m kela (300 LEDiä) riittää.

Virta: WS2812B-LED ottaa enintään 60 mA (täysi valkoinen, täysi kirkkaus). 238 LEDiä
ottaisi enintään 14,3 A, mihin ohjelma ei lähelläkään pääse: kirkkausraja (käyrän max) on 80/255, joten
vaikka kaikki LEDit olisivat valkoisia, kulutus olisi 4,5 A, ja animaatiot sytyttävät
vain osan LEDeistä. 5 V / 10 A -virtalähteessä on siis reilusti varaa. Älä nosta
käyrän maxia yli noin 150:n tällä virtalähteellä.

Muille koille aja
    python3 tools/size_calculator.py --width 60 --height 45 --arrow-width 45 --arrow-height 18
(ilman valitsimia se tulostaa yllä olevat arvot) ja kopioi tulostetut vakiot
sketch.ino:n CONFIGURATION-osaan. Aja sen jälkeen tools/generate_diagram.py.


4. Kytkennät: mikä menee mihinkin pinniin
-----------------------------------------
Sähköinen kytkentäkaavio on tiedostossa wiring-diagram.svg (symbolein piirretty kuva,
jossa kaikki kymmenen nauhaa, vastukset, kytkimet ja painike). Alla oleva teksti kertoo
saman.

Yleiskuva (5 V:n virta ja data):

    230 V:n verkkojännite
         |   (kiinteä johdotus vain sähköasennuksen osaavan tekemänä, kotelon sisällä)
    [ Mean Well LRS-50-5 :  5 V, 10 A ]
         | V+                                   | V-
    [ 10 A sulake ]                             |
         |                                      |
         +------ +5 V -kisko -+--- GND-kisko ---+------------------- Arduinon GND (mikä tahansa GND-pinni)
         |                    |                 |
         |           [ 1000 uF kondensaattori ] |
         |            (+ -> +5 V, raita = -)    |
         |                                      |
         +--> jokaisen LED-nauhapalan +5 V      +--> jokaisen LED-nauhapalan GND

    Tikkunuoli (Grove Base Shield Megan päällä, shieldin kytkin asennossa 5 V): shield välittää Megan 5V/GND:n
    tikuille, joten LED-virtalähteen +5 V -kiskon PITÄÄ syöttää myös MOLEMPIA Megan 5V-pinnejä (pitkän
    pinnirivin pää) ja GND-kiskon molempia Megan GND-pinnejä (kuten stick_test_directissä). Mega saa silloin
    virtansa virtalähteestä. ÄLÄ kytke USB:tä kun virtalähde on päällä. Ohjelman lataus: kytke virtalähde pois,
    kytke USB, lataa, irrota USB, kytke virtalähde takaisin päälle.

Datajohdot (Arduinon pinni -> 330 ohmin vastus -> nauhan ensimmäisen LEDin DIN):

    Pinni  Vastus    Menee                         Nauha (LEDejä)
    D9     330 ohm   frameCw DIN                   kehys, yläreuna ja sitten oikea reuna (53)
    D10    330 ohm   frameCcw DIN                  kehys, vasen reuna ja sitten alareuna (53)
    D11    330 ohm   ensimmäisen kirjaimen DIN     kirjaimet, C ... O, sitten O P E N (58)

Nuolitikut (ei vastusta, ei juottamista: jokainen tikku kytketään omalla Grove-kaapelillaan
Grove Base Shieldin liittimeen; tikun LED 1 on sen DIN-pää):

    Liitin  Tikku       Paikka
    D2      L-CAP       vasen pystypalkki
    D3      L-WING-UP   vasen yläsiipi
    D4      L-WING-DN   vasen alasiipi
    D5      RAIL-UP-L   yläkisko, vasen tikku
    D6      RAIL-UP-R   yläkisko, oikea tikku
    D7      RAIL-DN-L   alakisko, vasen tikku
    D8      RAIL-DN-R   alakisko, oikea tikku
    A0      R-CAP       oikea pystypalkki
    A1      R-WING-UP   oikea yläsiipi
    A2      R-WING-DN   oikea alasiipi
    A3      CLOSED      CLOSED-valo: 10 valkoista LEDiä levyn takana (ei kuulu nuoleen)

Syötteet (sisäinen ylösvetovastus, ohjelma käyttää INPUT_PULLUP-tilaa):

    D32  -> liukukytkimen keskimmäinen (yhteinen) kontakti
            liukukytkimen toinen ulkokontakti -> Arduinon GND
            (kahva GND-puolella = pinni LOW = nuoli osoittaa vasemmalle;
             kahva toisella puolella = pinni HIGH = nuoli osoittaa oikealle)
            Asennus: kahva osoittaa samaan suuntaan kuin nuoli oikeassa tilassa. Kilven
            TAKAA katsottuna GND-kontakti (1) tulee oikeanpuoleiseksi.
    D33  -> painikkeen yksi kontakti
            painikkeen toinen kontakti -> Arduinon GND
            (jokainen painallus = seuraava kirjainmoodi)
    D30  -> liukukytkimen SW5 "kasino suljettu" keskimmäinen (yhteinen) kontakti
            liukukytkimen toinen ulkokontakti -> Arduinon GND
            (kahva GND-puolella = pinni LOW = kasino suljettu: OPEN-kirjaimet pimenevät ja
             CLOSED-valo syttyy; kahva toisella puolella = pinni HIGH = kasino auki)
    D22, D23, D24, D25 -> heksakiertokytkimen SW3 pinnit 1, 2, 4, 8 (kehyksen ja nuolen nopeus)
    D26, D27, D28, D29 -> heksakiertokytkimen SW4 pinnit 1, 2, 4, 8 (kirjainten nopeus)
    kummankin heksakytkimen yhteinen pinni (C) -> Arduinon GND
            (kytkimen asento yhdistää sen bittipinnit yhteiseen pinniin; asento lasketaan
             binäärinä, 0..F. Kytkimessä on keskellä kaksi yhteistä pinniä: yhdistä toinen tai molemmat GND:hen.)

I2C-LCD 2x16 (LCD1, I2C-sovitinlevyllä; 4 johtoa):

    LCD GND  -> Arduinon GND
    LCD VCC  -> Arduinon 5V-pinni (noin 30 mA taustavalon kanssa; LED-virtalähteen 5 V:ia EI käytetä)
    LCD SDA  -> Arduinon D20 (SDA)
    LCD SCL  -> Arduinon D21 (SCL)
            (sovitinlevyllä on kontrastitrimmeri; I2C-osoite on Wokwissa 0x27. Jos oikea näyttö
             pysyy tyhjänä, käännä kontrastitrimmeriä ja kokeile sitten osoitetta 0x3F vakiossa
             LCD_I2C_ADDRESS. Näyttö on valinnainen: jos mikään ei vastaa käynnistyksessä, kyltti
             toimii ilman sitä.)

Virta jokaiselle LED-nauhalle (kaikki nauhat saavat virran samasta 5 V:n lähteestä):

    Nauhapala          +5 V ja GND liitetään kiskoon kohdasta
    frameCw            yläreunan alku JA oikean reunan loppu
    frameCcw           vasemman reunan alku JA alareunan loppu
    letters            jokainen kirjainryhmä (syötä jokainen ryhmä kiskosta)
    nuolitikut         Grove Base Shieldin kautta (Megan 5V/GND, katso yleiskuva yllä)
    CLOSED-valotikku   sekin Grove Base Shieldin kautta (ei johtoa virtalähteestä yhteenkään tikkuun)

Kun nauha koostuu useasta palasta (kehysnauhan kaksi reunaa, kärjen neljä palaa,
kirjainryhmät), yhdistä peräkkäiset palat kolmella johdolla: edellisen palan DOUT ->
seuraavan palan DIN sekä molempien palojen +5 V ja GND kiskoon. Yhdistä aina nauhan
DIN-pää datajohtoon ja DOUT-pää seuraavaan palaan (nauhaan painettu nuoli näyttää
suunnan).

Huomioita
  - Megan 5 V:n logiikkataso ohjaa WS2812B:tä suoraan, tasomuunninta ei tarvita.
  - Aseta 330 ohmin vastukset lähelle kunkin nauhan ensimmäistä LEDiä.
  - Käytä +5 V- ja GND-kiskoon paksua johtoa (vähintään 1 mm2, mitä lyhyempi sen
    parempi) ja pidä datajohdot lyhyinä.
  - Älä koskaan yhdistä LEDien virtalähteen +5 V:ia Arduinon 5 V -pinniin, kun USB on
    kiinni.
  - Yhdistä aina virtalähteen GND Arduinon GND:hen, muuten datajohdot eivät toimi.


5. Ostettavat osat (partco.fi)
------------------------------
Tarkistettu 2026-11-02 osoitteesta https://partco.fi. Hinnat sisältävät arvonlisäveron
(25,5 %). Saatavuus muuttuu, joten tarkista se uudestaan ennen tilaamista. Pelkkää
kytkentäjohtoa ei ole listattu tähän.
Täydellinen ostoslista (kaikki osat, määrät, summat ja linkit) on tiedostoissa parts-list.pdf / .csv / .xlsx.
HUOM: parts-list.* on tehty SUUNNITELLULLE 12 V:n väri-nauhalle (12 V:n virtalähde, 5 A:n sulake, 25 V:n kondensaattori,
nauhan testiosat, varmuusrivit); alla oleva taulukko kuvaa edelleen sketchin ja kuvien nykyistä 5 V:n suunnitelmaa.
Hinnat, varastotilanne ja linkit ovat tilannekuva 2.11.2026, eikä niitä päivitetä ilman erillistä pyyntöä:
tarkista ne uudestaan ennen tilaamista.

  Osa                   Kpl  Tuote (partco.fi)                           Tuotekoodi              Hinta      Varasto (2026-11-02)
  Arduino Mega 2560     1    Arduino Mega 2560 R3 (A000067)              ARD-A000067             58,00 EUR  varastossa (15 kpl 2026-11-02)
                             https://partco.fi/tuote/arduino-mega-2560-r3-a000067-2178
     vaihtoehto         1    Seeeduino Mega (ATmega2560)                 SEEED-102010007         46,70 EUR  varastossa
                             https://partco.fi/tuote/seeeduino-mega-atmega2560-2531
                             (Mega-yhteensopiva; aseta sen 5 V / 3,3 V -kytkin 5 V:iin. En ole tarkistanut sen
                              pinnien numerointia Arduino Megaan verrattuna, joten tarkista ennen kuin luotat siihen.)
  LED-nauha WS2812B     1    RGB-LEDNAUHA 60kpl/m WS2812 5m              LED-N-RGB-W2812X300-5M  136,00 EUR loppu, saatavuus ei tiedossa
  60 LEDiä/m, 5 V, 5 m       https://partco.fi/tuote/rgb-lednauha-60kpl-m-ws2812-5m-104
                             (5 V, WS2812, LEDit noin 17 mm välein, leikattavissa LEDien välistä. Tarkistushetkellä
                              tämä oli ainoa sopiva nauha, eikä se ollut varastossa.)
     ÄLÄ osta           -    RGB-LEDNAUHA 30kpl/m WS2811 5m              LED-N-RGB-W2812X150-5M  70,37 EUR  varastossa, mutta se on 12 V:n
                             https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103          WS2811-nauha: ei toimi tämän 5 V:n ohjelman kanssa
     pienempi vaihtoehto-    Digital RGB LED Flexi-Strip 60 LED - 1 Meter  SEEED-104990014      29,92 EUR  loppu, saatavuus ei tiedossa
                             https://partco.fi/tuote/digital-rgb-led-flexi-strip-60-led-1-meter-122
                             (näitä tarvittaisiin neljä)
  5 V:n virtalähde      1    HAKKURI MODULI 50W 5VDC 10A (Mean Well)     LRS-50-5                23,79 EUR  varastossa
                             https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777
                             (verkkojännitteellä toimiva moduuli ruuviliittimillä: se tarvitsee kotelon, verkkojohdon
                              ja pistokkeen, ja verkkopuolen saa johdottaa vain sähköasennuksen osaava)
  Sulakekanta + sulake  1+1  AUTOSULAKEPESÄ JOHDOT PÄÄSTÄ (johdoilla)   SULPES-APUR             3,56 EUR   varastossa
                             https://partco.fi/tuote/autosulakepes%C3%A4-johdot-p%C3%A4%C3%A4st%C3%A4-4732
                             AUTOSULAKE 32V 10A punainen                 SUA-10A                 0,31 EUR   varastossa
                             https://partco.fi/tuote/autosulake-32v-10a-punainen-5320
  Kondensaattori        1    ELEKTROLYYTTIKOND. 1000µF 16V 10x16mm       ELKO105-1000U-16V       1,62 EUR   varastossa
                             https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-16v-10x16mm-12237
  Liukukytkin           2    1-NAPAINEN LIUKUKYTKIN ON/ON                KYT-LIU15               0,50 EUR   varastossa
  (SPDT, nuoli + kasino suljettu)              https://partco.fi/tuote/1-napainen-liukukytkin-on-on-4857
  Painike               1    palautuva painike (normaalisti auki)        -                       -          ei tutkittu tarkemmin
  (kirjainmoodi)             Katso Kytkimet-kategorian "Painokytkimet"-alakategoriaa:
                             https://partco.fi/kategoria/komponentit/s%C3%A4hk%C3%B6mekaaniset-komponentit/kytkimet
                             (ensimmäisellä sivulla näkyvät painokytkimet olivat lukkiutuvia tai valaistuja;
                              valitse pieni palautuva (ON)-painike.)
  LCD-näyttö            1    LCD-NÄYTTÖ 2x16 SINIVALKO I2C-LIITÄNTÄ      LCD-2X16-I2C            13,40 EUR  varastossa (45 kpl)
  (I2C, 2x16, sininen)       https://partco.fi/tuote/lcd-n%C3%A4ytt%C3%B6-2x16-sinivalko-i2c-liit%C3%A4nt%C3%A4-5193
                             (Joy-IT SBC-LCD16x2: takana on valmiiksi juotettu I2C-sovitinlevy (PCF8574AT); 4 pinniä
                              GND, VCC, SDA, SCL; 5 V; kontrastitrimmeri ja taustavalon jumpperi levyllä; moduuli
                              80 x 36 mm. Sivun mukaan I2C-osoite on 0x27 ja sitä voi muuttaa juotosjumpperilla.)
  Heksakiertokytkin     2    HEX ENKOODERI/KIERTOKYTKIN TAPILLA 0..F     KIERTOKYTKIN-216        3,43 EUR   varastossa (4 kpl)
  (nopeus, SW3 + SW4)        https://partco.fi/tuote/hex-enkooderi-kiertokytkin-tapilla-0-f-4851
                             (16-asentoinen, binäärikoodattu kiertokytkin piirilevylle, runko 10 x 10 x 5,5 mm,
                              tapin halkaisija 3,8 mm ja pituus 7 mm. Se EI ole pulssi-enkooderi: se antaa asennon
                              0..F neljällä pinnillä, mitä nopeudensäätö käyttää. Sivu ei kerro, mikä pinni on mikä
                              (keskellä kaksi yhteistä pinniä, loput ovat binääripinnit 1, 2, 4, 8) eikä sitä, onko
                              koodi suora vai komplementti; kokeile ja aseta tarvittaessa SPEED_BIT_IS_LOW.)
  Nuppi kytkimelle      2    3,8 mm:n tappiin sopiva nuppi                -                       -          ei etsitty
  Reikälevy             1    KYTKENTÄLEVY 200x100mm JUOTOSLIUSKOILLA PERTINAX   KL-PL200          6,50 EUR   varastossa (10 kpl)
  (ohjauslevy)               https://partco.fi/tuote/kytkent%C3%A4levy-200x100mm-juotosliuskoilla-pertinax-14786
                             (200 x 100 mm, 2,54 mm:n väli, 1,5 mm paksu, 1,0 mm:n reiät. Sivu ei kerro, miten
                              kuparijuotosliuskat kulkevat. Käytä sitä kymmenelle 330 ohmin vastukselle,
                              kondensaattorille, sulakekannalle, kahdelle heksakytkimelle ja Arduinon ja nauhojen
                              välisille liitoksille.)
  Vastukset 330 ohm     10+  läpireikä 0,25 W 330 ohmin vastukset        -                       -          ei tutkittu tarkemmin
  (datajohdot)               Vastukset-kategoriassa käytä suodatinta "Resistanssi 330ohm"
                             (15 tuotetta tarkistushetkellä):
                             https://partco.fi/kategoria/komponentit/passiivikomponentit/vastukset
                             (parts-list.xlsx käyttää 2 W:n tyyppiä VASTUS-2W-330R, 0,45 EUR, varastossa: 0,5 W:n tyyppi
                              FRES-05W-330R on poistunut valikoimasta.) Osta vähintään 12, jotta jää varalle.
  USB-kaapeli A-B       1    USB A - USB B -kaapeli Arduino Megalle      -                       -          ei löytynyt tarkistamiltani sivuilta
                             (sinulla on luultavasti sellainen; muuten katso Tietokonekaapelit:
                              https://partco.fi/kategoria/kaapelit/tietokonekaapelit )

Tarvitaan myös, mutta ei etsitty: verkkojohto pistokkeella ja kotelo virtalähteelle,
juotostarvikkeet ja kutistesukka, kyltin oma levy (johon leikataan CLOSED-reikäteksti),
huurrettu muovi reikätekstin taakse sekä pleksikirjaimet. Raspberry Pi:tä
ei tarvita. Tietokonetta tarvitaan vain ohjelman lataamiseen.


6. Nauhat, LEDien järjestys ja leikkuulista
-------------------------------------------
layout-diagram.svg näyttää tämän kaiken piirrettynä: jokainen nauha paikallaan
50 x 45 cm:n levyllä, niiden nimet, pinnit, LED-määrät, jokaisen palan LED-määrä ja
jokaisen palan LED-indeksialue. "letters" on YKSI nauha (yksi pinni, 58 LEDiä), joka
leikataan 10 palaksi, yksi per kirjain; kuvan alareunan taulukossa on jokaisen
kirjaimen LED-määrä ja indeksialue.

Jokainen looginen nauha on oma Adafruit_NeoPixel-oliinsa ja sillä on oma datapinninsä.
Kaaviossa jokaisen nauhan ensimmäinen osa saa DIN:in datapinnistä, VDD:n 5V:sta ja
VSS:n GND:stä; seuraavat osat on ketjutettu (DOUT -> DIN, VDD.2 -> VDD, VSS.2 -> VSS).

Kehys      frameCw: yläreuna vasemmalta oikealle ja sitten oikea reuna alaspäin.
           frameCcw: vasen reuna alaspäin ja sitten alareuna vasemmalta oikealle.
           Molemmissa nauhoissa on FRAME_HALF_LEDS LEDiä. Ylä- ja alareunassa on
           FRAME_TOP_LEDS LEDiä, vasemmassa ja oikeassa reunassa
           FRAME_HALF_LEDS - FRAME_TOP_LEDS.
           Ohjelma tarvitsee FRAME_TOP_LEDS-arvon tietääkseen, mihin yksi reuna päättyy
           ja mistä seuraava alkaa. Kulma jätetään pimeäksi: leikkaa jokainen nauha
           kulmasta ja yhdistä palat johdoilla (data, 5 V, GND).
Kirjaimet  Yksi LED-ryhmä per kirjain lukujärjestyksessä: C A S I N O ja sitten O P E N.
           Leikkaa nauha ryhmien välistä ja yhdistä ryhmät johtosilloilla (data, 5 V,
           GND), jolloin LEDejä ei mene hukkaan kirjainten välissä. Ryhmän LEDit voi
           asetella kirjaimen taakse missä muodossa tahansa; määrä per kirjain on vapaa
           (yleensä 4–8). Wokwissa ne on piirretty ympyröinä vain selkeyden vuoksi.
Nuoli      Vartalon reunat kulkevat vasemmalta oikealle ja päädyt ylhäältä alas. Jokainen
           kärki on yksi nauha: pohjapala pään yläpuolella (alkaa pään vierestä ja
           menee ylös), ylempi siipi (kärkeen), alempi siipi (kärjestä) ja pohjapala
           pään alapuolella (päättyy pään viereen). Pääty on kolmion pohjan keskiosa ja
           kaksi pohjapalaa muodostavat pohjan loput.
           Nauhaa ei voi taivuttaa jyrkästi sen tasossa, joten leikkaa kärki neljään
           palaan ja yhdistä palat johdoilla siipien päissä ja kärjessä.
CLOSED-    closedLight: 11. Grove-tikku (runko 83 x 10 mm, LED-ikkuna 74 x 4 mm, 10 valkoista LEDiä)
valo       shieldin liittimessä A3; sitä ei leikata. Se on levyn TAKANA OPEN-rivin alla ja
           valaisee reikätekstin CLOSED (noin 10 x 2,5 cm, leikattu levyyn). Laita tikun ja
           reiän väliin huurrettua muovia, muuten LEDit näkyvät pisteinä. LED-ikkuna (74 mm) on
           kapeampi kuin teksti (noin 100 mm): uloimmat kirjaimet saavat valoa vain diffuusorin
           kautta (jätä tikun ja muovin väliin rako tai tee reikäteksti kapeammaksi). Toisin kuin
           muut LEDit nämä osoittavat levyn takapuolelle.

Leikkuulista oletuskoilla (leikkaa nauha vain merkityistä leikkauskohdista kahden LEDin
välistä, 16,7 mm välein):
  frameCw          28 (yläreuna) + 25 (oikea reuna)         2 palaa, 1 liitos
  frameCcw         25 (vasen reuna) + 28 (alareuna)         2 palaa, 1 liitos
  letters          6 6 6 4 6 6   6 6 6 6                    10 palaa, 9 liitosta
  (kuusi nuolinauhaa on poistettu: nuoli on nyt 10 Grove-tikkua, joita ei leikata)
  Yhteensä 14 palaa ja 164 LEDiä nauhaa (kehys, kirjaimet) sekä 11 tikkua à 10 LEDiä (nuoli + CLOSED-valo).


7. Koon tai tekstin muuttaminen
-------------------------------
Muokkaa sketch.ino:n alun CONFIGURATION-osaa:

  FRAME_HALF_LEDS       LEDejä kummassakin kehysnauhassa
  FRAME_TOP_LEDS        LEDejä ylä- ja alareunassa (loput menevät sivuille);
                        animaatio käyttää sitä, joten sen pitää vastata oikeaa kehystä
  LINE_1_TEXT, LINE_2_TEXT   kaksi tekstiriviä (kirjaimet A–Z)
  LETTER_LEDS[]         LEDejä kunkin kirjaimen takana, yksi arvo per kirjain: ensin
                        rivi 1 ja sitten rivi 2. Mikä tahansa määrä käy. Oletukset: 6
                        per kirjain, kapea "I" 4.
  ARROW_BODY_LEDS       LEDejä kummassakin vartalon reunanauhassa (ylä ja ala)
  ARROW_CAP_LEDS        LEDejä kummassakin vartalon päädyssä (parillinen luku)
  ARROW_HEAD_BASE_LEDS  LEDejä kärjen kummassakin pohjapalassa (pään ylä- ja alapuolella)
  ARROW_HEAD_WING_LEDS  LEDejä kärjen kummassakin siivessä
  STARTUP_TEST_MS       käynnistyksen LED-testin pituus (oletus 10000; 0 = ei testiä)
  STARTUP_TEST_PASSES   testin kierrokset (oletus 2: valkoinen, sitten punainen-vihreä-sininen)
  STARTUP_TEST_WINDOW_LEDS  yhtä aikaa palavat LEDit kussakin nauhassa (oletus 5)
  LCD_TEST_SWAP_MS, LCD_TEST_SCROLL_MS  testinäytön rivinvaihtoaika (2500) ja vierityksen askel (250)
  LCD_I2C_ADDRESS       näytön I2C-osoite (0x27 Wokwissa; 0x3F joissakin sovitinlevyissä)
  LCD_PAGE_MS           yhden LCD-sivun näyttöaika kierrossa (oletus 3000)
  LCD_HOLD_MS           muutetun asetuksen sivun pitoaika (oletus 6000)
  LETTER_START_MODE     moodi käynnistyksen jälkeen (oletus 0)
  LETTER_RANDOM_AUTO_MS random auto -moodin vaihtoväli (oletus 120000 = 2 min)
  LETTER_SCANNER_MS, LETTER_SPARKLE_STEP_MS, LETTER_CHASE_STEP_MS, LETTER_PULSE_MS   uusien moodien ajat
  CLOSED_LIGHT_LEDS     LEDejä CLOSED-valotikussa (oletus 10)
  PIN_CLOSED_LIGHT      CLOSED-valotikun shield-liitin (oletus A3)
  PIN_CASINO_CLOSED_SWITCH  kasino suljettu -kytkimen SW5 pinni (oletus D30)
  CASINO_CLOSED_WHEN_LOW    true (oletus): kahva GND-puolella = suljettu; false kääntää sen
  CLOSED_STARTUP_MS, CLOSED_STEADY_MIN_MS / _MAX_MS, CLOSED_FLICKER_MIN_MS / _MAX_MS,
  CLOSED_HOLD_MIN_MS / _MAX_MS    CLOSED-valon ajastus (kuvioajan millisekunteja, katso luku 8)
  PIN_SPEED_FRAME_ARROW[]  nopeuskytkimen SW3 neljä pinniä (bittiarvot 1, 2, 4, 8)
  PIN_SPEED_LETTERS[]   nopeuskytkimen SW4 neljä pinniä (bittiarvot 1, 2, 4, 8)
  SPEED_BIT_IS_LOW      true (oletus): bitti on päällä, kun sen pinni lukee LOW; false komplementtikoodatulle
                        kytkimelle
  SPEED_MIN_MULTIPLIER  nopeus kytkimen asennossa 0 (oletus 0,25 = neljä kertaa hitaampi)
  SPEED_MAX_MULTIPLIER  nopeus kytkimen asennossa F (oletus 4,0 = neljä kertaa nopeampi)
  MIN_UPDATE_MS         kehys ja nuoli eivät päivity tätä useammin (oletus 8 ms)

tools/size_calculator.py laskee kehyksen ja nuolen arvot kyltin koosta.
Aja sen jälkeen:  python3 tools/generate_diagram.py
ja liitä uusi diagram.json Wokwiin. Aja  python3 tools/generate_schematic.py  ja
python3 tools/generate_layout.py  piirtääksesi wiring-diagram.svg:n, layout-diagram.svg:n ja
layout-parts.svg:n uusilla määrillä, sitten  python3 tools/export_pdf.py  PDF-tiedostoja varten. (generate_layout.py hyväksyy valitsimen --leds-per-m, jos nauhan
tiheys ei ole 60.) Animaatiokoodi laskee kaiken nauhojen todellisista
pituuksista, joten muita muutoksia ei tarvita. Generaattori tarkistaa, että kaavion
LED-määrät täsmäävät ohjelman kanssa, ja varoittaa, jos teksti tai nuoli ei enää mahdu
kehykseen tai jos kärjen siivet ovat liian lyhyet yltääkseen kärkeen.


8. Animaatiot ja syötteet
-------------------------
Syötteet (sisäiset ylösvetovastukset, joten kukin syöte tarvitsee vain johdon GND:hen):
  D32 liukukytkin   nuolen suunta ja kehyksen suunta. Kahva vasemmalla = pinni 1 oikosulussa
                    yhteisen kanssa = LOW = nuoli osoittaa vasemmalle. Kahva oikealla = auki
                    = HIGH = nuoli osoittaa oikealle. Kytkin pysyy siinä asennossa, johon
                    sen klikkaat; jokainen klikkaus siirtää kahvan toiselle puolelle.
                    (Asennus: takaa katsottuna GND-kontakti (1) on oikealla, jolloin kahvan suunta
                    = nuolen todellinen suunta. Aseta ARROW_POINTS_LEFT_WHEN_LOW arvoon false, jos haluat
                    kääntää sen.)
  D33 painike       jokainen painallus valitsee seuraavan kirjainmoodin.
  D30 liukukytkin   kasino suljettu (SW5). Kahva GND-puolella = pinni LOW = kasino suljettu;
                    kahva toisella puolella = kasino auki (aseta CASINO_CLOSED_WHEN_LOW arvoon
                    false kääntääksesi). Kasinon ollessa suljettu OPEN-kirjaimet ovat pimeinä
                    jokaisessa kirjainmoodissa ja CLOSED-valo palaa; kaikki muu (CASINO-kirjaimet,
                    kehys, nuoli, nopeudet) toimii normaalisti.
Kaikki syötteet on suodatettu ohjelmallisesti (DEBOUNCE_MS). Wokwin sarjamonitori
näyttää valitun kirjainmoodin ja nuolen suunnan.

Nopeuskytkimet (SW3 ja SW4, heksakiertokytkimet, joissa asennot 0..F):
  SW3 (D22-D25) säätää kehyksen ja nuolen nopeutta, SW4 (D26-D29) kirjainkuvioiden nopeutta.
  Asento 0 on hitain ja F nopein. Nopeuskerroin kasvaa geometrisesti arvosta SPEED_MIN_MULTIPLIER
  arvoon SPEED_MAX_MULTIPLIER (oletus 0,25 .. 4,0). "kehys (ms)" ja "nuoli (ms)" ovat aika kahden
  animaatioaskeleen välillä (lyhyempi = nopeampi):

    asento            0      1      2      3      4      5      6      7
    nopeus        x0,25  x0,30  x0,36  x0,44  x0,52  x0,63  x0,76  x0,91
    kehys (ms)      400    332    276    230    191    159    132    110
    nuoli (ms)      140    116     97     80     67     56     46     38

    asento            8      9      A      B      C      D      E      F
    nopeus        x1,10  x1,32  x1,59  x1,91  x2,30  x2,76  x3,32  x4,00
    kehys (ms)       91     76     63     52     44     36     30     25
    nuoli (ms)       32     27     22     18     15     13     11      9

  Kirjainkuviot (moodit 0-9) kulkevat omalla
  kellollaan, joka etenee samalla kertoimella, joten kirjainkuvio kertoimella x2,00 on kaksi kertaa
  nopeampi. Kytkimen kääntäminen ei koskaan saa kuviota hyppäämään. Yhden kirjaimen satunnainen välkkyminen
  pitää oman oikean ajan aikataulunsa. Kokeillaksesi muita nopeusalueita muuta SPEED_MIN_MULTIPLIER- ja
  SPEED_MAX_MULTIPLIER-arvoja (ja perusnopeudelle FRAME_SPEED_MS- ja ARROW_SPEED_MS-arvoja) ja lataa ohjelma
  uudestaan. Kääntyvä kytkin käy läpi välissä olevat asennot, joten asento hyväksytään vasta, kun se on
  pysynyt samana DEBOUNCE_MS-ajan. Sarjamonitori tulostaa asennon ja kertoimen.
  Heksakiertokytkin ei ole pulssi-enkooderi: se kertoo asentonsa suoraan, joten nopeus on sama jokaisen
  käynnistyksen jälkeen eikä mitään tarvitse laskea.
  Wokwissa DIP-kytkinrivi korvaa kaksi heksakytkintä (kytkimet 1-4 = SW3, kytkimet 5-8 = SW4, ON = bitti
  päällä; klikkaa osaa ja paina näppäimiä 1-8). Kaikki OFF on asento 0, hitain nopeus.

Käynnistys- 10 sekunnin ajan virran kytkemisen (tai resetin tai Wokwissa Playn painamisen) jälkeen
LED-testi   kyltti ajaa LED-testin normaalien animaatioiden sijaan. Kaikki 10 nauhaa kulkevat
            yhtä aikaa: jokaisessa nauhassa 5 LEDin ikkuna liikkuu ensimmäisestä LEDistä viimeiseen,
            ja kaikki nauhat päättyvät yhtä aikaa (lyhyt nauha etenee hitaammin kuin pitkä).
            Kierroksia on 2, 5 sekuntia kumpikin: kierros 1 on valkoinen ja kierroksella 2 LEDit
            palavat punaisena, vihreänä, sinisenä, punaisena, ... (LED-numeron mukaan), joten väärä
            värijärjestys, vaihtunut nauha tai rikkinäinen LEDi näkyy helposti. Kerrallaan palaa
            enintään 5 LEDiä nauhaa kohden (yhteensä noin 50, noin 0,9 A kirkkaudella 80).
            LCD näyttää kaksi riviä, jotka vaihtavat paikkaa 2,5 sekunnin välein, jolloin jokainen
            merkki nähdään molemmilla riveillä: toisella rivillä vierii "press D33 to skip" ja
            toisella näkyy kulunut aika sekunteina #-merkeistä tehdyn etenemispalkin keskellä,
            esimerkiksi "#####4/10 s.....". Painikkeen D33 painallus testin aikana ohittaa testin;
            se painallus ei vaihda kirjainmoodia. Jo virtaa kytkettäessä pohjassa oleva painike ei
            ohita testiä: se pitää päästää ylös ja painaa uudestaan. Muut kytkimet (nuoli,
            kasino suljettu, nopeudet) luetaan uudestaan testin päätyttyä, joten ne vaikuttavat vasta
            sen jälkeen, ja testin aikana OPEN-kirjaimet ja CLOSED-valo palavat SW5:stä riippumatta.
            Testin jälkeen kyltti käynnistyy normaalisti moodiin 0. Sarjamonitori: "Power-on LED
            test", sitten "LED test done" tai "LED test skipped". Aseta STARTUP_TEST_MS arvoon 0,
            jos haluat testin pois.
Kehys      5 LEDiä valkoisena, 5 LEDiä pimeänä, toistuen kahta polkua pitkin, jotka alkavat
           samasta kulmasta ja kohtaavat vastakkaisessa kulmassa. Nuoli oikealle: ne alkavat
           vasemmasta yläkulmasta (toinen polku ylä- ja oikeaa reunaa pitkin, toinen
           vasenta ja alareunaa pitkin) ja kohtaavat oikeassa alakulmassa. Nuoli vasemmalle:
           ne alkavat oikeasta yläkulmasta (toinen polku ylä- ja vasenta reunaa pitkin,
           toinen oikeaa ja alareunaa pitkin) ja kohtaavat vasemmassa alakulmassa.
           Kuvio lasketaan etäisyydestä aloituskulmaan, joten mikä tahansa reunan pituus
           käy; se alkaa alusta uudesta kulmasta, kun kytkintä liikutetaan.
Kirjaimet  Jokainen kirjain on oma LED-ryhmänsä ja sillä on oma kirkkautensa. Painike käy
           läpi yksitoista moodia (viimeisen jälkeen palataan moodiin 0):
             0 breathe     kaikki kirjaimet hengittävät hitaasti, kukin hieman eri vaiheessa
                           naapurinsa kanssa (oletus)
             1 wave        hohde kulkee kirjainten läpi vasemmalta oikealle
             2 typewriter  kirjaimet syttyvät yksi kerrallaan, pitävät ja sammuvat kaikki
             3 ignite      kirjaimet käynnistyvät yksi kerrallaan kuin kylmät neonputket,
                           välkkyen ennen kuin palavat tasaisesti
             4 word blink  CASINO palaa tasaisesti, OPEN vilkkuu
             5 alternate   parilliset ja parittomat kirjaimet hengittävät vastakkaisessa
                           vaiheessa
             6 scanner     kirkas piste pyyhkäisee edestakaisin kirjainten yli, ja sen
                           takana olevat kirjaimet hehkuvat häntänä
             7 sparkle     satunnaiset kirjaimet välähtävät ja himmenevät pehmeästi kuin tähdet
             8 chase       yksi kirjain kustakin sanasta on himmeä ja himmeä kohta etenee;
                           CASINO ja OPEN kulkevat erikseen (ne ovat eripituisia, joten ne
                           eroavat toisistaan)
             9 pulse       kaikki kirjaimet sykkivät yhdessä kuin sydän: vahva isku, heikompi
                           isku ja tauko
            10 random auto valitsee satunnaisen kuvion moodeista 0..9 ja vaihtaa toiseen
                           satunnaiseen joka LETTER_RANDOM_AUTO_MS (oletus 120000 = 2 minuuttia);
                           se ei koskaan valitse jo näkyvää kuviota eikä itseään (sarjamonitori
                           tulostaa valitun kuvion)
           Kyltti käynnistyy moodiin 0 (LETTER_START_MODE). Kaikki moodit seuraavat kirjainnopeuden
           kytkintä SW4, ja kasinon ollessa suljettu OPEN-kirjaimet pysyvät pimeinä jokaisessa
           moodissa (chasessa ja scannerissa kuvio kulkee silti, OPEN-osa vain ei näy).
           Jokaisessa moodissa yksi satunnainen kirjain välkkyy kuin viallinen neonputki
           alle sekunnin ajan 5–12 sekunnin välein.
           Aseta LETTER_START_MODE valitaksesi moodin käynnistyksen jälkeen ja
           LETTER_MODE_AUTO_MS vaihtaaksesi moodia automaattisesti (0 = vain painike).
LCD        2x16 I2C-näyttö näyttää samat tiedot kuin sarjamonitori neljällä sivulla, jotka kiertävät
           3 sekunnin välein (LCD_PAGE_MS):
             sivu 0  moodi:   "Mode 7" / "sparkle"; random autossa "Mode 10 (auto)" / "now: chase"
             sivu 1  SW3:     "SW3:8  x1.10" / "Fr91ms Ar30ms"   (asento, nopeuskerroin, frame- ja
                              arrow-askeleen aika millisekunteina)
             sivu 2  SW4:     "SW4:8  x1.10" / "Letter patterns"  (asento ja kirjainten nopeuskerroin)
             sivu 3  tila:    "Casino: CLOSED" (tai OPEN) / "<-Customer right" (tai "->Customer left"); asiakas = edestä katsottuna, LCD on kilven takana, joten symboli on peilattu
           Heti kun muutat jotain, sen sivu tulee näkyviin ja pysyy 6 sekuntia (LCD_HOLD_MS) viimeisestä
           muutoksesta: painike ja moodin vaihto näyttävät sivun 0, SW3 sivun 1, SW4 sivun 2 sekä nuoli-
           ja kasinokytkimet sivun 3. Pidon jälkeen sivut kiertävät taas seuraavasta sivusta. Random auton
           automaattinen vaihto näyttää sivun 0 ilman pitoa. Teksti rakennetaan ohjelmassa ja vain
           muuttuneet merkit lähetetään muutaman kerrallaan, joten LED-animaatiot eivät koskaan odota
           näyttöä. Ohjelma käyttää vain Arduino IDE:n mukana tulevaa Wire-kirjastoa (ei lisäkirjastoa).
           Wokwissa LCD on osa "lcd" I2C-tilassa (osoite 0x27).
CLOSED-    Kuusi valkoista LEDiä levyn takana valaisee CLOSED-reikätekstin, kun kasino on
valo       suljettu, ja ne ovat sammuksissa kasinon ollessa auki. Valo käyttäytyy kuin viallinen
           loisteputki: kun kasino suljetaan, se välähtelee muutaman kerran ja jää sitten päälle;
           se palaa tasaisesti 5–14 sekuntia, välkkyy sitten epäsäännöllisesti (täysi, himmeä ja
           pimeä vaihe) 0,4–1,5 sekuntia, palaa taas tasaisesti ja niin edelleen. Se on aina valkoinen.
           Ajastus seuraa kirjainnopeuden kytkintä SW4. Ajat ovat sketch.ino:n alun CLOSED_*-vakiot.
Nuoli      Nuoli on kaksi reittiä, ylä- ja alareitti (36 LEDiä kumpikin), väännettynä nuolen
           muotoon. Kumpikin reitti alkaa hännän capin keskeltä (sen keskimmäiset 6 LEDiä
           palavat, 2 LEDiä kummassakin päässä pysyy pimeänä), kulkee kahden kisko-tikun
           läpi, ylös (tai alas) kärjen capin 3 päätyLEDiä pitkin (sen keskimmäiset 4 LEDiä
           pysyvät pimeinä) ja nuolenkärjen siipeä pitkin kärkeen. Sinisiä, syaaneja,
           violetteja ja jäävalkoisia väriaaltoja ja väreilyä kulkee molemmilla reiteillä
           (eri vaiheessa); häntäinen valkoinen pulssi kulkee hännästä kärkeen ensin
           yläreittiä ja sitten alareittiä, vuorotellen (aina täsmälleen yksi pulssin kärki
           kerrallaan). Vain se kärki palaa, johon nuoli osoittaa. Suunta seuraa
           liukukytkintä: kun nuoli osoittaa vasemmalle, vasen cap on kärjen cap ja oikea
           cap on hännän cap.

Nopeudet ja värit ovat vakioita CONFIGURATION-osassa (FRAME_SPEED_MS,
LETTERS_SPEED_MS, ARROW_SPEED_MS, CURVE_DEFAULT_*, ...). Kaikki ajastus käyttää millis()-funktiota;
ohjelma ei koskaan kutsu delay()-funktiota. Huomaa, että Adafruit_NeoPixel::show() on
synkroninen, joten nauhat päivitetään peräkkäin. Noin 30 mikrosekuntia per LED: päivityksen
kokonaisaika kasvaa LED-määrän mukana (noin 7 ms kaikille 238 oletus-LEDille); paljon
isoilla kylteillä nopeimpia animaationopeuksia ei enää saavuteta.


9. Huomioita oikeasta laitteistosta
-----------------------------------
- Syötä nauhat erillisestä 5 V:n virtalähteestä, älä koskaan Arduinosta. Laske enintään
  60 mA per LED täydellä valkoisella; käyrän max ja pimeät LEDit pienentävät todellista
  kulutusta.
- Yhdistä virtalähteen GND Arduinon GND:hen.
- Syötä virta jokaiseen nauhaan molemmista päistä tai useasta kohdasta pitkillä nauhoilla
  (katso virtataulukko luvussa 4).
- Laita jokaiseen datajohtoon 330 ohmin vastus sarjaan ja 1000 uF kondensaattori
  virtalähteen yli. Megan 5 V:n logiikkataso ohjaa WS2812B:tä suoraan, joten
  tasomuunninta ei tarvita.
- Asenna 10 A sulake +5 V -johtoon heti virtalähteen jälkeen.
- Yhdistä kummankin heksakytkimen yhteinen pinni GND:hen. Nopeuspinnit käyttävät Arduinon sisäisiä
  ylösvetovastuksia, joten niille ei tarvita vastuksia.
- Virtalähteessä on verkkojännitteellä toimivat liittimet. Asenna se koteloon ja anna
  verkkopuolen johdotus sähköasennuksen osaavan tehtäväksi.


Suuntakytkimen suunta-asetus (sketch.ino)
-----------------------------------------
ARROW_POINTS_LEFT_WHEN_LOW: true = kytkimen pinni D32 LOW (kahva GND-puolella) saa nuolen osoittamaan asiakkaan
vasemmalle; false = päinvastoin. Mekaanisen kytkimen toiminnasta ja asennuksesta riippuen voit kääntää tämän
yhden vakion ja vaihtaa, mikä kahvan asento antaa minkä nuolen suunnan (ei uudelleenjohdotusta).

Tikun pallo-asetus (sketch.ino)
-------------------------------
Asettelu- ja poraus­kuvien musta piste on oikean tikun PALLO. Asenna jokainen tikku pallo pisteessä
(vasemman puoliskon tikut osoittavat vasemmalle, oikean oikealle, molemmat päätytikut ylös).
Vakio STICK_BALL_AT_DIN_END kertoo ohjelmalle, missä oikea pallo on: true = DIN-päässä (LED 1), LED-numeroita
käytetään sellaisenaan; false = toisessa päässä, ohjelma laskee ledit toisesta päästä. Jos animaatio kulkee
oikeilla tikuilla väärään suuntaan, käännä tämä yksi vakio (Wokwi-generaattori seuraa sitä myös).
Poraussapluuna on kilven ETUPUOLELLE (tulosta peilattuna, jos poraat takaa).
Wokwissa kytkimen kahva osoittaa kuten nuoli edestä katsottuna; LCD:n nuoli näyttää ruudulla tarkoituksella
päinvastaiselta (LCD on kilven takana ja näyttää asiakkaan näkymän).
