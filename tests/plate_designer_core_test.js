// Tests of tools/plate-designer/plate-core.js (pure logic). Expected values are hard-coded.
//   node tests/plate_designer_core_test.js
'use strict';
const path = require('path');
const core = require(process.env.PLATE_CORE || path.join(__dirname, '..', 'tools', 'plate-designer', 'plate-core.js'));
const ARROW = require(path.join(__dirname, '..', 'tools', 'plate-designer', 'arrow-data.js'));
let fails = 0;
function check(cond, msg) { if (!cond) { console.log('FAIL: ' + msg); fails++; } }
function near(a, b, tol, msg) { check(Math.abs(a - b) <= tol, msg + ' (got ' + a + ', want ' + b + ' +-' + tol + ')'); }
function rect(x0, y0, x1, y1, rev) { const p = [[x0, y0], [x1, y0], [x1, y1], [x0, y1]]; return rev ? p.reverse() : p; }
function bbox(polys) { let a = 1e9, b = 1e9, c = -1e9, d = -1e9; polys.forEach(p => p.forEach(q => { a = Math.min(a, q[0]); b = Math.min(b, q[1]); c = Math.max(c, q[0]); d = Math.max(d, q[1]); })); return { x0: a, y0: b, x1: c, y1: d }; }

// ---- distance transform and signed field
{
  const w = 7, h = 1, src = new Uint8Array([1, 0, 0, 0, 0, 0, 1]);
  const d = core.distanceTo(src, w, h);
  check(Array.from(d).join() === '0,1,2,3,2,1,0', 'distanceTo on a line: ' + Array.from(d).join());
  const m = new Uint8Array(100); for (let y = 3; y < 7; y++) for (let x = 3; x < 7; x++) m[y * 10 + x] = 1;
  const s = core.signedField(m, 10, 10);
  near(s[3 * 10 + 3], 0.5, 1e-6, 'edge pixel of a square has field 0.5');
  near(s[4 * 10 + 4], 1.5, 1e-6, 'inner pixel of a square has field 1.5');
  near(s[0], -(Math.hypot(3, 3) - 0.5), 1e-4, 'outside pixel field is minus the distance');
}
// ---- rasterize (nonzero winding, hole by opposite orientation)
{
  const m = core.rasterize([rect(2, 2, 8, 8, false), rect(4, 4, 6, 6, true)], 10, 10);
  let n = 0; m.forEach(v => n += v);
  check(n === 36 - 4, 'rasterized square with a square hole has 32 pixels, got ' + n);
  check(m[5 * 10 + 5] === 0 && m[3 * 10 + 3] === 1, 'hole is empty, ring is filled');
}
{
  const m = core.rasterize([rect(2.5, 2.5, 7.5, 7.5, false)], 10, 10); let n = 0; m.forEach(v => n += v);
  check(n === 25 && m[2 * 10 + 2] === 1 && m[2 * 10 + 7] === 0 && m[7 * 10 + 2] === 0 && m[6 * 10 + 6] === 1, 'pixel centres decide: a 5 x 5 square at half-pixel positions fills pixels 2..6, got ' + n);
}
// ---- glyph from a 20 x 100 mm bar (the letter I)
const bar = core.glyphFromPolygons([rect(0, -100, 20, 0, false)], { ppm: 4, padMm: 8 });
{
  near(bar.areaMm2(), 2000, 5, 'bar area 20 x 100');
  const ink = bar.ink();
  near(ink.x0, 0, 0.01, 'ink x0'); near(ink.x1, 20, 0.01, 'ink x1'); near(ink.y0, -100, 0.01, 'ink y0'); near(ink.y1, 0, 0.01, 'ink y1');
  near(bar.insideDist(10, -50, 0), 9.875, 0.01, 'centre of the bar: 10 mm from the edge minus half a pixel (0.125 mm) of sampling');
  near(bar.insideDist(10, -50, 2), 11.875, 0.01, 'grown by 2 mm: 11.875 mm');
  near(bar.insideDist(-3, -50, 0), -3, 0.01, 'outside point is minus its distance to the edge');
  near(bar.inscribed(2).r, 11.875, 0.01, 'inscribed radius grows with the offset');
  check(bar.insideDist((bar.w - 0.5 - bar.ox) / bar.ppm + 0.5 / bar.ppm, -50, 0) === -1000, 'outside the sampled area insideDist is -1000');
  const ins = bar.inscribed(0);
  near(ins.r, 9.875, 0.01, 'largest circle in the bar has r = 9.875 (10 minus half a pixel)'); near(ins.x, 9.875, 0.2, 'inscribed centre x');
  const c0 = core.marching(bar.field(), bar.w, bar.h, 0);
  check(c0.length === 1, 'one contour for a bar');
  const b0 = bbox(bar.contours(0)); near(b0.x0, 0, 0.02, 'contour x0'); near(b0.x1, 20, 0.02, 'contour x1'); near(b0.y0, -100, 0.02, 'contour y0'); near(b0.y1, 0, 0.02, 'contour y1');
  const b2 = bbox(bar.contours(2)); near(b2.x0, -2, 0.02, 'grown contour x0'); near(b2.x1, 22, 0.02, 'grown contour x1'); near(b2.y0, -102, 0.02, 'grown contour y0'); near(b2.y1, 2, 0.02, 'grown contour y1');
  const bm = bbox(bar.contours(-2)); near(bm.x0, 2, 0.02, 'shrunk contour x0'); near(bm.x1, 18, 0.02, 'shrunk contour x1');
  check(bar.contours(0)[0].length <= 12, 'a bar outline is simplified to few points, got ' + bar.contours(0)[0].length);
}
// ---- ring (the letter O): outer 80 x 100, inner 40 x 60, so strokes are 20 mm wide
const ring = core.glyphFromPolygons([rect(0, -100, 80, 0, false), rect(20, -80, 60, -20, true)], { ppm: 4, padMm: 8 });
{
  check(ring.contours(0).length === 2, 'ring has two contours (outer and inner)');
  near(ring.areaMm2(), 8000 - 2400, 8, 'ring area');
  check(ring.insideDist(40, -50, 0) < 0, 'ring centre is empty');
  const best = ring.bestHole(40, -50, 5, 0, 0.5);
  check(best.fits, 'a 10 mm hole fits in the ring');
  near(Math.abs(best.x - 40), 25.5, 1, 'best hole is 25.5 mm left or right of the centre'); near(best.y, -50, 1, 'best hole y');
  const none = ring.bestHole(40, -50, 15, 0, 0.5);
  check(!none.fits, 'a 30 mm hole does not fit'); near(none.r, 11.72, 0.3, 'fallback is the thickest point: the corner of the ring, r = 20*sqrt2/(1+sqrt2) = 11.72');
  check(!bar.bestHole(10, -50, 9.5, 0, 0.5).fits && bar.bestHole(10, -50, 9.5, 0, 0).fits, 'the safety margin counts: r 9.5 + 0.5 does not fit a 20 mm bar, r 9.5 + 0 does');
  const nearer = ring.bestHole(5, -50, 5, 0, 0.5); check(nearer.x < 40, 'the nearest fitting point to a point left of the centre is on the left, got ' + nearer.x);
  const grown = ring.bestHole(40, -50, 15, 6, 0.5);
  check(grown.fits, 'with 6 mm thickening a 30 mm hole fits');
}
// ---- thin parts
{
  const thin = core.glyphFromPolygons([rect(0, -100, 1.5, 0, false)], { ppm: 4, padMm: 8 });
  near(thin.thinAreaMm2(3, 0), 150, 12, 'a 1.5 mm line is all thin for min stroke 3 mm');
  near(thin.thinAreaMm2(3, 2), 0, 3, 'grown by 2 mm (5.5 mm wide) it is no longer thin');
  check(bar.thinAreaMm2(3, 0) < 2, 'a 20 mm bar is not thin, got ' + bar.thinAreaMm2(3, 0));
  const spur = core.glyphFromPolygons([rect(0, -100, 20, 0, false), rect(20, -60, 60, -58.5, false)], { ppm: 4, padMm: 8 });
  near(spur.thinAreaMm2(3, 0), 61, 0.5, 'a 1.5 x 40 mm spur on a thick bar is found as thin (60 mm2)');
  near(spur.thinAreaMm2(3, 2), 0, 2, 'grown by 2 mm the spur is thick enough');
}
// ---- text layout
function fakeGlyph(wmm) { return core.glyphFromPolygons([rect(0, -100, wmm, 0, false)], { ppm: 2, padMm: 4, advanceMm: wmm + 10 }); }
{
  const gl = [fakeGlyph(20), fakeGlyph(60), null, fakeGlyph(40)];
  const def = { x: 50, y: 30, h: 100, mode: 'uniform', cellW: 90, spacing: 6, holeOffsets: [{ dx: 3, dy: -4 }] };
  const t = core.placeText(def, gl, true);
  check(t.cells.length === 4 && t.w === 360 && t.h === 100 && t.x === 50, 'uniform box: 4 cells of 90 mm = 360 mm wide');
  check(t.cells[1].x === 140 && t.cells[3].x === 320, 'cell x positions 140 and 320');
  near(t.cells[0].cx, 95, 1e-9, 'cell 0 centre x'); near(t.cells[0].cy, 80, 1e-9, 'cell 0 centre y');
  near(t.cells[0].hole.x, 98, 1e-9, 'hole 0 has its offset dx=3'); near(t.cells[0].hole.y, 76, 1e-9, 'hole 0 has its offset dy=-4');
  near(t.cells[1].hole.x, 185, 1e-9, 'hole 1 is at the cell centre');
  check(t.cells[2].hole === undefined && t.cells[2].glyph === null, 'a space has no hole');
  near(t.cells[1].gox, 140 + 45 - 30, 1.2, 'glyph 1 is centred by its ink in the cell');
  near(t.cells[1].goy, 130, 1e-9, 'baseline is at the bottom of the box');
  const nat = core.placeText({ x: 0, y: 0, h: 100, mode: 'natural', cellW: 90, spacing: 6, holeOffsets: [] }, [fakeGlyph(20), fakeGlyph(60)], true);
  near(nat.cells[0].w, 36, 1e-9, 'natural cell width = advance 30 + spacing 6'); near(nat.cells[1].x, 36, 1e-9, 'second cell starts after the first');
  near(nat.w, 36 + 76, 1e-9, 'natural total width'); near(nat.cells[0].gox, 3, 1e-9, 'glyph starts half a spacing in');
  check(core.placeText({ x: 0, y: 0, h: 50, mode: 'uniform', cellW: 40, spacing: 0 }, [fakeGlyph(20)], false).cells[0].hole === undefined, 'no holes when withHoles is false');
  // hole status and quick buttons
  const c = t.cells[1];
  check(core.holeStatus(c, 5, 0).covered, 'hole at the centre of a 60 mm bar is covered');
  near(core.holeStatus(c, 5, 0).clearance, 25, 1.2, 'clearance 30 - 5');
  check(core.holeStatus({ glyph: bar, hole: { x: 10, y: -50 }, gox: 0, goy: 0 }, 9.8, 0).covered && !core.holeStatus({ glyph: bar, hole: { x: 10, y: -50 }, gox: 0, goy: 0 }, 9.95, 0).covered, 'covered means the circle fits inside the letter (bar centre 9.875 mm from the edge)');
  check(core.holeOffsetFor({ glyph: bar, hole: { x: 10, y: -50 }, gox: 0, goy: 0, cx: 10, cy: -50 }, 9.5, 0, 'fit').fits === false, 'fit uses a 0.5 mm default margin');
  const shifted = core.placeText(Object.assign({}, def, { holeOffsets: [{ dx: 0, dy: 0 }, { dx: 40, dy: 0 }] }), gl, true).cells[1];
  check(!core.holeStatus(shifted, 5, 0).covered, 'a hole moved outside the letter is not covered');
  const off = core.holeOffsetFor(shifted, 5, 0, 'fit');
  near(off.dx, 0, 1, 'fit puts the hole back to the middle of the bar (centre already fits)');
  check(core.holeOffsetFor(c, 5, 0, 'centre').dx === 0, 'centre mode gives no offset');
}
// extra holes: several holes in one letter
{
  const gl2 = [fakeGlyph(20), fakeGlyph(60)];
  const def2 = { x: 0, y: 0, h: 100, mode: 'uniform', cellW: 90, spacing: 0, holeOffsets: [{ dx: 0, dy: -35 }], extraHoles: [{ i: 0, dx: 5, dy: 10 }, { i: 0, dx: 1, dy: 2 }, { i: 1, dx: 9, dy: 9 }] };
  const t2 = core.placeText(def2, gl2, true);
  check(t2.cells[0].extras.length === 2 && t2.cells[1].extras.length === 1, 'extra holes are grouped by letter: 2 in letter 0, 1 in letter 1');
  check(t2.cells[0].extras[0].x === 50 && t2.cells[0].extras[0].y === 60 && t2.cells[0].extras[0].k === 0, 'extra hole 0 = cell centre (45, 50) + (5, 10), list index 0');
  check(t2.cells[0].extras[1].x === 46 && t2.cells[0].extras[1].y === 52 && t2.cells[0].extras[1].k === 1, 'extra hole 1 = (46, 52), list index 1');
  check(t2.cells[1].extras[0].x === 144 && t2.cells[1].extras[0].y === 59 && t2.cells[1].extras[0].k === 2, 'extra hole of letter 1 = (135 + 9, 50 + 9), list index 2');
  check(core.placeText({ x: 0, y: 0, h: 100, mode: 'uniform', cellW: 90, spacing: 0, holeOffsets: [] }, gl2, true).cells[0].extras.length === 0, 'no extraHoles in the definition: empty list');
  check(core.placeText(def2, [null, fakeGlyph(60)], true).cells[0].extras === undefined, 'a space has no extra holes');
  check(core.holeStatus(t2.cells[0], 4, 0, { x: 45, y: 50 }).covered && !core.holeStatus(t2.cells[0], 4, 0, { x: 80, y: 50 }).covered, 'holeStatus can check a given extra hole');
  check(core.holeStatus(t2.cells[0], 4, 0, t2.cells[0].extras[0]).covered === core.holeStatus(t2.cells[0], 4, 0, { x: 50, y: 60 }).covered, 'extra hole object works as the hole argument');
  const st = core.defaultState(); st.export.plateOutline = false; st.export.arrow = false; st.export.closedHole = 'none';
  const it = core.drillItems(st, { casino: t2, open: null, closedRound: { x: 0, y: 0 }, closedSlot: { x: 0, y: 0, w: 1, h: 1 }, arrow: null });
  check(it.length === 5 && it.every(function (q) { return q.kind === 'circle' && q.d === 10; }), '2 main holes + 3 extra holes = 5 drill circles, got ' + it.length);
  check(it.some(function (q) { return q.x === 144 && q.y === 59; }) && it.some(function (q) { return q.x === 46 && q.y === 52; }), 'extra holes are in the drill items');
  // suggestion
  const cellB = core.placeText({ x: 0, y: 0, h: 100, mode: 'uniform', cellW: 90, spacing: 0, holeOffsets: [{ dx: 0, dy: -35 }] }, [fakeGlyph(20)], true).cells[0];
  const sg = core.suggestHole(cellB, 4, 0, [cellB.hole]);
  check(sg.covered === true && Math.abs(sg.dy - 45) <= 1.5 && Math.abs(sg.dx) <= 6, 'second hole goes to the far end of the bar: dy about +45, got ' + JSON.stringify(sg));
  check(core.holeStatus(cellB, 4, 0, { x: cellB.cx + sg.dx, y: cellB.cy + sg.dy }).covered, 'the suggested hole is covered by the letter');
  const sg2 = core.suggestHole(cellB, 15, 0, [cellB.hole]);
  check(sg2.covered === false && sg2.dx === 0 && sg2.dy === 0, 'no room (hole larger than the bar): centre, not covered');
  check(core.suggestHole({ glyph: null }, 4, 0, []).covered === false, 'no glyph: not covered');
  const sg3 = core.suggestHole(cellB, 4, 0, [cellB.hole, { x: cellB.cx + sg.dx, y: cellB.cy + sg.dy }]);
  check(Math.abs(sg3.dy - (-5)) <= 12, 'third hole goes into the gap between the two: dy near the middle, got ' + sg3.dy);
  const sc2 = core.scaledText({ h: 100, cellW: 90, spacing: 6, holeOffsets: [], extraHoles: [{ i: 1, dx: 10, dy: -4 }] }, 2);
  check(sc2.extraHoles.length === 1 && sc2.extraHoles[0].i === 1 && sc2.extraHoles[0].dx === 20 && sc2.extraHoles[0].dy === -8, 'scaling a text scales the extra holes (i unchanged)');
  check(core.scaledText({ h: 100, cellW: 90, spacing: 6, holeOffsets: [] }, 2).extraHoles.length === 0, 'scaledText without extraHoles gives an empty list');
  check(core.defaultState().texts.casino.extraHoles.length === 0 && core.defaultState().texts.open.extraHoles.length === 0 && core.defaultState().texts.closed.extraHoles.length === 0, 'default: no extra holes');
  const fj2 = core.fromJson({ version: 1, texts: { casino: { extraHoles: [{ i: 25, dx: 999, dy: -999 }, null, { i: 'x' }, { i: 2.4, dx: 1.5, dy: 2 }] } } }).texts.casino.extraHoles;
  check(fj2.length === 2 && fj2[0].i === 19 && fj2[0].dx === 300 && fj2[0].dy === -300 && fj2[1].i === 2 && fj2[1].dx === 1.5, 'fromJson clamps extra holes and drops invalid entries, got ' + JSON.stringify(fj2));
  const many = []; for (let q = 0; q < 60; q++) many.push({ i: 0, dx: 0, dy: 0 });
  check(core.fromJson({ version: 1, texts: { open: { extraHoles: many } } }).texts.open.extraHoles.length === 40, 'at most 40 extra holes');
  check(core.fromJson({ version: 1 }).texts.casino.extraHoles.length === 0 && JSON.stringify(core.fromJson(core.toJson(core.fromJson({ version: 1, texts: { casino: { extraHoles: [{ i: 1, dx: 2, dy: 3 }] } } }))).texts.casino.extraHoles) === '[{"i":1,"dx":2,"dy":3}]', 'extra holes round-trip through JSON');
}
// ---- frame strips, dimensions, snapping
{
  const f = core.frameCounts(980, 300, 'topbottom', 100, 10);
  check(f.top.n === 10 && f.top.left === 0 && f.left.n === 3 && f.left.left === 0 && f.totalUnits === 26 && f.totalLeds === 78, 'frame 980 x 300, top/bottom over the corners: 26 units, 78 LEDs');
  const g = core.frameCounts(980, 300, 'sides', 100, 10);
  check(g.top.n === 9 && Math.round(g.top.left) === 80 && g.left.n === 3 && Math.round(g.left.left) === 20 && g.totalUnits === 24, 'frame with sides over the corners: 24 units, leftovers 80 and 20');
  const h = core.frameCounts(599.9, 300, 'sides', 100, 10);
  check(h.top.n === 5 && Math.round(h.top.left) === 100, 'just under a multiple counts one unit less (599.9 mm: 5 units, 99.9 mm left over)');
  const fi = core.frameCounts(980, 300, 'topbottom', 100, 10, true);
  check(fi.top.len === 980 && fi.top.n === 9 && fi.left.len === 280 && fi.left.n === 2 && fi.totalUnits === 22 && fi.totalLeds === 66, 'inside, top and bottom over the corners: 980 / 280 mm, 9+9+2+2 units, got ' + fi.totalUnits);
  const fj = core.frameCounts(980, 300, 'sides', 100, 10, true);
  check(fj.top.len === 960 && fj.top.n === 9 && fj.left.len === 300 && fj.left.n === 3 && fj.totalUnits === 24, 'inside, sides over the corners: 960 / 300 mm, 9+9+3+3 units, got ' + fj.totalUnits);
  check(core.frameCounts(980, 300, 'topbottom', 100, 10, false).totalUnits === 26 && core.frameCounts(980, 300, 'topbottom', 100, 10).totalUnits === 26, 'outside is the default');
  const jo = JSON.stringify(core.frameSides(980, 300, 'topbottom', 10, false)), ji = JSON.stringify(core.frameSides(980, 300, 'topbottom', 10, true));
  check(jo === JSON.stringify([{ x: -10, y: -10, len: 1000, horiz: true }, { x: -10, y: 300, len: 1000, horiz: true }, { x: -10, y: 0, len: 300, horiz: false }, { x: 980, y: 0, len: 300, horiz: false }]), 'outside strip positions (top and bottom layout): ' + jo);
  check(ji === JSON.stringify([{ x: 0, y: 0, len: 980, horiz: true }, { x: 0, y: 290, len: 980, horiz: true }, { x: 0, y: 10, len: 280, horiz: false }, { x: 970, y: 10, len: 280, horiz: false }]), 'inside strip positions (top and bottom layout): ' + ji);
  const si = JSON.stringify(core.frameSides(980, 300, 'sides', 10, true)), so = JSON.stringify(core.frameSides(980, 300, 'sides', 10, false));
  check(si === JSON.stringify([{ x: 10, y: 0, len: 960, horiz: true }, { x: 10, y: 290, len: 960, horiz: true }, { x: 0, y: 0, len: 300, horiz: false }, { x: 970, y: 0, len: 300, horiz: false }]), 'inside strip positions (sides layout): ' + si);
  check(so === JSON.stringify([{ x: 0, y: -10, len: 980, horiz: true }, { x: 0, y: 300, len: 980, horiz: true }, { x: -10, y: -10, len: 320, horiz: false }, { x: 980, y: -10, len: 320, horiz: false }]), 'outside strip positions (sides layout): ' + so);
  check(core.frameSides(15, 12, 'topbottom', 10, true)[2].len === 0, 'a plate too small for the strips gives length 0, not negative');
  check(core.defaultState().frame.inside === false && core.fromJson({ version: 1, frame: { inside: true } }).frame.inside === true && core.fromJson({ version: 1, frame: {} }).frame.inside === false, 'frame.inside option: default false');
  check(core.defaultState().export.plateCut === false && core.fromJson({ version: 1, export: { plateCut: true } }).export.plateCut === true && core.fromJson({ version: 1, export: {} }).export.plateCut === false, 'export.plateCut option: default false');
  check(core.frameCounts(600, 300, 'sides', 100, 10).top.n === 6, 'exactly 600 mm holds 6 units');
  check(core.frameCounts(0.3, 0.3, 'sides', 0.1, 0).top.n === 3, 'floating point error does not lose a unit (0.3 / 0.1)');
  const d = core.dimsForPoint(100, 40, 980, 300);
  check(d.left === 100 && d.right === 880 && d.up === 40 && d.down === 260, 'dimension distances to the plate edges');
  const pts = core.boxPoints({ x: 10, y: 20, w: 100, h: 50 });
  check(pts.length === 5 && pts[0].x === 60 && pts[0].y === 45 && pts[4].x === 110 && pts[4].y === 70, 'box points: centre and corners');
  const sn = core.snapCentres({ x: 0, y: 0, w: 100, h: 50 }, [{ cx: 52, cy: 90 }, { cx: 300, cy: 27 }], 3);
  check(sn.dx === 2 && sn.vertical === 52 && sn.dy === 2 && sn.horizontal === 27, 'snaps to the centre lines within 3 mm');
  const sn2 = core.snapCentres({ x: 0, y: 0, w: 100, h: 50 }, [{ cx: 60, cy: 90 }], 3);
  check(sn2.dx === 0 && sn2.dy === 0 && sn2.vertical === null, 'no snap beyond the threshold');
  check(core.snapCentres({ x: 0, y: 0, w: 100, h: 50 }, [{ cx: 300, cy: 28 }], 3).dy === 3, 'vertical threshold boundary snaps too');
  const nn = core.snapCentres({ x: 0, y: 0, w: 100, h: 50 }, [{ cx: 52, cy: 27 }, { cx: 51, cy: 26 }], 3); check(nn.dx === 1 && nn.dy === 1, 'the nearest centre line wins');
  check(core.snapCentres({ x: 0, y: 0, w: 100, h: 50 }, [{ cx: 53, cy: 200 }], 3).dx === 3, 'a centre line exactly at the threshold still snaps');
}
// ---- scaling with handles
{
  // box 200 x 100 centred at (300, 200)
  near(core.handleScale(1, 0, 300, 200, 200, 100, 460, 999), 1.6, 1e-9, 'right side handle: half width 100 -> 160 mm = 1.6');
  near(core.handleScale(-1, 0, 300, 200, 200, 100, 150, 0), 1.5, 1e-9, 'left side handle uses the distance from the centre');
  near(core.handleScale(0, 1, 300, 200, 200, 100, 0, 275), 1.5, 1e-9, 'bottom handle: half height 50 -> 75 = 1.5');
  near(core.handleScale(0, -1, 300, 200, 200, 100, 0, 100), 2, 1e-9, 'top handle');
  near(core.handleScale(1, 1, 300, 200, 200, 100, 300 + 200, 200 + 100), 2, 1e-9, 'corner handle: distance 223.6 / half diagonal 111.8 = 2');
  const sn = core.snapHandleScale(1.6, 1, 0, 300, 200, 200, 100, [100, 455, 600], [], 6);
  near(sn.s, 1.55, 1e-9, 'right handle at x 460 snaps to the line x 455 (scale 1.55)'); check(sn.vertical === 455 && sn.horizontal === null, 'guide line reported');
  check(core.snapHandleScale(1.6, 1, 0, 300, 200, 200, 100, [100, 466.5], [], 6).s === 1.6 && core.snapHandleScale(1.6, 1, 0, 300, 200, 200, 100, [466.5], [], 6).vertical === null, 'beyond the threshold nothing snaps');
  check(core.snapHandleScale(1.6, 1, 0, 300, 200, 200, 100, [466], [], 6).s === 1.66, 'exactly at the threshold still snaps');
  const sy = core.snapHandleScale(1.5, 0, -1, 300, 200, 200, 100, [], [120, 126, 500], 6);
  near(sy.s, 1.48, 1e-9, 'top handle (y 125) snaps to the nearest horizontal line, y 126 (scale 1.48)'); check(sy.horizontal === 126 && sy.vertical === null, 'horizontal guide reported');
  check(core.snapHandleScale(1.5, 0, 1, 300, 200, 200, 100, [999], [], 6).vertical === null, 'a bottom handle ignores vertical lines');
  const nr = core.snapHandleScale(1.5, 1, 0, 300, 200, 200, 100, [300, 599], [], 400); check(nr.vertical === 599, 'the nearest line wins, and lines on the wrong side of the centre are ignored (x 300 would give scale 0)');
  const cs = core.snapHandleScale(1.5, 1, 1, 300, 200, 200, 100, [452], [276], 6); check(cs.horizontal === 276 && cs.vertical === null && Math.abs(cs.s - 1.52) < 1e-9, 'a corner handle may snap to a horizontal line (nearer than the vertical one)');
  const sl = core.snapHandleScale(1.5, -1, 0, 300, 200, 200, 100, [148, 153, 600], [], 6); near(sl.s, 1.52, 1e-9, 'left handle (x 150) snaps to the nearest line x 148 (scale 1.52)'); check(sl.vertical === 148, 'left guide line');
  const wrong = core.snapHandleScale(1.5, 1, 0, 300, 200, 200, 100, [100, 900], [], 1000); check(wrong.vertical === 900 && wrong.s === 6, 'a line on the other side of the centre cannot be used (scale would be negative)');
  const t0 = { h: 100, cellW: 90, spacing: 6, holeOffsets: [{ dx: 2, dy: -4 }, { dx: 0, dy: 1.5 }] };
  const sc = core.scaledText(t0, 0.6); check(sc.h === 60 && sc.cellW === 54 && sc.spacing === 3.6 && sc.holeOffsets[0].dx === 1.2 && sc.holeOffsets[0].dy === -2.4 && sc.holeOffsets[1].dy === 0.9, 'uniform scaling of height, cell width, gap and hole offsets');
  check(core.scaledText({ h: 33.3333, cellW: 10, spacing: 0, holeOffsets: [] }, 1 / 3).h === 11.111, 'rounded to 3 decimals');
  const lm = core.scaleLimits(t0); near(lm.min, 5 / 90, 1e-12, 'smallest scale keeps the cell width at 5 mm'); near(lm.max, 6, 1e-12, 'largest scale keeps the height at 600 mm'); near(core.scaleLimits({ h: 100, cellW: 90, spacing: 40 }).max, 2.5, 1e-12, 'largest scale keeps the gap at 100 mm');
  const l2 = core.scaleLimits({ h: 100, cellW: 400, spacing: -10 }); near(l2.max, 1.5, 1e-12, 'cell width 600 limit'); near(l2.min, 0.05, 1e-12, 'height 5 limit');
  near(core.scaleLimits({ h: 100, cellW: 50, spacing: -20 }).max, 2.5, 1e-12, 'negative gap limit -50');
}
// ---- hole dimensions for the PDF
{
  const dm = core.holeDimItems([{ kind: 'outline', w: 100, h: 50 }, { kind: 'circle', x: 15, y: 20, d: 10 }], { w: 100, h: 50 });
  const lines = dm.filter(i => i.kind === 'line'), txt = dm.filter(i => i.kind === 'text');
  check(dm.length === 8 && lines.length === 4 && txt.length === 4, 'a hole gets 4 lines and 4 numbers, the outline none, got ' + dm.length);
  const byS = {}; txt.forEach(t => byS[t.s] = t);
  check(Object.keys(byS).sort().join() === '15,20,30,85', 'distances: left 15, right 85, up 20, down 30 (' + Object.keys(byS).sort().join() + ')');
  near(byS['15'].x, 15 - 5 - 1 - 1.0583, 0.01, 'left number sits left of the hole'); near(byS['15'].y, 18.8, 1e-9, 'left number height');
  near(byS['85'].x, 15 + 5 + 1 + 1.0583, 0.01, 'right number sits right of the hole');
  check(byS['20'].x === 15 && byS['20'].y === 20 - 5 - 1.5, 'upper number above the hole'); check(byS['30'].x === 15 && byS['30'].y === 20 + 5 + 3.5, 'lower number below the hole');
  const l = lines.map(q => [q.x1, q.y1, q.x2, q.y2].join()).sort().join('|');
  check(l === [[0, 20, 10, 20], [20, 20, 100, 20], [15, 0, 15, 15], [15, 25, 15, 50]].map(q => q.join()).sort().join('|'), 'dimension lines run from the hole edge to the plate edge: ' + l);
  const sd = core.holeDimItems([{ kind: 'rect', x: 20, y: 10, w: 40, h: 4 }], { w: 100, h: 50 }).filter(i => i.kind === 'text').map(i => i.s).sort().join();
  check(sd === '10,20,36,40', 'slot: distances to its edges (left 20, right 40, up 10, down 36), got ' + sd);
  const dr = core.holeDimItems([{ kind: 'circle', x: 33.33, y: 20.06, d: 10 }], { w: 100, h: 50 }).filter(i => i.kind === 'text').map(i => i.s).sort().join();
  check(dr === '20.1,29.9,33.3,66.7', 'numbers are rounded to 0.1 mm: ' + dr);
  const pdf = core.pdfTiles([], { w: 100, h: 50 }, 'A4', 'T', dm).text;
  check(pdf.indexOf('0 0.45 0.15 RG 0.1 w [1 1] 0 d ') > 0 && pdf.indexOf('(85) Tj') > 0 && pdf.indexOf('0 0.45 0.15 rg') > 0, 'PDF has dotted green dimension lines and green numbers');
  const lm = pdf.match(/0\.45 0\.15 RG 0\.1 w \[1 1\] 0 d ([0-9.]+) ([0-9.]+) m ([0-9.]+) ([0-9.]+) l S/); near(+lm[1], 10 * 72 / 25.4, 0.01, 'first line starts at the plate edge (x = margin 10 mm)'); near(+lm[2], (10 + 190 - 20) * 72 / 25.4, 0.01, 'at the hole height'); near(+lm[3], 20 * 72 / 25.4, 0.01, 'and ends at the hole edge (10 + 10 mm)');
  check(core.defaultState().export.pdfDims === true && core.fromJson({ version: 1, export: { pdfDims: false } }).export.pdfDims === false && core.fromJson({ version: 1, export: {} }).export.pdfDims === true, 'pdfDims option: default true, false is kept');
}
// ---- DXF
{
  const dx = core.dxfFromItems([{ kind: 'outline', w: 100, h: 50 }, { kind: 'circle', x: 15, y: 20, d: 10 }, { kind: 'rect', x: 20, y: 10, w: 74, h: 4 }], { w: 100, h: 50 });
  check(dx.startsWith('0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n') && dx.endsWith('0\nENDSEC\n0\nEOF\n'), 'DXF header (R12) and end');
  check(dx.indexOf('0\nCIRCLE\n8\nHOLES\n10\n15\n20\n30\n30\n0\n40\n5\n') > 0, 'circle (15, 50-20 = 30) radius 5 on layer HOLES');
  check(dx.indexOf('0\nPOLYLINE\n8\nPLATE\n66\n1\n70\n1\n0\nVERTEX\n8\nPLATE\n10\n0\n20\n0\n30\n0\n0\nVERTEX\n8\nPLATE\n10\n100\n20\n0\n') > 0, 'closed outline polyline starting at the origin');
  check(dx.indexOf('8\nSLOT\n10\n20\n20\n36\n') > 0 && dx.indexOf('8\nSLOT\n10\n94\n20\n40\n') > 0, 'slot corners: lower-left (20, 50-10-4 = 36), upper-right (94, 40)');
  check((dx.match(/0\nSEQEND/g) || []).length === 2, 'two polylines closed with SEQEND');
  const dp = core.dxfFromPolys([[[0, 0], [10, 0], [10, 20]], [[1, 1], [2, 2]]], 50, [{ x: 0, y: 0, w: 10, h: 20 }]);
  check(dp.indexOf('8\nLETTERS\n10\n10\n20\n50\n') > 0 && dp.indexOf('8\nLETTERS\n10\n10\n20\n30\n') > 0, 'letter polygon y flipped (0 -> 50, 20 -> 30)');
  const dc = core.dxfFromPolys([], 50, [], [[[0, 0], [100, 0], [100, 50], [0, 50]]]);
  check(dc.indexOf('POLYLINE\n8\nPLATE_CUT\n66\n1\n70\n1\n0\nVERTEX\n8\nPLATE_CUT\n10\n0\n20\n50\n') > 0 && dc.indexOf('8\nPLATE_CUT\n10\n100\n20\n0\n') > 0 && dc.indexOf('LETTERS') < 0, 'plate edge on layer PLATE_CUT, y flipped (0 -> 50, 50 -> 0)');
  check((dp.match(/POLYLINE\n8\nLETTERS/g) || []).length === 1, 'polygons with fewer than 3 points are skipped');
  check(dp.indexOf('8\nBOXES\n10\n0\n20\n30\n') > 0, 'bounding box on layer BOXES, lower-left (0, 50-0-20 = 30)');
}
// ---- strip pieces behind letters
{
  const g = core.glyphFromPolygons([rect(0, -100, 20, 0, false)], { ppm: 2, padMm: 4 });
  const cell = { glyph: g, gox: 100, goy: 150 };    // the bar covers x 100..120, y 50..150
  check(Math.abs(core.stripCoverage({ x: 105, y: 50, rot: 90 }, [cell], 0) - 1) < 0.02, 'a vertical strip piece inside the bar is fully covered');
  check(Math.abs(core.stripCoverage({ x: 115, y: 50, rot: 90 }, [cell], 0) - 0.5) < 0.03, 'half of the piece is outside: 0.5');
  check(core.stripCoverage({ x: 300, y: 50, rot: 0 }, [cell], 0) === 0, 'a piece far away has no coverage');
  check(core.stripRect({ x: 5, y: 6, rot: 0 }).w === 100 && core.stripRect({ x: 5, y: 6, rot: 90 }).h === 100 && core.stripRect({ x: 5, y: 6, rot: 90 }).w === 10, 'strip piece is 100 x 10 mm, rotated 10 x 100');
}
// ---- drilling pattern, EPS, PDF
function scene() {
  const g = fakeGlyph(20), st = core.defaultState();
  st.plate = { w: 100, h: 50 }; st.holeD = 10;
  const mk = (x) => core.placeText({ x: x, y: 5, h: 40, mode: 'uniform', cellW: 30, spacing: 0, holeOffsets: [] }, [g, g], true);
  return { st, scene: { casino: mk(0), open: mk(60), arrow: { holes: [{ x: 10, y: 10, d: 8 }, { x: 20, y: 10, d: 8 }] }, closedRound: { x: 50, y: 25 }, closedSlot: { x: 50, y: 25, w: 74, h: 4 } } };
}
{
  const s = scene(), items = core.drillItems(s.st, s.scene);
  const circles = items.filter(i => i.kind === 'circle'), rects = items.filter(i => i.kind === 'rect');
  check(circles.length === 4 + 2 + 1 && rects.length === 0 && items[0].kind === 'outline', 'drill items: outline, 4 letter holes, 2 arrow holes, 1 CLOSED round hole');
  check(circles.filter(c => c.d === 10).length === 5 && circles.filter(c => c.d === 8).length === 2, 'letter and CLOSED holes use 10 mm, arrow holes 8 mm');
  s.st.export.closedHole = 'slot'; s.st.export.arrow = false; s.st.export.plateOutline = false;
  const it2 = core.drillItems(s.st, s.scene);
  check(it2.filter(i => i.kind === 'circle').length === 4 && it2.filter(i => i.kind === 'rect').length === 1 && !it2.some(i => i.kind === 'outline'), 'slot mode: 4 circles, 1 slot, no outline, no arrow holes');
  const slot = it2.find(i => i.kind === 'rect'); check(slot.x === 13 && slot.y === 23 && slot.w === 74 && slot.h === 4, 'slot rectangle is centred at the CLOSED slot position');
  s.st.export.closedHole = 'none';
  check(core.drillItems(s.st, s.scene).length === 4, 'none: only the 4 letter holes');
  s.st.export.closedHole = 'round'; s.st.export.arrow = true; s.st.export.plateOutline = true;
  const eps = core.epsFromItems(core.drillItems(s.st, s.scene), s.st.plate, 'T');
  check(eps.indexOf('%!PS-Adobe-3.0 EPSF-3.0') === 0, 'EPS starts with the EPSF header');
  check(eps.indexOf('%%BoundingBox: 0 0 312 171') > 0, 'bounding box of a 100 x 50 mm plate plus 5 mm margins (110 x 60 mm) is 312 x 171 pt');
  check((eps.match(/ arc /g) || []).length === 7, 'EPS has 7 circles');
  check(eps.indexOf('0.1 setlinewidth') > 0 && eps.trim().endsWith('%%EOF'), 'EPS line width and end marker');
  check(/\n0 0 1 setrgbcolor 5 mm 5 mm moveto 100 mm 0 rlineto 0 50 mm rlineto -100 mm 0 rlineto closepath stroke/.test(eps), 'plate outline path');
  check(eps.indexOf('newpath 20 mm 30 mm 5 mm 0 360 arc') > 0 && eps.indexOf('newpath 15 mm 45 mm 4 mm 0 360 arc') > 0, 'first letter hole: centre (15, 25) -> +5 margin = 20 mm, y flipped 50-25+5 = 30 mm, radius 5 mm; first arrow hole (10, 10) -> 15 mm, 45 mm, radius 4 mm');
  const poly = core.epsFromPolys([[[0, 0], [10, 0], [10, 20]]], 100, 50, 'L', [{ x: 0, y: 0, w: 10, h: 20 }]);
  check(poly.indexOf('closepath stroke') > 0 && poly.indexOf('[1 1] 0 setdash') > 0 && poly.indexOf('10 mm 20 mm lineto') < 0 && poly.indexOf('15 mm 35 mm lineto') > 0, 'polygon EPS with a dashed bounding box');
}
{
  const s = scene(), items = core.drillItems(s.st, s.scene);
  const a4 = core.pdfTiles(items, { w: 980, h: 300 }, 'A4', 'T'), a3 = core.pdfTiles(items, { w: 980, h: 300 }, 'A3', 'T');
  check(a4.cols === 4 && a4.rows === 2 && a4.pages === 8, 'A4 landscape tiles of 277 x 190 mm: 4 x 2 = 8 pages, got ' + a4.pages);
  check(a3.cols === 3 && a3.rows === 2 && a3.pages === 6, 'A3 landscape tiles of 400 x 277 mm: 3 x 2 = 6 pages, got ' + a3.pages);
  const ref = [{ kind: 'poly', loops: [[[10, 10], [60, 10], [60, 40], [10, 40]], [[20, 20], [30, 20], [30, 30]]], fill: 0.9, stroke: 0.5 }, { kind: 'text', x: 35, y: 25, s: 'L-CAP D2 (a)', size: 6 }];
  const wr = core.pdfTiles(items, { w: 250, h: 100 }, 'A4', 'T', ref).text, wo = core.pdfTiles(items, { w: 250, h: 100 }, 'A4', 'T').text;
  check(wr.indexOf('0.9 g 0.5 G 0.2 w ') > 0, 'reference polygon uses its grey levels'); check(wo.indexOf(' B*') < 0 && wo.indexOf('Tj') > 0, 'no reference drawing without ref (only the tile label text)');
  check(/ m [0-9. ]+ l [0-9. ]+ l [0-9. ]+ l h [0-9. ]+ m [0-9. ]+ l [0-9. ]+ l h B\*/.test(wr), 'polygon has two loops (outer + hole), closed, filled even-odd');
  check(wr.indexOf('(L-CAP D2 \\(a\\)) Tj') > 0, 'reference text is written with escaped brackets');
  const m1 = wr.match(/0\.9 g 0\.5 G 0\.2 w ([0-9.]+) ([0-9.]+) m /); near(+m1[1], (10 + 10) * 72 / 25.4, 0.01, 'first polygon point x = margin 10 + 10 mm in pt'); near(+m1[2], (10 + 190 - 10) * 72 / 25.4, 0.01, 'first polygon point y: A4 tile 190 mm high, plate y 10 from the top');
  check(wr.indexOf('0.9 g') < wr.indexOf('0 0 1 RG'), 'reference is drawn before the drill pattern');
  check(core.pdfTiles(items, { w: 250, h: 100 }, 'A4', 'T').pages === 1, 'a small plate fits one A4 page');
  check(a4.text.indexOf('%PDF-1.4') === 0 && a4.text.indexOf('/Count 8') > 0 && a4.text.indexOf('[0 0 841.89 595.276]') > 0, 'PDF header, page count and A4 landscape media box 841.89 x 595.276 pt');
  const sx = a4.text.lastIndexOf('startxref'), off = parseInt(a4.text.slice(sx + 10), 10);
  check(a4.text.slice(off, off + 4) === 'xref', 'startxref points at the xref table');
  const xr = a4.text.slice(off).split('\n'), n = parseInt(xr[1].split(' ')[1], 10);
  let okOffsets = true; for (let i = 1; i < n; i++) { const o = parseInt(xr[2 + i], 10); if (a4.text.slice(o, o + 12).indexOf(i + ' 0 obj') !== 0) okOffsets = false; }
  check(okOffsets, 'every xref offset points at its object');
  try {
    const { PDFDocument } = require(process.env.PDF_LIB || '/home/claude/.npm-global/lib/node_modules/pdf-lib');
    PDFDocument.load(Buffer.from(a4.text, 'latin1')).then(d => { check(d.getPageCount() === 8, 'pdf-lib reads 8 pages'); const p = d.getPage(0).getSize(); near(p.width, 841.89, 0.01, 'pdf-lib page width'); finish(); }).catch(e => { check(false, 'pdf-lib could not read the PDF: ' + e.message); finish(); });
  } catch (e) { console.log('(pdf-lib not found, skipped the parser check)'); finish(); }
}
// ---- nesting
{
  const r = core.nestShelves([{ id: 'a', w: 100, h: 80 }, { id: 'b', w: 150, h: 120 }, { id: 'c', w: 200, h: 60 }, { id: 'd', w: 120, h: 100 }], 300, 300, 3);
  const pos = {}; r.placed.forEach(p => pos[p.id] = p.x + ',' + p.y);
  check(pos.b === '0,0' && pos.d === '153,0' && pos.a === '0,123' && pos.c === '0,206' && r.fits && r.usedH === 266, 'shelf nesting: ' + JSON.stringify(pos) + ' usedH ' + r.usedH);
  check(!core.nestShelves([{ id: 'x', w: 100, h: 200 }, { id: 'y', w: 100, h: 200 }, { id: 'z', w: 250, h: 120 }], 300, 300, 3).fits, 'does not fit when the shelves exceed the sheet');
  check(!core.nestShelves([{ id: 'w', w: 400, h: 10 }], 300, 300, 3).fits, 'a piece wider than the sheet does not fit');
}
// ---- JSON
{
  const d = core.defaultState();
  check(d.plate.w === 980 && d.plate.h === 300 && d.holeD === 10 && d.texts.casino.text === 'CASINO' && d.texts.open.text === 'OPEN' && d.texts.closed.text === 'CLOSED', 'defaults: plate 980 x 300, hole 10 mm, texts');
  check(d.texts.closed.h === d.texts.casino.h / 2 && d.texts.open.sameFont && d.texts.closed.sameFont, 'CLOSED is half as high; OPEN and CLOSED use the CASINO font by default');
  const back = core.fromJson(core.toJson(d));
  check(JSON.stringify(back) === JSON.stringify(d), 'JSON round trip keeps the state');
  const odd = core.fromJson(JSON.stringify({ version: 1, plate: { w: 5000, h: -3 }, holeD: 99, strips: [{ x: 1, y: 2, rot: 45 }], export: { closedHole: 'bogus' } }));
  check(odd.plate.w === 980 && odd.plate.h === 100 && odd.holeD === 30 && odd.strips[0].rot === 0 && odd.export.closedHole === 'round', 'values are clamped and checked');
  check(core.defaultState().export.pdfRef === true && core.fromJson({ version: 1, export: { pdfRef: false } }).export.pdfRef === false && core.fromJson({ version: 1, export: {} }).export.pdfRef === true, 'pdfRef option: default true, false is kept');
  check(core.fromJson({ version: 1, ui: { dims: 'bogus' } }).ui.dims === 'selected', 'unknown dimension mode falls back to selected');
  const low = core.fromJson({ version: 1, holeD: 1 }); check(low.holeD === 5, 'hole diameter minimum is 5 mm');
  let threw = false; try { core.fromJson('{"version":2}'); } catch (e) { threw = true; } check(threw, 'unknown version is rejected');
  threw = false; try { core.fromJson('not json'); } catch (e) { threw = true; } check(threw, 'bad JSON is rejected');
}
// ---- arrow data (made from tools/arrow_geometry.py)
{
  check(ARROW.w === 349.76 && ARROW.h === 102.32 && ARROW.sticks.length === 10 && ARROW.holeD === 8, 'arrow data: 349.76 x 102.32 mm, 10 sticks, 8 mm cable holes');
  check(ARROW.sticks[0].name === 'L-CAP' && ARROW.sticks[7].name === 'R-CAP' && ARROW.sticks[0].hole[0] === 83.88 && ARROW.sticks[0].hole[1] === 51.16, 'L-CAP first, R-CAP eighth, L-CAP hole at (83.88, 51.16)');
}
let finished = false;
function finish() {
  if (finished) return; finished = true;
  console.log(fails ? 'RESULT: ' + fails + ' failures' : 'RESULT: all checks passed');
  process.exit(fails ? 1 : 0);
}
setTimeout(finish, 3000);

// resizing the plate around its centre
{
  const st = core.defaultState(); st.plate.w = 600; st.plate.h = 200; st.texts.casino.x = 20; st.texts.casino.y = 10; st.texts.open.x = 100.5; st.texts.closed.y = 150; st.arrow.x = 30; st.arrow.y = 50; st.strips = [{ x: 5, y: 7, rot: 0 }];
  core.resizePlate(st, 700, undefined);
  check(st.plate.w === 700 && st.plate.h === 200, 'width 600 -> 700');
  check(st.texts.casino.x === 70 && st.texts.casino.y === 10 && st.texts.open.x === 150.5 && st.texts.closed.y === 150 && st.arrow.x === 80 && st.arrow.y === 50 && st.strips[0].x === 55 && st.strips[0].y === 7, 'both side edges move 50 mm: x +50, y unchanged');
  core.resizePlate(st, undefined, 150);
  check(st.plate.h === 150 && st.texts.casino.y === -15 && st.texts.closed.y === 125 && st.arrow.y === 25 && st.strips[0].y === -18 && st.texts.casino.x === 70, 'height 200 -> 150: y -25');
  core.resizePlate(st, 701.5, 150);
  check(st.texts.casino.x === 70.75 && st.arrow.x === 80.75 && st.strips[0].x === 55.75, 'half of 1.5 mm = 0.75 mm');
  core.resizePlate(st, 5000, 5000);
  check(st.plate.w === 980 && st.plate.h === 300 && st.texts.casino.x === 70.75 + 139.25 && st.texts.casino.y === -15 + 75, 'clamped to 980 x 300, the shift uses the real change (139.25, 75)');
  core.resizePlate(st, 1, 1);
  check(st.plate.w === 100 && st.plate.h === 100, 'clamped to 100 x 100');
  const before = JSON.stringify(st); core.resizePlate(st, 100, 100);
  check(JSON.stringify(st) === before, 'no change, no shift');
  const rd = core.defaultState(); rd.plate.w = 500; rd.texts.casino.x = 0; core.resizePlate(rd, 500.014, undefined);
  check(rd.texts.casino.x === 0.01, 'shifts are rounded to 0.01 mm (0.007 -> 0.01), got ' + rd.texts.casino.x);
  const centre = core.defaultState(); centre.plate.w = 400; centre.texts.casino.x = 100; const cxBefore = 100 - 400 / 2;
  core.resizePlate(centre, 800, undefined);
  check(centre.texts.casino.x - 800 / 2 === cxBefore, 'the distance from the plate centre stays the same');
}

