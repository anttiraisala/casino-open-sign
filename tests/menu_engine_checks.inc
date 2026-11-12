// C++-specific checks of the menu engine and the test shell (included inside main() of menu_engine_harness.cpp).
// Expected values are written out here on purpose, not taken from the sketch constants.
{
  shellApplySwitches = false;
  char t[40];
  auto text = [&](uint8_t prop, uint16_t pos) { menuValueText(prop, pos, t); return std::string(t); };
  // ---- value texts ----
  CHECK(text(P_stripMax, 0) == "0" && text(P_stripMax, 255) == "255" && text(P_stripMax, 80) == "80", "int texts");
  CHECK(text(P_speedFrame, 0) == "0.125x" && text(P_speedFrame, 18) == "1.000x" && text(P_speedFrame, 30) == "4.000x", "speed texts");
  CHECK(text(P_stripExp, 0) == "0.10" && text(P_stripExp, 31) == "2.67" && text(P_stripExp, 50) == "20.00", "exponent texts");
  CHECK(text(P_stripMix, 0) == "0.00" && text(P_stripMix, 25) == "0.50" && text(P_stripMix, 50) == "1.00", "mix texts");
  CHECK(text(P_arrowReverse, 0) == "No" && text(P_arrowReverse, 1) == "Yes", "bool texts");
  CHECK(text(P_arrow, 0) == "Left" && text(P_arrow, 1) == "Right" && text(P_casino, 0) == "Closed", "choice texts");
  CHECK(text(P_lettersEffect, 0) == "Random" && text(P_lettersEffect, 13) == "13 Slot machine" && text(P_frameEffect, 15) == "15 Grey bands" && text(P_frameEffect, 2) == "2 Comet", "effect texts");
  CHECK(text(P_randomDelay, 0) == "random" && text(P_randomDelay, 2) == "2 min" && text(P_randomDelay, 30) == "30 min", "value texts");

  // ---- start-up with an empty EEPROM ----
  menuMode = MM_MENU; menuDepth = 3;
  CHECK(menuBegin() == false, "empty EEPROM is not valid");
  CHECK(menuMode == MM_STATUS && menuDepth == 0, "start-up shows the status page");
  CHECK(menuPos(P_stripMax) == 80 && menuPos(P_stripExp) == 31 && menuPos(P_stripMix) == 50 && menuPos(P_speedFrame) == 18 && menuPos(P_randomDelay) == 2 && menuPos(P_frameEffect) == 1 && menuPos(P_lettersEffect) == 1, "defaults");
  CHECK(EEPROM.writes == 0, "begin writes nothing");

  uint8_t roFirst = P_arrow;
  menuSave(&roFirst, 1);
  CHECK(EEPROM.writes == 0 && !menuEepromValid, "saving a status property on an empty EEPROM writes nothing");
  // ---- menu time-out boundary ----
  menuKey(MENU_KEY_OK, 1000);
  CHECK(menuMode == MM_MENU, "menu open for the time-out check");
  menuTick(60999); CHECK(menuMode == MM_MENU, "menu stays open before 60 s");
  menuTick(61000); CHECK(menuMode == MM_STATUS, "menu closes after exactly 60 s without input");
  // ---- first save writes the whole image ----
  menuSet(P_stripMax, 200);
  uint8_t one = P_stripMax;
  menuSave(&one, 1);
  CHECK(EEPROM.mem[0] == 0xEF && EEPROM.mem[1] == 0x98 && EEPROM.mem[2] == 0x5C && EEPROM.mem[3] == 0x6E, "tag, little endian");
  CHECK(EEPROM.mem[4 + 10] == 200, "stripMax in slot 10");
  CHECK(EEPROM.mem[4 + 2] == 0 && EEPROM.mem[4 + 4] == 18 && EEPROM.mem[4 + 7] == 2 && EEPROM.mem[4 + 8] == 1, "other slots hold defaults (arrowReverse, speedFrame, randomDelay, monochrome Yes)");
  CHECK(EEPROM.mem[4 + 0] == 1 && EEPROM.mem[4 + 1] == 1, "both effects default to 1 in slots 0 and 1");
  CHECK(EEPROM.mem[4 + 11] == 31 && EEPROM.mem[4 + 15] == 50 && EEPROM.mem[4 + 20] == 50, "exponent and mix slots");
  CHECK(menuEepromValid, "valid after the first save");
  CHECK(EEPROM.writes > 4, "image written");
  // later saves write only the changed slot
  long w = EEPROM.writes;
  menuSet(P_stripMax, 201); menuSave(&one, 1);
  CHECK(EEPROM.writes == w + 1 && EEPROM.mem[4 + 10] == 201, "one changed slot");
  w = EEPROM.writes;
  uint8_t ro = P_casino;
  menuSave(&ro, 1);
  CHECK(EEPROM.writes == w, "status properties are not stored");
  uint8_t none = 0; menuSave(&none, 0);
  CHECK(EEPROM.writes == w, "empty batch");
  // ---- loading ----
  menuSet(P_stripMax, 5); menuSet(P_stripExp, 3);
  CHECK(menuBegin() == true, "saved image is valid");
  // the generated index of the menu used through the menuRef "brightnessTest": it has 5 items, the first is the Pattern, the second the Level
  CHECK(menuRd8(MN_COUNT, MENU_REF_brightnessTest) == 5 && menuRd8(IT_A, menuItemBase(MENU_REF_brightnessTest)) == P_testPattern && menuRd8(IT_A, menuItemBase(MENU_REF_brightnessTest) + 1) == P_testLevel, "MENU_REF_brightnessTest is the menu of the Brightness test");
  CHECK(menuRd8(MN_COUNT, MENU_REF_stripBrightness) == 4 && menuRd8(IT_A, menuItemBase(MENU_REF_stripBrightness)) == P_stripMin, "MENU_REF_stripBrightness is the frame curve group");
  CHECK(menuPos(P_stripMax) == 201 && menuPos(P_stripExp) == 31, "saved values loaded");
  menuSet(P_arrow, 1);
  menuBegin();
  CHECK(menuPos(P_arrow) == 0, "status properties start at their default");
  EEPROM.mem[1] ^= 1;
  CHECK(menuBegin() == false && menuPos(P_stripMax) == 80, "wrong tag gives defaults");
  EEPROM.mem[1] ^= 1;
  EEPROM.mem[4 + 2] = 2;                      // arrowReverse has 2 positions
  CHECK(menuBegin() == false && menuPos(P_stripMax) == 80, "slot out of range gives defaults");
  EEPROM.mem[4 + 2] = 1;
  CHECK(menuBegin() == true && menuPos(P_arrowReverse) == 1, "slot at the upper end is valid");
  EEPROM.mem[4 + 0] = 16;                     // an effect has 16 positions (0..15)
  CHECK(menuBegin() == false, "effect slot 16 is out of range");
  EEPROM.mem[4 + 0] = 15; EEPROM.mem[4 + 1] = 0;
  CHECK(menuBegin() == true && menuPos(P_frameEffect) == 15 && menuPos(P_lettersEffect) == 0, "effect slots 15 and 0 are valid");
  EEPROM.mem[4 + 0] = 1; EEPROM.mem[4 + 1] = 1;
  // after an invalid image the first save rewrites everything
  EEPROM.mem[0] = 0;
  menuBegin();
  menuSet(P_stickMin, 7); one = P_stickMin; menuSave(&one, 1);
  CHECK(EEPROM.mem[0] == 0xEF && EEPROM.mem[4 + 12] == 7 && EEPROM.mem[4 + 10] == 80, "rewritten after a mismatch");

  // ---- status page strings ----
  menuBegin(); menuSet(P_casino, 1); menuSet(P_arrow, 0);
  char r0[17], r1[17];
  menuRows(0, r0, r1);
  CHECK(std::string(r0) == "F01 L01 Open    " && std::string(r1) == "\x7F Customer right", "status page: default effects, casino open, arrow left");
  menuSet(P_arrow, 1);
  menuRows(0, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "status page, arrow right");
  menuSet(P_frameEffect, 12); menuSet(P_lettersEffect, 3);
  menuRows(0, r0, r1);
  CHECK(std::string(r0) == "F12 L03 Open    ", "effect numbers use two digits");
  menuSet(P_casino, 0); menuSet(P_lettersEffect, 0); menuActiveLetters = 7; menuChangeInSec = 125;
  menuSince = 0;
  menuRows(1000, r0, r1);
  CHECK(std::string(r0) == "F12 L*07 Closed ", "random letters effect: star and the pattern really showing");
  CHECK(std::string(r1) == "\x7E Customer left ", "countdown: first phase shows the arrow");
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "Change in 2:05  ", "countdown: second phase");
  menuChangeInSec = 3601;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "Change in 60:01 ", "countdown minutes");
  menuChangeInSec = -1;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "no countdown without a value");
  menuSet(P_lettersEffect, 4); menuChangeInSec = 5;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "no countdown while no effect is random");
  menuSet(P_frameEffect, 0); menuActiveFrame = 0;
  menuRows(3000, r0, r1);
  CHECK(std::string(r0) == "F*-- L04 Closed ", "random frame effect with an unknown pattern shows --");
  CHECK(std::string(r1) == "Change in 0:05  ", "a random frame effect also gives the countdown");
  menuSet(P_lettersEffect, 0); menuActiveFrame = 15; menuActiveLetters = 9; menuChangeInSec = -1;
  menuRows(0, r0, r1);
  CHECK(std::string(r0) == "F*15 L*09 Closed", "both random: 16 characters");
  menuSet(P_frameEffect, 1); menuSet(P_lettersEffect, 1); menuActiveFrame = 0; menuActiveLetters = 0; menuChangeInSec = -1;

  // ---- shell: debounced inputs on the real pins ----
  for (int k = 0; k < 64; k++) g_pinLevel[k] = HIGH;
  menuBegin();
  menuSwitch.stable = menuSwitch.candidate = readHexRaw(HEX_MENU);
  CHECK(menuSwitch.stable == 0, "all legs open reads 0");
  unsigned long now = 100;
  auto settle = [&](unsigned long ms) { pollInputs(now); now += ms; pollInputs(now); };
  const int pins[4] = {38, 41, 42, 45};
  for (int v = 0; v < 16; v++) {
    for (int k = 0; k < 4; k++) g_pinLevel[pins[k]] = (v >> k) & 1 ? LOW : HIGH;
    pollInputs(now); now += 5;
    pollInputs(now); now += 20; pollInputs(now); now += 20;
    CHECK(menuSwitch.stable == (uint8_t)v, "menu switch value");
  }
  for (int k = 0; k < 4; k++) g_pinLevel[pins[k]] = HIGH;
  settle(30); settle(30);
  CHECK(menuSwitch.stable == 0, "menu switch back to 0");
  // debounce: a new level is not accepted after 5 ms
  g_pinLevel[38] = LOW;
  pollInputs(now); now += 5; pollInputs(now);
  CHECK(menuSwitch.stable == 0, "hex debounce holds a new level for 10 ms");
  now += 10; pollInputs(now);
  CHECK(menuSwitch.stable == 1, "hex debounce accepts it afterwards");
  g_pinLevel[38] = HIGH; settle(30); settle(30);
  // each menu leg alone
  g_pinLevel[41] = LOW; settle(30); settle(30); CHECK(menuSwitch.stable == 2, "leg 2 = D41");
  g_pinLevel[41] = HIGH; g_pinLevel[42] = LOW; settle(30); settle(30); CHECK(menuSwitch.stable == 4, "leg 4 = D42");
  g_pinLevel[42] = HIGH; g_pinLevel[45] = LOW; settle(30); settle(30); CHECK(menuSwitch.stable == 8, "leg 8 = D45");
  g_pinLevel[45] = HIGH; settle(30); settle(30);
  menuSwitch.stable = menuSwitch.candidate = readHexRaw(HEX_MENU);

  // arrow and casino slide switches (LOW = the arrow points to the customer's left = "Right" seen from behind / casino closed)
  g_pinLevel[32] = LOW; settle(40); settle(40);
  CHECK(menuPos(P_arrow) == 1, "arrow switch LOW = customer left = Right seen from behind");
  g_pinLevel[32] = HIGH; settle(40); settle(40);
  CHECK(menuPos(P_arrow) == 0, "arrow switch HIGH = customer right = Left seen from behind");
  g_pinLevel[30] = LOW; settle(40); settle(40);
  CHECK(menuPos(P_casino) == 0, "casino switch LOW = closed");
  g_pinLevel[30] = HIGH; settle(40); settle(40);
  CHECK(menuPos(P_casino) == 1, "casino switch HIGH = open");
  // button debounce
  g_pinLevel[46] = LOW; pollInputs(now); now += 10; pollInputs(now);
  CHECK(menuMode == MM_STATUS, "button debounce holds 30 ms");
  now += 30; pollInputs(now);
  CHECK(menuMode == MM_MENU && menuDepth == 1, "OK on D46 opens the menu (on press only)");
  g_pinLevel[46] = HIGH; settle(40); settle(40);
  CHECK(menuMode == MM_MENU && menuDepth == 1 && menuLevelItem[0] == 0, "releasing OK does nothing");

  // menu switch on D38, D41, D42, D45, free running
  const int mpins[4] = {38, 41, 42, 45};
  auto menuTo = [&](int v) {
    for (int k = 0; k < 4; k++) g_pinLevel[mpins[k]] = (v >> k) & 1 ? LOW : HIGH;
    settle(20); settle(20);
  };
  menuTo(1);  CHECK(menuLevelItem[0] == 1, "menu switch 0 -> 1 moves one item down");
  menuTo(3);  CHECK(menuLevelItem[0] == 3, "menu switch 1 -> 3 moves two items");
  menuTo(1);  CHECK(menuLevelItem[0] == 1, "menu switch 3 -> 1 moves back two items");
  menuTo(9);  CHECK(menuLevelItem[0] == 1, "a jump of exactly half a turn is ignored");
  menuTo(10); CHECK(menuLevelItem[0] == 2, "9 -> 10 moves one item");
  menuTo(1);  CHECK(menuLevelItem[0] == 7, "10 -> 1 (seven steps forward) is clamped to the last item");
  menuTo(15); CHECK(menuLevelItem[0] == 5, "1 -> 15 (two steps back) moves two items up");
  // the pins map to the right bits one by one
  menuTo(0); menuLevelItem[0] = 2;
  g_pinLevel[41] = LOW; settle(20); settle(20);
  CHECK(menuLevelItem[0] == 4, "leg 2 = D41 (0 -> 2)");
  g_pinLevel[41] = HIGH; settle(20); settle(20);
  menuLevelItem[0] = 0;
  g_pinLevel[42] = LOW; settle(20); settle(20);
  CHECK(menuLevelItem[0] == 4, "leg 4 = D42 (0 -> 4)");
  g_pinLevel[42] = HIGH; settle(20); settle(20);

  // SHIFT on D50: ten times larger steps while editing
  menuMode = MM_EDIT; menuEditProp = P_stripMax; menuEditStart = menuPos(P_stripMax); menuSet(P_stripMax, 80);
  menuTo(0); menuTo(1);
  CHECK(menuPos(P_stripMax) == 81, "one step without SHIFT");
  g_pinLevel[50] = LOW; settle(40); settle(40);
  menuTo(0);
  CHECK(menuPos(P_stripMax) == 71, "SHIFT on D50 makes the step ten times larger");
  g_pinLevel[50] = HIGH; settle(40); settle(40);
  // CANCEL on D49 leaves an edit and restores the value
  g_pinLevel[49] = LOW; settle(40); settle(40);
  CHECK(menuMode == MM_MENU && menuPos(P_stripMax) == menuEditStart, "CANCEL on D49 cancels the edit");
  g_pinLevel[49] = HIGH; settle(40); settle(40);

  // reverse flags and applySwitches
  shellApplySwitches = true;
  switchArrow.stable = true; switchCasino.stable = true;
  menuSet(P_arrowReverse, 0); menuSet(P_casinoReverse, 0);
  CHECK(arrowPosition() == 1 && casinoPosition() == 0, "LOW means arrow to the customer's left (Right from behind) and closed");
  menuSet(P_arrowReverse, 1); menuSet(P_casinoReverse, 1);
  CHECK(arrowPosition() == 0 && casinoPosition() == 1, "reverse flags swap them");
  menuSet(P_arrow, 1); menuSet(P_casino, 0);
  menuSet(P_arrowReverse, 0); menuSet(P_arrowReverse, 1);       // changes the property again: the callback applies the switches
  CHECK(menuPos(P_arrow) == 0, "applySwitches updates the arrow value");
  menuSet(P_casinoReverse, 0); menuSet(P_casinoReverse, 1);
  CHECK(menuPos(P_casino) == 1, "applySwitches updates the casino value");
  shellApplySwitches = false;

  // ---- random effects countdown (test shell emulation) ----
  menuSet(P_frameEffect, 0); menuSet(P_lettersEffect, 1); randomDelaySec = 0; menuSet(P_randomDelay, 2); menuActiveFrame = 0; menuActiveLetters = 0;
  updateRandomCountdown(1000);
  CHECK(menuChangeInSec == 120, "2 minutes = 120 s");
  CHECK(menuActiveFrame == 2 && menuActiveLetters == 1, "random frame gets a made-up pattern number, a fixed letters effect shows its own number");
  updateRandomCountdown(11000);
  CHECK(menuChangeInSec == 110, "counts down");
  updateRandomCountdown(121000);
  CHECK(menuChangeInSec == 0, "reaches 0");
  CHECK(menuActiveFrame == 6 && menuActiveLetters == 1, "at the end the random frame effect picks another number, the fixed one stays");
  menuSet(P_frameEffect, 5); menuSet(P_lettersEffect, 3); updateRandomCountdown(130000);
  CHECK(menuChangeInSec == -1, "no countdown while no effect is random");
  CHECK(menuActiveFrame == 5 && menuActiveLetters == 3, "fixed effects show their own numbers");
  menuSet(P_frameEffect, 0); menuSet(P_lettersEffect, 0); menuActiveFrame = 0; menuActiveLetters = 0; randomDelaySec = 0;
  updateRandomCountdown(1000);
  CHECK(menuChangeInSec == 120 && menuActiveFrame == 2 && menuActiveLetters == 2, "both random: one countdown, both get numbers");
  updateRandomCountdown(121000);
  CHECK(menuActiveFrame == 6 && menuActiveLetters == 8, "both random: both pick new numbers at the end of the countdown");
  menuSet(P_frameEffect, 1); menuSet(P_lettersEffect, 1);

  // ---- sign of life ----
  Serial.capture = true; Serial.captured.clear();
  menuMode = MM_STATUS; menuDepth = 0; menuSet(P_frameEffect, 12); menuSet(P_lettersEffect, 0); menuSet(P_arrow, 1); menuSet(P_casino, 1);
  menuStepCount = 0; keyCount = 0; callbackCount = 0; lcdOk = false; menuEepromValid = false;
  Serial.captured.clear();                    // the menuSet calls above print CB lines for the effect callbacks
  printDebug(5000);
  CHECK(Serial.captured == "DBG up=5 mode=S fx=12/0 arrow=R casino=O menu=0 keys=0 cb=0 eeprom=defaults lcd=0\n", "debug line");
  menuMode = MM_EDIT; CHECK(modeLetter() == 'E', "mode E");
  menuMode = MM_MENU; CHECK(modeLetter() == 'M', "mode M");
  menuMode = MM_CONFIRM; CHECK(modeLetter() == 'C', "mode C");
  menuMode = MM_VIEW; CHECK(modeLetter() == 'V', "mode V");
  menuMode = MM_STATUS; CHECK(modeLetter() == 'S', "mode S");
  Serial.capture = false;
  beatOn = false; lastBeat = 0; eventWindowStart = 0; lastDebug = 0; eventsNow = eventsPrev = 0; anyEvent = false;
  signOfLife(499);
  CHECK(g_pinOut[13] == LOW, "LED off before 500 ms");
  signOfLife(500);
  CHECK(g_pinOut[13] == HIGH, "LED on after 500 ms");
  eventsNow = 4; signOfLife(500 + 125);
  CHECK(g_pinOut[13] == LOW, "faster blinking while many events happen");

  // ---- set-up ----
  Serial.captured.clear(); Serial.capture = true;
  Wire.present = true;
  setup();
  CHECK(Serial.baud == 115200, "serial speed 115200");
  CHECK(Serial.captured.find("Serial speed 115200 baud") != std::string::npos, "baud printed at start-up");
  CHECK(lcdOk && Wire.address == 0x27 && Wire.fourBit, "LCD found at 0x27");
  Serial.capture = false;
  CHECK(menuPos(P_arrow) == 0 && menuPos(P_casino) == 1, "start-up reads the switches");

  // ---- scroll phase at the list ends and after contact bounce ----
  menuMode = MM_MENU; menuDepth = 1; menuLevelItem[0] = 3; menuSince = 100; menuUndoValid = false;
  menuTurn(1, false, 1000);
  CHECK(menuLevelItem[0] == 4 && menuSince == 1000, "a real move starts a new view");
  menuTurn(-1, false, 1100);
  CHECK(menuLevelItem[0] == 3 && menuSince == 100, "a move straight back (100 ms) keeps the scroll of the item left");
  menuTurn(1, false, 1150); menuTurn(-1, false, 1600);
  CHECK(menuLevelItem[0] == 3 && menuSince == 1600, "a move back after 450 ms is a new view");
  menuLevelItem[0] = 0; menuSince = 777; menuTurn(-1, false, 2000);
  CHECK(menuLevelItem[0] == 0 && menuSince == 777, "a turn at the first item changes nothing");
  menuLevelItem[0] = 3; menuTurn(1, false, 3000); menuKey(MENU_KEY_OK, 3010);
  CHECK(!menuUndoValid, "a key press forgets the bounce memory");
}
