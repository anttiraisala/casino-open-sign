VARMUUSKOPIO - vanha kuudesta ledinauhasta tehty nuoli (poistettu, EI enää kytkettynä)
=======================================================================================

Mikä tämä on
------------
Kasinokyltti sellaisena kuin se oli ennen Grove-tikkunuolta: sen version sketch.ino ja diagram.json. Säilytetään vain
varmuuskopiolistana ("muistina") vanhasta suunnitelmasta. Älä kytke sitä: sen pinnit ovat ristiriidassa uuden nuolen
käyttämien Grove Base Shield -liittimien kanssa (uusi kyltti käyttää tikuille nastoja D2-D8 ja A0-A2).

Tiedostot
---------
sketch.ino     Vanha kyltin ohjelma (nuoli = kuusi WS2812B-nauhaa). Aukeaa Wokwissa yhdessä diagram.json:n kanssa.
diagram.json   Vastaava Wokwi-piiri.
README.txt     Englanninkielinen versio.   README-fi.txt   Tämä tiedosto (suomi).

Vanhan nuolen poistetut osat (nauha, datapinni, LEDit)
------------------------------------------------------
  arrowBodyTop      D9    10 LEDiä   vartalon yläreuna
  arrowBodyBottom   D10   10 LEDiä   vartalon alareuna
  arrowHeadLeft     D11   20 LEDiä   vasen nuolenkärki (palat 2 + 8 + 8 + 2)
  arrowHeadRight    D12   20 LEDiä   oikea nuolenkärki (palat 2 + 8 + 8 + 2)
  arrowCapLeft      D4     4 LEDiä   vasen pääty
  arrowCapRight     D5     4 LEDiä   oikea pääty
  Yhteensä 68 LEDiä, 6 nauhaa. Jokaisen nauhan datajohdossa oli 330 ohmin vastus.

Sen version muut vanhat pinnit: kehys D6/D7, kirjaimet D8, CLOSED-valo D31, nuolikytkin D2, painike D3.
(Uusi kyltti: kehys D9/D10, kirjaimet D11, CLOSED-valo D31, nuolikytkin D32, painike D33.)

Varmuuskopion käyttö
--------------------
Vain katseltavaksi tai ajettavaksi Wokwi-simulaattorissa: uusi projekti -> Arduino Mega, liitä sketch.ino ja
diagram.json samannimisiin välilehtiin, lisää kirjasto Adafruit NeoPixel. Mitään ei tarvitse ostaa.
