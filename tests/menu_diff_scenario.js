// Makes a scenario (key and switch events) and the trace the JS menu engine (menu-core.js) gives for it.
//   node tests/menu_diff_scenario.js <menu.json> <seed> <events> <scenario-out> <trace-out>
// Scenario line:  <t> turn <steps> <shift> | key <0 OK|1 BACK> | phys <prop> <pos> | set <prop> <pos> | rt <seconds|-1> | tick
// Trace line:     hex of row 0 and row 1 at t, the same at t+1700, callbacks "cb:prop:pos,...", EEPROM batches "p.p.p,...",
//                 all property positions, mode letter
'use strict';
const fs = require('fs'), path = require('path');
const core = require(process.env.MENU_CORE || path.join(__dirname, '..', 'tools', 'menu-editor', 'menu-core.js'));
const [file, seedArg, nArg, scenOut, traceOut] = process.argv.slice(2);
const model = core.build(JSON.parse(fs.readFileSync(file, 'utf8')));
if (!model.ok) throw new Error(model.errors.join('\n'));
const ids = model.order, st = model.status;
const cbNames = []; ids.forEach(id => model.props[id].callbacks.forEach(c => { if (cbNames.indexOf(c) < 0) cbNames.push(c); }));
let seed = Number(seedArg) >>> 0;
function rnd() { seed = (seed + 0x6D2B79F5) >>> 0; let t = seed; t = Math.imul(t ^ (t >>> 15), t | 1); t ^= t + Math.imul(t ^ (t >>> 7), t | 61); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }
function ri(a, b) { return a + Math.floor(rnd() * (b - a + 1)); }
const hex = s => Array.from(s).map(c => ('0' + c.charCodeAt(0).toString(16)).slice(-2)).join('');
const e = new core.Engine(model);
const scen = [], trace = [];
let t = 1000, cbSeen = 0, batchSeen = 0;
const statusIdx = [st.program, st.casino, st.arrow].map(id => ids.indexOf(id));
function emit(line, apply) {
  scen.push(t + ' ' + line);
  e.tick(t);
  apply();
  const r = e.rows(t), r2 = e.rows(t + 1700);
  const cbs = e.callLog.slice(cbSeen).map(c => cbNames.indexOf(c.callback) + ':' + ids.indexOf(c.id) + ':' + c.pos).join(',') || '-';
  const bs = e.batches.slice(batchSeen).map(b => b.map(id => ids.indexOf(id)).join('.')).join(',') || '-';
  cbSeen = e.callLog.length; batchSeen = e.batches.length;
  trace.push([hex(r[0]), hex(r[1]), hex(r2[0]), hex(r2[1]), cbs, bs, ids.map(id => e.getPos(id)).join('.'), e.mode.charAt(0)].join(' '));
}
const N = Number(nArg);
for (let i = 0; i < N; i++) {
  const w = rnd();
  t += rnd() < 0.02 ? ri(20000, 70000) : (rnd() < 0.5 ? ri(1, 120) : ri(100, 4000));
  if (w < 0.34) { const steps = ri(-3, 3) || 1, shift = rnd() < 0.3 ? 1 : 0; emit('turn ' + steps + ' ' + shift, () => e.turn(steps, !!shift, t)); }
  else if (w < 0.62) { const k = rnd() < 0.7 ? 0 : 1; emit('key ' + k, () => e.key(k ? 'BACK' : 'OK', t)); }
  else if (w < 0.70) {
    const which = ri(0, 2), id = [st.program, st.casino, st.arrow][which], p = model.props[id], pos = ri(0, p.count - 1);
    emit('phys ' + statusIdx[which] + ' ' + pos, () => e.physical(id, core.valueAt(p, pos), t));
  } else if (w < 0.76) {
    const cand = ids.filter(id => !model.props[id].readOnly), id = cand[ri(0, cand.length - 1)], p = model.props[id], pos = ri(0, p.count - 1);
    emit('set ' + ids.indexOf(id) + ' ' + pos, () => e.set(id, core.valueAt(p, pos)));
  } else if (w < 0.82) { const secs = rnd() < 0.2 ? -1 : ri(0, 1800); emit('rt ' + secs, () => e.setRuntime({ changeInSec: secs < 0 ? null : secs })); }
  else emit('tick', () => {});
}
fs.writeFileSync(scenOut, scen.join('\n') + '\n');
fs.writeFileSync(traceOut, trace.join('\n') + '\n');
