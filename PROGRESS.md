# PROGRESS.md - status (2026-11-07)

## Done
- Sign sketch (`sketch.ino`) for the first design: 10 WS2812B strips (pins D4-D12, D31), inputs D2, D3, D30, D22-D29, I2C LCD. Simulated in Wokwi by the owner earlier.
- Drawings, parts list (snapshot 2026-11-02), host test harnesses (all four pass on a host compiler with mock libraries).
- `strip_test/` (WS2812B strip test), `stick_test/` (sticks on a separate 5 V supply, modified cables), `stick_test_direct/` (11 sticks in the Grove shield, moving patterns, sign of life).

## Tested
- `stick_test_direct`: run on real hardware by the owner. All 11 Grove RGB LED sticks plugged unmodified into the Grove Base Shield (D2-D8, A0-A3), Arduino + shield fed from an external 5 V supply, every stick shows its patterns. The load test (button held, all LEDs white) was not reported separately; heat and voltage drop at full load are not confirmed.
- Host harnesses: logic only (mock libraries), no AVR build, no Wokwi run by the AI.

## Not tested
- `strip_test` and `stick_test` on real hardware; the sign sketch on real hardware.

## Planned / open
- Rebuild the sign around the stick arrow (10 sticks + spare) and possibly 12 V WS2811 strips; versioned releases.
- Add the sign-of-life rule to `sketch.ino`, `strip_test/`, `stick_test/`.
- Open: second Grove connector on the sticks?, shield 5 V switch position (user says: 5 V), final stick count (11 work), whether the partco parts list needs a refresh (only on request).
