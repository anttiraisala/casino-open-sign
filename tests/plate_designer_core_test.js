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
// ---- frame strips, dimensions, snapping
{
  const f = core.frameCounts(980, 300, 'topbottom', 100, 10);
  check(f.top.n === 10 && f.top.left === 0 && f.left.n === 3 && f.left.left === 0 && f.totalUnits === 26 && f.totalLeds === 78, 'frame 980 x 300, top/bottom over the corners: 26 units, 78 LEDs');
  const g = core.frameCounts(980, 300, 'sides', 100, 10);
  check(g.top.n === 9 && Math.round(g.top.left) === 80 && g.left.n === 3 && Math.round(g.left.left) === 20 && g.totalUnits === 24, 'frame with sides over the corners: 24 units, leftovers 80 and 20');
  const h = core.frameCounts(599.9, 300, 'sides', 100, 10);
  check(h.top.n === 5 && Math.round(h.top.left) === 100, 'just under a multiple counts one unit less (599.9 mm: 5 units, 99.9 mm left over)');
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
