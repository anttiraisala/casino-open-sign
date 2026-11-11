// C++-specific checks of the menu engine and the test shell (included inside main() of menu_engine_harness.cpp).
// Expected values are written out here on purpose, not taken from the sketch constants.
{
  shellApplySwitches = false;
  char t[40];
  auto text = [&](uint8_t prop, uint16_t pos) { menuValueText(prop, pos, t); return std::string(t); };
  // ---- value texts ----
  CHECK(text(P_stripMax, 0) == "0" && text(P_stripMax, 255) == "255" && text(P_stripMax, 80) == "80", "int texts");
  CHECK(text(P_speedFrame, 0) == "0.25x" && text(P_speedFrame, 12) == "1.00x" && text(P_speedFrame, 24) == "4.00x", "speed texts");
  CHECK(text(P_stripExp, 0) == "0.10" && text(P_stripExp, 31) == "2.67" && text(P_stripExp, 50) == "20.00", "exponent texts");
  CHECK(text(P_stripMix, 0) == "0.00" && text(P_stripMix, 25) == "0.50" && text(P_stripMix, 50) == "1.00", "mix texts");
  CHECK(text(P_arrowReverse, 0) == "No" && text(P_arrowReverse, 1) == "Yes", "bool texts");
  CHECK(text(P_arrow, 0) == "Left" && text(P_arrow, 1) == "Right" && text(P_casino, 0) == "Closed", "choice texts");
  CHECK(text(P_program, 11) == "B" && text(P_program, 15) == "F", "hex texts");
  CHECK(text(P_randomDelay, 0) == "random" && text(P_randomDelay, 2) == "2 min" && text(P_randomDelay, 30) == "30 min", "value texts");

  // ---- start-up with an empty EEPROM ----
  menuMode = MM_MENU; menuDepth = 3;
  CHECK(menuBegin() == false, "empty EEPROM is not valid");
  CHECK(menuMode == MM_STATUS && menuDepth == 0, "start-up shows the status page");
  CHECK(menuPos(P_stripMax) == 80 && menuPos(P_stripExp) == 31 && menuPos(P_stripMix) == 50 && menuPos(P_speedFrame) == 12 && menuPos(P_randomDelay) == 2, "defaults");
  CHECK(EEPROM.writes == 0, "begin writes nothing");

  uint8_t roFirst = P_program;
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
  CHECK(EEPROM.mem[0] == 0x48 && EEPROM.mem[1] == 0x2E && EEPROM.mem[2] == 0x80 && EEPROM.mem[3] == 0xA1, "tag, little endian");
  CHECK(EEPROM.mem[4 + 7] == 200, "stripMax in slot 7");
  CHECK(EEPROM.mem[4 + 0] == 0 && EEPROM.mem[4 + 2] == 12 && EEPROM.mem[4 + 5] == 2, "other slots hold defaults (arrowReverse, speedFrame, randomDelay)");
  CHECK(EEPROM.mem[4 + 8] == 31 && EEPROM.mem[4 + 12] == 50 && EEPROM.mem[4 + 17] == 50, "exponent and mix slots");
  CHECK(menuEepromValid, "valid after the first save");
  CHECK(EEPROM.writes > 4, "image written");
  // later saves write only the changed slot
  long w = EEPROM.writes;
  menuSet(P_stripMax, 201); menuSave(&one, 1);
  CHECK(EEPROM.writes == w + 1 && EEPROM.mem[4 + 7] == 201, "one changed slot");
  w = EEPROM.writes;
  uint8_t ro = P_program;
  menuSave(&ro, 1);
  CHECK(EEPROM.writes == w, "status properties are not stored");
  uint8_t none = 0; menuSave(&none, 0);
  CHECK(EEPROM.writes == w, "empty batch");
  // ---- loading ----
  menuSet(P_stripMax, 5); menuSet(P_stripExp, 3);
  CHECK(menuBegin() == true, "saved image is valid");
  CHECK(menuPos(P_stripMax) == 201 && menuPos(P_stripExp) == 31, "saved values loaded");
  menuSet(P_program, 9);
  menuBegin();
  CHECK(menuPos(P_program) == 0, "status properties start at their default");
  EEPROM.mem[1] ^= 1;
  CHECK(menuBegin() == false && menuPos(P_stripMax) == 80, "wrong tag gives defaults");
  EEPROM.mem[1] ^= 1;
  EEPROM.mem[4 + 0] = 2;                      // arrowReverse has 2 positions
  CHECK(menuBegin() == false && menuPos(P_stripMax) == 80, "slot out of range gives defaults");
  EEPROM.mem[4 + 0] = 1;
  CHECK(menuBegin() == true && menuPos(P_arrowReverse) == 1, "slot at the upper end is valid");
  // after an invalid image the first save rewrites everything
  EEPROM.mem[0] = 0;
  menuBegin();
  menuSet(P_stickMin, 7); one = P_stickMin; menuSave(&one, 1);
  CHECK(EEPROM.mem[0] == 0x48 && EEPROM.mem[4 + 9] == 7 && EEPROM.mem[4 + 7] == 80, "rewritten after a mismatch");

  // ---- status page strings ----
  menuBegin(); menuSet(P_program, 11); menuSet(P_casino, 1); menuSet(P_arrow, 0);
  char r0[17], r1[17];
  menuRows(0, r0, r1);
  CHECK(std::string(r0) == "#B Casino Open  " && std::string(r1) == "\x7F Customer right", "status page, arrow left");
  menuSet(P_arrow, 1);
  menuRows(0, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "status page, arrow right");
  menuSet(P_program, 15); menuChangeInSec = 125;
  menuSince = 0;
  menuRows(1000, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "countdown: first phase shows the arrow");
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "Change in 2:05  ", "countdown: second phase");
  menuChangeInSec = 3601;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "Change in 60:01 ", "countdown minutes");
  menuChangeInSec = -1;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "no countdown without a value");
  menuSet(P_program, 14); menuChangeInSec = 5;
  menuRows(3000, r0, r1);
  CHECK(std::string(r1) == "\x7E Customer left ", "no countdown outside program F");
  menuChangeInSec = -1;

  // ---- shell: debounced inputs on the real pins ----
  for (int k = 0; k < 64; k++) g_pinLevel[k] = HIGH;
  menuBegin();
  programSwitch.stable = programSwitch.candidate = readHexRaw(HEX_PROGRAM);
  menuSwitch.stable = menuSwitch.candidate = readHexRaw(HEX_MENU);
  CHECK(programSwitch.stable == 0 && menuSwitch.stable == 0, "all legs open reads 0");
  unsigned long now = 100;
  auto settle = [&](unsigned long ms) { pollInputs(now); now += ms; pollInputs(now); };
  const int pins[4] = {34, 35, 36, 37};
  for (int v = 0; v < 16; v++) {
    for (int k = 0; k < 4; k++) g_pinLevel[pins[k]] = (v >> k) & 1 ? LOW : HIGH;
    pollInputs(now); now += 5;
    pollInputs(now); now += 20; pollInputs(now); now += 20;
    CHECK(menuPos(P_program) == (uint16_t)v, "program switch value");
  }
  for (int k = 0; k < 4; k++) g_pinLevel[pins[k]] = HIGH;
  settle(30); settle(30);
  CHECK(menuPos(P_program) == 0, "program switch back to 0");
  // debounce: a new level is not accepted after 5 ms
  g_pinLevel[34] = LOW;
  pollInputs(now); now += 5; pollInputs(now);
  CHECK(menuPos(P_program) == 0, "hex debounce holds a new level for 10 ms");
  now += 10; pollInputs(now);
  CHECK(menuPos(P_program) == 1, "hex debounce accepts it afterwards");
  g_pinLevel[34] = HIGH; settle(30); settle(30);
  // each program leg alone
  g_pinLevel[35] = LOW; settle(30); settle(30); CHECK(menuPos(P_program) == 2, "leg 2 = D35");
  g_pinLevel[35] = HIGH; g_pinLevel[36] = LOW; settle(30); settle(30); CHECK(menuPos(P_program) == 4, "leg 4 = D36");
  g_pinLevel[36] = HIGH; g_pinLevel[37] = LOW; settle(30); settle(30); CHECK(menuPos(P_program) == 8, "leg 8 = D37");
  g_pinLevel[37] = HIGH; settle(30); settle(30);

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
  g_pinLevel[48] = LOW; pollInputs(now); now += 10; pollInputs(now);
  CHECK(menuMode == MM_STATUS, "button debounce holds 30 ms");
  now += 30; pollInputs(now);
  CHECK(menuMode == MM_MENU && menuDepth == 1, "OK on D48 opens the menu (on press only)");
  g_pinLevel[48] = HIGH; settle(40); settle(40);
  CHECK(menuMode == MM_MENU && menuDepth == 1 && menuLevelItem[0] == 0, "releasing OK does nothing");

  // menu switch on D42-D45, free running
  const int mpins[4] = {42, 43, 44, 45};
  auto menuTo = [&](int v) {
    for (int k = 0; k < 4; k++) g_pinLevel[mpins[k]] = (v >> k) & 1 ? LOW : HIGH;
    settle(20); settle(20);
  };
  menuTo(1);  CHECK(menuLevelItem[0] == 1, "menu switch 0 -> 1 moves one item down");
  menuTo(3);  CHECK(menuLevelItem[0] == 3, "menu switch 1 -> 3 moves two items");
  menuTo(1);  CHECK(menuLevelItem[0] == 1, "menu switch 3 -> 1 moves back two items");
  menuTo(9);  CHECK(menuLevelItem[0] == 1, "a jump of exactly half a turn is ignored");
  menuTo(10); CHECK(menuLevelItem[0] == 2, "9 -> 10 moves one item");
  menuTo(1);  CHECK(menuLevelItem[0] == 4, "10 -> 1 (seven steps forward) is clamped to the last item");
  menuTo(15); CHECK(menuLevelItem[0] == 2, "1 -> 15 (two steps back) moves two items up");
  // the pins map to the right bits one by one
  menuTo(0); menuLevelItem[0] = 2;
  g_pinLevel[43] = LOW; settle(20); settle(20);
  CHECK(menuLevelItem[0] == 4, "leg 2 = D43 (0 -> 2)");
  g_pinLevel[43] = HIGH; settle(20); settle(20);
  menuLevelItem[0] = 0;
  g_pinLevel[44] = LOW; settle(20); settle(20);
  CHECK(menuLevelItem[0] == 4, "leg 4 = D44 (0 -> 4 clamps at 4)");
  g_pinLevel[44] = HIGH; settle(20); settle(20);

  // SHIFT on D50: ten times larger steps while editing
  menuMode = MM_EDIT; menuEditProp = P_stripMax; menuEditStart = menuPos(P_stripMax); menuSet(P_stripMax, 80);
  menuTo(0); menuTo(1);
  CHECK(menuPos(P_stripMax) == 81, "one step without SHIFT");
  g_pinLevel[50] = LOW; settle(40); settle(40);
  menuTo(0);
  CHECK(menuPos(P_stripMax) == 71, "SHIFT on D50 makes the step ten times larger");
  g_pinLevel[50] = HIGH; settle(40); settle(40);
  // BACK on D49 leaves an edit and restores the value
  g_pinLevel[49] = LOW; settle(40); settle(40);
  CHECK(menuMode == MM_MENU && menuPos(P_stripMax) == menuEditStart, "BACK on D49 cancels the edit");
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

  // ---- random program countdown ----
  menuSet(P_program, 15); randomDelaySec = 0; menuSet(P_randomDelay, 2);
  updateRandomCountdown(1000);
  CHECK(menuChangeInSec == 120, "2 minutes = 120 s");
  updateRandomCountdown(11000);
  CHECK(menuChangeInSec == 110, "counts down");
  updateRandomCountdown(121000);
  CHECK(menuChangeInSec == 0, "reaches 0");
  menuSet(P_program, 3); updateRandomCountdown(130000);
  CHECK(menuChangeInSec == -1, "no countdown outside program F");

  // ---- sign of life ----
  Serial.capture = true; Serial.captured.clear();
  menuMode = MM_STATUS; menuDepth = 0; menuSet(P_program, 15); menuSet(P_arrow, 1); menuSet(P_casino, 1);
  menuStepCount = 0; keyCount = 0; callbackCount = 0; lcdOk = false; menuEepromValid = false;
  printDebug(5000);
  CHECK(Serial.captured == "DBG up=5 mode=S prog=F arrow=R casino=O menu=0 keys=0 cb=0 eeprom=defaults lcd=0\n", "debug line");
  menuMode = MM_EDIT; CHECK(modeLetter() == 'E', "mode E");
  menuMode = MM_MENU; CHECK(modeLetter() == 'M', "mode M");
  menuMode = MM_CONFIRM; CHECK(modeLetter() == 'C', "mode C");
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
  CHECK(menuPos(P_program) == 0 && menuPos(P_arrow) == 0 && menuPos(P_casino) == 1, "start-up reads the switches");
}
