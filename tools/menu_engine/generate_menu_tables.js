// Turns the menu JSON (format 1, see tools/menu-editor/README.md) into the PROGMEM tables of the menu engine
// (tools/menu_engine/engine.inc). Uses the same validation and maths as the editor (menu-core.js).
//   node tools/menu_engine/generate_menu_tables.js menu.json        (prints the tables)
'use strict';
const path = require('path');
const core = require(path.join(__dirname, '..', 'menu-editor', 'menu-core.js'));

const NONE16 = 0xFFFF;

function tables(src, sourceName) {
  const m = core.build(src);
  if (!m.ok) throw new Error('menu has errors:\n  ' + m.errors.join('\n  '));
  if (!m.status) throw new Error('the menu needs a status section (casino, arrow, effectFrame, effectLetters)');
  const strings = [], strIndex = {};
  const S = (t) => {
    t = String(t);
    if (!/^[\x20-\x7e]*$/.test(t)) throw new Error('text with a non-ASCII character: ' + JSON.stringify(t));
    if (strIndex[t] === undefined) { strIndex[t] = strings.length; strings.push(t); }
    return strIndex[t];
  };
  const optS = (t) => (t === undefined || t === null || t === '' ? NONE16 : S(t));
  const ids = m.order, propIdx = {};
  ids.forEach((id, i) => { propIdx[id] = i; });
  const slots = {};
  core.persistIds(m).forEach((id, i) => { slots[id] = i; });

  const cbNames = [], cbList = [];
  const P = { kind: [], flags: [], shift: [], dec: [], count: [], defpos: [], min: [], max: [], aux: [], name: [], unit: [], vtFirst: [], vtCount: [], chFirst: [], cbFirst: [], cbCount: [], slot: [] };
  const vtKey = [], vtStr = [], chStr = [];
  ids.forEach((id) => {
    const p = m.props[id];
    const numeric = p.kind === 'int' || p.kind === 'float';
    P.kind.push(p.kind === 'int' ? 0 : p.kind === 'float' ? (p.scale === 'log' ? 2 : 1) : 3);
    P.flags.push((p.readOnly ? 1 : 0) | (p.persist ? 2 : 0) | (p.wrap ? 4 : 0) | (p.format === 'hex' ? 8 : 0));
    if (p.shift > 255 || p.shift !== Math.floor(p.shift)) throw new Error(id + ': shift must be an integer 1..255');
    P.shift.push(p.shift);
    P.dec.push(numeric ? (p.decimals || 0) : 0);
    P.count.push(p.count);
    P.defpos.push(p.defaultPos);
    P.min.push(numeric ? p.min : 0);
    P.max.push(numeric ? p.max : 0);
    P.aux.push(p.kind === 'int' ? p.step : p.kind === 'float' ? p.steps : 0);
    P.name.push(S(p.name));
    P.unit.push(numeric ? optS(p.unit) : NONE16);
    P.vtFirst.push(vtKey.length);
    let n = 0;
    if (numeric && p.valueTexts) Object.keys(p.valueTexts).forEach((k) => { vtKey.push(Number(k)); vtStr.push(S(p.valueTexts[k])); n++; });
    P.vtCount.push(n);
    P.chFirst.push(chStr.length);
    if (p.kind === 'choice') p.choices.forEach((c) => chStr.push(S(c)));
    if (p.kind === 'bool') { chStr.push(S(p.falseText)); chStr.push(S(p.trueText)); }
    P.cbFirst.push(cbList.length);
    p.callbacks.forEach((c) => {
      if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(c)) throw new Error('callback name is not a C identifier: ' + c);
      if (cbNames.indexOf(c) < 0) cbNames.push(c);
      cbList.push(cbNames.indexOf(c));
    });
    P.cbCount.push(p.callbacks.length);
    if (slots[id] !== undefined && p.count > 256) throw new Error(id + ': more than 256 positions cannot be stored in one EEPROM byte');
    P.slot.push(slots[id] === undefined ? 255 : slots[id]);
  });
  if (vtKey.length > 255 || cbList.length > 255 || chStr.length > 65535) throw new Error('too many value texts or callbacks');

  // menus, items, presets
  const menus = [], items = { type: [], a: [], name: [], text: [] };
  const presets = { name: [], text: [], confirm: [], flags: [], first: [], count: [] }, pv = { prop: [], pos: [] };
  const memo = {};
  function addPreset(it) {
    const idx = presets.name.length;
    presets.name.push(S(it.name));
    presets.text.push(optS(it.text));
    presets.confirm.push(optS(it.confirmText));
    presets.flags.push((it.confirm ? 1 : 0) | (it.setAll === 'default' ? 2 : 0));
    presets.first.push(pv.prop.length);
    const set = it.set || {};
    Object.keys(set).forEach((id) => {
      const p = m.props[id], v = set[id];
      pv.prop.push(propIdx[id]);
      pv.pos.push(v === 'default' ? p.defaultPos : (p.kind === 'choice' && typeof v === 'string' ? p.choices.indexOf(v) : (p.kind === 'bool' ? (v ? 1 : 0) : core.posOf(p, v))));
    });
    presets.count.push(Object.keys(set).length);
    return idx;
  }
  function emit(menu) {
    if (menu.ref && memo[menu.ref] !== undefined) return memo[menu.ref];
    const idx = menus.length;
    menus.push({ first: items.type.length, count: menu.items.length });
    if (menu.ref) memo[menu.ref] = idx;
    const base = items.type.length;
    menu.items.forEach((it) => {
      items.type.push(it.type === 'menu' ? 0 : it.type === 'prop' ? 1 : 2);
      items.a.push(0);
      items.name.push(it.type === 'prop' ? NONE16 : S(it.name));
      items.text.push(optS(it.text));
    });
    menu.items.forEach((it, k) => {
      items.a[base + k] = it.type === 'menu' ? emit(it) : it.type === 'prop' ? propIdx[it.id] : addPreset(it);
    });
    return idx;
  }
  const cbNameStr = cbNames.map(S);
  const rootName = S(m.root.name);
  emit(m.root);
  if (items.type.length > 255 || menus.length > 255 || presets.name.length > 255 || pv.prop.length > 255) throw new Error('menu too large for 8-bit indexes');

  const off = []; let o = 0;
  strings.forEach((t) => { off.push(o); o += t.length + 1; });
  if (o > 65535) throw new Error('string table too large');

  const arr = (type, name, vals, fmt) => 'const ' + type + ' ' + name + '[] PROGMEM = {' + (vals.length ? vals.map(fmt || String).join(', ') : '0') + '};\n';
  const f = (x) => { const t = Number(x).toPrecision(9); return (t.indexOf('e') >= 0 ? Number(x).toExponential(8) : t) + 'f'; };
  const hex16 = (x) => (x === NONE16 ? 'MENU_NO_STR16' : String(x));
  let t = '// BEGIN GENERATED MENU TABLES (do not edit: made by tools/menu_engine/generate_menu_tables.js from ' + sourceName + ')\n';
  t += '// ' + ids.length + ' properties, ' + items.type.length + ' items, ' + menus.length + ' menus, ' + presets.name.length + ' presets; EEPROM layout tag ' + core.layoutTag(m) + ', ' + core.persistIds(m).length + ' stored properties\n';
  t += 'constexpr uint8_t MENU_PROP_COUNT = ' + ids.length + ';\n';
  t += 'constexpr uint8_t MENU_STORED_COUNT = ' + core.persistIds(m).length + ';\n';
  t += 'constexpr uint32_t MENU_LAYOUT_TAG = 0x' + core.layoutTag(m).toUpperCase() + 'UL;\n';
  t += 'constexpr uint16_t MENU_NO_STR16 = 0xFFFF;\n';
  ids.forEach((id, i) => { t += 'constexpr uint8_t P_' + id + ' = ' + i + ';\n'; });
  cbNames.forEach((c, i) => { t += 'constexpr uint8_t CB_' + c + ' = ' + i + ';\n'; });
  t += 'constexpr uint8_t MENU_CB_COUNT = ' + cbNames.length + ';\n';
  t += 'constexpr uint8_t MENU_STATUS_CASINO = P_' + m.status.casino + ';\n';
  t += 'constexpr uint8_t MENU_STATUS_ARROW = P_' + m.status.arrow + ';\n';
  t += 'constexpr uint8_t MENU_STATUS_EFFECT_FRAME = P_' + m.status.effectFrame + ';\n';
  t += 'constexpr uint8_t MENU_STATUS_EFFECT_LETTERS = P_' + m.status.effectLetters + ';\n';
  t += 'constexpr bool MENU_STATUS_ARROW_ASCII = ' + (m.status.arrowAscii ? 'true' : 'false') + ';\n';
  t += 'constexpr uint16_t MENU_ROOT_NAME_STR = ' + rootName + ';\n';
  t += '// strings\nconst char MENU_STR[] PROGMEM =\n' + strings.map((s) => '  "' + s.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '\\0"').join('\n') + ';\n';
  t += arr('uint16_t', 'MENU_STR_OFF', off);
  t += '// properties\n';
  t += arr('uint8_t', 'PP_KIND', P.kind) + arr('uint8_t', 'PP_FLAGS', P.flags) + arr('uint8_t', 'PP_SHIFT', P.shift) + arr('uint8_t', 'PP_DEC', P.dec);
  t += arr('uint16_t', 'PP_COUNT', P.count) + arr('uint16_t', 'PP_DEFPOS', P.defpos);
  t += arr('float', 'PP_MIN', P.min, f) + arr('float', 'PP_MAX', P.max, f) + arr('uint16_t', 'PP_AUX', P.aux);
  t += arr('uint16_t', 'PP_NAME', P.name) + arr('uint16_t', 'PP_UNIT', P.unit, hex16);
  t += arr('uint8_t', 'PP_VT_FIRST', P.vtFirst) + arr('uint8_t', 'PP_VT_COUNT', P.vtCount) + arr('uint16_t', 'PP_CH_FIRST', P.chFirst);
  t += arr('uint8_t', 'PP_CB_FIRST', P.cbFirst) + arr('uint8_t', 'PP_CB_COUNT', P.cbCount) + arr('uint8_t', 'PP_SLOT', P.slot);
  t += arr('float', 'VT_KEY', vtKey, f) + arr('uint16_t', 'VT_STR', vtStr) + arr('uint16_t', 'CH_STR', chStr) + arr('uint8_t', 'CB_LIST', cbList);
  t += arr('uint16_t', 'CB_NAME_STR', cbNameStr);
  t += '// menus and items\n';
  t += arr('uint8_t', 'MN_FIRST', menus.map((x) => x.first)) + arr('uint8_t', 'MN_COUNT', menus.map((x) => x.count));
  t += arr('uint8_t', 'IT_TYPE', items.type) + arr('uint8_t', 'IT_A', items.a) + arr('uint16_t', 'IT_NAME', items.name, hex16) + arr('uint16_t', 'IT_TEXT', items.text, hex16);
  t += '// presets\n';
  t += arr('uint16_t', 'PR_NAME', presets.name) + arr('uint16_t', 'PR_TEXT', presets.text, hex16) + arr('uint16_t', 'PR_CONFIRM', presets.confirm, hex16);
  t += arr('uint8_t', 'PR_FLAGS', presets.flags) + arr('uint8_t', 'PR_FIRST', presets.first) + arr('uint8_t', 'PR_COUNT', presets.count);
  t += arr('uint8_t', 'PV_PROP', pv.prop) + arr('uint16_t', 'PV_POS', pv.pos);
  t += '// END GENERATED MENU TABLES\n';
  return t;
}
module.exports = { tables };
if (require.main === module) {
  const file = process.argv[2];
  if (!file) { console.error('usage: node generate_menu_tables.js menu.json'); process.exit(2); }
  process.stdout.write(tables(JSON.parse(require('fs').readFileSync(file, 'utf8')), path.basename(file)));
}
