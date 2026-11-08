TIKKUNUOLI - 10 Grove RGB LED -tikkua nuolena (malli B, piikkipää): porausmalli, asettelu ja johdotus
=====================================================================================================

Mikä tämä on
------------
Kasinokyltin nuoli 10 Grove RGB LED -tikusta (10x WS2813 mini, 10 lediä kussakin). Tässä kansiossa on tarkka
porausmalli, suuntaa antava asettelukuva ja johdotus. Kaikki kolme tulevat samasta geometriatiedostosta
(tools/arrow_geometry.py), joten tikkujen nimet ovat kaikkialla samat. Nuolen OHJELMAA ei ole vielä kirjoitettu.
Laitteisto on todistettu: stick_test_direct osoitti, että 11 tikkua muuttamattomina Grove Base Shieldissä ja
ulkoisesta 5 V:n virtalähteestä toimii.

Tiedostot
---------
drill-template-A3.pdf        Porausmalli 1:1 yhdellä A3-vaakasivulla (420 x 297 mm). Tulosta 100 %:lla.
drill-template-A4-tiled.pdf  Sama 1:1 kahdella A4-vaakasivulla: vasen puolisko sivulla 1, oikea sivulla 2; leikkaa katkoviivaa (CUT LINE) pitkin, aseta leikkausreunat vastakkain ja teippaa.
layout-diagram.svg/.pdf      Suuntaa antava asettelu (A4 vaaka): nimet, värit, DIN-päät ja shieldin liittimet.
wiring-diagram.svg/.pdf      Sähkökaavio (A4 pysty): virtalähde, Grove Base Shieldin liittimet, nimetyt tikut.
README.txt                   Englanninkielinen ohje.   README-fi.txt   Tämä suomennos.
Generaattorit: tools/arrow_geometry.py (geometria), tools/generate_arrow_drill_template.py, tools/generate_arrow_layout.py,
tools/generate_arrow_wiring.py. Mittojen tarkistus: python3 tests/arrow_geometry_test.py

Mitat (tarkat; mm)
------------------
  - Tikku: ulkomitta 83 x 10 mm; ledi-ikkuna 74 x 4 mm, keskitetty; 10 lediä. Ledien väliksi oletetaan
    74 / 10 = 7,4 mm (datalehti ei kerro; tarkista oikeasta tikusta). Grove-liitin on tikun keskellä.
  - 2 mm vapaata tilaa jokaisen kahden tikun välissä (myös siipien välissä kärjessä).
  - Vaakapalkkien ledirivit ovat pystypalkkien ledien 3 ja 8 kohdalla (ylhäältä 3. ja alhaalta 3.):
    palkit ovat 37 mm:n päässä toisistaan (keskilinjat), 27 mm sisäreunojen välissä.
  - Pystypalkit ovat 172 mm:n päässä toisistaan (sisäpinnat), keskeltä keskelle 182 mm.
  - Siivet 30 asteen kulmassa vaakatasosta (83 mm siipi, 83 mm palkki). Jokainen siipi alkaa 2 mm:n päästä palkistaan ja
    pään kaksi siipeä ovat kärjessä 2 mm:n päässä toisistaan, joten siivet pistävät noin 10 mm palkin ylä- ja alapuolelle.
  - Koko nuoli: 349,76 x 102,32 mm.
  Porauspaikat (tikun keskipiste = Grove-liitin = kaapelireikä), mm nuolen vasemmasta yläkulmasta:
  L-CAP      x=  83.88  y=  51.16  angle= 90 deg  D2   left vertical bar
  L-WING-UP  x=  38.44  y=  25.08  angle=330 deg  D3   left upper wing
  L-WING-DN  x=  38.44  y=  77.24  angle= 30 deg  D4   left lower wing
  RAIL-UP-L  x= 132.38  y=  32.66  angle=  0 deg  D5   upper rail, left stick
  RAIL-UP-R  x= 217.38  y=  32.66  angle=180 deg  D6   upper rail, right stick
  RAIL-DN-L  x= 132.38  y=  69.66  angle=  0 deg  D7   lower rail, left stick
  RAIL-DN-R  x= 217.38  y=  69.66  angle=180 deg  D8   lower rail, right stick
  R-CAP      x= 265.88  y=  51.16  angle= 90 deg  A0   right vertical bar
  R-WING-UP  x= 311.32  y=  25.08  angle=210 deg  A1   right upper wing
  R-WING-DN  x= 311.32  y=  77.24  angle=150 deg  A2   right lower wing
  Kaapelireiän halkaisija on arvaus (8 mm, pitää olla alle tikun 10 mm leveyden): tarkista Grove-liittimestäsi
  (asetus CABLE_HOLE_D on tiedostossa tools/arrow_geometry.py).
  Musta piste = pallero: jokaisen oikean tikun piirilevyllä on pyöreä merkki ("pallero"). Asenna jokainen tikku niin,
  että sen pallero on piirroksen mustan pisteen kohdalla. Pisteet osoittavat samaan suuntaan kuin se nuolen puolisko,
  jossa ne ovat: vasemmalla puolella vasemmalle (kiskot, siivet: siivillä piste on kärjessä), oikealla puolella
  oikealle ja kummallakin pystypalkilla (capilla) ylös. CLOSED-tikku (A3, levyn takana) asennetaan pallero
  vasemmalle. Onko pallero tikun DIN-päässä (LED 1), määrää sketch.ino:n asetus STICK_BALL_AT_DIN_END (true = pallero
  on DIN-päässä, false = toisessa päässä): jos animaatio kulkee tikkuja pitkin väärään suuntaan, käännä tämä arvo;
  älä asenna tikkuja uudelleen. Porausmalli on levyn ETUPUOLELLE (tulosta peilattuna, jos poraat takaa).

Johdotus (tarkasti)
-------------------
  Jokainen tikku kytketään omalla muuttamattomalla Grove-kaapelillaan Arduino Mega 2560:n Grove Base Shieldin
  Grove-liittimeen (shieldin kytkin 5 V:ssa):
    L-CAP D2, L-WING-UP D3, L-WING-DN D4, RAIL-UP-L D5, RAIL-UP-R D6, RAIL-DN-L D7, RAIL-DN-R D8,
    R-CAP A0, R-WING-UP A1, R-WING-DN A2.  (A3:ssa on kyltin CLOSED-valotikku; se ei kuulu nuoleen.)
  Arduino + shield saa virran ulkoisesta 5 V:n virtalähteestä: LRS-50-5 V+ -> 10 A sulake -> Megan 5V-navat (molemmat,
  pitkän liittimen päässä), V- -> Megan GND-navat (molemmat); 1000 uF 25 V kondensaattori syöttöjohtojen yli, plus 5 V:ään.
  Ei USB-kaapelia kun virtalähde on päällä. Virtalähteen 230 V:n puoli kuuluu koteloon ja sen saa kytkeä vain sen osaava
  henkilö. Katso wiring-diagram.pdf.
  Kaapelin pituus: tavallinen Grove-kaapeli on lyhyt; tarkista, että jokainen kaapeli yltää shieldistä tikulleen.

Ostettavat osat (partco.fi; tilannekuva 2.11.2026, mitään ei ole päivitetty): katso ../parts-list.csv ja ../stick_test_direct/README-fi.txt
  - 10 x Grove RGB LED Stick (10x WS2813 mini) Grove-kaapeleineen (11 on olemassa)
  - Mean Well LRS-50-5 5 V 10 A virtalähde, 10 A autosulake + pidike, kondensaattori 1000 uF 25 V, Arduino Mega 2560, Grove Base Shield
  - Pidemmät Grove-kaapelit, jos tavalliset eivät riitä (ei selvitetty)

Avoimet kohdat
--------------
  - Ledien väli ja Grove-liittimen tarkka paikka on oletettu (katso yllä); kaapelireiän halkaisija on arvaus.
  - Kansioiden stick_test/ ja stick_test_direct/ vanhemmat asettelukuvat näyttävät vielä nuolen ensimmäisen luonnoksen
    80 mm:n tikuilla; tämä kansio korvaa sen.
  - Nuolen ohjelmaa (kuviot, 100 ledin vastaavuus yllä oleviin nimiin) ei ole kirjoitettu.
