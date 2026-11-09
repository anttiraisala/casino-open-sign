/*
 * Plate Designer core: pure geometry and file-writing logic (no DOM). Works in the browser (window.PlateCore) and in
 * Node (require). All lengths are millimetres, x to the right, y down, origin at the top-left corner of the plate.
 *
 * Letters are handled as masks (pixels) because the browser can draw any font into a canvas: from a mask this file
 * makes a signed distance field, and from that the offset (thickening / kerf) outlines, the largest circle that fits
 * into a letter (hole placement), thin-part detection and the cut outlines (marching squares).
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.PlateCore = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  var MM_TO_PT = 72 / 25.4;
  var BIG = 1e20;

  // ------------------------------------------------------------------ distance transform

  // Squared 1-D distance transform (Felzenszwalb & Huttenlocher), in place on f (length n).
  function edt1d(f, n, d, v, z) {
    var k = 0, q, s;
    v[0] = 0; z[0] = -BIG; z[1] = BIG;
    for (q = 1; q < n; q++) {
      s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
      while (s <= z[k]) {
        k--;
        s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
      }
      k++; v[k] = q; z[k] = s; z[k + 1] = BIG;
    }
    k = 0;
    for (q = 0; q < n; q++) {
      while (z[k + 1] < q) k++;
      d[q] = (q - v[k]) * (q - v[k]) + f[v[k]];
    }
  }

  // Distance (in pixels) from every pixel to the nearest pixel where src[i] is truthy (0 on the source itself).
  function distanceTo(src, w, h) {
    var n = Math.max(w, h), f = new Float64Array(n), d = new Float64Array(n), v = new Int32Array(n), z = new Float64Array(n + 1);
    var g = new Float64Array(w * h), x, y;
    for (x = 0; x < w; x++) {
      for (y = 0; y < h; y++) f[y] = src[y * w + x] ? 0 : BIG;
      edt1d(f, h, d, v, z);
      for (y = 0; y < h; y++) g[y * w + x] = d[y];
    }
    var out = new Float32Array(w * h);
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) f[x] = g[y * w + x];
      edt1d(f, w, d, v, z);
      for (x = 0; x < w; x++) out[y * w + x] = d[x] >= BIG / 2 ? 1e6 : Math.sqrt(d[x]);
    }
    return out;
  }

  // Signed distance field in pixels: positive inside the shape (distance to the edge), negative outside.
  function signedField(mask, w, h) {
    var dOut = distanceTo(mask, w, h);
    var inv = new Uint8Array(w * h), i;
    for (i = 0; i < w * h; i++) inv[i] = mask[i] ? 0 : 1;
    var dIn = distanceTo(inv, w, h);
    var s = new Float32Array(w * h);
    for (i = 0; i < w * h; i++) s[i] = mask[i] ? dIn[i] - 0.5 : -(dOut[i] - 0.5);
    return s;
  }

  // Fills polygons (pixel coordinates, nonzero winding) into a w x h mask sampled at the pixel centres.
  function rasterize(polys, w, h) {
    var mask = new Uint8Array(w * h), y, i, j, p, a, b, xs, k, wind, x;
    for (y = 0; y < h; y++) {
      var yc = y + 0.5;
      xs = [];
      for (p = 0; p < polys.length; p++) {
        var pts = polys[p];
        for (i = 0; i < pts.length; i++) {
          a = pts[i]; b = pts[(i + 1) % pts.length];
          if (a[1] === b[1]) continue;
          var lo = Math.min(a[1], b[1]), hi = Math.max(a[1], b[1]);
          if (yc < lo || yc >= hi) continue;
          xs.push([a[0] + (yc - a[1]) / (b[1] - a[1]) * (b[0] - a[0]), b[1] > a[1] ? 1 : -1]);
        }
      }
      xs.sort(function (m, n) { return m[0] - n[0]; });
      wind = 0;
      for (k = 0; k < xs.length - 1; k++) {
        wind += xs[k][1];
        if (wind !== 0) {
          var x0 = Math.ceil(xs[k][0] - 0.5), x1 = Math.ceil(xs[k + 1][0] - 0.5);
          for (x = Math.max(0, x0); x < Math.min(w, x1); x++) mask[y * w + x] = 1;
        }
      }
    }
    return mask;
  }

  // ------------------------------------------------------------------ contours (marching squares)

  // Closed polylines (pixel coordinates) where the field crosses `level`; inside = field >= level.
  function marching(field, w, h, level) {
    var adj = {}, i, j;
    function key(t, a, b) { return t + ':' + a + ',' + b; }
    function pointOf(kk) {
      var t = kk.charAt(0), parts = kk.slice(2).split(','), a = +parts[0], b = +parts[1], va, vb, tt;
      if (t === 'h') {
        va = field[b * w + a] - level; vb = field[b * w + a + 1] - level; tt = va / (va - vb);
        return [a + 0.5 + tt, b + 0.5];
      }
      va = field[b * w + a] - level; vb = field[(b + 1) * w + a] - level; tt = va / (va - vb);
      return [a + 0.5, b + 0.5 + tt];
    }
    function link(p, q) { (adj[p] = adj[p] || []).push(q); (adj[q] = adj[q] || []).push(p); }
    for (j = 0; j < h - 1; j++) {
      for (i = 0; i < w - 1; i++) {
        var a = field[j * w + i] - level, b = field[j * w + i + 1] - level, c = field[(j + 1) * w + i + 1] - level, d = field[(j + 1) * w + i] - level;
        var idx = (a >= 0 ? 1 : 0) | (b >= 0 ? 2 : 0) | (c >= 0 ? 4 : 0) | (d >= 0 ? 8 : 0);
        if (idx === 0 || idx === 15) continue;
        var T = key('h', i, j), R = key('v', i + 1, j), B = key('h', i, j + 1), L = key('v', i, j);
        var centreInside = (a + b + c + d) / 4 >= 0;
        switch (idx) {
          case 1: case 14: link(L, T); break;
          case 2: case 13: link(T, R); break;
          case 3: case 12: link(L, R); break;
          case 4: case 11: link(R, B); break;
          case 5: if (centreInside) { link(T, R); link(L, B); } else { link(L, T); link(R, B); } break;
          case 6: case 9: link(T, B); break;
          case 7: case 8: link(L, B); break;
          case 10: if (centreInside) { link(L, T); link(R, B); } else { link(T, R); link(L, B); } break;
        }
      }
    }
    var loops = [], seen = {}, kk;
    for (kk in adj) {
      if (seen[kk]) continue;
      var loop = [], cur = kk;
      while (cur !== undefined && !seen[cur]) {
        seen[cur] = true; loop.push(pointOf(cur));
        var nb = adj[cur];
        cur = seen[nb[0]] ? nb[1] : nb[0];
      }
      if (loop.length >= 3) loops.push(loop);
    }
    return loops;
  }

  // Douglas-Peucker for a closed polygon.
  function simplifyClosed(pts, tol) {
    if (pts.length < 4 || tol <= 0) return pts.slice();
    var far = 0, dmax = -1, i;
    for (i = 1; i < pts.length; i++) {
      var dd = Math.pow(pts[i][0] - pts[0][0], 2) + Math.pow(pts[i][1] - pts[0][1], 2);
      if (dd > dmax) { dmax = dd; far = i; }
    }
    function dp(arr) {
      if (arr.length < 3) return arr.slice(0, arr.length - 1);
      var a = arr[0], b = arr[arr.length - 1], dx = b[0] - a[0], dy = b[1] - a[1], len = Math.sqrt(dx * dx + dy * dy), best = -1, bi = -1, k;
      for (k = 1; k < arr.length - 1; k++) {
        var dist = len === 0 ? Math.hypot(arr[k][0] - a[0], arr[k][1] - a[1]) : Math.abs(dy * arr[k][0] - dx * arr[k][1] + b[0] * a[1] - b[1] * a[0]) / len;
        if (dist > best) { best = dist; bi = k; }
      }
      if (best > tol) return dp(arr.slice(0, bi + 1)).concat(dp(arr.slice(bi)));
      return [a];
    }
    var first = pts.slice(0, far + 1), second = pts.slice(far).concat([pts[0]]);
    return dp(first).concat(dp(second));
  }

  // ------------------------------------------------------------------ glyphs

  // A glyph is a mask with a scale and an origin (the baseline start of the letter). Local coordinates are
  // millimetres from that origin, y down (letters stand above y = 0).
  function Glyph(o) {
    this.mask = o.mask; this.w = o.w; this.h = o.h; this.ppm = o.ppm; this.ox = o.ox; this.oy = o.oy;
    this.advanceMm = o.advanceMm || 0;
    this._field = null; this._ink = null; this._area = null;
  }
  Glyph.prototype.field = function () {
    if (!this._field) this._field = signedField(this.mask, this.w, this.h);
    return this._field;
  };
  Glyph.prototype.ink = function () {
    if (this._ink) return this._ink;
    var x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9, x, y;
    for (y = 0; y < this.h; y++) for (x = 0; x < this.w; x++) if (this.mask[y * this.w + x]) {
      if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y;
    }
    if (x1 < 0) return (this._ink = null);
    var p = this.ppm;
    this._ink = { x0: (x0 - this.ox) / p, x1: (x1 + 1 - this.ox) / p, y0: (y0 - this.oy) / p, y1: (y1 + 1 - this.oy) / p };
    return this._ink;
  };
  Glyph.prototype.areaMm2 = function () {
    if (this._area === null) {
      var n = 0, i;
      for (i = 0; i < this.mask.length; i++) if (this.mask[i]) n++;
      this._area = n / (this.ppm * this.ppm);
    }
    return this._area;
  };
  // Distance (mm) from a local point to the edge of the letter grown by offsetMm; negative outside.
  Glyph.prototype.insideDist = function (x, y, offsetMm) {
    var px = x * this.ppm + this.ox - 0.5, py = y * this.ppm + this.oy - 0.5;
    var i = Math.floor(px), j = Math.floor(py), f = this.field();
    if (i < 0 || j < 0 || i >= this.w - 1 || j >= this.h - 1) return -1e3;
    var fx = px - i, fy = py - j;
    var v = f[j * this.w + i] * (1 - fx) * (1 - fy) + f[j * this.w + i + 1] * fx * (1 - fy) + f[(j + 1) * this.w + i] * (1 - fx) * fy + f[(j + 1) * this.w + i + 1] * fx * fy;
    return v / this.ppm + (offsetMm || 0);
  };
  // The point of the letter farthest from its edge (the largest circle that fits): {x, y, r} in mm.
  Glyph.prototype.inscribed = function (offsetMm) {
    var f = this.field(), best = -1e9, bi = 0, i;
    for (i = 0; i < f.length; i++) if (f[i] > best) { best = f[i]; bi = i; }
    return { x: ((bi % this.w) + 0.5 - this.ox) / this.ppm, y: (Math.floor(bi / this.w) + 0.5 - this.oy) / this.ppm, r: best / this.ppm + (offsetMm || 0) };
  };
  // The point closest to (cx, cy) where a circle of radius r (plus margin) fits inside the letter grown by offsetMm.
  // Falls back to the thickest point when no circle fits. Returns {x, y, r, fits}.
  Glyph.prototype.bestHole = function (cx, cy, r, offsetMm, marginMm) {
    var f = this.field(), need = (r + (marginMm || 0) - (offsetMm || 0)) * this.ppm, best = 1e18, bi = -1, i;
    for (i = 0; i < f.length; i++) {
      if (f[i] < need) continue;
      var x = ((i % this.w) + 0.5 - this.ox) / this.ppm, y = (Math.floor(i / this.w) + 0.5 - this.oy) / this.ppm;
      var dd = (x - cx) * (x - cx) + (y - cy) * (y - cy);
      if (dd < best) { best = dd; bi = i; }
    }
    if (bi < 0) { var t = this.inscribed(offsetMm); return { x: t.x, y: t.y, r: t.r, fits: false }; }
    return { x: ((bi % this.w) + 0.5 - this.ox) / this.ppm, y: (Math.floor(bi / this.w) + 0.5 - this.oy) / this.ppm, r: f[bi] / this.ppm + (offsetMm || 0), fits: true };
  };
  // Area (mm2) of the letter (grown by offsetMm) that is thinner than minStrokeMm.
  Glyph.prototype.thinAreaMm2 = function (minStrokeMm, offsetMm) {
    var f = this.field(), off = (offsetMm || 0) * this.ppm, r = minStrokeMm / 2 * this.ppm, n = f.length, core = new Uint8Array(n), i, any = false;
    for (i = 0; i < n; i++) if (f[i] + off >= r) { core[i] = 1; any = true; }
    var thin = 0;
    if (!any) { for (i = 0; i < n; i++) if (f[i] + off >= 0) thin++; return thin / (this.ppm * this.ppm); }
    var d = distanceTo(core, this.w, this.h);
    for (i = 0; i < n; i++) if (f[i] + off >= 0 && d[i] > r + 1) thin++;
    return thin / (this.ppm * this.ppm);
  };
  // Cut outlines of the letter grown by offsetMm: closed polygons in local millimetres.
  Glyph.prototype.contours = function (offsetMm, tolMm) {
    var loops = marching(this.field(), this.w, this.h, -(offsetMm || 0) * this.ppm), out = [], k, p = this.ppm, ox = this.ox, oy = this.oy;
    for (k = 0; k < loops.length; k++) {
      var mm = loops[k].map(function (q) { return [(q[0] - ox) / p, (q[1] - oy) / p]; });
      out.push(simplifyClosed(mm, tolMm === undefined ? 0.02 : tolMm));
    }
    return out;
  };

  // Builds a glyph from polygons in millimetres (used by the tests; the browser draws masks with a canvas).
  function glyphFromPolygons(polysMm, o) {
    var ppm = o.ppm, pad = Math.ceil((o.padMm === undefined ? 6 : o.padMm) * ppm), x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
    polysMm.forEach(function (p) { p.forEach(function (q) { x0 = Math.min(x0, q[0]); y0 = Math.min(y0, q[1]); x1 = Math.max(x1, q[0]); y1 = Math.max(y1, q[1]); }); });
    var ox = pad - Math.floor(x0 * ppm), oy = pad - Math.floor(y0 * ppm);
    var w = Math.ceil(x1 * ppm) - Math.floor(x0 * ppm) + 2 * pad, h = Math.ceil(y1 * ppm) - Math.floor(y0 * ppm) + 2 * pad;
    var polysPx = polysMm.map(function (p) { return p.map(function (q) { return [q[0] * ppm + ox, q[1] * ppm + oy]; }); });
    return new Glyph({ mask: rasterize(polysPx, w, h), w: w, h: h, ppm: ppm, ox: ox, oy: oy, advanceMm: o.advanceMm === undefined ? x1 : o.advanceMm });
  }

  // ------------------------------------------------------------------ text layout

  // glyphs: array of Glyph or null (a space). def: {x, y, h, mode:'uniform'|'natural', cellW, spacing, holeOffsets:[{dx,dy}]}.
  // Returns the box and the cells: each cell has its rectangle, the absolute glyph origin and the (default) hole position.
  function placeText(def, glyphs, withHoles) {
    var cells = [], x = 0, i;
    var uniformW = def.cellW;
    for (i = 0; i < glyphs.length; i++) {
      var g = glyphs[i], cw, gx;
      if (def.mode === 'natural') {
        var adv = g ? g.advanceMm : (def.h * 0.3);
        cw = adv + def.spacing; gx = x + def.spacing / 2;
      } else {
        cw = uniformW;
        var ink = g && g.ink();
        gx = x + cw / 2 - (ink ? (ink.x0 + ink.x1) / 2 : 0);
      }
      var cell = { index: i, glyph: g, x: def.x + x, y: def.y, w: cw, h: def.h, gox: def.x + gx, goy: def.y + def.h };
      cell.cx = cell.x + cw / 2; cell.cy = cell.y + def.h / 2;
      if (withHoles && g) {
        var o = (def.holeOffsets && def.holeOffsets[i]) || { dx: 0, dy: 0 };
        cell.hole = { x: cell.cx + (o.dx || 0), y: cell.cy + (o.dy || 0) };
      }
      cells.push(cell);
      x += cw;
    }
    return { x: def.x, y: def.y, w: x, h: def.h, cells: cells };
  }

  // Is the hole circle (radius r) fully behind the letter (grown by offsetMm)? clearance = how much room is left (mm).
  function holeStatus(cell, r, offsetMm) {
    if (!cell.glyph || !cell.hole) return { covered: false, clearance: -1e3 };
    var d = cell.glyph.insideDist(cell.hole.x - cell.gox, cell.hole.y - cell.goy, offsetMm);
    return { covered: d >= r - 1e-6, clearance: d - r };
  }

  // Hole offset (relative to the cell centre) for the quick buttons.
  function holeOffsetFor(cell, r, offsetMm, mode, marginMm) {
    if (mode === 'centre') return { dx: 0, dy: 0 };
    var b = cell.glyph.bestHole(cell.cx - cell.gox, cell.cy - cell.goy, r, offsetMm, marginMm === undefined ? 0.5 : marginMm);
    return { dx: b.x + cell.gox - cell.cx, dy: b.y + cell.goy - cell.cy, fits: b.fits };
  }

  // ------------------------------------------------------------------ frame strips, dimensions, snapping

  // How many whole 3-LED units (100 mm) fit on each side of the frame. layout: 'topbottom' (top and bottom run over the
  // corners) or 'sides' (the sides run over the corners). The strip (width sw) lies just outside the plate edge.
  function frameCounts(W, H, layout, unit, sw) {
    var topLen = layout === 'sides' ? W : W + 2 * sw, sideLen = layout === 'sides' ? H + 2 * sw : H;
    function side(len) { var n = Math.floor(len / unit + 1e-9); return { len: len, n: n, left: len - n * unit }; }
    var t = side(topLen), b = side(topLen), l = side(sideLen), r = side(sideLen);
    var total = t.n + b.n + l.n + r.n;
    return { top: t, bottom: b, left: l, right: r, totalUnits: total, totalLeds: total * 3 };
  }

  // Distances from a point to the four plate edges (the little dimension arrows).
  function dimsForPoint(x, y, W, H) { return { left: x, right: W - x, up: y, down: H - y }; }

  // Centre and the four corners of a box.
  function boxPoints(b) {
    return [{ name: 'centre', x: b.x + b.w / 2, y: b.y + b.h / 2 }, { name: 'tl', x: b.x, y: b.y }, { name: 'tr', x: b.x + b.w, y: b.y },
      { name: 'bl', x: b.x, y: b.y + b.h }, { name: 'br', x: b.x + b.w, y: b.y + b.h }];
  }

  // Snaps the vertical and horizontal centre lines of a moving box to the centre lines of targets (boxes and the
  // plate). Returns the extra movement and the guide lines that were used.
  function snapCentres(box, targets, thr) {
    var cx = box.x + box.w / 2, cy = box.y + box.h / 2, bestX = null, bestY = null, i;
    for (i = 0; i < targets.length; i++) {
      var dx = targets[i].cx - cx, dy = targets[i].cy - cy;
      if (Math.abs(dx) <= thr && (bestX === null || Math.abs(dx) < Math.abs(bestX.d))) bestX = { d: dx, at: targets[i].cx };
      if (Math.abs(dy) <= thr && (bestY === null || Math.abs(dy) < Math.abs(bestY.d))) bestY = { d: dy, at: targets[i].cy };
    }
    return { dx: bestX ? bestX.d : 0, dy: bestY ? bestY.d : 0, vertical: bestX ? bestX.at : null, horizontal: bestY ? bestY.at : null };
  }

  // ------------------------------------------------------------------ strip pieces behind the letters

  function stripRect(s) { return s.rot === 90 ? { x: s.x, y: s.y, w: 10, h: 100 } : { x: s.x, y: s.y, w: 100, h: 10 }; }

  // Fraction (0..1) of a 100 x 10 mm strip piece that lies behind a letter (any of the placed cells).
  function stripCoverage(s, cells, offsetMm) {
    var r = stripRect(s), n = 0, hit = 0, x, y, step = 0.5, c, k;
    for (y = r.y + step / 2; y < r.y + r.h; y += step) {
      for (x = r.x + step / 2; x < r.x + r.w; x += step) {
        n++;
        for (k = 0; k < cells.length; k++) {
          c = cells[k];
          if (!c.glyph) continue;
          if (c.glyph.insideDist(x - c.gox, y - c.goy, offsetMm) >= 0) { hit++; break; }
        }
      }
    }
    return n ? hit / n : 0;
  }

  // ------------------------------------------------------------------ drill pattern items, EPS, PDF

  function fmt(n) { return String(Math.round(n * 1000) / 1000); }

  // Primitives of the drilling pattern in plate millimetres. state.holes lists are made by the caller (app or test):
  // items: {kind:'circle', x, y, d, tag} | {kind:'rect', x, y, w, h, tag} | {kind:'outline', w, h}
  function drillItems(state, scene) {
    var items = [];
    if (state.export.plateOutline) items.push({ kind: 'outline', w: state.plate.w, h: state.plate.h, tag: 'plate' });
    var d = state.holeD;
    ['casino', 'open'].forEach(function (key) {
      var t = scene[key];
      if (!t) return;
      t.cells.forEach(function (c) { if (c.hole) items.push({ kind: 'circle', x: c.hole.x, y: c.hole.y, d: d, tag: key }); });
    });
    if (state.export.arrow && scene.arrow) {
      scene.arrow.holes.forEach(function (h) { items.push({ kind: 'circle', x: h.x, y: h.y, d: h.d, tag: 'arrow' }); });
    }
    var mode = state.export.closedHole;
    if (mode === 'round') items.push({ kind: 'circle', x: scene.closedRound.x, y: scene.closedRound.y, d: d, tag: 'closed' });
    if (mode === 'slot') items.push({ kind: 'rect', x: scene.closedSlot.x - scene.closedSlot.w / 2, y: scene.closedSlot.y - scene.closedSlot.h / 2, w: scene.closedSlot.w, h: scene.closedSlot.h, tag: 'closed' });
    return items;
  }

  function epsHeader(title, w, h, margin) {
    var W = (w + 2 * margin) * MM_TO_PT, H = (h + 2 * margin) * MM_TO_PT;
    return '%!PS-Adobe-3.0 EPSF-3.0\n%%Title: ' + title + '\n%%Creator: Plate Designer (casino-open-sign)\n%%BoundingBox: 0 0 ' + Math.ceil(W) + ' ' + Math.ceil(H) +
      '\n%%HiResBoundingBox: 0 0 ' + fmt(W) + ' ' + fmt(H) + '\n%%EndComments\n/mm { ' + fmt(MM_TO_PT) + ' mul } def\n';
  }

  // EPS of the drilling pattern: 1 unit = 1 mm (converted to points), origin at the lower-left of the plate (plus margin).
  function epsFromItems(items, plate, title) {
    var m = 5, s = epsHeader(title || 'Drill pattern', plate.w, plate.h, m);
    s += '0.1 setlinewidth\n';
    function Y(y) { return plate.h - y + m; }
    items.forEach(function (it) {
      if (it.kind === 'outline') {
        s += '0 0 1 setrgbcolor ' + fmt(m) + ' mm ' + fmt(m) + ' mm moveto ' + fmt(it.w) + ' mm 0 rlineto 0 ' + fmt(it.h) + ' mm rlineto ' + fmt(-it.w) + ' mm 0 rlineto closepath stroke\n';
      } else if (it.kind === 'circle') {
        s += '0 0 0 setrgbcolor newpath ' + fmt(it.x + m) + ' mm ' + fmt(Y(it.y)) + ' mm ' + fmt(it.d / 2) + ' mm 0 360 arc closepath stroke\n';
      } else if (it.kind === 'rect') {
        s += '0 0 0 setrgbcolor newpath ' + fmt(it.x + m) + ' mm ' + fmt(Y(it.y + it.h)) + ' mm moveto ' + fmt(it.w) + ' mm 0 rlineto 0 ' + fmt(it.h) + ' mm rlineto ' + fmt(-it.w) + ' mm 0 rlineto closepath stroke\n';
      }
    });
    return s + 'showpage\n%%EOF\n';
  }

  // EPS of closed polygons (letter outlines), polys in plate-like millimetres (y down) inside a w x h area.
  function epsFromPolys(polys, w, h, title, boxes) {
    var m = 5, s = epsHeader(title || 'Letters', w, h, m);
    s += '0.1 setlinewidth 0 0 0 setrgbcolor\n';
    function Y(y) { return h - y + m; }
    polys.forEach(function (p) {
      if (p.length < 3) return;
      s += 'newpath ' + fmt(p[0][0] + m) + ' mm ' + fmt(Y(p[0][1])) + ' mm moveto\n';
      for (var i = 1; i < p.length; i++) s += fmt(p[i][0] + m) + ' mm ' + fmt(Y(p[i][1])) + ' mm lineto\n';
      s += 'closepath stroke\n';
    });
    (boxes || []).forEach(function (b) {
      s += '0 0 1 setrgbcolor [1 1] 0 setdash newpath ' + fmt(b.x + m) + ' mm ' + fmt(Y(b.y + b.h)) + ' mm moveto ' + fmt(b.w) + ' mm 0 rlineto 0 ' + fmt(b.h) + ' mm rlineto ' + fmt(-b.w) + ' mm 0 rlineto closepath stroke [] 0 setdash\n';
    });
    return s + 'showpage\n%%EOF\n';
  }

  // ASCII DXF (R12, the most widely read version; units are millimetres, y up). Entities: CIRCLE, closed POLYLINE.
  function dxfStart() { return '0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n'; }
  function dxfEnd() { return '0\nENDSEC\n0\nEOF\n'; }
  function dxfCircle(x, y, r, layer) { return '0\nCIRCLE\n8\n' + layer + '\n10\n' + fmt(x) + '\n20\n' + fmt(y) + '\n30\n0\n40\n' + fmt(r) + '\n'; }
  function dxfPoly(pts, layer) {
    var t = '0\nPOLYLINE\n8\n' + layer + '\n66\n1\n70\n1\n', i;
    for (i = 0; i < pts.length; i++) t += '0\nVERTEX\n8\n' + layer + '\n10\n' + fmt(pts[i][0]) + '\n20\n' + fmt(pts[i][1]) + '\n30\n0\n';
    return t + '0\nSEQEND\n8\n' + layer + '\n';
  }
  function dxfRect(x, y, w, h, layer) { return dxfPoly([[x, y], [x + w, y], [x + w, y + h], [x, y + h]], layer); }
  // Drilling pattern (items as for the EPS), plate-millimetres with y down -> DXF with y up. Layers: PLATE, HOLES, SLOT.
  function dxfFromItems(items, plate) {
    var t = dxfStart();
    items.forEach(function (it) {
      if (it.kind === 'outline') t += dxfRect(0, 0, it.w, it.h, 'PLATE');
      else if (it.kind === 'circle') t += dxfCircle(it.x, plate.h - it.y, it.d / 2, 'HOLES');
      else if (it.kind === 'rect') t += dxfRect(it.x, plate.h - it.y - it.h, it.w, it.h, 'SLOT');
    });
    return t + dxfEnd();
  }
  // Letter outlines (closed polygons, y down) in an area of height h; layer LETTERS, optional bounding boxes on layer BOXES.
  function dxfFromPolys(polys, h, boxes) {
    var t = dxfStart();
    polys.forEach(function (p) { if (p.length >= 3) t += dxfPoly(p.map(function (q) { return [q[0], h - q[1]]; }), 'LETTERS'); });
    (boxes || []).forEach(function (b) { t += dxfRect(b.x, h - b.y - b.h, b.w, b.h, 'BOXES'); });
    return t + dxfEnd();
  }

  // Tiled 1:1 PDF of the drilling pattern. paper: 'A4' | 'A3', landscape tiles. Returns {bytes: Uint8Array-like string, pages}.
  var PAPER = { A4: [297, 210], A3: [420, 297] };
  // ref (optional): reference drawing under the drill pattern, so the sheet doubles as a layout drawing:
  //   {kind:'poly', loops:[[[x,y],...],...], fill:0..1, stroke:0..1} (even-odd fill) and {kind:'text', x, y, s, size} (centred).
  function pdfTiles(items, plate, paper, title, ref) {
    var pw = PAPER[paper][0], ph = PAPER[paper][1], margin = 10, tw = pw - 2 * margin, th = ph - 2 * margin;
    var cols = Math.max(1, Math.ceil(plate.w / tw - 1e-9)), rows = Math.max(1, Math.ceil(plate.h / th - 1e-9));
    var objs = [], pages = [];
    function add(s) { objs.push(s); return objs.length; }
    var K = MM_TO_PT;
    var r, c;
    for (r = 0; r < rows; r++) {
      for (c = 0; c < cols; c++) {
        var x0 = c * tw, y0 = r * th; // tile origin in plate mm (y down)
        var cs = 'q ' + fmt(margin * K) + ' ' + fmt(margin * K) + ' ' + fmt(tw * K) + ' ' + fmt(th * K) + ' re W n\n0.1 w\n';
        // transform plate mm (y down) to page points: X = margin + (x - x0), Y = margin + th - (y - y0) -> page y up
        var X = function (x) { return (margin + x - x0) * K; };
        var Yp = function (y) { return (margin + th - (y - y0)) * K; };
        (ref || []).forEach(function (it) {
          if (it.kind === 'poly') {
            cs += fmt(it.fill) + ' g ' + fmt(it.stroke) + ' G 0.2 w ';
            it.loops.forEach(function (lp) { lp.forEach(function (q, i) { cs += fmt(X(q[0])) + ' ' + fmt(Yp(q[1])) + (i ? ' l ' : ' m '); }); cs += 'h '; });
            cs += 'B*\n0.1 w\n';
          } else if (it.kind === 'text') {
            var size = it.size || 7, tw = pdfEscape(it.s).length * size * 0.5;
            cs += 'BT /F1 ' + fmt(size) + ' Tf 0.25 g ' + fmt(X(it.x) - tw / 2) + ' ' + fmt(Yp(it.y) - size * 0.35) + ' Td (' + pdfEscape(it.s) + ') Tj ET\n';
          }
        });
        items.forEach(function (it) {
          if (it.kind === 'outline') {
            cs += '0 0 1 RG ' + fmt(X(0)) + ' ' + fmt(Yp(it.h)) + ' ' + fmt(it.w * K) + ' ' + fmt(it.h * K) + ' re S\n';
          } else if (it.kind === 'rect') {
            cs += '0 0 0 RG ' + fmt(X(it.x)) + ' ' + fmt(Yp(it.y + it.h)) + ' ' + fmt(it.w * K) + ' ' + fmt(it.h * K) + ' re S\n';
          } else if (it.kind === 'circle') {
            var cx = X(it.x), cy = Yp(it.y), rr = it.d / 2 * K, q = 0.5522847498 * rr;
            cs += '0 0 0 RG ' + fmt(cx + rr) + ' ' + fmt(cy) + ' m ' +
              fmt(cx + rr) + ' ' + fmt(cy + q) + ' ' + fmt(cx + q) + ' ' + fmt(cy + rr) + ' ' + fmt(cx) + ' ' + fmt(cy + rr) + ' c ' +
              fmt(cx - q) + ' ' + fmt(cy + rr) + ' ' + fmt(cx - rr) + ' ' + fmt(cy + q) + ' ' + fmt(cx - rr) + ' ' + fmt(cy) + ' c ' +
              fmt(cx - rr) + ' ' + fmt(cy - q) + ' ' + fmt(cx - q) + ' ' + fmt(cy - rr) + ' ' + fmt(cx) + ' ' + fmt(cy - rr) + ' c ' +
              fmt(cx + q) + ' ' + fmt(cy - rr) + ' ' + fmt(cx + rr) + ' ' + fmt(cy - q) + ' ' + fmt(cx + rr) + ' ' + fmt(cy) + ' c S\n';
            // crosshair at the centre
            cs += fmt(cx - 2 * K) + ' ' + fmt(cy) + ' m ' + fmt(cx + 2 * K) + ' ' + fmt(cy) + ' l ' + fmt(cx) + ' ' + fmt(cy - 2 * K) + ' m ' + fmt(cx) + ' ' + fmt(cy + 2 * K) + ' l S\n';
          }
        });
        cs += 'Q\n';
        // tile frame and label
        cs += '0.6 G 0.3 w ' + fmt(margin * K) + ' ' + fmt(margin * K) + ' ' + fmt(tw * K) + ' ' + fmt(th * K) + ' re S\n';
        cs += 'BT /F1 8 Tf 0 g ' + fmt(margin * K) + ' ' + fmt((ph - margin + 3) * K) + ' Td (' + pdfEscape((title || 'Drill pattern') + ' - tile ' + (r + 1) + '/' + rows + ' row, ' + (c + 1) + '/' + cols +
          ' col - print at 100% (no scaling) - plate ' + fmt(plate.w) + ' x ' + fmt(plate.h) + ' mm') + ') Tj ET\n';
        pages.push(cs);
      }
    }
    // object numbers: 1 catalog, 2 pages, 3 font, then per page: content, page
    objs.push(''); objs.push(''); objs.push('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>');
    var kids = [];
    pages.forEach(function (cs) {
      var contentNo = add('<< /Length ' + cs.length + ' >>\nstream\n' + cs + 'endstream');
      var pageNo = add('<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ' + fmt(pw * K) + ' ' + fmt(ph * K) + '] /Contents ' + contentNo + ' 0 R /Resources << /Font << /F1 3 0 R >> >> >>');
      kids.push(pageNo + ' 0 R');
    });
    objs[0] = '<< /Type /Catalog /Pages 2 0 R >>';
    objs[1] = '<< /Type /Pages /Kids [' + kids.join(' ') + '] /Count ' + kids.length + ' >>';
    var out = '%PDF-1.4\n', offsets = [];
    objs.forEach(function (o, i) { offsets.push(out.length); out += (i + 1) + ' 0 obj\n' + o + '\nendobj\n'; });
    var xref = out.length;
    out += 'xref\n0 ' + (objs.length + 1) + '\n0000000000 65535 f \n';
    offsets.forEach(function (o) { out += ('0000000000' + o).slice(-10) + ' 00000 n \n'; });
    out += 'trailer\n<< /Size ' + (objs.length + 1) + ' /Root 1 0 R >>\nstartxref\n' + xref + '\n%%EOF\n';
    return { text: out, pages: pages.length, cols: cols, rows: rows };
  }
  function pdfEscape(s) { return String(s).replace(/[\\()]/g, '\\$&').replace(/[^\x20-\x7e]/g, '?'); }

  // ------------------------------------------------------------------ nesting of letters on a raw sheet

  // items: [{id, w, h}] (bounding boxes). Shelf packing, tallest first. Returns {placed:[{id,x,y}], fits, usedH}.
  function nestShelves(items, sheetW, sheetH, spacing) {
    var sorted = items.slice().sort(function (a, b) { return b.h - a.h; });
    var x = 0, y = 0, rowH = 0, placed = [], fits = true;
    sorted.forEach(function (it) {
      if (x > 0 && x + it.w > sheetW + 1e-9) { x = 0; y += rowH + spacing; rowH = 0; }
      if (it.w > sheetW + 1e-9) fits = false;
      placed.push({ id: it.id, x: x, y: y });
      x += it.w + spacing; rowH = Math.max(rowH, it.h);
      if (y + it.h > sheetH + 1e-9) fits = false;
    });
    return { placed: placed, fits: fits, usedH: y + rowH };
  }

  // ------------------------------------------------------------------ state, JSON

  function defaultState() {
    return {
      version: 1,
      plate: { w: 980, h: 300 },
      holeD: 10,
      minStroke: 3,
      frame: { layout: 'topbottom', unit: 100, stripWidth: 10 },
      texts: {
        casino: { text: 'CASINO', font: 'Limelight', weight: 400, x: 20, y: 20, h: 100, mode: 'uniform', cellW: 90, spacing: 6, thicken: 0, holeOffsets: [] },
        open: { text: 'OPEN', font: 'Limelight', weight: 400, x: 600, y: 20, h: 100, mode: 'uniform', cellW: 90, spacing: 6, thicken: 0, holeOffsets: [], sameFont: true },
        closed: { text: 'CLOSED', font: 'Limelight', weight: 400, x: 560, y: 190, h: 50, mode: 'natural', cellW: 45, spacing: 4, thicken: 0, holeOffsets: [], sameFont: true }
      },
      arrow: { x: 20, y: 150 },
      closedRound: { dx: 0, dy: 0 },
      closedSlot: { dx: 0, dy: 0, w: 74, h: 4 },
      strips: [],
      export: { closedHole: 'round', plateOutline: true, arrow: true, pdfRef: true, mirror: false, kerf: 0, sheetW: 600, sheetH: 300, spacing: 3, boxes: false, nest: false },
      ui: { dims: 'selected', snap: true, preview: false, grid: true }
    };
  }

  function num(v, lo, hi, dflt) { v = Number(v); if (!isFinite(v)) return dflt; return Math.min(hi, Math.max(lo, v)); }

  // Parses and checks a saved layout (JSON text); missing values come from the defaults. Throws on bad input.
  function fromJson(text) {
    var o = typeof text === 'string' ? JSON.parse(text) : text, d = defaultState(), s = defaultState();
    if (!o || typeof o !== 'object') throw new Error('Not a layout');
    if (o.version !== 1) throw new Error('Unknown layout version ' + o.version);
    if (o.plate) { s.plate.w = num(o.plate.w, 100, 980, d.plate.w); s.plate.h = num(o.plate.h, 100, 300, d.plate.h); }
    s.holeD = num(o.holeD, 5, 30, d.holeD);
    s.minStroke = num(o.minStroke, 0.5, 20, d.minStroke);
    if (o.frame) { s.frame.layout = o.frame.layout === 'sides' ? 'sides' : 'topbottom'; }
    ['casino', 'open', 'closed'].forEach(function (k) {
      var t = (o.texts && o.texts[k]) || {}, e = s.texts[k], dd = d.texts[k];
      e.text = typeof t.text === 'string' ? t.text.slice(0, 20) : dd.text;
      e.font = typeof t.font === 'string' && t.font ? t.font.slice(0, 80) : dd.font;
      e.weight = t.weight === 700 ? 700 : 400;
      e.x = num(t.x, -2000, 3000, dd.x); e.y = num(t.y, -2000, 3000, dd.y); e.h = num(t.h, 5, 600, dd.h);
      e.mode = t.mode === 'natural' ? 'natural' : (t.mode === 'uniform' ? 'uniform' : dd.mode);
      e.cellW = num(t.cellW, 5, 600, dd.cellW); e.spacing = num(t.spacing, -50, 100, dd.spacing); e.thicken = num(t.thicken, -5, 10, dd.thicken);
      e.holeOffsets = Array.isArray(t.holeOffsets) ? t.holeOffsets.slice(0, 20).map(function (h) { return h ? { dx: num(h.dx, -300, 300, 0), dy: num(h.dy, -300, 300, 0) } : { dx: 0, dy: 0 }; }) : [];
      if (k !== 'casino') e.sameFont = t.sameFont !== false;
    });
    if (o.arrow) { s.arrow.x = num(o.arrow.x, -2000, 3000, d.arrow.x); s.arrow.y = num(o.arrow.y, -2000, 3000, d.arrow.y); }
    if (o.closedRound) { s.closedRound.dx = num(o.closedRound.dx, -300, 300, 0); s.closedRound.dy = num(o.closedRound.dy, -300, 300, 0); }
    if (o.closedSlot) { s.closedSlot.dx = num(o.closedSlot.dx, -300, 300, 0); s.closedSlot.dy = num(o.closedSlot.dy, -300, 300, 0); s.closedSlot.w = num(o.closedSlot.w, 1, 300, 74); s.closedSlot.h = num(o.closedSlot.h, 1, 100, 4); }
    s.strips = Array.isArray(o.strips) ? o.strips.slice(0, 100).map(function (t) { return { x: num(t.x, -2000, 3000, 0), y: num(t.y, -2000, 3000, 0), rot: t.rot === 90 ? 90 : 0 }; }) : [];
    if (o.export) {
      var e2 = s.export, x = o.export;
      e2.closedHole = x.closedHole === 'slot' || x.closedHole === 'none' ? x.closedHole : 'round';
      e2.plateOutline = x.plateOutline !== false; e2.arrow = x.arrow !== false; e2.pdfRef = x.pdfRef !== false; e2.mirror = !!x.mirror; e2.boxes = !!x.boxes; e2.nest = !!x.nest;
      e2.kerf = num(x.kerf, -2, 2, 0); e2.sheetW = num(x.sheetW, 50, 3000, 600); e2.sheetH = num(x.sheetH, 50, 3000, 300); e2.spacing = num(x.spacing, 0, 50, 3);
    }
    if (o.ui) {
      s.ui.dims = ['selected', 'all', 'none'].indexOf(o.ui.dims) >= 0 ? o.ui.dims : 'selected';
      s.ui.snap = o.ui.snap !== false; s.ui.preview = !!o.ui.preview; s.ui.grid = o.ui.grid !== false;
    }
    return s;
  }

  function toJson(state) { return JSON.stringify(state, null, 2); }

  return {
    MM_TO_PT: MM_TO_PT, distanceTo: distanceTo, signedField: signedField, rasterize: rasterize, marching: marching, simplifyClosed: simplifyClosed,
    Glyph: Glyph, glyphFromPolygons: glyphFromPolygons, placeText: placeText, holeStatus: holeStatus, holeOffsetFor: holeOffsetFor,
    frameCounts: frameCounts, dimsForPoint: dimsForPoint, boxPoints: boxPoints, snapCentres: snapCentres, stripRect: stripRect, stripCoverage: stripCoverage,
    drillItems: drillItems, epsFromItems: epsFromItems, dxfFromItems: dxfFromItems, dxfFromPolys: dxfFromPolys, epsFromPolys: epsFromPolys, pdfTiles: pdfTiles, nestShelves: nestShelves,
    defaultState: defaultState, fromJson: fromJson, toJson: toJson
  };
});
