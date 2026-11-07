"""The bill of materials of the casino sign (every component, with partco.fi links).

Prices include VAT (25.5 %) and were checked on partco.fi on CHECKED_ON. Quantities are the
quantities needed (no spare parts). Rows with counted = False are either insurance (insurance = True: extra parts for the uncertain
situations, added to the 'with insurance' total) or alternatives/options (not in any total). Rows with unit_price = None have no price (not sold
at partco.fi, or the price was not looked up).

NOTE FOR AI: do NOT refresh prices, stock status or links in this file unless the user explicitly asks
for it. Remind the user now and then that they are a snapshot (see SNAPSHOT_NOTE) and offer a refresh.

Used by generate_parts_list.py (PDF, CSV and XLSX).
"""

import math

CHECKED_ON = "2026-11-02"
VAT_NOTE = "Prices include VAT 25.5 %"
SNAPSHOT_NOTE = ("Prices, stock status and links are a snapshot from %s. They are not updated unless the user "
                 "explicitly asks for it, so check them before ordering." % CHECKED_ON)
SITE = "https://partco.fi"
STRIP_REEL_M = 5.0      # one reel of the strip is 5 m

G_CTRL, G_POWER, G_LED, G_TEST, G_WIRING, G_MECH, G_INS, G_OPT = (
    "Control and display", "Power", "LED strip", "Strip test (do this first)", "Wiring and assembly",
    "Plate and letters (not at partco.fi)", "Insurance for the uncertain parts",
    "Alternatives and options (not in any total)")


def part(group, name, used, qty, unit, price, avail, product, sku, url, note="", counted=True, certainty="sure",
         insurance=False):
    """certainty: sure / probable / uncertain (how sure we are that this exact part is the right one)."""
    return dict(group=group, part=name, used_for=used, qty=qty, unit=unit, unit_price=price, availability=avail,
                product=product, sku=sku, url=url, note=note, counted=counted, certainty=certainty, insurance=insurance)


STRIP_URL = "https://partco.fi/tuote/rgb-lednauha-30kpl-m-ws2812-5m-103"


def bill_of_materials(total_leds=0, strips=10, leds_per_m=60):
    """Return the list of rows (the sketch values are only used for the number of data resistors)."""
    rows = [
        part(G_CTRL, "Arduino Mega 2560", "the controller", 1, "pcs", 58.00, "in stock (15 pcs)",
             "Arduino Mega 2560 R3 (A000067)", "ARD-A000067",
             "https://partco.fi/tuote/arduino-mega-2560-r3-a000067-2178"),
        part(G_CTRL, "LCD 2x16, I2C", "shows mode, speeds, casino state, arrow", 1, "pcs", 13.40, "in stock (45 pcs)",
             "LCD-NÄYTTÖ 2x16 SINIVALKO I2C-LIITÄNTÄ", "LCD-2X16-I2C",
             "https://partco.fi/tuote/lcd-n%C3%A4ytt%C3%B6-2x16-sinivalko-i2c-liit%C3%A4nt%C3%A4-5193",
             "I2C backpack with PCF8574AT, address 0x27 per the page (try 0x3F if blank)", certainty="probable"),
        part(G_CTRL, "Hex rotary switch 0-F", "SW3 and SW4: speed of frame + arrow, speed of letters", 2, "pcs", 3.43,
             "in stock (4 pcs)", "HEX ENKOODERI/KIERTOKYTKIN TAPILLA 0..F", "KIERTOKYTKIN-216",
             "https://partco.fi/tuote/hex-enkooderi-kiertokytkin-tapilla-0-f-4851",
             "PCB mount, 16 positions, binary coded, NOT a pulse encoder; the pin order and the code (true or "
             "complement) are not on the page: test it", certainty="probable"),
        part(G_CTRL, "Slide switch DPDT, long knob", "SW1 arrow direction, SW5 casino closed (one pole used)", 2, "pcs",
             0.90, "in stock (25 pcs)", "2-NAP LIUKUKYTKIN ON/ON PITKÄ NUPPI", "KYT-LIU19-B",
             "https://partco.fi/tuote/2-nap-liukukytkin-on-on-pitk%C3%A4-nuppi-4859",
             "the user's choice; holes 19 mm apart, knob 6.5 mm; the page does not say if it is panel or PCB mount",
             certainty="probable"),
        part(G_CTRL, "Push button, panel mount", "SW2: next letter mode, skips the power-on test", 1, "pcs", 6.53,
             "in stock (19 pcs)", "SULKEUTUVA PAINOKYTKIN 1A 125VAC MUSTA", "KYT-DS102-MUS",
             "https://partco.fi/tuote/sulkeutuva-painokytkin-1a-125vac-musta-14704",
             "hole 7 mm; the page does not state outright that it is normally open (the name means it closes "
             "when pressed)", certainty="probable"),
        part(G_CTRL, "USB cable A-B", "programming and power of the Arduino", 1, "pcs", 3.90, "in stock (per listing)",
             "USB-2.0 VÄLIJOHTO A-UROS / B-UROS 1m", "DK-USB-VJ2-1M", "https://www.partco.fi/fi/1416-usb-kaapelit",
             "price and stock are from partco.fi's Arduino accessories listing; the product page was not opened",
             certainty="probable"),
        part(G_POWER, "12 V power supply 4.2 A", "powers the 12 V LED strip", 1, "pcs", 23.79, "in stock (4 pcs)",
             "HAKKURI MODULI 50W 12VDC 4,2A (Mean Well)", "LRS-50-12",
             "https://partco.fi/tuote/hakkuri-moduli-50w-12vdc-4-2a-5775",
             "99 x 82 x 30 mm, screw terminals; the page does not mention an earth terminal: check the datasheet",
             certainty="probable"),
        part(G_POWER, "Fuse holder", "5 A fuse in the +12 V line", 1, "pcs", 3.56, "in stock (8 pcs)",
             "AUTOSULAKEPESÄ JOHDOT PÄÄSTÄ", "SULPES-APUR",
             "https://partco.fi/tuote/autosulakepes%C3%A4-johdot-p%C3%A4%C3%A4st%C3%A4-4732"),
        part(G_POWER, "Blade fuse 5 A", "F1 (the strip needs about 3 A at most)", 1, "pcs", 0.33, "in stock (per listing)",
             "AUTOSULAKE 32V 5A oranssi", "SUA-5A",
             "https://partco.fi/kategoria/komponentit/s%C3%A4hk%C3%B6mekaaniset-komponentit/sulakkeet/autosulakkeet",
             "price and stock are from the fuse listing", certainty="probable"),
        part(G_POWER, "Capacitor 1000 uF 25 V", "C1 across the LED supply (16 V is too tight at 12 V)", 1, "pcs", 1.73,
             "in stock (100+ pcs)", "ELEKTROLYYTTIKOND. 1000µF 25V 10x21mm", "ELKO105-1000U-25V",
             "https://partco.fi/tuote/elektrolyyttikond-1000%C2%B5f-25v-10x21mm-12238"),
        part(G_POWER, "Mains cord, Schuko plug, free end", "230 V into the power supply", 1, "pcs", 5.20, "in stock",
             "LAITEJOHTO SUKO KULMA / VAPAA PÄÄ 2m", "LAITEJOHTO-2V-KULMA",
             "https://partco.fi/tuote/laitejohto-suko-kulma-vapaa-p%C3%A4%C3%A4-2m-4301",
             "3-wire cord; the mains side must be wired by someone who knows how"),
        part(G_POWER, "Plastic enclosure with cable glands", "encloses the power supply and the mains wiring", 1, "pcs",
             12.70, "in stock (per listing)", "VEDENPITÄVÄ MUOVIKOTELO LÄPIVIENNEILLÄ 95x151x175mm", "KOTEV-171",
             "https://www.partco.fi/fi/1517-teollisuuskotelot",
             "price and stock are from the industrial enclosures listing; the product page was not opened",
             certainty="probable"),
        part(G_LED, "RGB LED strip, 12 V, WS2811, 30 LEDs/m, 5 m", "all lighting of the sign (colour, addressable)", 1,
             "reel", 70.37, "in stock (1 pc only!)", "RGB-LEDNAUHA 30kpl/m WS2811 5m", "LED-N-RGB-W2812X150-5M",
             STRIP_URL,
             "probably one control chip per 3 LEDs = 50 pixels of 10 cm; colour order, bit speed, current and width "
             "are unknown (test it with strip_test/); only one reel in stock; the sign needs about 38-50 pixels",
             certainty="uncertain"),
        part(G_TEST, "USB to 12 V step-up converter, 500 mA", "powers a short test piece of the strip safely (no mains)",
             1, "pcs", 14.19, "in stock (6 pcs)", "USB to 2,1mm DC STEP-UP DC-DC MUUNNIN 12VDC", "ADA2778",
             "https://www.partco.fi/fi/teholaehteet/dcdc-jaennitemuuntimet/step-up-dcdc-jaennitettae-nostava/23817-ada2778.html",
             "500 mA is enough for 30-60 cm at brightness 60; needs a USB charger", certainty="probable"),
        part(G_TEST, "DC jack with screw terminals, 2.1 mm", "connects the step-up converter to the strip wires", 1, "pcs",
             3.40, "in stock (154 pcs)", "DC-JATKOLIITIN 2,1mm RUUVILIITOKSELLA", "DCJ21RUU",
             "https://partco.fi/tuote/dc-jatkoliitin-2-1mm-ruuviliitoksella-1875",
             "check that it fits the barrel of the converter (2.1 mm, plus in the middle)", certainty="probable"),
        part(G_WIRING, "Resistor 330 ohm", "R1.. in the data lines (one per strip chain)", strips, "pcs", 0.45,
             "in stock (89 pcs)", "METALLIOKSIDIKALVOVASTUS 2W: 330ohm", "VASTUS-2W-330R",
             "https://partco.fi/tuote/metallioksidikalvovastus-2w-330ohm-9515",
             "2 W type (body 5 x 12 mm) because the 0.5 W type FRES-05W-330R has been discontinued; the new design "
             "may need only 4-6 data lines"),
        part(G_WIRING, "Perfboard 200 x 100 mm", "resistors, capacitor, fuse holder, hex switches, connections", 1, "pcs",
             6.50, "in stock (10 pcs)", "KYTKENTÄLEVY 200x100mm JUOTOSLIUSKOILLA PERTINAX", "KL-PL200",
             "https://partco.fi/tuote/kytkent%C3%A4levy-200x100mm-juotosliuskoilla-pertinax-14786"),
        part(G_WIRING, "Heat-shrink tubing set", "insulates soldered joints", 1, "set", 10.50, "in stock (39 pcs)",
             "KUTISTESUKKASARJA 100kpl laatikko", "K-SARJA-86271",
             "https://www.partco.fi/fi/kaapelitarvikkeet/kutistemuovit/2-1-kutistemuovit/18196-k-sarja-86271.html"),
        part(G_WIRING, "Hook-up wire 0.75 mm2, red", "+12 V feeds and data wires (length is an estimate)", 10, "m", None,
             "in stock", "ASENNUSJOHTO 0,75mm2 PUNAINEN 1m", "KAA-AJ-1X0.75-PUN",
             "https://www.partco.fi/fi/kaapelit-metritavarana/monisaeikeiset-asennusjohdot/14290-kaa-aj-1x075-pun.html",
             "price per metre not checked (the page did not load)", certainty="probable"),
        part(G_WIRING, "Hook-up wire 0.75 mm2, black", "GND feeds (length is an estimate)", 10, "m", None, "in stock",
             "ASENNUSJOHTO 0,75mm2 MUSTA (sold by the metre; 100 m reel shown)", "KAA-AJ-1X0.75-MUS",
             "https://www.partco.fi/fi/kaapelit-metritavarana/monisaeikeiset-asennusjohdot/18135-kaa-aj-1x075-mus-100m.html",
             "price per metre not checked; the 1 m item is listed as a related product on this page",
             certainty="probable"),
        part(G_WIRING, "Solder wire", "soldering", 1, "roll", None, "not looked up",
             "lead-free solder wire (partco.fi had only tips and wick in my search)", "-", SITE,
             "buy where convenient; link is the partco.fi front page", certainty="probable"),
        part(G_MECH, "Black plate 60 x 30 cm", "the sign plate (with the CLOSED hole text cut out)", 1, "pcs", None,
             "-", "your choice of material, with a 10 x 2.5 cm CLOSED text cut-out", "-", SITE,
             "not a partco.fi item; the link is the partco.fi front page"),
        part(G_MECH, "Red plexiglass letters", "CASINO and OPEN", 1, "set", None, "-",
             "10 letters (C A S I N O O P E N), size to fit the plate", "-", SITE,
             "not a partco.fi item; the link is the partco.fi front page"),
        part(G_MECH, "Frosted plastic", "behind the CLOSED hole text, so the light does not show as dots", 1, "pcs", None,
             "-", "frosted/opal plastic, about 12 x 4 cm", "-", SITE,
             "not a partco.fi item; the link is the partco.fi front page"),
        part(G_MECH, "Mounting hardware", "fixing the strips, board, display and letters to the plate", 1, "set", None,
             "-", "double-sided tape, screws, spacers (as needed)", "-", SITE,
             "not a partco.fi item; the link is the partco.fi front page"),
        # ---- insurance: extra parts for the uncertain situations (added to the 'with insurance' total) ----
        part(G_INS, "Spare resistors 330 ohm", "in case a data line needs more or one is damaged", 2, "pcs", 0.45,
             "in stock (89 pcs)", "METALLIOKSIDIKALVOVASTUS 2W: 330ohm", "VASTUS-2W-330R",
             "https://partco.fi/tuote/metallioksidikalvovastus-2w-330ohm-9515", counted=False, insurance=True),
        part(G_INS, "Spare blade fuse 5 A", "a blown fuse is the usual first mishap", 2, "pcs", 0.33,
             "in stock (per listing)", "AUTOSULAKE 32V 5A oranssi", "SUA-5A",
             "https://partco.fi/kategoria/komponentit/s%C3%A4hk%C3%B6mekaaniset-komponentit/sulakkeet/autosulakkeet",
             counted=False, insurance=True),
        part(G_INS, "Logic buffer 74HCT125 (DIP14)", "cleans up the 5 V data signal if the strip is fussy about it", 1,
             "pcs", 1.05, "in stock (20 pcs)", "TTL-LOGIIKKAPIIRI BUF 74125 HCT-PERHE DIP14", "74HCT125",
             "https://partco.fi/fi/elektroniikan-komponentit/aktiivit/ttlcmos-logiikkapiirit/logiikkapiirit-74xx/2192-74hct125.html",
             "only helps if the 5 V signal is poor; it does not raise the level to 12 V; a DIP14 socket was not looked up",
             counted=False, certainty="uncertain", insurance=True),
        part(G_INS, "5 V power supply 10 A (for a 5 V strip bought elsewhere)", "only if the 12 V strip turns out to be unusable", 1,
             "pcs", 23.79, "in stock (10 pcs)", "HAKKURI MODULI 50W 5VDC 10A (Mean Well)", "LRS-50-5",
             "https://partco.fi/tuote/hakkuri-moduli-50w-5vdc-10a-5777",
             "the 5 V strip of the old design (60 LEDs/m) is sold out at partco.fi", counted=False,
             certainty="uncertain", insurance=True),
        part(G_INS, "Blade fuse 10 A (for the 5 V supply)", "same reason as the 5 V supply", 1, "pcs", 0.31,
             "in stock (76 pcs)", "AUTOSULAKE 32V 10A punainen", "SUA-10A",
             "https://partco.fi/tuote/autosulake-32v-10a-punainen-5320", counted=False, certainty="uncertain",
             insurance=True),
        # ---- alternatives and options: not in any total ----
        part(G_OPT, "Alternative: Seeeduino Mega", "instead of the Arduino Mega", 1, "pcs", 46.70, "in stock (1 pc)",
             "Seeeduino Mega (ATmega2560)", "SEEED-102010007",
             "https://partco.fi/tuote/seeeduino-mega-atmega2560-2531",
             "Mega compatible; set its 5 V / 3.3 V switch to 5 V and check the pin numbering", counted=False),
        part(G_OPT, "Old design: 5 V strip, 60 LEDs/m, 5 m", "the strip of the earlier 5 V design", 1, "reel", 136.00,
             "SOLD OUT, availability unknown", "RGB-LEDNAUHA 60kpl/m WS2812 5m", "LED-N-RGB-W2812X300-5M",
             "https://partco.fi/tuote/rgb-lednauha-60kpl-m-ws2812-5m-104", counted=False),
        part(G_OPT, "White strip, 12 V, 60 LEDs/m, 5 m (not addressable)", "only on/off and dimming of whole zones", 1,
             "reel", 35.14, "in stock (2 pcs)", "EDULLINEN LED-NAUHA KYLMÄNVALKOINEN 6000K 24W IP20 5m kela",
             "LEDN-24W-VAL", "https://partco.fi/tuote/edullinen-led-nauha-kylm%C3%A4nvalkoinen-6000k-24w-ip20-5m-kela-120",
             "rejected: no per-LED control", counted=False),
        part(G_OPT, "Option: knob for the hex switches", "SW3 and SW4 (pin 3.8 mm; a standard knob may not fit)", 2, "pcs",
             None, "-", "Knobs category (3 mm, 4 mm and 6 mm shafts)", "-", "https://www.partco.fi/fi/2088-nupit",
             "the hex switch has a 3.8 mm pin: check the fit, or use a small screwdriver", counted=False,
             certainty="uncertain"),
        part(G_OPT, "Option: screw terminal block 2-pole", "removable connections for the strip pieces", 1, "pcs", 0.79,
             "in stock (per listing)", "IRROTETTAVA RIVILIITIN 2X R5,08", "TBW5-2P",
             "https://partco.fi/tuote/irrotettava-riviliitin-2x-r5-08-1787", "buy as many as you need", counted=False),
    ]
    return rows


def line_total(row):
    return None if row["unit_price"] is None else round(row["qty"] * row["unit_price"], 2)


def totals(rows):
    """(total EUR of the counted priced rows, number of counted rows without a price, insurance total EUR)"""
    total = sum(line_total(r) for r in rows if r["counted"] and r["unit_price"] is not None)
    unpriced = sum(1 for r in rows if r["counted"] and r["unit_price"] is None)
    ins = sum(line_total(r) for r in rows if r["insurance"] and r["unit_price"] is not None)
    return round(total, 2), unpriced, round(ins, 2)
