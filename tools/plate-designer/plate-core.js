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

  var MM_TO_PT_INV = 25.4 / 72;
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
        // extra holes of this letter: def.extraHoles [{i, dx, dy}] (offsets from the cell centre); k = index in that list
        cell.extras = [];
        (def.extraHoles || []).forEach(function (e, k) { if (e.i === i) cell.extras.push({ x: cell.cx + (e.dx || 0), y: cell.cy + (e.dy || 0), k: k }); });
      }
      cells.push(cell);
      x += cw;
    }
    return { x: def.x, y: def.y, w: x, h: def.h, cells: cells };
  }

  // Is the hole circle (radius r) fully behind the letter (grown by offsetMm)? clearance = how much room is left (mm).
  // hole (optional): another hole of the same cell (an extra hole); default is the main hole.
  function holeStatus(cell, r, offsetMm, hole) {
    var h = hole || cell.hole;
    if (!cell.glyph || !h) return { covered: false, clearance: -1e3 };
    var d = cell.glyph.insideDist(h.x - cell.gox, h.y - cell.goy, offsetMm);
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
  function frameCounts(W, H, layout, unit, sw, inside) {
    var fs = frameSides(W, H, layout, sw, inside);
    function side(len) { var n = Math.floor(len / unit + 1e-9); return { len: len, n: n, left: len - n * unit }; }
    var t = side(fs[0].len), b = side(fs[1].len), l = side(fs[2].len), r = side(fs[3].len);
    var total = t.n + b.n + l.n + r.n;
    return { top: t, bottom: b, left: l, right: r, totalUnits: total, totalLeds: total * 3 };
  }
  // The four strips of the frame in plate millimetres (y down), in the order top, bottom, left, right:
  // {x, y, len, horiz}. Outside the plate (default) the strips lie beyond the plate edge; inside they lie on the plate,
  // against its edge. layout 'topbottom': the top and bottom strips run over the corners, 'sides': the side strips do.
  function frameSides(W, H, layout, sw, inside) {
    var tb = layout !== 'sides', o = inside ? 0 : -sw, lenH, lenV, y0, x0;
    if (inside) { lenH = tb ? W : W - 2 * sw; lenV = tb ? H - 2 * sw : H; x0 = tb ? 0 : sw; y0 = tb ? sw : 0; }
    else { lenH = tb ? W + 2 * sw : W; lenV = tb ? H : H + 2 * sw; x0 = tb ? -sw : 0; y0 = tb ? 0 : -sw; }
    lenH = Math.max(0, lenH); lenV = Math.max(0, lenV);
    return [
      { x: x0, y: o, len: lenH, horiz: true },
      { x: x0, y: inside ? H - sw : H, len: lenH, horiz: true },
      { x: o, y: y0, len: lenV, horiz: false },
      { x: inside ? W - sw : W, y: y0, len: lenV, horiz: false }
    ];
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

  // ------------------------------------------------------------------ scaling a text box with its handles

  // Uniform scale factor of a box (centre cx, cy; size w0 x h0) when a handle is dragged to (px, py). The handle is
  // given by its direction (hx, hy) in -1/0/1: corners have both, sides only one. The centre stays where it is.
  function handleScale(hx, hy, cx, cy, w0, h0, px, py) {
    var dx = px - cx, dy = py - cy;
    if (hx !== 0 && hy !== 0) return Math.sqrt(dx * dx + dy * dy) / Math.sqrt(w0 * w0 / 4 + h0 * h0 / 4);
    if (hx !== 0) return Math.abs(dx) / (w0 / 2);
    return Math.abs(dy) / (h0 / 2);
  }
  // Snaps the dragged handle to the nearest line in xs (vertical lines, x values) or ys (horizontal lines, y values)
  // within thr mm by adjusting the scale factor. Returns {s, vertical, horizontal} (the line that was used, or null).
  function snapHandleScale(s, hx, hy, cx, cy, w0, h0, xs, ys, thr) {
    var best = null, i, d, ns, X = cx + hx * w0 * s / 2, Y = cy + hy * h0 * s / 2;
    if (hx !== 0) for (i = 0; i < xs.length; i++) {
      d = xs[i] - X; ns = 2 * hx * (xs[i] - cx) / w0;
      if (ns > 0 && Math.abs(d) <= thr && (best === null || Math.abs(d) < Math.abs(best.d))) best = { d: d, s: ns, v: xs[i], h: null };
    }
    if (hy !== 0) for (i = 0; i < ys.length; i++) {
      d = ys[i] - Y; ns = 2 * hy * (ys[i] - cy) / h0;
      if (ns > 0 && Math.abs(d) <= thr && (best === null || Math.abs(d) < Math.abs(best.d))) best = { d: d, s: ns, v: null, h: ys[i] };
    }
    return best ? { s: best.s, vertical: best.v, horizontal: best.h } : { s: s, vertical: null, horizontal: null };
  }
  // Allowed scale factors of a text (height 5-600, cell width 5-600, gap -50..100 mm), so the state stays valid.
  // Where to put one more hole in a letter: the point inside the letter (hole circle fully covered, 0.5 mm spare) that is
  // farthest from the holes already there (cell holes: [{x, y}], absolute mm). Sampled on a 1 mm grid over the cell.
  // Returns {dx, dy} relative to the cell centre and covered (false = no covered spot, the cell centre is used).
  function suggestHole(cell, r, offsetMm, holes) {
    var best = null, bestD = -1, x, y, min = 1e9;
    if (!cell.glyph) return { dx: 0, dy: 0, covered: false };
    for (x = cell.x; x <= cell.x + cell.w; x += 1) for (y = cell.y; y <= cell.y + cell.h; y += 1) {
      if (cell.glyph.insideDist(x - cell.gox, y - cell.goy, offsetMm) < r + 0.5) continue;
      min = 1e9;
      holes.forEach(function (h) { min = Math.min(min, Math.hypot(h.x - x, h.y - y)); });
      if (min > bestD + 1e-9) { bestD = min; best = { x: x, y: y }; }
    }
    if (!best) return { dx: 0, dy: 0, covered: false };
    return { dx: Math.round((best.x - cell.cx) * 100) / 100, dy: Math.round((best.y - cell.cy) * 100) / 100, covered: true };
  }
  function scaleLimits(t) {
    var lo = Math.max(5 / t.h, 5 / t.cellW), hi = Math.min(600 / t.h, 600 / t.cellW);
    if (t.spacing > 0) hi = Math.min(hi, 100 / t.spacing); else if (t.spacing < 0) hi = Math.min(hi, 50 / -t.spacing);
    return { min: lo, max: hi };
  }
  // The sizes of a text scaled uniformly by s (3 decimals): height, cell width, gap and the hole offsets.
  function scaledText(t, s) {
    function r(v) { return Math.round(v * 1000) / 1000; }
    return { h: r(t.h * s), cellW: r(t.cellW * s), spacing: r(t.spacing * s), holeOffsets: (t.holeOffsets || []).map(function (o) { return { dx: r(o.dx * s), dy: r(o.dy * s) }; }), extraHoles: (t.extraHoles || []).map(function (o) { return { i: o.i, dx: r(o.dx * s), dy: r(o.dy * s) }; }) };
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
      t.cells.forEach(function (c) { if (c.hole) items.push({ kind: 'circle', x: c.hole.x, y: c.hole.y, d: d, tag: key }); (c.extras || []).forEach(function (e) { items.push({ kind: 'circle', x: e.x, y: e.y, d: d, tag: key }); }); });
    });
    if (state.export.arrow && scene.arrow) {
      scene.arrow.holes.forEach(function (h) { items.push({ kind: 'circle', x: h.x, y: h.y, d: h.d, tag: 'arrow' }); });
    }
    var mode = state.export.closedHole;
    if (mode === 'round') items.push({ kind: 'circle', x: scene.closedRound.x, y: scene.closedRound.y, d: d, tag: 'closed' });
    if (mode === 'slot') items.push({ kind: 'rect', x: scene.closedSlot.x - scene.closedSlot.w / 2, y: scene.closedSlot.y - scene.closedSlot.h / 2, w: scene.closedSlot.w, h: scene.closedSlot.h, tag: 'closed' });
    return items;
  }

  // ---- EPS for Roland CutStudio: only m / l / c (absolute), no closepath, no fills, thin outlines, y up, points (1 pt = 25.4/72 mm),
  // lower-left corner of the sheet area = 0 0, fixed +-10000 pt bounding box, one extra "0 0 m" at the end (the reader skips the last line otherwise).
  var CUT_HEAD = ['%!PS-Adobe-3.0 EPSF-3.0', '%%LanguageLevel: 2', '%%BoundingBox: -10000 -10000 10000 10000', '%%EndComments', '%%BeginSetup', '%%EndSetup', '%%BeginProlog',
    '% Short operator names used by the path below (standard PostScript, one line each).', '50 dict begin', '/q { gsave } bind def', '/Q { grestore } bind def', '/w { setlinewidth } bind def',
    '/J { setlinecap } bind def', '/j { setlinejoin } bind def', '/M { setmiterlimit } bind def', '/d { setdash } bind def', '/g { setgray } bind def', '/m { moveto } bind def',
    '/l { lineto } bind def', '/c { curveto } bind def', '/h { closepath } bind def', '/S { stroke } bind def', '/n { newpath } bind def', '%%EndProlog', '%%Page: 1 1',
    '%%BeginPageSetup', '%%PageBoundingBox: -10000 -10000 10000 10000', '%%EndPageSetup', '% Cut lines only: thin outlines, no fills, no groups, no clipping.', 'q q', '0 g', '1.41732 w',
    '0 J', '0 j', '[] 0.0 d', '4 M q'].join('\n') + '\n';
  var CUT_TAIL = ['% CutStudio skips the last line unless one more moveto follows it:', '0 0 m', 'S Q', 'Q Q', 'showpage', '%%Trailer', 'end', '%%EOF'].join('\n') + '\n';
  function cutNum(v) {
    var t = (Math.round(v * 1000) / 1000).toFixed(3).replace(/0+$/, '').replace(/\.$/, '');
    return (t === '' || t === '-0') ? '0' : t;
  }
  // Paths are {cmds: [['m', x, y], ['l', x, y], ['c', x1, y1, x2, y2, x3, y3]]} in millimetres with y down.
  function polyPath(pts, closed) {
    var cmds = pts.map(function (q, i) { return [i ? 'l' : 'm', q[0], q[1]]; });
    if (closed && pts.length && (pts[0][0] !== pts[pts.length - 1][0] || pts[0][1] !== pts[pts.length - 1][1])) cmds.push(['l', pts[0][0], pts[0][1]]);
    return { cmds: cmds, pts: pts.length ? [pts[0]].concat(pts.slice(1)) : [] };
  }
  function rectPath(x, y, w, h) { return polyPath([[x, y], [x + w, y], [x + w, y + h], [x, y + h]], true); }
  function circlePath(cx, cy, r) {   // four cubic Beziers, 90 degrees each
    var k = 0.5522847498 * r;
    return { cmds: [['m', cx + r, cy], ['c', cx + r, cy + k, cx + k, cy + r, cx, cy + r], ['c', cx - k, cy + r, cx - r, cy + k, cx - r, cy], ['c', cx - r, cy - k, cx - k, cy - r, cx, cy - r], ['c', cx + k, cy - r, cx + r, cy - k, cx + r, cy]], pts: [[cx + r, cy], [cx, cy + r]] };
  }
  // paths shifted by (ox, oy) mm inside an area of height areaH mm (y down), written as the CutStudio EPS. Returns {text, warnings}.
  function cutEps(paths, areaH, ox, oy) {
    var out = [], warnings = [], K = MM_TO_PT, bad = false, n = 0;
    function P(x, y) { var X = (x + ox) * K, Y = (areaH - (y + oy)) * K; if (Math.abs(X) > 10000 || Math.abs(Y) > 10000) bad = true; return cutNum(X) + ' ' + cutNum(Y); }
    paths.forEach(function (p) {
      var distinct = {}, k = 0;
      (p.pts || []).forEach(function (q) { distinct[q[0] + ',' + q[1]] = 1; });
      Object.keys(distinct).forEach(function () { k++; });
      if (k < 2) { warnings.push('a path with fewer than two distinct points was skipped'); return; }
      n++;
      p.cmds.forEach(function (c) {
        if (c[0] === 'c') out.push(P(c[1], c[2]) + ' ' + P(c[3], c[4]) + ' ' + P(c[5], c[6]) + ' c');
        else out.push(P(c[1], c[2]) + ' ' + c[0]);
      });
    });
    if (!n) warnings.push('the drawing is empty (nothing to cut)');
    if (bad) warnings.push('part of the drawing is outside +-10000 pt (+-3527 mm) and will not import correctly');
    return { text: CUT_HEAD + (out.length ? out.join('\n') + '\n' : '') + CUT_TAIL, warnings: warnings };
  }

  // Drill pattern for CutStudio. The plate is centred in the sheet area (sheet w x h, at least the plate size).
  function epsFromItems(items, plate, title, sheet) {
    var aw = Math.max(plate.w, sheet ? sheet.w : 0), ah = Math.max(plate.h, sheet ? sheet.h : 0), ox = (aw - plate.w) / 2, oy = (ah - plate.h) / 2, paths = [];
    items.forEach(function (it) {
      if (it.kind === 'outline') paths.push(rectPath(0, 0, it.w, it.h));
      else if (it.kind === 'circle') paths.push(circlePath(it.x, it.y, it.d / 2));
      else if (it.kind === 'rect') paths.push(rectPath(it.x, it.y, it.w, it.h));
    });
    var r = cutEps(paths, ah, ox, oy);
    if (sheet && (sheet.w < plate.w || sheet.h < plate.h)) r.warnings.push('the plate is larger than the sheet: the area was enlarged to the plate size');
    return r;
  }

  // Closed polygons (letter outlines), polys in plate-like millimetres (y down), shifted by (ox, oy) inside an area of height areaH.
  function epsFromPolys(polys, areaH, ox, oy) {
    return cutEps(polys.map(function (p) { return polyPath(p, true); }), areaH, ox || 0, oy || 0);
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
  // plateCuts (optional): closed polygons of the plate edge, written on the layer PLATE_CUT.
  function dxfFromPolys(polys, h, boxes, plateCuts) {
    var t = dxfStart();
    polys.forEach(function (p) { if (p.length >= 3) t += dxfPoly(p.map(function (q) { return [q[0], h - q[1]]; }), 'LETTERS'); });
    (plateCuts || []).forEach(function (p) { t += dxfPoly(p.map(function (q) { return [q[0], h - q[1]]; }), 'PLATE_CUT'); });
    (boxes || []).forEach(function (b) { t += dxfRect(b.x, h - b.y - b.h, b.w, b.h, 'BOXES'); });
    return t + dxfEnd();
  }

  // Tiled 1:1 PDF of the drilling pattern. paper: 'A4' | 'A3', landscape tiles. Returns {bytes: Uint8Array-like string, pages}.
  var PAPER = { A4: [297, 210], A3: [420, 297] };
  // Dimension drawing for the PDF: for every hole (circle) and the centre of every slot (rect) the distances to all four
  // plate edges as thin dotted lines (from the hole edge to the plate edge) with the numbers written around the hole.
  // Returns reference items (kind 'line' and 'text') in plate millimetres, y down.
  function holeDimItems(items, plate) {
    var out = [], size = 6, green = [0, 0.45, 0.15];
    function r1(v) { return String(Math.round(v * 10) / 10); }
    function tw(t) { return t.length * 0.5 * size * MM_TO_PT_INV; }
    // (x, y) centre, rx/ry half sizes; edge = measure to the edges of the shape (slots) instead of to the centre (holes)
    function one(x, y, rx, ry, edge) {
      var L = edge ? x - rx : x, R = edge ? plate.w - x - rx : plate.w - x, U = edge ? y - ry : y, D = edge ? plate.h - y - ry : plate.h - y;
      var sl = r1(L), sr = r1(R), su = r1(U), sd = r1(D);
      function line(x1, y1, x2, y2) { out.push({ kind: 'line', x1: x1, y1: y1, x2: x2, y2: y2, rgb: green, w: 0.1, dash: 1 }); }
      line(0, y, x - rx, y); line(x + rx, y, plate.w, y); line(x, 0, x, y - ry); line(x, y + ry, x, plate.h);
      out.push({ kind: 'text', x: x - rx - 1 - tw(sl) / 2, y: y - 1.2, s: sl, size: size, rgb: green });
      out.push({ kind: 'text', x: x + rx + 1 + tw(sr) / 2, y: y - 1.2, s: sr, size: size, rgb: green });
      out.push({ kind: 'text', x: x, y: y - ry - 1.5, s: su, size: size, rgb: green });
      out.push({ kind: 'text', x: x, y: y + ry + 3.5, s: sd, size: size, rgb: green });
    }
    items.forEach(function (it) {
      if (it.kind === 'circle') one(it.x, it.y, it.d / 2, it.d / 2, false);
      else if (it.kind === 'rect') one(it.x + it.w / 2, it.y + it.h / 2, it.w / 2, it.h / 2, true);
    });
    return out;
  }

  // ref (optional): reference drawing under the drill pattern, so the sheet doubles as a layout drawing:
  //   {kind:'poly', loops:[[[x,y],...],...], fill:0..1, stroke:0..1} (even-odd fill) and {kind:'text', x, y, s, size} (centred).
  function pdfTiles(items, plate, paper, title, ref, extras) {
    var pw = PAPER[paper][0], ph = PAPER[paper][1], margin = 10, tw = pw - 2 * margin, th = ph - 2 * margin;
    var cols = Math.max(1, Math.ceil(plate.w / tw - 1e-9)), rows = items ? Math.max(1, Math.ceil(plate.h / th - 1e-9)) : 0;   // items null = only the extra pages
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
          } else if (it.kind === 'line') {
            cs += (it.rgb ? it.rgb.join(' ') + ' RG ' : '0 G ') + fmt(it.w || 0.2) + ' w ' + (it.dash ? '[' + fmt(it.dash) + ' ' + fmt(it.dash) + '] 0 d ' : '') +
              fmt(X(it.x1)) + ' ' + fmt(Yp(it.y1)) + ' m ' + fmt(X(it.x2)) + ' ' + fmt(Yp(it.y2)) + ' l S [] 0 d 0.1 w\n';
          } else if (it.kind === 'text') {
            var size = it.size || 7, tw = pdfEscape(it.s).length * size * 0.5;
            cs += 'BT /F1 ' + fmt(size) + ' Tf ' + (it.rgb ? it.rgb.join(' ') + ' rg ' : '0.25 g ') + ' ' + fmt(X(it.x) - tw / 2) + ' ' + fmt(Yp(it.y) - size * 0.35) + ' Td (' + pdfEscape(it.s) + ') Tj ET\n';
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
        pages.push({ cs: cs, w: pw, h: ph });
      }
    }
    (extras || []).forEach(function (e) { pages.push(e); });
    var tilePages = rows * cols;
    // object numbers: 1 catalog, 2 pages, 3 font, then per page: content, page
    objs.push(''); objs.push(''); objs.push('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>');
    var kids = [];
    pages.forEach(function (pg) {
      var cs = pg.cs, contentNo = add('<< /Length ' + cs.length + ' >>\nstream\n' + cs + 'endstream');
      var pageNo = add('<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ' + fmt(pg.w * K) + ' ' + fmt(pg.h * K) + '] /Contents ' + contentNo + ' 0 R /Resources << /Font << /F1 3 0 R >> >> >>');
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
    return { text: out, pages: pages.length, cols: cols, rows: rows, tilePages: tilePages };
  }
  function pdfEscape(s) { return String(s).replace(/[\\()]/g, '\\$&').replace(/[^\x20-\x7e]/g, '?'); }

  // ------------------------------------------------------------------ frame material (picture-frame border around the plate)

  // The plate edge is the inner edge of the frame; the four bars are mitred at 45 degrees.
  // Top/bottom bars: long edge = plate width + 2 x frame width, short edge = plate width; left/right likewise with the height.
  function frameBars(plate, fw) {
    fw = Number(fw) || 0;
    if (!(fw > 0)) return { width: 0, bars: [], inner: 2 * (plate.w + plate.h), outer: 2 * (plate.w + plate.h), material: 0, plateEdge: 2 * (plate.w + plate.h) };
    var bars = [
      { name: 'Top', long: plate.w + 2 * fw, short: plate.w },
      { name: 'Bottom', long: plate.w + 2 * fw, short: plate.w },
      { name: 'Left', long: plate.h + 2 * fw, short: plate.h },
      { name: 'Right', long: plate.h + 2 * fw, short: plate.h }
    ];
    var outer = 2 * (plate.w + plate.h) + 8 * fw;
    return { width: fw, bars: bars, inner: 2 * (plate.w + plate.h), outer: outer, material: outer, plateEdge: 2 * (plate.w + plate.h) };
  }

  // One A4 landscape page (297 x 210 mm) with the four mitred bars drawn to one common scale. Returns {w, h, cs} (PDF content stream).
  function frameSheet(plate, fw, title) {
    var W = 297, H = 210, M = 10, K = MM_TO_PT, fb = frameBars(plate, fw), cs = '';
    function T(x, y, str, size, centre) { // x, y in page mm, y down from the top
      var sz = size || 7, w = pdfEscape(str).length * sz * 0.5, px = x * K - (centre ? w / 2 : 0);
      return 'BT /F1 ' + fmt(sz) + ' Tf 0.15 g ' + fmt(px) + ' ' + fmt((H - y) * K - sz * 0.35) + ' Td (' + pdfEscape(str) + ') Tj ET\n';
    }
    cs += T(M, M - 2, (title || 'Frame') + ' - frame material, plate ' + fmt(plate.w) + ' x ' + fmt(plate.h) + ' mm', 9, false);
    if (!fb.bars.length) { cs += T(W / 2, H / 2, 'No frame (frame width 0 mm)', 12, true); return { w: W, h: H, cs: cs }; }
    var maxLong = Math.max(fb.bars[0].long, fb.bars[2].long), availW = W - 2 * M, availH = H - 2 * M - 22;
    var rowGap = 14, sc = Math.min(availW / maxLong, (availH / 4 - rowGap) / fw), thick = Math.max(fw * sc, 2), drawnThick = thick > fw * sc + 1e-9;
    var y = M + 6;
    fb.bars.forEach(function (b, i) {
      var L = b.long * sc, S2 = Math.max(b.short * sc, 0), x0 = M + (availW - L) / 2, off = (L - S2) / 2;
      var outerAtTop = (i % 2 === 0), ya = y, yb = y + thick;
      // trapezoid: long edge on the outer side (top for Top/Left, bottom for Bottom/Right)
      var pts = outerAtTop ? [[x0, ya], [x0 + L, ya], [x0 + L - off, yb], [x0 + off, yb]] : [[x0 + off, ya], [x0 + L - off, ya], [x0 + L, yb], [x0, yb]];
      cs += '0.93 g 0 G 0.4 w ';
      pts.forEach(function (q, k) { cs += fmt(q[0] * K) + ' ' + fmt((H - q[1]) * K) + (k ? ' l ' : ' m '); });
      cs += 'h B\n';
      cs += T(x0 + L / 2, yb + 5.5, b.name + ' bar: long edge ' + fmt(b.long) + ' mm, short edge ' + fmt(b.short) + ' mm, width ' + fmt(fw) + ' mm, 45 deg mitres', 7.5, true);
      y += thick + rowGap;
    });
    cs += T(M, H - M - 8, 'Plate edge (frame inner edge) ' + fmt(fb.inner) + ' mm; frame outer edge ' + fmt(fb.outer) + ' mm; material needed (sum of the long edges) ' + fmt(fb.material) + ' mm.', 8, false);
    cs += T(M, H - M - 2, 'Scale 1:' + fmt(1 / sc) + ' (all bars to the same scale' + (drawnThick ? '; width drawn at least 2 mm' : '') + '). Not for printing at 100%.', 7, false);
    return { w: W, h: H, cs: cs };
  }

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
      plate: { w: 600, h: 300 },
      frameWidth: 30,
      sameHeight: true,
      holeD: 10,
      minStroke: 3,
      frame: { layout: 'topbottom', unit: 100, stripWidth: 10, inside: false },
      texts: {
        casino: { text: 'CASINO', font: 'Limelight', weight: 400, x: 30, y: 10, h: 100, mode: 'uniform', cellW: 90, spacing: 6, thicken: 0, holeOffsets: [], extraHoles: [] },
        open: { text: 'OPEN', font: 'Limelight', weight: 400, x: 120, y: 112, h: 100, mode: 'uniform', cellW: 90, spacing: 6, thicken: 0, holeOffsets: [], extraHoles: [], sameFont: true },
        closed: { text: 'CLOSED', font: 'Limelight', weight: 400, x: 20, y: 245, h: 50, mode: 'natural', cellW: 45, spacing: 4, thicken: 0, holeOffsets: [], extraHoles: [], sameFont: true }
      },
      arrow: { x: 125, y: 198 },
      closedRound: { dx: 0, dy: 0 },
      closedSlot: { dx: 0, dy: 0, w: 74, h: 4 },
      strips: [],
      export: { closedHole: 'round', plateOutline: true, arrow: true, pdfRef: true, pdfDims: true, plateCut: false, mirror: false, kerf: 0, sheetW: 600, sheetH: 300, spacing: 3, boxes: false, nest: false },
      ui: { dims: 'selected', snap: true, preview: false, grid: true }
    };
  }

  function num(v, lo, hi, dflt) { v = Number(v); if (!isFinite(v)) return dflt; return Math.min(hi, Math.max(lo, v)); }

  // Parses and checks a saved layout (JSON text); missing values come from the defaults. Throws on bad input.
  function fromJson(text) {
    var o = typeof text === 'string' ? JSON.parse(text) : text, d = defaultState(), s = defaultState();
    if (!o || typeof o !== 'object') throw new Error('Not a layout');
    if (o.version !== 1) throw new Error('Unknown layout version ' + o.version);
    s.sameHeight = o.sameHeight !== false;
    s.frameWidth = num(o.frameWidth, 0, 100, d.frameWidth);
    if (o.plate) { s.plate.w = num(o.plate.w, 1, 1000, d.plate.w); s.plate.h = num(o.plate.h, 1, 1000, d.plate.h); }
    s.holeD = num(o.holeD, 5, 30, d.holeD);
    s.minStroke = num(o.minStroke, 0.5, 20, d.minStroke);
    if (o.frame) { s.frame.layout = o.frame.layout === 'sides' ? 'sides' : 'topbottom'; s.frame.inside = !!o.frame.inside; }
    ['casino', 'open', 'closed'].forEach(function (k) {
      var t = (o.texts && o.texts[k]) || {}, e = s.texts[k], dd = d.texts[k];
      e.text = typeof t.text === 'string' ? t.text.slice(0, 20) : dd.text;
      e.font = typeof t.font === 'string' && t.font ? t.font.slice(0, 80) : dd.font;
      e.weight = t.weight === 700 ? 700 : 400;
      e.x = num(t.x, -2000, 3000, dd.x); e.y = num(t.y, -2000, 3000, dd.y); e.h = num(t.h, 5, 600, dd.h);
      e.mode = t.mode === 'natural' ? 'natural' : (t.mode === 'uniform' ? 'uniform' : dd.mode);
      e.cellW = num(t.cellW, 5, 600, dd.cellW); e.spacing = num(t.spacing, -50, 100, dd.spacing); e.thicken = num(t.thicken, -5, 10, dd.thicken);
      e.holeOffsets = Array.isArray(t.holeOffsets) ? t.holeOffsets.slice(0, 20).map(function (h) { return h ? { dx: num(h.dx, -300, 300, 0), dy: num(h.dy, -300, 300, 0) } : { dx: 0, dy: 0 }; }) : [];
      e.extraHoles = Array.isArray(t.extraHoles) ? t.extraHoles.slice(0, 40).filter(function (h) { return h && isFinite(h.i); }).map(function (h) { return { i: Math.max(0, Math.min(19, Math.round(h.i))), dx: num(h.dx, -300, 300, 0), dy: num(h.dy, -300, 300, 0) }; }) : [];
      if (k !== 'casino') e.sameFont = t.sameFont !== false;
    });
    if (o.arrow) { s.arrow.x = num(o.arrow.x, -2000, 3000, d.arrow.x); s.arrow.y = num(o.arrow.y, -2000, 3000, d.arrow.y); }
    if (o.closedRound) { s.closedRound.dx = num(o.closedRound.dx, -300, 300, 0); s.closedRound.dy = num(o.closedRound.dy, -300, 300, 0); }
    if (o.closedSlot) { s.closedSlot.dx = num(o.closedSlot.dx, -300, 300, 0); s.closedSlot.dy = num(o.closedSlot.dy, -300, 300, 0); s.closedSlot.w = num(o.closedSlot.w, 1, 300, 74); s.closedSlot.h = num(o.closedSlot.h, 1, 100, 4); }
    s.strips = Array.isArray(o.strips) ? o.strips.slice(0, 100).map(function (t) { return { x: num(t.x, -2000, 3000, 0), y: num(t.y, -2000, 3000, 0), rot: t.rot === 90 ? 90 : 0 }; }) : [];
    if (o.export) {
      var e2 = s.export, x = o.export;
      e2.closedHole = x.closedHole === 'slot' || x.closedHole === 'none' ? x.closedHole : 'round';
      e2.plateOutline = x.plateOutline !== false; e2.arrow = x.arrow !== false; e2.pdfRef = x.pdfRef !== false; e2.pdfDims = x.pdfDims !== false; e2.plateCut = !!x.plateCut; e2.mirror = !!x.mirror; e2.boxes = !!x.boxes; e2.nest = !!x.nest;
      e2.kerf = num(x.kerf, -2, 2, 0); e2.sheetW = num(x.sheetW, 50, 3000, 600); e2.sheetH = num(x.sheetH, 50, 3000, 300); e2.spacing = num(x.spacing, 0, 50, 3);
    }
    if (o.ui) {
      s.ui.dims = ['selected', 'all', 'none'].indexOf(o.ui.dims) >= 0 ? o.ui.dims : 'selected';
      s.ui.snap = o.ui.snap !== false; s.ui.preview = !!o.ui.preview; s.ui.grid = o.ui.grid !== false;
    }
    return s;
  }

  // Resizes the plate around its centre: both opposite edges move by half the change, so everything on the plate
  // (texts, arrow, strip pieces) is shifted by half of the width / height change and keeps its place relative to the
  // plate centre. w or h undefined = unchanged. Width and height 1-1000 mm. Mutates state.
  function resizePlate(state, w, h) {
    function r2(v) { return Math.round(v * 100) / 100; }
    var nw = w === undefined ? state.plate.w : Math.max(1, Math.min(1000, w)), nh = h === undefined ? state.plate.h : Math.max(1, Math.min(1000, h));
    var dx = (nw - state.plate.w) / 2, dy = (nh - state.plate.h) / 2;
    state.plate.w = nw; state.plate.h = nh;
    if (!dx && !dy) return state;
    ['casino', 'open', 'closed'].forEach(function (k) { var t = state.texts[k]; t.x = r2(t.x + dx); t.y = r2(t.y + dy); });
    state.arrow.x = r2(state.arrow.x + dx); state.arrow.y = r2(state.arrow.y + dy);
    state.strips.forEach(function (q) { q.x = r2(q.x + dx); q.y = r2(q.y + dy); });
    return state;
  }
  // How far a scrolling panel must scroll so that a section is fully visible with a margin of 1/5 of the panel height
  // above and below it (when it fits). top/bottom: the section's edges relative to the top of the panel's visible area,
  // viewH: the visible height. Fully visible already: 0 (no scrolling). Positive = scroll down. If the section plus both
  // margins is taller than the panel, its top edge is put at the margin.
  function scrollDelta(top, bottom, viewH) {
    if (top >= 0 && bottom <= viewH) return 0;
    var m = viewH / 5;
    if (bottom - top + 2 * m > viewH) return top - m;
    if (top < 0) return top - m;
    return bottom - (viewH - m);
  }
  // ------------------------------------------------------------------ selecting several texts (group)
  var TEXT_KEYS = ['casino', 'open', 'closed'];
  // Bounding box {x, y, w, h} of several boxes.
  function unionBox(boxes) {
    var x0 = 1e18, y0 = 1e18, x1 = -1e18, y1 = -1e18;
    boxes.forEach(function (b) { x0 = Math.min(x0, b.x); y0 = Math.min(y0, b.y); x1 = Math.max(x1, b.x + b.w); y1 = Math.max(y1, b.y + b.h); });
    return { x: x0, y: y0, w: x1 - x0, h: y1 - y0 };
  }
  // True when two boxes overlap or touch (rubber band selection).
  function boxesTouch(a, b) { return a.x <= b.x + b.w && b.x <= a.x + a.w && a.y <= b.y + b.h && b.y <= a.y + a.h; }
  // New selection (keys in the order casino, open, closed) from the current one and the keys under a rubber band:
  // mode 'replace' = the band keys, 'add' = both, 'remove' = current without the band keys.
  function bandSelection(current, hit, mode) {
    return TEXT_KEYS.filter(function (k) {
      var c = current.indexOf(k) >= 0, h = hit.indexOf(k) >= 0;
      return mode === 'add' ? c || h : mode === 'remove' ? c && !h : h;
    });
  }
  // Allowed uniform scale factors of a group of texts (snapshots with h, cellW, spacing): the range common to all of them.
  function groupScaleLimits(texts) {
    var lo = 0, hi = 1e18;
    texts.forEach(function (t) { var l = scaleLimits(t); lo = Math.max(lo, l.min); hi = Math.min(hi, l.max); });
    return { min: lo, max: hi };
  }
  // Where the centre of a box ends up when the group is scaled by s around the group centre g.
  function groupCentre(c, g, s) { return { x: g.x + s * (c.x - g.x), y: g.y + s * (c.y - g.y) }; }
  function toJson(state) { return JSON.stringify(state, null, 2); }

  return {
    MM_TO_PT: MM_TO_PT, distanceTo: distanceTo, signedField: signedField, rasterize: rasterize, marching: marching, simplifyClosed: simplifyClosed,
    Glyph: Glyph, glyphFromPolygons: glyphFromPolygons, placeText: placeText, holeStatus: holeStatus, suggestHole: suggestHole, holeOffsetFor: holeOffsetFor,
    frameCounts: frameCounts, unionBox: unionBox, boxesTouch: boxesTouch, bandSelection: bandSelection, groupScaleLimits: groupScaleLimits, groupCentre: groupCentre, scrollDelta: scrollDelta, resizePlate: resizePlate, frameSides: frameSides, dimsForPoint: dimsForPoint, boxPoints: boxPoints, snapCentres: snapCentres, handleScale: handleScale, snapHandleScale: snapHandleScale, scaleLimits: scaleLimits, scaledText: scaledText, stripRect: stripRect, stripCoverage: stripCoverage,
    drillItems: drillItems, epsFromItems: epsFromItems, cutEps: cutEps, polyPath: polyPath, circlePath: circlePath, cutNum: cutNum, dxfFromItems: dxfFromItems, holeDimItems: holeDimItems, dxfFromPolys: dxfFromPolys, epsFromPolys: epsFromPolys, pdfTiles: pdfTiles, frameBars: frameBars, frameSheet: frameSheet, nestShelves: nestShelves,
    defaultState: defaultState, fromJson: fromJson, toJson: toJson
  };
});
