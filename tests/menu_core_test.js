// Tests of tools/menu-editor/menu-core.js (pure logic). Expected values are hard-coded.
//   node tests/menu_core_test.js
'use strict';
const path = require('path');
const core = require(process.env.MENU_CORE || path.join(__dirname, '..', 'tools', 'menu-editor', 'menu-core.js'));
const V2 = require(path.join(__dirname, '..', 'tools', 'menu-editor', 'menu-v2.json'));
let fails = 0;
function check(c, msg) { if (!c) { console.log('FAIL: ' + msg); fails++; } }
function eq(a, b, msg) { check(JSON.stringify(a) === JSON.stringify(b), msg + ' (got ' + JSON.stringify(a) + ', want ' + JSON.stringify(b) + ')'); }
function near(a, b, tol, msg) { check(Math.abs(a - b) <= tol, msg + ' (got ' + a + ', want ' + b + ')'); }
function mk(props, items, extra) {
  return Object.assign({ format: 1, common: { properties: props }, root: { items: items } }, extra || {});
}
function errs(src) { return core.build(src).errors.join(' | '); }
const clone = o => JSON.parse(JSON.stringify(o));

// ---- the shipped V2 menu is valid
{
  const m = core.build(V2);
  eq(m.errors, [], 'V2 menu has no errors'); eq(m.warnings, [], 'V2 menu has no warnings');
  eq(m.order.length, 25, 'V2 has 25 properties');
  eq(core.persistIds(m).length, 21, 'V2 stores 21 properties (arrow and casino are not stored)');
  eq(['strip', 'letters', 'stick'].map(g => ['Min', 'Max', 'Exp', 'Mix'].map(x => m.props[g + x].name).join()).join('|'), 'Min,Max,Exponent,Mix|Min,Max,Exponent,Mix|Min,Max,Exponent,Mix', 'strip, letters and stick each have min, max, exponent and mix');
  eq(m.props.lettersMix.count, 51, 'mix 0..1 linear has 51 positions'); eq(core.valueAt(m.props.stickMix, m.props.stickMix.defaultPos), 1, 'mix default 1.0'); eq(m.props.letersMix === undefined, true, 'no typo property');
  eq(m.props.lettersExp.callbacks, ['rebuildLettersCurve'], 'letters curve callback'); eq(m.props.stripMix.scale, 'linear', 'mix is linear');
  eq(m.props.stripExp.count, 51, 'exponent log 50 steps has 51 positions');
  near(core.valueAt(m.props.stripExp, 31), 2.67, 0.005, 'exponent default 2.67 is on the grid at position 31');
  eq(m.props.stripExp.defaultPos, 31, 'exponent default position');
  eq(m.props.speedFrame.count, 31, 'speed has 31 positions');
  eq(m.props.speedFrame.defaultPos, 18, 'speed default 1.0 is position 18');
  near(core.valueAt(m.props.speedFrame, 0), 0.125, 1e-9, 'speed min'); near(core.valueAt(m.props.speedFrame, 30), 4, 1e-9, 'speed max');
  eq(m.props.stripMax.default, 80, 'brightness max default 80'); eq(m.props.randomDelay.count, 31, 'delay positions 0..30');
  eq(m.props.arrow.readOnly, true, 'arrow is read-only via the type template');
  eq(m.props.frameEffect.count, 16, 'frame effect has positions 0..15'); eq(m.props.lettersEffect.count, 16, 'letters effect has positions 0..15');
  eq(m.props.frameEffect.defaultPos, 1, 'frame effect default is 1'); eq(m.props.lettersEffect.defaultPos, 1, 'letters effect default is 1');
  eq(core.valueText(m.props.lettersEffect, 0), 'Random', 'effect 0 is random'); eq(core.valueText(m.props.lettersEffect, 13), '13 Slot machine', 'letters effect 13'); eq(core.valueText(m.props.frameEffect, 15), '15 Grey bands', 'frame effect 15'); eq(core.valueText(m.props.frameEffect, 2), '2 Comet', 'frame effect 2');
  eq(m.props.frameEffect.callbacks, ['effectFrameChanged'], 'frame effect callback'); eq(m.props.lettersEffect.callbacks, ['effectLettersChanged'], 'letters effect callback');
  eq(m.props.arrowReverse.trueText, 'Yes', 'bool texts from the type template');
  // brightness test: Brightness > Brightness test (Pattern, Level, then the three curve groups); nothing of it is stored
  eq(m.props.testPattern.choices, ['Ramp', 'All white', 'Red', 'Green', 'Blue', 'Level'], 'test patterns'); eq(m.props.testPattern.defaultPos, 0, 'the pattern starts at Ramp');
  eq(m.props.testPattern.persist, false, 'the pattern is not stored'); eq(m.props.testLevel.persist, false, 'the level is not stored');
  eq([m.props.testLevel.min, m.props.testLevel.max, m.props.testLevel.default, m.props.testLevel.step], [0, 255, 128, 1], 'test level 0..255, default 128, step 1');
  eq(core.persistIds(m).indexOf('testPattern') < 0 && core.persistIds(m).indexOf('testLevel') < 0, true, 'the test properties have no EEPROM slot');
  eq(core.layoutTag(m), '6e5c98ef', 'the EEPROM layout tag is unchanged by the test mode');
  const br = m.root.items.filter(i => i.name === 'Brightness')[0];
  eq(br.items.map(i => i.name), ['Brightness test', 'Frame', 'Letters', 'Arrow'], 'Brightness: the test submenu first, then the three groups');
  eq(br.items[0].ref, 'brightnessTest', 'the test submenu is the menu brightnessTest');
  eq(br.items[0].items.map(i => i.name || i.id), ['testPattern', 'testLevel', 'Frame', 'Letters', 'Arrow'], 'Brightness test: Pattern, Level and the three curve groups');
}

// ---- property types, steps, limits, wrap
{
  const m = core.build(mk({
    i: { kind: 'int', min: -50, max: 50, default: 0 },
    s: { kind: 'int', min: 0, max: 255, default: 250 },
    f: { kind: 'float', min: 0, max: 10, default: 5 },
    w: { kind: 'int', min: 0, max: 9, wrap: true, default: 8 },
    c: { kind: 'choice', choices: ['a', 'b', 'c'], default: 'c', wrap: true },
    b: { kind: 'bool' },
    st: { kind: 'int', min: 0, max: 100, step: 25, default: 50 }
  }, [{ propertyRef: 'i' }, { propertyRef: 's' }, { propertyRef: 'f' }, { propertyRef: 'w' }, { propertyRef: 'c' }, { propertyRef: 'b' }, { propertyRef: 'st' }]));
  eq(m.errors, [], 'sample builds');
  const P = m.props;
  eq(P.i.count, 101, 'int -50..50 has 101 positions'); eq(P.f.count, 51, 'float default 50 intervals');
  near(core.valueAt(P.f, 1), 0.2, 1e-9, 'float step is range/50'); eq(P.st.count, 5, 'int with step 25 has 5 positions');
  eq(core.valueAt(P.st, 3), 75, 'int step 25 position 3');
  eq(core.movePos(P.i, 0, -10), 0, 'stop at min'); eq(core.movePos(P.i, 100, 10), 100, 'stop at max');
  eq(core.movePos(P.s, 5, 10), 15, 'step 10 inside'); eq(core.movePos(P.s, 250, 10), 255, 'step capped to the range');
  eq(core.movePos(P.w, 9, 3), 2, 'wrap forward continues from the start'); eq(core.movePos(P.w, 0, -3), 7, 'wrap backward continues from the end');
  eq(core.movePos(P.c, 2, 1), 0, 'choice wraps'); eq(core.movePos(P.c, 0, -1), 2, 'choice wraps backward');
  eq(core.movePos(P.b, 1, 1), 1, 'bool without wrap stops');
  eq(P.c.defaultPos, 2, 'choice default by text'); eq(core.valueText(P.c, 2), 'c', 'choice text');
  eq(core.valueText(P.b, 0), 'Off', 'bool false text'); eq(core.valueText(P.b, 1), 'On', 'bool true text');
  eq(core.valueText(P.i, 40), '-10', 'negative int text'); eq(core.valueText(P.f, 25), '5.00', 'float text 2 decimals');
  eq(P.s.shift, 10, 'int shift default 10'); eq(P.c.shift, 1, 'choice shift default 1');
  eq(core.posOf(P.f, 5.01), 25, 'posOf snaps to nearest');
}

// ---- log scale
{
  const m = core.build(mk({ g: { kind: 'float', scale: 'log', min: 1, max: 100, steps: 2, default: 10 } }, [{ propertyRef: 'g' }]));
  const g = m.props.g;
  near(core.valueAt(g, 1), 10, 1e-9, 'log midpoint of 1..100 is 10'); eq(g.defaultPos, 1, 'log default 10 at position 1');
  near(core.valueAt(g, 2), 100, 1e-9, 'log max'); eq(core.posOf(g, 30), 1, 'log posOf compares in log space (30 is nearer 10 than 100)'); eq(core.posOf(g, 40), 2, '40 is nearer 100 in log space');
}

// ---- value texts, hex, unit
{
  const m = core.build(mk({ h: { kind: 'int', min: 0, max: 15, format: 'hex', default: 11 }, d: { kind: 'int', min: 0, max: 30, unit: ' min', valueTexts: { '0': 'random' }, default: 0 }, u: { kind: 'int', min: 0, max: 30, unit: ' min', default: 7 } },
    [{ propertyRef: 'h' }, { propertyRef: 'd' }, { propertyRef: 'u' }]));
  eq(core.valueText(m.props.h, 11), 'B', 'hex text'); eq(core.valueText(m.props.d, 0), 'random', 'valueTexts'); eq(core.valueText(m.props.d, 5), '5 min', 'unit');
}

// ---- type templates and overrides
{
  const src = mk({ a: { type: 'T2', default: 3 }, b: { type: 'T2', min: 5, default: 6 }, c: { type: 'T1' } }, [{ propertyRef: 'a' }, { propertyRef: 'b' }, { propertyRef: 'c' }]);
  src.common.types = { T1: { kind: 'int', min: 0, max: 10, step: 1 }, T2: { type: 'T1', max: 20, step: 2 } };
  const m = core.build(src);
  eq(m.errors, [], 'template chain builds');
  eq(m.props.a.max, 20, 'derived template overrides base max'); eq(m.props.a.step, 2, 'derived step'); eq(m.props.b.min, 5, 'property overrides template min');
  eq(m.props.c.max, 10, 'base template used alone'); eq(m.props.c.step, 1, 'int step default 1'); eq(m.props.a.count, 11, 'a has 0..20 step 2 = 11 positions');
  src.common.types.T1.type = 'T2';
  check(/type loop/.test(errs(src)), 'type loop detected');
}

// ---- validation errors
{
  const base = () => ({ a: { kind: 'int', min: 0, max: 10 }, ro: { kind: 'int', min: 0, max: 10, readOnly: true } });
  check(/menu loop/.test(errs({ format: 1, common: { menus: { A: { items: [{ menuRef: 'B' }] }, B: { items: [{ menuRef: 'A' }] } } }, root: { items: [{ menuRef: 'A' }] } })), 'menu loop A-B detected');
  check(/menu loop/.test(errs({ format: 1, common: { menus: { A: { items: [{ menuRef: 'A' }] } } }, root: { items: [{ menu: 'x', items: [{ menuRef: 'A' }] }] } })), 'self loop detected');
  eq(errs({ format: 1, common: { properties: base(), menus: { S: { name: 'S', items: [{ propertyRef: 'a' }] } } }, root: { items: [{ menuRef: 'S' }, { menu: 'Other', items: [{ menuRef: 'S' }] }] } }), '', 'a shared menu in two places is allowed');
  check(/unknown menuRef/.test(errs(mk({}, [{ menuRef: 'zz' }]))), 'unknown menuRef');
  check(/unknown property q/.test(errs(mk(base(), [{ propertyRef: 'q' }]))), 'unknown propertyRef');
  check(/defined twice/.test(errs(mk(base(), [{ property: 'a', kind: 'int' }]))), 'duplicate property id');
  check(/log scale needs min > 0/.test(errs(mk({ g: { kind: 'float', scale: 'log', min: 0, max: 5 } }, [{ propertyRef: 'g' }]))), 'log needs min > 0');
  check(/min\/max invalid/.test(errs(mk({ g: { kind: 'int', min: 5, max: 1 } }, [{ propertyRef: 'g' }]))), 'min > max');
  check(/at least 2 choices/.test(errs(mk({ g: { kind: 'choice', choices: ['x'] } }, [{ propertyRef: 'g' }]))), 'choice needs 2');
  check(/default outside/.test(errs(mk({ g: { kind: 'int', min: 0, max: 5, default: 9 } }, [{ propertyRef: 'g' }]))), 'default outside range');
  check(/not one of the choices/.test(errs(mk({ g: { kind: 'choice', choices: ['a', 'b'], default: 'z' } }, [{ propertyRef: 'g' }]))), 'bad choice default');
  check(/kind missing/.test(errs(mk({ g: { min: 1 } }, [{ propertyRef: 'g' }]))), 'kind missing');
  check(/unknown type/.test(errs(mk({ g: { type: 'nope' } }, [{ propertyRef: 'g' }]))), 'unknown type');
  check(/step must be an integer/.test(errs(mk({ g: { kind: 'int', min: 0, max: 5, step: 0 } }, [{ propertyRef: 'g' }]))), 'int step 0');
  { const wro = core.build(mk(base(), [{ preset: 'P', set: { ro: 3, a: 2 } }]));
    check(wro.ok && /read-only property ro/.test(wro.warnings.join()), 'quick-pick on a read-only property is a warning, not an error');
    const pre = wro.root.items[wro.root.items.length - 1];
    check(!('ro' in pre.set) && pre.set.a === 2, 'the read-only target is dropped from the preset, the other value stays'); }
  check(/outside 0\.\.10/.test(errs(mk(base(), [{ preset: 'P', set: { a: 11 } }]))), 'quick-pick value out of range');
  check(/unknown property zz/.test(errs(mk(base(), [{ preset: 'P', set: { zz: 1 } }]))), 'quick-pick unknown property');
  check(/neither set nor setAll/.test(errs(mk(base(), [{ preset: 'P' }]))), 'empty preset');
  check(/setAll must be/.test(errs(mk(base(), [{ preset: 'P', setAll: 'x' }]))), 'bad setAll');
  const many = {}, big = {}; for (let i = 0; i < 17; i++) big['p' + i] = { kind: 'int', min: 0, max: 5 }, many['p' + i] = 1;
  check(/max 16/.test(errs(mk(big, [{ preset: 'P', set: many }]))), 'more than 16 values rejected');
  const ok16 = {}; for (let i = 0; i < 16; i++) ok16['p' + i] = 1;
  eq(errs(mk(big, [{ preset: 'P', set: ok16 }])), '', '16 values are accepted');
  eq(errs(mk(base(), [{ preset: 'P', set: { a: 'default' } }])), '', 'default keyword accepted');
  check(/format must be 1/.test(errs({ format: 2 })), 'format check');
  check(/root\.items missing/.test(errs({ format: 1 })), 'root needed');
  check(/status\.arrow/.test(errs(mk(base(), [{ propertyRef: 'a' }], { status: { casino: 'a', arrow: 'zz', effectFrame: 'a', effectLetters: 'a' } }))), 'status property must exist');
  check(/no items/.test(errs(mk({}, [{ menu: 'Empty', items: [] }]))), 'empty menu rejected');
  const w = core.build(mk({ g: { kind: 'int', min: 0, max: 100, step: 10, default: 14 } }, [{ propertyRef: 'g' }]));
  check(w.ok && /snapped to 10/.test(w.warnings.join()), 'off-grid default snaps with a warning');
  const w2 = core.build(mk({ g: { kind: 'int', min: 0, max: 100, step: 10, default: 40 } }, [{ preset: 'P', set: { g: 44 } }]));
  check(w2.ok && /snapped to 40/.test(w2.warnings.join()), 'off-grid preset value warns');
  eq(core.build(mk({ g: { kind: 'int', min: 0, max: 3, default: 1 } }, [{ property: 'inl', kind: 'int', min: 0, max: 9 }])).order, ['g', 'inl'], 'inline property registered after common ones');
}

// ---- fit / marquee
{
  eq(core.fit('Hello', '', 0, 0), 'Hello           ', 'fit pads to 16');
  eq(core.fit('Frame', '1.00x', 0, 0), 'Frame      1.00x', 'fit right aligns the value');
  eq(core.fit('Frame speed', '1.00x', 0, 0), 'Frame spee 1.00x', 'long name is cut to the free width');
  const t = 'ABCDEFGHIJKLMNOPQRSTUV';   // 22 chars, window 16, maxOff 6
  eq(core.fit(t, '', 999, 0), 'ABCDEFGHIJKLMNOP', 'marquee holds the start for 1 s');
  eq(core.fit(t, '', 1000, 0), 'ABCDEFGHIJKLMNOP', 'first step not yet at 1000');
  eq(core.fit(t, '', 1400, 0), 'BCDEFGHIJKLMNOPQ', 'marquee moves one char per 400 ms');
  eq(core.fit(t, '', 1000 + 6 * 400, 0), 'GHIJKLMNOPQRSTUV', 'marquee reaches the end');
  eq(core.fit(t, '', 1000 + 6 * 400 + 1000, 0), 'ABCDEFGHIJKLMNOP', 'marquee restarts after the end pause');
  eq(core.fit(t, '', 5000, 4000), 'ABCDEFGHIJKLMNOP', 'marquee time counts from the row start');
}

// ---- engine: status page
function eng() { const e = new core.Engine(core.build(clone(V2))); return e; }
{
  const e = eng();
  eq(e.rows(0), ['F01 L01 Closed  ', '\x7f Customer right'], 'status page default (effects 1 and 1, arrow Left seen from behind)');
  e.physical('casino', 1, 5); e.physical('arrow', 1, 5);
  eq(e.rows(10), ['F01 L01 Open    ', '\x7e Customer left '], 'status page after the switches moved');
  e.set('frameEffect', 12); e.set('lettersEffect', 3);
  eq(e.rows(10)[0], 'F12 L03 Open    ', 'effect numbers use two digits');
  e.set('lettersEffect', 0); e.setRuntime({ activeLetters: 7, changeInSec: 125 });
  eq(e.rows(20)[0], 'F12 L*07 Open   ', 'a random letters effect shows a star and the pattern really showing');
  eq(e.rows(20)[1], '\x7e Customer left ', 'a random effect shows the direction first');
  eq(e.rows(20 + 3000)[1], 'Change in 2:05  ', 'a random effect alternates to the countdown after 3 s');
  eq(e.rows(20 + 6000)[1], '\x7e Customer left ', 'and back after 6 s');
  e.setRuntime({ changeInSec: 61 }); eq(e.rows(20 + 3000)[1], 'Change in 1:01  ', 'countdown seconds padded');
  e.set('lettersEffect', 4); eq(e.rows(40 + 3000)[1], '\x7e Customer left ', 'no countdown while no effect is random');
  e.set('frameEffect', 0); e.setRuntime({ activeFrame: 0 }); eq(e.rows(0)[0], 'F*-- L04 Open   ', 'a random effect with an unknown pattern shows --');
  eq(e.rows(40 + 3000)[1], 'Change in 1:01  ', 'a random frame effect also gives the countdown');
  e.set('lettersEffect', 0); e.setRuntime({ activeFrame: 15, activeLetters: 9 });
  eq(e.rows(0)[0], 'F*15 L*09 Open  ', 'both random: 16 characters with Open'); e.physical('casino', 0, 50);
  eq(e.rows(0)[0], 'F*15 L*09 Closed', 'both random: Closed also fits in 16 characters');
  const a = new core.Engine(core.build(Object.assign(clone(V2), { status: { casino: 'casino', arrow: 'arrow', effectFrame: 'frameEffect', effectLetters: 'lettersEffect', arrowAscii: true } })));
  eq(a.rows(0)[1], '<-Customer right', 'ascii fallback left is 16 chars'); a.physical('arrow', 1, 1); eq(a.rows(1)[1], '->Customer left ', 'ascii fallback right');
  eq(e.rows(0)[0].length, 16, 'row 0 is 16 chars');
}

// ---- engine: navigation, editing, EEPROM batches
{
  const e = eng(); let log = [];
  e.opts.onCallback = (cb, id, v) => log.push(cb + ':' + id + ':' + v);
  e.turn(1, false, 0); eq(e.mode, 'status', 'turning does nothing on the status page');
  e.key('CANCEL', 0); eq(e.mode, 'status', 'CANCEL does nothing on the status page');
  e.key('OK', 1000);
  eq(e.mode, 'menu', 'OK opens the menu'); eq(e.rows(1000), ['Menu         60s', 'Controls       >'], 'menu opens at the root, first item');
  e.turn(-1, false, 1100); eq(e.rows(1100)[1], 'Controls       >', 'list stops at the first item');
  e.turn(10, false, 1200); eq(e.rows(1200)[1], 'Restore defaults', 'list stops at the last item (no wrap)');
  e.turn(-1, false, 1300); eq(e.rows(1300)[1], 'Brightness     >', 'previous item'); for (let k = 0; k < 4; k++) e.turn(-1, false, 1400 + k);
  eq(e.rows(1500)[1], 'Speed          >', 'back to Speed');
  e.key('OK', 2000); eq(e.rows(2000), ['Speed           ', 'Frame speed: 1.0'], 'submenu shows its first property (name and value scroll as one text)');
  eq(e.rows(3399)[1], 'Frame speed: 1.0', 'still at the start during the first pause'); eq(e.rows(3400)[1], 'rame speed: 1.00', 'then one character per 400 ms'); eq(e.rows(5199)[1], 'me speed: 1.000x', 'the last character stays at the right edge during the end pause');
  e.turn(1, false, 2100); eq(e.rows(2100)[1], 'Letters speed: 1', 'second property');
  e.key('CANCEL', 2200); eq(e.rows(2200)[1], 'Speed          >', 'CANCEL returns to the submenu item that was selected');
  e.key('OK', 2300); e.key('OK', 2400);
  eq(e.mode, 'edit', 'OK on a property starts editing'); eq(e.rows(2400), ['Frame speed     ', '> 1.000x        '], 'edit rows');
  e.turn(1, false, 2500); eq(e.text('speedFrame'), '1.122x', 'one step up'); eq(log, ['applySpeeds:speedFrame:' + e.get('speedFrame')], 'callback runs on every change while editing');
  e.turn(1, true, 2600); eq(e.getPos('speedFrame'), 18 + 1 + 10, 'shift multiplies the step by 10');
  e.turn(5, true, 2700); eq(e.getPos('speedFrame'), 30, 'shift step is capped at the range end');
  eq(e.batches.length, 0, 'nothing is written to EEPROM while editing');
  e.key('CANCEL', 2800); eq(e.getPos('speedFrame'), 18, 'CANCEL restores the value from the start of editing'); eq(e.mode, 'menu', 'CANCEL returns to the menu');
  eq(e.batches.length, 0, 'cancel writes nothing'); eq(log.length >= 4, true, 'restoring also calls the callback');
  e.key('OK', 2900); e.turn(-1, false, 2950); e.key('OK', 3000);
  eq(e.mode, 'menu', 'OK accepts and returns'); eq(e.batches, [['speedFrame']], 'OK writes the property to EEPROM'); eq(e.getPos('speedFrame'), 17, 'accepted value kept');
  eq(e.eeprom().slots[e.m.order.filter(i => e.m.props[i].persist && !e.m.props[i].readOnly).indexOf('speedFrame')], 17, 'EEPROM image holds the position');
}
{
  const e = eng();
  e.key('OK', 0); e.key('OK', 10); e.key('OK', 20);        // Controls, Arrow (read-only)
  eq(e.mode, 'view', 'OK on a read-only property opens the view page'); eq(e.rows(20), ['Arrow           ', '\u0001 Left          '], 'view page: name, lock and value');
  e.turn(3, true, 25); eq(e.text('arrow'), 'Left', 'turning does not change a read-only value'); e.key('OK', 30);
  eq(e.mode, 'menu', 'OK on the view page goes back'); eq(e.rows(30)[1], 'Arrow       Left', 'read-only row shows its value');
  e.key('OK', 32); eq(e.mode, 'view', 'opened again'); e.key('CANCEL', 34); eq(e.mode, 'menu', 'CANCEL on the view page goes back');
  e.key('OK', 36); e.physical('arrow', 1, 38); eq(e.mode, 'status', 'a physical change closes the view page'); e.physical('arrow', 0, 39);
  e.key('OK', 39); e.key('OK', 39.5); e.key('OK', 39.6);
  e.physical('casino', 1, 40); eq(e.mode, 'status', 'a slide switch while browsing goes to the status page');
  e.key('OK', 50); eq(e.stack.length, 1, 'menu always opens at the root'); eq(e.rows(50)[1], 'Controls       >', 'at the first item again');
  e.key('OK', 60);
  // edit a reverse flag, then turn the arrow switch: edit cancelled
  e.turn(1, false, 90); eq(e.rows(90)[1], 'Arrow reverse No', 'bool row'); e.key('OK', 100); e.turn(1, false, 110);
  eq(e.text('arrowReverse'), 'Yes', 'edited live'); e.physical('arrow', 1, 120);
  eq(e.text('arrowReverse'), 'No', 'physical switch cancels the edit and restores the value'); eq(e.mode, 'status', 'and shows the status page');
  eq(e.batches, [], 'nothing saved');
  // casino switch also cancels, other physical events do not close the menu
  e.key('OK', 130); e.key('OK', 140); e.physical('casino', 0, 150); eq(e.mode, 'status', 'casino switch goes to the status page');
}

// ---- timeout
{
  const e = eng();
  e.key('OK', 1000);
  e.tick(1000 + 59999); eq(e.mode, 'menu', 'no timeout before 60 s');
  eq(e.rows(1000 + 30000)[0], 'Menu         30s', 'countdown shown at the right edge');
  eq(e.rows(1000 + 59001)[0], 'Menu          1s', 'remaining seconds round up');
  e.turn(1, false, 1000 + 40000); eq(e.rows(1000 + 40000)[0], 'Menu         60s', 'input restarts the countdown');
  e.tick(1000 + 40000 + 60000); eq(e.mode, 'status', 'timeout after 60 s without input');
  const s = eng(); s.key('OK', 0); s.key('OK', 10); s.tick(1e6); eq(s.mode, 'menu', 'no timeout inside a submenu');
  const t = eng(); t.key('OK', 0); t.key('OK', 10); t.key('OK', 20); t.key('OK', 30); t.physical('arrow', 0, 1e6);
  const u = eng(); u.key('OK', 0); u.turn(2, false, 5); u.key('OK', 10); u.key('OK', 20); u.tick(1e6); eq(u.mode, 'edit', 'no timeout while editing');
  const v = eng(); v.key('OK', 0); v.turn(7, false, 5); v.key('OK', 10); eq(v.mode, 'confirm', 'confirm page open'); v.tick(1e6); eq(v.mode, 'confirm', 'no timeout on the confirm page');
}

// ---- presets
{
  const src = mk({
    a: { kind: 'int', min: 0, max: 20, default: 5, callbacks: ['cbA'] }, b: { kind: 'bool', default: false, callbacks: ['cbB1', 'cbB2'] },
    c: { kind: 'choice', choices: ['x', 'y', 'z'], default: 'x' }, ro: { kind: 'int', min: 0, max: 3, readOnly: true, default: 2 },
    n: { kind: 'int', min: 0, max: 9, default: 3, persist: false, callbacks: ['cbN'] }
  }, [
    { preset: 'Calm', set: { a: 14, b: true, c: 'z' } },
    { preset: 'Reset a', set: { a: 'default' } },
    { preset: 'All', setAll: 'default', confirm: true },
    { propertyRef: 'ro' }
  ]);
  const e = new core.Engine(core.build(src)), log = [];
  e.opts.onCallback = (cb, id, v) => log.push(cb + ':' + id + ':' + v);
  e.key('OK', 0); e.key('OK', 1);
  eq(log, ['cbA:a:14', 'cbB1:b:1', 'cbB2:b:1'], 'preset calls each changed property callbacks once, in order'); eq(e.text('c'), 'z', 'choice set by text');
  eq(e.batches, [['a', 'b', 'c']], 'preset writes one EEPROM batch');
  log.length = 0; e.key('OK', 2); eq(log, [], 'applying the same preset again calls nothing'); eq(e.batches.length, 1, 'and writes nothing');
  e.turn(1, false, 3); e.key('OK', 4); eq(log, ['cbA:a:5'], 'default keyword restores the default'); eq(e.batches.length, 2, 'second batch');
  e.turn(1, false, 5); e.key('OK', 6);
  eq(e.mode, 'confirm', 'a confirm preset waits'); eq(e.rows(6), ['All?            ', 'OK=yes CANCEL=no'], 'confirm rows');
  e.key('CANCEL', 7); eq(e.mode, 'menu', 'CANCEL cancels the confirm'); eq(e.getPos('c'), 2, 'nothing applied after cancel');
  e.key('OK', 8); e.key('OK', 9);
  eq(e.mode, 'menu', 'OK on the confirm page applies and returns'); eq(e.text('c'), 'x', 'setAll default restored c'); eq(e.get('b'), 0, 'setAll default restored b');
  eq(e.batches[e.batches.length - 1], ['b', 'c'], 'batch lists only the stored properties that changed');
  eq(e.get('ro'), 2, 'setAll skips read-only properties');
  // a preset that matches the current value runs no callback
  log.length = 0; e.turn(-2, false, 20); e.key('OK', 21); eq(log, ['cbA:a:14', 'cbB1:b:1', 'cbB2:b:1'], 'preset applies again after reset');
  log.length = 0; e.key('OK', 22); eq(log, [], 'applying unchanged values calls no callback');
}

// ---- EEPROM load
{
  const e = eng(), img0 = e.eeprom();
  eq(img0.slots.length, 21, 'image has one slot per stored property'); eq(typeof img0.tag, 'string', 'layout tag');
  eq(img0.tag.length, 8, 'tag is 8 hex digits');
  e.set('stripMax', 100); e.set('speedArrow', 2); const img = e.eeprom();
  const f = eng(); eq(f.load(img), true, 'valid image loads'); eq(f.get('stripMax'), 100, 'value back after restart'); near(f.get('speedArrow'), 2, 1e-9, 'float value back');
  const bad = clone(img); bad.tag = '00000000'; const g = eng(); g.set('stripMax', 100); eq(g.load(bad), false, 'layout mismatch rejected'); eq(g.get('stripMax'), 80, 'defaults after a layout mismatch');
  const bad2 = clone(img); bad2.slots[0] = 9999; const h = eng(); eq(h.load(bad2), false, 'out of range slot rejected'); eq(h.get('stripMax'), 80, 'defaults after invalid data');
  eq(eng().load(null), false, 'empty EEPROM gives defaults');
  const bad3 = clone(img); bad3.slots.pop(); eq(eng().load(bad3), false, 'wrong slot count rejected');
  const k = eng(); k.load(img); eq(k.getPos('arrow'), 0, 'arrow is not part of the image');
  // tag depends on the layout
  const other = clone(V2); delete other.common.properties.speedLetters; other.root.items[2].items.pop();
  check(core.layoutTag(core.build(other)) !== img.tag, 'removing a property changes the layout tag');
  const same = clone(V2); same.common.properties.stripMax.default = 90;
  eq(core.layoutTag(core.build(same)), img.tag, 'changing a default keeps the layout tag');
  const range = clone(V2); range.common.properties.randomDelay.max = 31;
  check(core.layoutTag(core.build(range)) !== img.tag, 'changing the number of positions changes the layout tag');
}

// ---- external set (bound control) follows the same path and shows immediately
{
  const e = eng(), log = [];
  e.opts.onCallback = (cb, id, v) => log.push(cb);
  e.key('OK', 0); e.key('OK', 1); e.turn(1, false, 2);
  e.set('arrowReverse', 1); eq(e.rows(3)[1], 'Arrow reverse: Y', 'a value changed elsewhere shows in the next rows() call');
  eq(log, ['applySwitches'], 'external set calls the callbacks'); eq(e.batches, [], 'external set does not write EEPROM');
}

// ---- custom row text
{
  const src = mk({ a: { kind: 'int', min: 0, max: 5, default: 1 } }, [{ propertyRef: 'a', text: 'Custom name' }, { menu: 'A rather long submenu name', items: [{ propertyRef: 'a' }] }]);
  const e = new core.Engine(core.build(src));
  e.key('OK', 0); eq(e.rows(0)[1], 'Custom name    1', 'custom row text replaces the name');
  e.turn(1, false, 0); eq(e.rows(0)[1], 'A rather long  >', 'long submenu name is cut to fit');
  eq(e.rows(1000 + 400 * 3)[1], 'ather long sub >', 'and scrolls');
}

// ---- extra checks (added after the first mutation run)
{
  const m = core.build(mk({ c: { kind: 'choice', choices: ['a', 'b', 'c'] }, b: { kind: 'bool' }, ro: { kind: 'int', min: 0, max: 3, readOnly: true }, x: { kind: 'int', min: 0, max: 3 } },
    [{ propertyRef: 'c' }, { propertyRef: 'b' }, { propertyRef: 'ro' }, { propertyRef: 'x' }]));
  eq(core.posOf(m.props.c, 99), 2, 'choice posOf clamps high'); eq(core.posOf(m.props.c, -4), 0, 'choice posOf clamps low');
  eq(core.persistIds(m), ['c', 'b', 'x'], 'a read-only property is not stored even when persist is left on');
  check(/min\/max invalid/.test(errs(mk({ f: { kind: 'float', min: 5, max: 5 } }, [{ propertyRef: 'f' }]))), 'float min = max rejected');
  check(/min\/max invalid/.test(errs(mk({ f: { kind: 'float', min: 6, max: 5 } }, [{ propertyRef: 'f' }]))), 'float min > max rejected');
  check(/loop/.test(errs({ format: 1, common: { menus: { A: { items: [{ menuRef: 'B' }] }, B: { items: [{ menuRef: 'A' }] } } }, root: { items: [{ menu: 'x', items: [{ menu: 'y', items: [{ menu: 'z', items: [] }] }] }] } })), 'loop in a menu that is not used from the root is still reported');
  check(/is not a choice of c/.test(errs(mk({ c: { kind: 'choice', choices: ['a', 'b'] } }, [{ preset: 'P', set: { c: 'q' } }]))), 'quick-pick choice that does not exist');
  eq(errs(mk({ c: { kind: 'choice', choices: ['a', 'b'] } }, [{ preset: 'P', set: { c: 1 } }])), '', 'quick-pick choice by index');
  check(/must be true or false/.test(errs(mk({ b: { kind: 'bool' } }, [{ preset: 'P', set: { b: 5 } }]))), 'quick-pick bool needs true/false');
  const r1 = core.build(clone(V2)), ren = clone(V2); ren.common.properties = Object.fromEntries(Object.entries(ren.common.properties).map(([k, v]) => [k === 'speedLetters' ? 'renamed' : k, v])); ren.root.items[1].items[2] = { propertyRef: 'renamed' };
  check(core.layoutTag(core.build(ren)) !== core.layoutTag(r1), 'renaming a property changes the layout tag');
  eq(core.fit('abc', 'x'.repeat(20), 0, 0).length, 16, 'fit never returns more than 16 characters');
  const t = 'ABCDEFGHIJKLMNOPQRSTUV';
  eq(core.fit(t, '', 1000 + 6 * 400 + 500, 0), 'GHIJKLMNOPQRSTUV', 'marquee holds the last window and does not run past the end');
  const e = eng();
  e.key('OK', 0); eq(e.mode, 'menu', 'opened'); e.key('CANCEL', 10); eq(e.mode, 'status', 'CANCEL at the root leaves the menu');
  e.key('OK', 20); e.turn(10, false, 30); e.turn(1, false, 40000); eq(e.rows(40000)[0], 'Menu         60s', 'turning against the end of the list still counts as input');
  const f = eng(), log = [];
  f.opts.onCallback = (cb) => log.push(cb);
  f.key('OK', 0); f.key('OK', 1); f.turn(5, false, 2); f.turn(1, false, 3); f.key('OK', 4);   // Controls > Arrow reverse? idx 5 clamps to the last: Casino reverse
  f.turn(1, false, 5); eq(log.length, 1, 'first step changes the value and calls back'); f.turn(1, false, 6); eq(log.length, 1, 'a step against the limit changes nothing and calls nothing');
  const g = eng(); g.set('frameEffect', 0); g.setRuntime({ changeInSec: null }); eq(g.rows(3000)[1], '\x7f Customer right', 'no countdown data: the direction stays');
  const long = mk({ a: { kind: 'int', min: 0, max: 5 } }, [{ menu: 'A rather long submenu name', items: [{ propertyRef: 'a' }] }]);
  const i = new core.Engine(core.build(long)); i.key('OK', 0); i.key('OK', 5000);
  eq(i.rows(5000)[0], 'A rather long su', 'long title starts at its beginning when the submenu is entered'); eq(i.rows(5000 + 1400)[0], ' rather long sub', 'and scrolls from that moment');
}
{
  const src = mk({ ro: { kind: 'int', min: 0, max: 3, readOnly: true, default: 2 }, a: { kind: 'int', min: 0, max: 5, default: 1 }, n: { kind: 'int', min: 0, max: 9, default: 3, persist: false, callbacks: ['cbN'] } },
    [{ preset: 'All', setAll: 'default' }, { propertyRef: 'ro' }]);
  const e = new core.Engine(core.build(src));
  e.set('ro', 0); e.set('a', 4); e.set('n', 7); e.key('OK', 0); e.key('OK', 1);
  eq(e.get('ro'), 0, 'setAll leaves a read-only property alone'); eq(e.get('a'), 1, 'setAll restores a'); eq(e.get('n'), 3, 'setAll restores a property that is not stored');
  eq(e.batches, [['a']], 'the EEPROM batch leaves out properties that are not stored');
  const img = e.eeprom(); img.slots.push(0); eq(new core.Engine(core.build(src)).load(img), false, 'an image with extra slots is rejected');
  const img2 = e.eeprom(); img2.slots[0] = 1.5; eq(new core.Engine(core.build(src)).load(img2), false, 'a non-integer slot is rejected');
}

// ---- other physical events do not disturb an edit
{
  const e = eng();
  e.key('OK', 0); e.key('OK', 1); e.turn(3, false, 2); e.key('OK', 3); e.turn(1, false, 4);
  e.physical('casino', 1, 5);
  eq(e.mode, 'status', 'a casino switch change closes the menu');
}
{
  const src = mk({ a: { kind: 'int', min: 0, max: 9, default: 1 }, ext: { kind: 'int', min: 0, max: 9, readOnly: true } }, [{ propertyRef: 'a' }, { propertyRef: 'ext' }]);
  const e = new core.Engine(core.build(src));
  e.key('OK', 0); e.key('OK', 1); e.turn(1, false, 2); e.physical('ext', 4, 3);
  eq(e.mode, 'edit', 'a bound control that is not a status property leaves the edit alone'); eq(e.get('a'), 2, 'and the edited value stays'); eq(e.get('ext'), 4, 'while the bound value follows');
}
// ---- scroll keeps going at the list ends and after contact bounce
{
  const e = new core.Engine(core.build(V2));
  e.key('OK', 0); e.turn(2, false, 10); e.key('OK', 20); e.turn(5, false, 30);
  eq(e.rows(30)[1], 'Arrow speed: 1.0', 'last row of Speed starts scrolling');
  e.turn(1, false, 1500); eq(e.rows(1500)[1], 'rrow speed: 1.00', 'turning down at the last item does not restart the scroll');
  e.turn(-1, false, 1600); e.turn(1, false, 1650); eq(e.rows(1700)[1], 'rrow speed: 1.00', 'a move away and straight back within 300 ms keeps the scroll of the item');
  e.turn(-1, false, 2000); e.turn(1, false, 2400); eq(e.rows(2400)[1], 'Arrow speed: 1.0', 'a return after more than 300 ms restarts the scroll');
  e.turn(-1, false, 7000); e.turn(-1, false, 7100); eq(e.rows(7100)[1], 'Frame speed: 1.0', 'two moves in a row are not a bounce: the new item starts from the beginning');
  e.turn(1, false, 7200); e.turn(-1, false, 9000); e.key('OK', 9001); e.key('CANCEL', 9002); e.turn(1, false, 9100); eq(e.rows(9100)[1], 'Letters speed: 1', 'a key press forgets the bounce memory');
}
// ---- effects: one value in several places, live preview, cancel restores
{
  const e = new core.Engine(core.build(V2)), log = [];
  e.opts.onCallback = (cb, id, v) => log.push(cb + ':' + id);
  e.key('OK', 0); e.turn(1, false, 10); eq(e.rows(10)[1], 'Effects        >', 'Effects is the second root item'); e.key('OK', 20);
  eq(e.rows(20), ['Effects         ', 'Frame effect: 1 '], 'first row of Effects');
  e.turn(1, false, 30); eq(e.rows(30)[1], 'Letters effect: ', 'second row of Effects'); e.turn(1, false, 40); eq(e.rows(40)[1], 'Random delay: 2 ', 'third row of Effects');
  e.turn(-1, false, 50); e.key('OK', 60);
  eq(e.rows(60), ['Letters effect  ', '> 1 Breathe     '], 'edit page of an effect: number first, then the name');
  e.turn(2, false, 70); eq(e.rows(70)[1], '> 3 Typewriter  ', 'live value text while turning'); eq(log, ['effectLettersChanged:lettersEffect'], 'a turn calls the callback at once');
  e.key('CANCEL', 80); eq(e.text('lettersEffect'), '1 Breathe', 'CANCEL restores the value from the start of editing'); eq(log.length, 2, 'and calls the callback again, so the effect goes back too'); eq(e.batches, [], 'nothing saved after cancel');
  e.key('OK', 90); e.turn(3, false, 100); e.key('OK', 110); eq(e.text('lettersEffect'), '4 Ignite', 'OK keeps the value'); eq(e.batches.length, 1, 'and saves it once');
  e.key('OK', 130); e.turn(-10, false, 140); eq(e.text('lettersEffect'), 'Random', 'effects stop at 0 = random (no wrap)'); e.turn(40, false, 150); eq(e.text('lettersEffect'), '15 Grey bands', 'and at 15 (no wrap)'); e.key('CANCEL', 160);
  // the same property under the Letters menu
  e.key('CANCEL', 170); e.turn(1, false, 180); eq(e.rows(180)[1], 'Speed          >', 'back at the root'); e.turn(2, false, 190); e.key('OK', 200);
  eq(e.rows(200)[0], 'Letters         ', 'Letters menu'); eq(e.rows(200)[1], 'Letters effect: ', 'its first row is the letters effect'); e.key('OK', 210); eq(e.rows(210)[1], '> 4 Ignite      ', 'same value as under Effects');
  e.key('CANCEL', 220);
  // Frame menu: effect first, then speed
  e.key('CANCEL', 230); e.turn(-1, false, 240); e.key('OK', 250); eq(e.rows(250)[1], 'Frame effect: 1 ', 'Frame menu starts with the frame effect'); e.turn(1, false, 260); eq(e.rows(260)[1], 'Frame speed: 1.0', 'then the frame speed');
  // Restore defaults sets both effects back to 1
  const f = new core.Engine(core.build(V2)); f.set('frameEffect', 9); f.set('lettersEffect', 0);
  f.key('OK', 0); f.turn(7, false, 5); f.key('OK', 10); f.key('OK', 20); eq([f.getPos('frameEffect'), f.getPos('lettersEffect')], [1, 1], 'Restore defaults puts both effects back to 1');
}

// ---- Monochrome: the fourth row of Effects, on by default, stored, restored by Restore defaults
{
  const e = new core.Engine(core.build(V2));
  eq(e.getPos('monochrome'), 1, 'Monochrome is on by default');
  e.key('OK', 0); e.turn(1, false, 10); e.key('OK', 20);                      // Controls .. Effects
  e.turn(3, false, 30); eq(e.rows(30), ['Effects         ', 'Monochrome   Yes'], 'fourth row of Effects: Monochrome, Yes by default');
  e.turn(1, false, 40); eq(e.rows(40)[1], 'Monochrome   Yes', 'Monochrome is the last row of Effects (no wrap)');
  e.key('OK', 50); e.turn(-1, false, 60); eq(e.text('monochrome'), 'No', 'turning changes Monochrome live');
  e.key('CANCEL', 70); eq(e.text('monochrome'), 'Yes', 'CANCEL restores Monochrome'); eq(e.batches, [], 'nothing saved after cancel');
  e.key('OK', 80); e.turn(-1, false, 90); e.key('OK', 100); eq(e.text('monochrome'), 'No', 'OK keeps Monochrome'); eq(e.batches.length, 1, 'and saves it once');
  const f = new core.Engine(core.build(V2)); f.set('monochrome', 0);
  f.key('OK', 0); f.turn(8, false, 5); f.key('OK', 10); f.key('OK', 20); eq(f.getPos('monochrome'), 1, 'Restore defaults turns Monochrome on');
  eq(core.persistIds(core.build(V2)).indexOf('monochrome') >= 0, true, 'Monochrome is a stored property');
}

if (fails) { console.log(fails + ' check(s) failed'); process.exit(1); }
console.log('menu core tests passed');
