// Plate Designer - browser app for the CASINO OPEN front plate (drill pattern, letter cut, LED strip frame).
// Geometry and file writers live in plate-core.js (tested in tests/plate_designer_core_test.js).
(function () {
  'use strict';
  var core = PlateCore, ARROW = ARROW_DATA;
  var S = core.defaultState();
  var sel = { type: 'casino', idx: 0 };           // casino | open | closed | arrow | strip
  var view = { cx: 490, cy: 150, k: 1 };           // centre (mm) and pixels per mm
  var svg = document.getElementById('svg');
  var KEYS = ['casino', 'open', 'closed'], NAMES = { casino: 'CASINO', open: 'OPEN', closed: 'CLOSED' };
  var LS_AUTO = 'plateDesigner.autosave', LS_LAYOUTS = 'plateDesigner.layouts';
  var PPM = 4, PPM_EXPORT = 10;
  function $(id) { return document.getElementById(id); }
  function cl(v, lo, hi) { return Math.min(hi, Math.max(lo, v)); }
  function f2(n) { return (Math.round(n * 100) / 100).toString(); }

  // ------------------------------------------------------------------ fonts
  var FONT_GROUPS = [
    ['1920s / Art Deco', ['Limelight', 'Poiret One!', 'Federo', 'Monoton', 'Gruppo!', 'Josefin Sans', 'Josefin Slab', 'Italiana!', 'Marcellus', 'Cinzel', 'Cinzel Decorative', 'Forum', 'Bellefair!', 'Julius Sans One!', 'Megrim!', 'Syncopate', 'Tenor Sans', 'Fascinate', 'Fascinate Inline', 'Faster One', 'Bungee Inline', 'Bungee Shade', 'Diplomata', 'Diplomata SC', 'Rozha One', 'Abril Fatface', 'Playfair Display', 'Playfair Display SC', 'Bodoni Moda', 'Libre Bodoni', 'Yeseva One', 'Elsie', 'Elsie Swash Caps', 'Emblema One', 'Vast Shadow', 'Ribeye Marrow', 'Chonburi', 'Almendra Display', 'Gravitas One', 'Stint Ultra Expanded']],
    ['Bold and clear', ['Anton', 'Bebas Neue', 'Oswald', 'League Gothic', 'Fjalla One', 'Alfa Slab One', 'Bevan', 'Ultra', 'Bungee', 'Righteous', 'Archivo Black', 'Black Ops One', 'Passion One', 'Russo One', 'Squada One', 'Titan One', 'Chango', 'Fugaz One', 'Paytone One', 'Lilita One', 'Bowlby One', 'Bowlby One SC', 'Carter One', 'Changa One', 'Luckiest Guy', 'Concert One', 'Francois One', 'Big Shoulders Display', 'Staatliches', 'Teko', 'Unica One', 'Krona One', 'Rubik Mono One', 'Holtwood One SC', 'Sigmar One', 'Rammetto One', 'Shrikhand', 'Stint Ultra Condensed', 'Londrina Solid', 'Black Han Sans']],
    ['Sans and serif', ['Montserrat', 'Poppins', 'Raleway', 'Lato', 'Quicksand!', 'Comfortaa', 'Audiowide', 'Orbitron', 'Michroma', 'Arvo', 'Rye', 'Sancreek', 'Ewert', 'Ranchers', 'Slackey', 'Special Elite', 'Kelly Slab', 'Cabin Sketch', 'Smokum', 'Ribeye']],
    ['Script', ['Lobster', 'Lobster Two', 'Pacifico', 'Berkshire Swash', 'Kaushan Script', 'Yellowtail', 'Sacramento!', 'Playball', 'Cherry Swash', 'Oleo Script', 'Spicy Rice', 'Wellfleet', 'Bigshot One']]
  ];
  var fontState = {};
  var meas = document.createElement('canvas').getContext('2d');
  function widthOf(spec) { meas.font = '72px ' + spec; return meas.measureText('MWmwiI0123456789 CASINO OPEN').width; }
  function fontAvailable(name) {
    return widthOf('"' + name + '", monospace') !== widthOf('monospace') || widthOf('"' + name + '", serif') !== widthOf('serif');
  }
  function ensureFont(name) {
    if (fontState[name]) return fontState[name].promise;
    var st = fontState[name] = { status: 'loading' };
    st.promise = new Promise(function (resolve) {
      var done = false;
      function finish() {
        if (done) return; done = true;
        Promise.all([document.fonts.load('400 40px "' + name + '"'), document.fonts.load('700 40px "' + name + '"')]).catch(function () { }).then(function () {
          st.status = fontAvailable(name) ? 'ok' : 'missing';
          clearGlyphs(name); render(); updateInfo(); resolve(st.status);
        });
      }
      var l = document.createElement('link');
      l.rel = 'stylesheet'; l.onload = finish; l.onerror = finish;
      l.href = 'https://fonts.googleapis.com/css?family=' + encodeURIComponent(name).replace(/%20/g, '+') + ':400,700&display=swap';
      document.head.appendChild(l);
      setTimeout(finish, 6000);
    });
    return st.promise;
  }

  // ------------------------------------------------------------------ glyphs (canvas -> mask)
  var glyphCache = {}, capCache = {}, contourCache = typeof WeakMap === 'function' ? new WeakMap() : null;
  var gcan = document.createElement('canvas'), gctx = gcan.getContext('2d', { willReadFrequently: true });
  function clearGlyphs(font) {
    Object.keys(glyphCache).forEach(function (k) { if (!font || k.indexOf(font + '|') === 0) delete glyphCache[k]; });
    Object.keys(capCache).forEach(function (k) { if (!font || k.indexOf(font + '|') === 0) delete capCache[k]; });
  }
  function capRatio(font, weight) {
    var key = font + '|' + weight;
    if (!capCache[key]) { gctx.font = weight + ' 200px "' + font + '"'; capCache[key] = Math.max(0.2, gctx.measureText('H').actualBoundingBoxAscent / 200); }
    return capCache[key];
  }
  function getGlyph(font, weight, ch, hMm, ppm) {
    var key = font + '|' + weight + '|' + ch + '|' + hMm + '|' + ppm;
    if (key in glyphCache) return glyphCache[key];
    if (Object.keys(glyphCache).length > 600) glyphCache = {};
    var px = hMm * ppm / capRatio(font, weight);
    gctx.font = weight + ' ' + px + 'px "' + font + '"';
    var m = gctx.measureText(ch);
    var pad = Math.ceil(12 * ppm), left = Math.ceil(m.actualBoundingBoxLeft), right = Math.ceil(m.actualBoundingBoxRight), asc = Math.ceil(m.actualBoundingBoxAscent), desc = Math.ceil(m.actualBoundingBoxDescent);
    var w = left + right + 2 * pad + 2, h = asc + desc + 2 * pad + 2, ox = pad + left + 1, oy = pad + asc + 1;
    gcan.width = w; gcan.height = h;
    gctx.font = weight + ' ' + px + 'px "' + font + '"'; gctx.textBaseline = 'alphabetic'; gctx.textAlign = 'left'; gctx.fillStyle = '#000';
    gctx.fillText(ch, ox, oy);
    var data = gctx.getImageData(0, 0, w, h).data, mask = new Uint8Array(w * h), any = false, i;
    for (i = 0; i < w * h; i++) if (data[i * 4 + 3] >= 128) { mask[i] = 1; any = true; }
    var g = any ? new core.Glyph({ mask: mask, w: w, h: h, ppm: ppm, ox: ox, oy: oy, advanceMm: m.width / ppm }) : null;
    glyphCache[key] = g;
    return g;
  }
  function contours(g, offset, tol) {
    var key = offset + ':' + tol, e = contourCache && contourCache.get(g);
    if (!e) { e = {}; if (contourCache) contourCache.set(g, e); }
    if (!e[key]) e[key] = g.contours(offset, tol);
    return e[key];
  }

  // ------------------------------------------------------------------ scene
  function effFont(key) {
    var t = S.texts[key];
    if (key !== 'casino' && t.sameFont) return { font: S.texts.casino.font, weight: S.texts.casino.weight };
    return { font: t.font, weight: t.weight };
  }
  function textScene(key, ppm) {
    var t = S.texts[key], f = effFont(key);
    var glyphs = Array.from(t.text).map(function (ch) { return ch.trim() ? getGlyph(f.font, f.weight, ch, t.h, ppm) : null; });
    return core.placeText(t, glyphs, key !== 'closed');
  }
  function scene(ppm) {
    var sc = { casino: textScene('casino', ppm), open: textScene('open', ppm), closed: textScene('closed', ppm) };
    var cb = sc.closed;
    sc.arrow = { x: S.arrow.x, y: S.arrow.y, w: ARROW.w, h: ARROW.h, holes: ARROW.sticks.filter(function (s) { return s.hole; }).map(function (s) { return { x: S.arrow.x + s.hole[0], y: S.arrow.y + s.hole[1], d: ARROW.holeD }; }) };
    sc.closedRound = { x: cb.x + cb.w / 2 + S.closedRound.dx, y: cb.y + cb.h / 2 + S.closedRound.dy };
    sc.closedSlot = { x: cb.x + cb.w / 2 + S.closedSlot.dx, y: cb.y + cb.h / 2 + S.closedSlot.dy, w: S.closedSlot.w, h: S.closedSlot.h };
    return sc;
  }
  function boxOf(sc, key) {
    if (key === 'arrow') return { x: sc.arrow.x, y: sc.arrow.y, w: ARROW.w, h: ARROW.h };
    return { x: sc[key].x, y: sc[key].y, w: sc[key].w, h: sc[key].h };
  }
  var cur = null; // last scene used for drawing / hit tests

  // ------------------------------------------------------------------ view
  function setViewBox() {
    var w = svg.clientWidth / view.k, h = svg.clientHeight / view.k;
    svg.setAttribute('viewBox', (view.cx - w / 2) + ' ' + (view.cy - h / 2) + ' ' + w + ' ' + h);
  }
  function fit() {
    var sw = S.frame.stripWidth, W = S.plate.w + 2 * sw + 60, H = S.plate.h + 2 * sw + 60;
    view.k = Math.min(svg.clientWidth / W, svg.clientHeight / H) || 1;
    view.cx = S.plate.w / 2; view.cy = S.plate.h / 2; render();
  }
  function toMm(e) {
    var r = svg.getBoundingClientRect();
    return { x: view.cx + (e.clientX - r.left - r.width / 2) / view.k, y: view.cy + (e.clientY - r.top - r.height / 2) / view.k };
  }
  function zoomAt(factor) { view.k = cl(view.k * factor, 0.2, 60); render(); }

  // ------------------------------------------------------------------ drawing
  function pathOf(polys, dx, dy) {
    var s = '';
    polys.forEach(function (p) {
      for (var i = 0; i < p.length; i++) s += (i ? 'L' : 'M') + f2(p[i][0] + dx) + ' ' + f2(p[i][1] + dy);
      s += 'Z';
    });
    return s;
  }
  function frameSvg(preview) {
    var W = S.plate.w, H = S.plate.h, sw = S.frame.stripWidth, unit = S.frame.unit, fc = core.frameCounts(W, H, S.frame.layout, unit, sw), s = '';
    var tb = S.frame.layout === 'topbottom';
    var sides = [
      { x: tb ? -sw : 0, y: -sw, len: tb ? W + 2 * sw : W, horiz: true, info: fc.top },
      { x: tb ? -sw : 0, y: H, len: tb ? W + 2 * sw : W, horiz: true, info: fc.bottom },
      { x: -sw, y: tb ? 0 : -sw, len: tb ? H : H + 2 * sw, horiz: false, info: fc.left },
      { x: W, y: tb ? 0 : -sw, len: tb ? H : H + 2 * sw, horiz: false, info: fc.right }
    ];
    sides.forEach(function (sd, si) {
      var w = sd.horiz ? sd.len : sw, h = sd.horiz ? sw : sd.len;
      s += '<rect x="' + sd.x + '" y="' + sd.y + '" width="' + w + '" height="' + h + '" fill="' + (preview ? '#000' : '#4a4a4a') + '" stroke="#222" stroke-width="' + 0.4 / view.k * 2 + '"/>';
      var n = sd.info.n, i, j;
      for (i = 0; i < n; i++) {
        var hue = (si * 70 + i * 47) % 360, col = 'hsl(' + hue + ',90%,58%)';
        for (j = 0; j < 3; j++) {
          var t = i * unit + unit * (1 + 2 * j) / 6;
          var cx = sd.horiz ? sd.x + t : sd.x + sw / 2, cy = sd.horiz ? sd.y + sw / 2 : sd.y + t;
          s += '<circle cx="' + f2(cx) + '" cy="' + f2(cy) + '" r="2.4" fill="' + col + '"' + (preview ? ' filter="url(#glow)"' : '') + '/>';
        }
        if (i > 0) { var b = i * unit; s += sd.horiz ? '<line x1="' + (sd.x + b) + '" y1="' + sd.y + '" x2="' + (sd.x + b) + '" y2="' + (sd.y + sw) + '" stroke="#999" stroke-width="' + 0.5 / view.k + '"/>' : '<line x1="' + sd.x + '" y1="' + (sd.y + b) + '" x2="' + (sd.x + sw) + '" y2="' + (sd.y + b) + '" stroke="#999" stroke-width="' + 0.5 / view.k + '"/>'; }
      }
      if (sd.info.left > 0.05) {
        var o = n * unit;
        s += sd.horiz ? '<rect x="' + (sd.x + o) + '" y="' + sd.y + '" width="' + sd.info.left + '" height="' + sw + '" fill="url(#hatch)"/>' : '<rect x="' + sd.x + '" y="' + (sd.y + o) + '" width="' + sw + '" height="' + sd.info.left + '" fill="url(#hatch)"/>';
      }
    });
    return s;
  }
  function gridSvg() {
    var W = S.plate.w, H = S.plate.h, px = 1 / view.k, steps = [1, 2, 5, 10, 20, 50, 100, 200], st = 100, i, s = '';
    for (i = 0; i < steps.length; i++) if (steps[i] * view.k >= 9) { st = steps[i]; break; }
    var major = st * 5 >= 50 ? st * 5 : st * 10, x, y;
    for (x = st; x < W; x += st) s += '<line x1="' + x + '" y1="0" x2="' + x + '" y2="' + H + '" stroke="' + (x % major === 0 ? '#bbb' : '#e2dcc8') + '" stroke-width="' + px * 0.7 + '"/>';
    for (y = st; y < H; y += st) s += '<line x1="0" y1="' + y + '" x2="' + W + '" y2="' + y + '" stroke="' + (y % major === 0 ? '#bbb' : '#e2dcc8') + '" stroke-width="' + px * 0.7 + '"/>';
    for (x = 0; x <= W; x += major) s += '<text x="' + (x + 1 * px) + '" y="' + 10 * px + '" font-size="' + 9 * px + '" fill="#666">' + x + '</text>';
    for (y = major; y <= H; y += major) s += '<text x="' + 2 * px + '" y="' + (y - 2 * px) + '" font-size="' + 9 * px + '" fill="#666">' + y + '</text>';
    return s;
  }
  function dimSvg(points) {
    var W = S.plate.w, H = S.plate.h, px = 1 / view.k, s = '', fs = 10 * px;
    function line(x1, y1, x2, y2, v) {
      if (v < 0.5) return '';
      return '<line x1="' + f2(x1) + '" y1="' + f2(y1) + '" x2="' + f2(x2) + '" y2="' + f2(y2) + '" stroke="#0a7a3c" stroke-width="' + px + '" marker-start="url(#arr)" marker-end="url(#arr)"/>' +
        '<text x="' + f2((x1 + x2) / 2) + '" y="' + f2((y1 + y2) / 2) + '" font-size="' + fs + '" fill="#0a5" text-anchor="middle" paint-order="stroke" stroke="#fff" stroke-width="' + 3 * px + '">' + f2(v) + '</text>';
    }
    points.forEach(function (p) {
      var d = core.dimsForPoint(p.x, p.y, W, H);
      s += line(p.x, p.y, 0, p.y, d.left) + line(p.x, p.y, W, p.y, d.right) + line(p.x, p.y, p.x, 0, d.up) + line(p.x, p.y, p.x, H, d.down);
      s += '<circle cx="' + p.x + '" cy="' + p.y + '" r="' + 2 * px + '" fill="#0a7a3c"/>';
    });
    return s;
  }
  function render() {
    if (!svg.clientWidth) return;
    setViewBox();
    var sc = scene(PPM), pv = S.ui.preview, W = S.plate.w, H = S.plate.h, px = 1 / view.k, s = '';
    cur = sc;
    s += '<defs><marker id="arr" viewBox="0 0 6 6" refX="3" refY="3" markerWidth="5" markerHeight="5" orient="auto-start-reverse"><path d="M0 0L6 3L0 6z" fill="#0a7a3c"/></marker>' +
      '<pattern id="hatch" width="4" height="4" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="4" height="4" fill="#777"/><line x1="0" y1="0" x2="0" y2="4" stroke="#bbb" stroke-width="1.5"/></pattern>' +
      '<filter id="glow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="3" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter></defs>';
    s += frameSvg(pv);
    s += '<rect x="0" y="0" width="' + W + '" height="' + H + '" fill="' + (pv ? '#101010' : '#f3efe0') + '" stroke="#000" stroke-width="' + px + '"/>';
    if (S.ui.grid && !pv) s += gridSvg();
    // strip pieces (behind the letters)
    S.strips.forEach(function (t, i) {
      var r = core.stripRect(t), on = sel.type === 'strip' && sel.idx === i;
      s += '<rect x="' + r.x + '" y="' + r.y + '" width="' + r.w + '" height="' + r.h + '" fill="' + (pv ? '#ffd27a' : '#6a8dff') + '" fill-opacity="' + (pv ? 0.9 : 0.55) + '" stroke="' + (on ? '#d00' : '#1f3fb0') + '" stroke-width="' + (on ? 2 : 1) * px + '"' + (pv ? ' filter="url(#glow)"' : '') + '/>';
    });
    // arrow
    ARROW.sticks.forEach(function (st) {
      var pts = st.pts.map(function (q) { return (q[0] + S.arrow.x) + ',' + (q[1] + S.arrow.y); }).join(' ');
      s += '<polygon points="' + pts + '" fill="' + (pv ? '#fff' : st.fill) + '" fill-opacity="' + (pv ? 0.85 : 0.8) + '" stroke="' + st.stroke + '" stroke-width="' + 0.5 * px + '"' + (pv ? ' filter="url(#glow)"' : '') + '/>';
    });
    // letters
    var letterFill = pv ? '#ff2a2a' : '#b01030';
    KEYS.forEach(function (key) {
      var t = S.texts[key], poly = [];
      sc[key].cells.forEach(function (c) { if (c.glyph) poly.push(pathOf(contours(c.glyph, t.thicken, 0.05), c.gox, c.goy)); });
      s += '<path d="' + poly.join('') + '" fill="' + letterFill + '" fill-opacity="' + (pv ? 1 : 0.82) + '" fill-rule="evenodd"' + (pv ? ' filter="url(#glow)"' : '') + '/>';
    });
    if (!pv) {
      // holes
      ARROW.sticks.forEach(function (st) { if (st.hole) s += '<circle cx="' + (st.hole[0] + S.arrow.x) + '" cy="' + (st.hole[1] + S.arrow.y) + '" r="' + ARROW.holeD / 2 + '" fill="#fff" stroke="#333" stroke-width="' + px + '"/>'; });
      ['casino', 'open'].forEach(function (key) {
        sc[key].cells.forEach(function (c) {
          if (!c.hole) return;
          var ok = core.holeStatus(c, S.holeD / 2, S.texts[key].thicken).covered;
          s += '<circle cx="' + f2(c.hole.x) + '" cy="' + f2(c.hole.y) + '" r="' + S.holeD / 2 + '" fill="#fff" fill-opacity="0.9" stroke="' + (ok ? '#1b9a35' : '#e00') + '" stroke-width="' + 1.5 * px + '"/>' +
            '<path d="M' + (c.hole.x - 2 * px * 3) + ' ' + c.hole.y + 'H' + (c.hole.x + 6 * px) + 'M' + c.hole.x + ' ' + (c.hole.y - 6 * px) + 'V' + (c.hole.y + 6 * px) + '" stroke="' + (ok ? '#1b9a35' : '#e00') + '" stroke-width="' + px + '"/>';
        });
      });
      var mode = S.export.closedHole;
      s += '<circle cx="' + f2(sc.closedRound.x) + '" cy="' + f2(sc.closedRound.y) + '" r="' + S.holeD / 2 + '" fill="#fff" fill-opacity="0.9" stroke="#1565c0" stroke-width="' + 1.5 * px + '"' + (mode === 'round' ? '' : ' stroke-dasharray="' + 3 * px + ' ' + 3 * px + '" opacity="0.5"') + '/>';
      var sl = sc.closedSlot;
      s += '<rect x="' + f2(sl.x - sl.w / 2) + '" y="' + f2(sl.y - sl.h / 2) + '" width="' + sl.w + '" height="' + sl.h + '" fill="#fff" fill-opacity="0.9" stroke="#ef6c00" stroke-width="' + 1.5 * px + '"' + (mode === 'slot' ? '' : ' stroke-dasharray="' + 3 * px + ' ' + 3 * px + '" opacity="0.5"') + '/>';
      // boxes
      var all = KEYS.concat(['arrow']);
      all.forEach(function (key) {
        var b = boxOf(sc, key), on = sel.type === key;
        s += '<rect x="' + f2(b.x) + '" y="' + f2(b.y) + '" width="' + f2(b.w) + '" height="' + f2(b.h) + '" fill="none" stroke="' + (on ? '#0057d8' : '#8a8a8a') + '" stroke-width="' + (on ? 1.6 : 0.8) * px + '" stroke-dasharray="' + 5 * px + ' ' + 3 * px + '"/>' +
          '<text x="' + f2(b.x) + '" y="' + f2(b.y - 3 * px) + '" font-size="' + 10 * px + '" fill="' + (on ? '#0057d8' : '#777') + '">' + (key === 'arrow' ? 'ARROW' : NAMES[key]) + ' ' + f2(b.w) + ' x ' + f2(b.h) + '</text>';
      });
      // dimension arrows
      var pts = [];
      function boxPts(b) { return core.boxPoints(b); }
      if (S.ui.dims === 'selected') {
        if (sel.type === 'strip' && S.strips[sel.idx]) pts = boxPts(core.stripRect(S.strips[sel.idx]));
        else if (sel.type) {
          pts = boxPts(boxOf(sc, sel.type));
          if (sel.type === 'casino' || sel.type === 'open') sc[sel.type].cells.forEach(function (c) { if (c.hole) pts.push(c.hole); });
          if (sel.type === 'closed') pts.push(sc.closedRound);
          if (sel.type === 'arrow') sc.arrow.holes.forEach(function (h) { pts.push(h); });
        }
      } else if (S.ui.dims === 'all') {
        all.forEach(function (key) { pts = pts.concat(boxPts(boxOf(sc, key))); });
      }
      if (pts.length) s += dimSvg(pts);
      if (guides.v !== null) s += '<line x1="' + guides.v + '" y1="-40" x2="' + guides.v + '" y2="' + (H + 40) + '" stroke="#e0007a" stroke-width="' + px + '" stroke-dasharray="' + 4 * px + ' ' + 3 * px + '"/>';
      if (guides.h !== null) s += '<line x1="-40" y1="' + guides.h + '" x2="' + (W + 40) + '" y2="' + guides.h + '" stroke="#e0007a" stroke-width="' + px + '" stroke-dasharray="' + 4 * px + ' ' + 3 * px + '"/>';
    }
    svg.innerHTML = s;
    updateFrameInfo();
    updateStatus();
  }
  var guides = { v: null, h: null };

  // ------------------------------------------------------------------ info panels
  var thinCache = new Map();
  function thinOf(g, offset) {
    var key = S.minStroke + ':' + offset, e = thinCache.get(g);
    if (!e) { e = {}; thinCache.set(g, e); }
    if (e[key] === undefined) e[key] = g.thinAreaMm2(S.minStroke, offset);
    return e[key];
  }
  function updateInfo() {
    var sc = cur || scene(PPM), r = S.holeD / 2;
    KEYS.forEach(function (key) {
      var t = S.texts[key], f = effFont(key), el = $('info-' + key), cells = sc[key].cells, thin = [], cov = 0, holes = 0, st = fontState[f.font];
      cells.forEach(function (c, i) {
        if (!c.glyph) return;
        if (c.hole) { holes++; if (core.holeStatus(c, r, t.thicken).covered) cov++; }
        var a = thinOf(c.glyph, t.thicken);
        if (a > 1) thin.push(t.text[i] + ' ' + Math.round(a) + ' mm2');
      });
      var h = 'Box ' + f2(sc[key].w) + ' x ' + f2(sc[key].h) + ' mm. ';
      if (holes) h += '<span class="' + (cov === holes ? 'ok' : 'warn') + '">Holes covered by the letters: ' + cov + '/' + holes + '.</span> ';
      h += thin.length ? '<span class="warn">Thin parts (below ' + S.minStroke + ' mm): ' + thin.join(', ') + '.</span> ' : '<span class="ok">No thin parts.</span> ';
      if (st && st.status === 'missing') h += '<br><span class="warn">Font "' + f.font + '" not available (no network or wrong name) - showing the browser default.</span>';
      else if (st && st.status === 'loading') h += '<br>Loading font ' + f.font + '...';
      el.innerHTML = h;
    });
    updateFrameInfo();
  }
  function updateFrameInfo() {
    var fc = core.frameCounts(S.plate.w, S.plate.h, S.frame.layout, S.frame.unit, S.frame.stripWidth);
    function sd(n, o) { return o.n + ' x 100 mm' + (o.left > 0.05 ? ' + ' + f2(o.left) + ' mm left over' : ''); }
    $('frameInfo').innerHTML = 'Top: ' + sd('top', fc.top) + '<br>Bottom: ' + sd('bottom', fc.bottom) + '<br>Left: ' + sd('left', fc.left) + '<br>Right: ' + sd('right', fc.right) +
      '<br><b>Total ' + fc.totalUnits + ' units = ' + fc.totalLeds + ' LEDs</b> (3 LEDs per 100 mm)';
    var si = $('stripInfo');
    if (sel.type === 'strip' && S.strips[sel.idx]) si.textContent = 'Piece ' + (sel.idx + 1) + ' of ' + S.strips.length + ': ' + Math.round(stripCoverage(S.strips[sel.idx]) * 100) + ' % behind a letter.';
    else si.textContent = S.strips.length + ' piece(s). Click one to see how much of it is behind a letter.';
    $('json').title = 'Layout JSON';
  }
  function stripCoverage(s) {
    var sc = cur || scene(PPM), r = core.stripRect(s), n = 0, hit = 0, x, y;
    for (y = r.y + 0.25; y < r.y + r.h; y += 0.5) for (x = r.x + 0.25; x < r.x + r.w; x += 0.5) {
      n++;
      var inside = KEYS.some(function (key) {
        var off = S.texts[key].thicken;
        return sc[key].cells.some(function (c) { return c.glyph && c.glyph.insideDist(x - c.gox, y - c.goy, off) >= 0; });
      });
      if (inside) hit++;
    }
    return n ? hit / n : 0;
  }
  function updateStatus() {
    var name = sel.type === 'strip' ? 'strip piece ' + (sel.idx + 1) : sel.type ? (sel.type === 'arrow' ? 'ARROW' : NAMES[sel.type]) : 'nothing';
    $('status').textContent = 'Plate ' + f2(S.plate.w) + ' x ' + f2(S.plate.h) + ' mm - selected: ' + name + ' - zoom ' + Math.round(view.k * 100) / 100 + ' px/mm' + (statusMsg ? ' - ' + statusMsg : '');
  }
  var statusMsg = '';

  // ------------------------------------------------------------------ state history, storage
  var hist = [], histPos = -1, commitTimer = null;
  function lsGet(k) { try { return localStorage.getItem(k); } catch (e) { return null; } }
  function lsSet(k, v) { try { localStorage.setItem(k, v); } catch (e) { } }
  function commit() {
    clearTimeout(commitTimer); commitTimer = null;
    var j = core.toJson(S);
    if (hist[histPos] !== j) {
      hist = hist.slice(0, histPos + 1); hist.push(j); if (hist.length > 100) hist.shift(); histPos = hist.length - 1;
      lsSet(LS_AUTO, j);
    }
    if (document.activeElement !== $('json')) $('json').value = j;
    updateInfo(); updateUndo();
  }
  function scheduleCommit() { clearTimeout(commitTimer); commitTimer = setTimeout(commit, 350); }
  function updateUndo() { $('undo').disabled = histPos <= 0; $('redo').disabled = histPos >= hist.length - 1; }
  function loadState(st) { S = st; ensureAllFonts(); syncControls(); render(); }
  function undo() { commit(); if (histPos > 0) { histPos--; loadState(core.fromJson(hist[histPos])); $('json').value = hist[histPos]; updateInfo(); updateUndo(); } }
  function redo() { commit(); if (histPos < hist.length - 1) { histPos++; loadState(core.fromJson(hist[histPos])); $('json').value = hist[histPos]; updateInfo(); updateUndo(); } }
  function ensureAllFonts() { KEYS.forEach(function (k) { ensureFont(effFont(k).font); }); }
  function changed() { render(); scheduleCommit(); }

  // ------------------------------------------------------------------ controls
  var binds = [];
  function bind(id, get, set) {
    var el = $(id), chk = el.type === 'checkbox', num = el.type === 'number' || el.type === 'range';
    binds.push({ el: el, get: get });
    el.addEventListener(el.tagName === 'SELECT' || chk ? 'change' : 'input', function () {
      var v = chk ? el.checked : num ? parseFloat(el.value) : el.value;
      if (num && !isFinite(v)) return;
      set(v); syncControls(el); changed();
    });
  }
  function syncControls(except) {
    KEYS.forEach(function (k) { addFontOption(k, S.texts[k].font); });
    binds.forEach(function (b) {
      if (b.el === except) return;
      var v = b.get();
      if (b.el.type === 'checkbox') b.el.checked = !!v; else b.el.value = v;
    });
  }
  function fontOptions() {
    var h = '';
    FONT_GROUPS.forEach(function (g) {
      h += '<optgroup label="' + g[0] + '">' + g[1].map(function (n) { var thin = n.slice(-1) === '!', nm = thin ? n.slice(0, -1) : n; return '<option value="' + nm + '">' + nm + (thin ? ' (thin)' : '') + '</option>'; }).join('') + '</optgroup>';
    });
    return h;
  }
  function buildTextPanels() {
    var h = '';
    KEYS.forEach(function (key) {
      var k = key;
      h += '<div class="sect"><b>' + NAMES[k] + '</b>' +
        '<div class="row"><span>Text</span><input type="text" id="t-' + k + '-text" maxlength="20"></div>' +
        '<div class="row"><span>Font</span><select id="t-' + k + '-font">' + fontOptions() + '</select></div>' +
        (k === 'casino' ? '' : '<label><input type="checkbox" id="t-' + k + '-same"> same font as CASINO</label>') +
        '<div class="row"><span>Other Google font</span><input type="text" id="t-' + k + '-other" placeholder="e.g. Bungee Hairline"><button id="t-' + k + '-use">Use</button></div>' +
        '<label><input type="checkbox" id="t-' + k + '-bold"> bold (700)</label>' +
        '<div class="row"><span>Spacing</span><select id="t-' + k + '-mode"><option value="uniform">uniform cells</option><option value="natural">natural widths</option></select></div>' +
        '<div class="row"><span>Height mm</span><input type="number" id="t-' + k + '-h" min="5" max="600" step="1"><span style="width:auto">cell</span><input type="number" id="t-' + k + '-cw" min="5" max="600" step="1"><span style="width:auto">gap</span><input type="number" id="t-' + k + '-sp" min="-50" max="100" step="0.5"></div>' +
        '<div class="row"><span>Thicken mm</span><input type="range" id="t-' + k + '-th" min="-3" max="8" step="0.1"><input type="number" id="t-' + k + '-thn" min="-5" max="10" step="0.1"></div>' +
        '<div class="row"><span>Position x, y</span><input type="number" id="t-' + k + '-x" step="1"><input type="number" id="t-' + k + '-y" step="1"></div>' +
        (k === 'closed' ? '<div class="row"><span>Hole offset</span><input type="number" id="t-closed-hdx" step="0.5"><input type="number" id="t-closed-hdy" step="0.5"></div>' :
          '<div class="row"><button id="t-' + k + '-centre">Centre holes in cells</button><button id="t-' + k + '-fit">Fit holes into thick parts</button></div>') +
        '<div class="note" id="info-' + k + '"></div></div>';
    });
    $('texts').innerHTML = h;
  }
  function addFontOption(key, name) {
    var sel2 = $('t-' + key + '-font');
    if (!sel2 || Array.prototype.some.call(sel2.options, function (o) { return o.value === name; })) return;
    var og = sel2.querySelector('optgroup[label="Custom"]');
    if (!og) { og = document.createElement('optgroup'); og.label = 'Custom'; sel2.insertBefore(og, sel2.firstChild); }
    var op = document.createElement('option'); op.value = op.textContent = name; og.appendChild(op);
  }
  function useFont(key, name) {
    name = name.trim(); if (!name) return;
    addFontOption(key, name);
    S.texts[key].font = name; if (key !== 'casino') S.texts[key].sameFont = false;
    ensureFont(name); syncControls(); changed();
  }
  function placeHolesBy(key, mode) {
    var sc = scene(PPM), t = S.texts[key], r = S.holeD / 2;
    t.holeOffsets = sc[key].cells.map(function (c) {
      if (!c.glyph) return { dx: 0, dy: 0 };
      var o = core.holeOffsetFor(c, r, t.thicken, mode);
      return { dx: Math.round(o.dx * 100) / 100, dy: Math.round(o.dy * 100) / 100 };
    });
    changed();
  }
  function buildControls() {
    buildTextPanels();
    bind('plateW', function () { return S.plate.w; }, function (v) { S.plate.w = cl(v, 100, 980); });
    bind('plateWn', function () { return S.plate.w; }, function (v) { S.plate.w = cl(v, 100, 980); });
    bind('plateH', function () { return S.plate.h; }, function (v) { S.plate.h = cl(v, 100, 300); });
    bind('plateHn', function () { return S.plate.h; }, function (v) { S.plate.h = cl(v, 100, 300); });
    bind('holeD', function () { return S.holeD; }, function (v) { S.holeD = cl(v, 5, 30); });
    bind('holeDn', function () { return S.holeD; }, function (v) { S.holeD = cl(v, 5, 30); });
    bind('minStroke', function () { return S.minStroke; }, function (v) { S.minStroke = cl(v, 0.5, 20); });
    bind('minStrokeN', function () { return S.minStroke; }, function (v) { S.minStroke = cl(v, 0.5, 20); });
    bind('frameLayout', function () { return S.frame.layout; }, function (v) { S.frame.layout = v; });
    bind('dims', function () { return S.ui.dims; }, function (v) { S.ui.dims = v; });
    bind('snap', function () { return S.ui.snap; }, function (v) { S.ui.snap = v; });
    bind('grid', function () { return S.ui.grid; }, function (v) { S.ui.grid = v; });
    bind('closedHole', function () { return S.export.closedHole; }, function (v) { S.export.closedHole = v; });
    bind('slotW', function () { return S.closedSlot.w; }, function (v) { S.closedSlot.w = cl(v, 1, 300); });
    bind('slotH', function () { return S.closedSlot.h; }, function (v) { S.closedSlot.h = cl(v, 1, 100); });
    bind('exOutline', function () { return S.export.plateOutline; }, function (v) { S.export.plateOutline = v; });
    bind('exArrow', function () { return S.export.arrow; }, function (v) { S.export.arrow = v; });
    bind('exMirror', function () { return S.export.mirror; }, function (v) { S.export.mirror = v; });
    bind('exBoxes', function () { return S.export.boxes; }, function (v) { S.export.boxes = v; });
    bind('exNest', function () { return S.export.nest; }, function (v) { S.export.nest = v; });
    bind('exKerf', function () { return S.export.kerf; }, function (v) { S.export.kerf = cl(v, -2, 2); });
    bind('exSpacing', function () { return S.export.spacing; }, function (v) { S.export.spacing = cl(v, 0, 50); });
    bind('exSheetW', function () { return S.export.sheetW; }, function (v) { S.export.sheetW = cl(v, 50, 3000); });
    bind('exSheetH', function () { return S.export.sheetH; }, function (v) { S.export.sheetH = cl(v, 50, 3000); });
    KEYS.forEach(function (k) {
      var T = function () { return S.texts[k]; };
      bind('t-' + k + '-text', function () { return T().text; }, function (v) { T().text = v.slice(0, 20); });
      bind('t-' + k + '-font', function () { return T().font; }, function (v) { T().font = v; if (k !== 'casino') T().sameFont = false; ensureFont(v); });
      if (k !== 'casino') bind('t-' + k + '-same', function () { return T().sameFont; }, function (v) { T().sameFont = v; ensureFont(effFont(k).font); });
      bind('t-' + k + '-bold', function () { return T().weight === 700; }, function (v) { T().weight = v ? 700 : 400; ensureFont(T().font); clearGlyphs(); });
      bind('t-' + k + '-mode', function () { return T().mode; }, function (v) { T().mode = v; });
      bind('t-' + k + '-h', function () { return T().h; }, function (v) { T().h = cl(v, 5, 600); });
      bind('t-' + k + '-cw', function () { return T().cellW; }, function (v) { T().cellW = cl(v, 5, 600); });
      bind('t-' + k + '-sp', function () { return T().spacing; }, function (v) { T().spacing = cl(v, -50, 100); });
      bind('t-' + k + '-th', function () { return T().thicken; }, function (v) { T().thicken = cl(v, -5, 10); });
      bind('t-' + k + '-thn', function () { return T().thicken; }, function (v) { T().thicken = cl(v, -5, 10); });
      bind('t-' + k + '-x', function () { return T().x; }, function (v) { T().x = v; });
      bind('t-' + k + '-y', function () { return T().y; }, function (v) { T().y = v; });
      $('t-' + k + '-use').addEventListener('click', function () { useFont(k, $('t-' + k + '-other').value); });
      $('t-' + k + '-other').addEventListener('keydown', function (e) { if (e.key === 'Enter') useFont(k, e.target.value); });
      // font combobox: arrows switch the font live, Enter accepts, Esc puts the old one back
      var fs = $('t-' + k + '-font'), backup = null;
      fs.addEventListener('focus', function () { backup = T().font; });
      fs.addEventListener('keydown', function (e) {
        if (e.key === 'Escape' && backup !== null) { T().font = backup; fs.value = backup; changed(); e.preventDefault(); e.stopPropagation(); }
        else if (e.key === 'Enter') { backup = T().font; fs.blur(); commit(); }
      });
      if (k === 'closed') {
        bind('t-closed-hdx', function () { return S.closedRound.dx; }, function (v) { S.closedRound.dx = v; S.closedSlot.dx = v; });
        bind('t-closed-hdy', function () { return S.closedRound.dy; }, function (v) { S.closedRound.dy = v; S.closedSlot.dy = v; });
      } else {
        $('t-' + k + '-centre').addEventListener('click', function () { placeHolesBy(k, 'centre'); });
        $('t-' + k + '-fit').addEventListener('click', function () { placeHolesBy(k, 'fit'); });
      }
    });
    $('undo').onclick = undo; $('redo').onclick = redo;
    $('zin').onclick = function () { zoomAt(1.3); }; $('zout').onclick = function () { zoomAt(1 / 1.3); }; $('zfit').onclick = fit;
    $('preview').onclick = function () { S.ui.preview = !S.ui.preview; $('preview').style.fontWeight = S.ui.preview ? 'bold' : 'normal'; changed(); };
    $('stripAdd').onclick = function () { S.strips.push({ x: Math.round(S.plate.w / 2 - 50), y: Math.round(S.plate.h / 2 - 5), rot: 0 }); sel = { type: 'strip', idx: S.strips.length - 1 }; changed(); updateInfo(); };
    $('stripRot').onclick = rotateStrip; $('stripDel').onclick = deleteStrip;
    $('exEps').onclick = function () { download('plate-drill-pattern.eps', buildDrillEps(), 'application/postscript'); };
    $('exA4').onclick = function () { download('plate-drill-A4.pdf', buildDrillPdf('A4').text, 'application/pdf'); };
    $('exA3').onclick = function () { download('plate-drill-A3.pdf', buildDrillPdf('A3').text, 'application/pdf'); };
    $('exDxf').onclick = function () { download('plate-drill-pattern.dxf', buildDrillDxf(), 'image/vnd.dxf'); };
    $('exLettersDxf').onclick = function () { var r = buildLettersEps(); statusMsg = r.message; updateStatus(); download('plate-letters-cut.dxf', r.dxf, 'image/vnd.dxf'); };
    $('exLetters').onclick = function () { var r = buildLettersEps(); statusMsg = r.message; updateStatus(); download('plate-letters-cut.eps', r.text, 'application/postscript'); };
    $('jsonApply').onclick = function () {
      try { var st = core.fromJson($('json').value); loadState(st); commit(); $('jsonMsg').textContent = 'Layout loaded.'; $('jsonMsg').className = 'note ok'; }
      catch (e) { $('jsonMsg').textContent = 'Not loaded: ' + e.message; $('jsonMsg').className = 'note warn'; }
    };
    $('exCopy').onclick = function () { $('exText').select(); try { document.execCommand('copy'); statusMsg = 'copied ' + $('exName').value; updateStatus(); } catch (e) { } };
    $('jsonCopy').onclick = function () { $('json').select(); try { document.execCommand('copy'); $('jsonMsg').textContent = 'Copied.'; $('jsonMsg').className = 'note ok'; } catch (e) { } };
    $('reset').onclick = function () { loadState(core.defaultState()); fit(); commit(); };
    $('layoutSave').onclick = function () {
      var name = $('layoutName').value.trim(); if (!name) return;
      var all = layouts(); all[name] = core.toJson(S); lsSet(LS_LAYOUTS, JSON.stringify(all)); fillLayouts(name);
    };
    $('layoutLoad').onclick = function () { var all = layouts(), n = $('layoutList').value; if (all[n]) { loadState(core.fromJson(all[n])); fit(); commit(); } };
    $('layoutDel').onclick = function () { var all = layouts(); delete all[$('layoutList').value]; lsSet(LS_LAYOUTS, JSON.stringify(all)); fillLayouts(); };
  }
  function layouts() { try { return JSON.parse(lsGet(LS_LAYOUTS) || '{}'); } catch (e) { return {}; } }
  function fillLayouts(pick) {
    var all = layouts(), names = Object.keys(all).sort();
    $('layoutList').innerHTML = names.map(function (n) { return '<option>' + n.replace(/</g, '&lt;') + '</option>'; }).join('');
    if (pick) $('layoutList').value = pick;
  }

  // ------------------------------------------------------------------ mouse and keyboard
  var drag = null;
  function hit(p) {
    var sc = cur, r = S.holeD / 2, tol = 4 / view.k, i, k, key;
    for (k = 0; k < 2; k++) {
      key = KEYS[k];
      for (i = 0; i < sc[key].cells.length; i++) { var c = sc[key].cells[i]; if (c.hole && Math.hypot(p.x - c.hole.x, p.y - c.hole.y) <= Math.max(r, tol)) return { kind: 'hole', key: key, idx: i }; }
    }
    if (S.export.closedHole === 'round' && Math.hypot(p.x - sc.closedRound.x, p.y - sc.closedRound.y) <= Math.max(r, tol)) return { kind: 'closedRound' };
    var sl = sc.closedSlot;
    if (S.export.closedHole === 'slot' && Math.abs(p.x - sl.x) <= sl.w / 2 && Math.abs(p.y - sl.y) <= Math.max(sl.h / 2, tol)) return { kind: 'closedSlot' };
    for (i = S.strips.length - 1; i >= 0; i--) { var rc = core.stripRect(S.strips[i]); if (p.x >= rc.x && p.x <= rc.x + rc.w && p.y >= rc.y && p.y <= rc.y + rc.h) return { kind: 'strip', idx: i }; }
    var best = null, bestA = 1e18;
    KEYS.concat(['arrow']).forEach(function (kk) {
      var b = boxOf(sc, kk);
      if (p.x >= b.x && p.x <= b.x + b.w && p.y >= b.y && p.y <= b.y + b.h && b.w * b.h < bestA) { best = { kind: 'box', key: kk }; bestA = b.w * b.h; }
    });
    return best;
  }
  svg.addEventListener('pointerdown', function (e) {
    var p = toMm(e), h = e.button === 1 ? null : hit(p);
    svg.setPointerCapture(e.pointerId);
    if (!h) { drag = { kind: 'pan', sx: e.clientX, sy: e.clientY, cx: view.cx, cy: view.cy, moved: false }; return; }
    var sc = cur;
    if (h.kind === 'hole') { sel = { type: h.key }; var c = sc[h.key].cells[h.idx]; drag = { kind: 'hole', key: h.key, idx: h.idx, gx: c.hole.x - p.x, gy: c.hole.y - p.y }; }
    else if (h.kind === 'closedRound') { sel = { type: 'closed' }; drag = { kind: 'closedRound', gx: sc.closedRound.x - p.x, gy: sc.closedRound.y - p.y }; }
    else if (h.kind === 'closedSlot') { sel = { type: 'closed' }; drag = { kind: 'closedSlot', gx: sc.closedSlot.x - p.x, gy: sc.closedSlot.y - p.y }; }
    else if (h.kind === 'strip') { sel = { type: 'strip', idx: h.idx }; var s0 = S.strips[h.idx]; drag = { kind: 'strip', idx: h.idx, ox: s0.x, oy: s0.y, sx: p.x, sy: p.y }; }
    else { sel = { type: h.key }; var o = h.key === 'arrow' ? S.arrow : S.texts[h.key]; drag = { kind: 'box', key: h.key, ox: o.x, oy: o.y, sx: p.x, sy: p.y }; }
    drag.moved = false; render(); updateInfo();
  });
  svg.addEventListener('pointermove', function (e) {
    if (!drag) return;
    if (drag.kind === 'pan') {
      var dx = e.clientX - drag.sx, dy = e.clientY - drag.sy;
      if (Math.abs(dx) + Math.abs(dy) > 2) drag.moved = true;
      view.cx = drag.cx - dx / view.k; view.cy = drag.cy - dy / view.k; render(); return;
    }
    var p = toMm(e); drag.moved = true; guides = { v: null, h: null };
    if (drag.kind === 'box') {
      var o = drag.key === 'arrow' ? S.arrow : S.texts[drag.key], b = boxOf(cur, drag.key);
      var nx = drag.ox + p.x - drag.sx, ny = drag.oy + p.y - drag.sy;
      if (S.ui.snap) {
        var targets = [{ cx: S.plate.w / 2, cy: S.plate.h / 2 }];
        KEYS.concat(['arrow']).forEach(function (kk) { if (kk !== drag.key) { var q = boxOf(cur, kk); targets.push({ cx: q.x + q.w / 2, cy: q.y + q.h / 2 }); } });
        var sn = core.snapCentres({ x: nx, y: ny, w: b.w, h: b.h }, targets, 3);
        nx += sn.dx; ny += sn.dy; guides = { v: sn.vertical, h: sn.horizontal };
      }
      o.x = Math.round(nx * 100) / 100; o.y = Math.round(ny * 100) / 100;
      syncControls();
    } else if (drag.kind === 'hole') {
      var c = cur[drag.key].cells[drag.idx], t = S.texts[drag.key];
      while (t.holeOffsets.length <= drag.idx) t.holeOffsets.push({ dx: 0, dy: 0 });
      t.holeOffsets[drag.idx] = { dx: Math.round((p.x + drag.gx - c.cx) * 100) / 100, dy: Math.round((p.y + drag.gy - c.cy) * 100) / 100 };
    } else if (drag.kind === 'closedRound' || drag.kind === 'closedSlot') {
      var cb = cur.closed, dx2 = Math.round((p.x + drag.gx - (cb.x + cb.w / 2)) * 100) / 100, dy2 = Math.round((p.y + drag.gy - (cb.y + cb.h / 2)) * 100) / 100;
      S.closedRound.dx = S.closedSlot.dx = dx2; S.closedRound.dy = S.closedSlot.dy = dy2; syncControls();
    } else if (drag.kind === 'strip') {
      var s1 = S.strips[drag.idx]; s1.x = Math.round((drag.ox + p.x - drag.sx) * 100) / 100; s1.y = Math.round((drag.oy + p.y - drag.sy) * 100) / 100;
    }
    render();
  });
  function endDrag() {
    if (!drag) return;
    var was = drag; drag = null; guides = { v: null, h: null };
    if (was.kind === 'pan' && !was.moved) { sel = { type: null }; }
    if (was.kind !== 'pan' && was.moved) commit();
    render(); updateInfo();
  }
  svg.addEventListener('pointerup', endDrag); svg.addEventListener('pointercancel', endDrag);
  svg.addEventListener('wheel', function (e) {
    e.preventDefault();
    var p = toMm(e), f = Math.pow(1.15, -e.deltaY / 100), nk = cl(view.k * f, 0.2, 60), r = svg.getBoundingClientRect();
    view.k = nk;
    view.cx = p.x - (e.clientX - r.left - r.width / 2) / nk; view.cy = p.y - (e.clientY - r.top - r.height / 2) / nk;
    render();
  }, { passive: false });
  function rotateStrip() { if (sel.type === 'strip' && S.strips[sel.idx]) { var s0 = S.strips[sel.idx]; s0.rot = s0.rot === 90 ? 0 : 90; changed(); updateInfo(); } }
  function deleteStrip() { if (sel.type === 'strip' && S.strips[sel.idx]) { S.strips.splice(sel.idx, 1); sel = { type: null }; changed(); updateInfo(); } }
  document.addEventListener('keydown', function (e) {
    var tag = (e.target.tagName || '').toLowerCase();
    if ((e.ctrlKey || e.metaKey) && (e.key === 'z' || e.key === 'Z')) { if (tag === 'textarea' || (tag === 'input' && e.target.type === 'text')) return; e.preventDefault(); e.shiftKey ? redo() : undo(); return; }
    if ((e.ctrlKey || e.metaKey) && (e.key === 'y' || e.key === 'Y')) { if (tag === 'textarea' || (tag === 'input' && e.target.type === 'text')) return; e.preventDefault(); redo(); return; }
    if (tag === 'input' || tag === 'select' || tag === 'textarea') return;
    if (e.key === 'r' || e.key === 'R') { rotateStrip(); return; }
    if (e.key === 'Delete' || e.key === 'Backspace') { deleteStrip(); return; }
    if (e.key === 'Escape') { sel = { type: null }; render(); updateInfo(); return; }
    var step = e.altKey ? 0.1 : e.shiftKey ? 10 : 1, dx = 0, dy = 0;
    if (e.key === 'ArrowLeft') dx = -step; else if (e.key === 'ArrowRight') dx = step; else if (e.key === 'ArrowUp') dy = -step; else if (e.key === 'ArrowDown') dy = step; else return;
    e.preventDefault();
    var o = sel.type === 'strip' ? S.strips[sel.idx] : sel.type === 'arrow' ? S.arrow : sel.type ? S.texts[sel.type] : null;
    if (!o) return;
    o.x = Math.round((o.x + dx) * 100) / 100; o.y = Math.round((o.y + dy) * 100) / 100; syncControls(); changed();
  });
  window.addEventListener('resize', render);

  // ------------------------------------------------------------------ exports
  function drillScene() { return scene(PPM); }
  function buildDrillEps() { return core.epsFromItems(core.drillItems(S, drillScene()), S.plate, 'Casino plate drill pattern'); }
  function buildDrillPdf(paper) { return core.pdfTiles(core.drillItems(S, drillScene()), S.plate, paper, 'Casino plate drill pattern'); }
  function buildDrillDxf() { return core.dxfFromItems(core.drillItems(S, drillScene()), S.plate); }
  function bboxOf(polys) {
    var a = 1e9, b = 1e9, c = -1e9, d = -1e9;
    polys.forEach(function (p) { p.forEach(function (q) { a = Math.min(a, q[0]); b = Math.min(b, q[1]); c = Math.max(c, q[0]); d = Math.max(d, q[1]); }); });
    return { x0: a, y0: b, x1: c, y1: d };
  }
  function letterPieces(ppm) {
    var sc = scene(ppm), pieces = [];
    KEYS.forEach(function (key) {
      var off = S.texts[key].thicken + S.export.kerf / 2;
      sc[key].cells.forEach(function (c) {
        if (!c.glyph) return;
        var polys = c.glyph.contours(off, 0.02).map(function (p) { return p.map(function (q) { return [q[0] + c.gox, q[1] + c.goy]; }); });
        if (polys.length) pieces.push({ key: key, index: c.index, polys: polys, bb: bboxOf(polys) });
      });
    });
    return { pieces: pieces, sc: sc };
  }
  function buildLettersEps() {
    var lp = letterPieces(PPM_EXPORT), pieces = lp.pieces, ex = S.export, W = S.plate.w, H = S.plate.h, polys = [], boxes = [], message = '', areaW = W, areaH = H;
    if (!ex.nest) {
      pieces.forEach(function (p) { p.polys.forEach(function (poly) { polys.push(ex.mirror ? poly.map(function (q) { return [W - q[0], q[1]]; }) : poly); }); });
      if (ex.boxes) KEYS.forEach(function (k) { var b = boxOf(lp.sc, k); boxes.push({ x: ex.mirror ? W - b.x - b.w : b.x, y: b.y, w: b.w, h: b.h }); });
      message = pieces.length + ' letters as placed on the plate' + (ex.mirror ? ' (mirrored)' : '');
    } else {
      var items = pieces.map(function (p, i) { return { id: i, w: p.bb.x1 - p.bb.x0, h: p.bb.y1 - p.bb.y0 }; });
      var res = core.nestShelves(items, ex.sheetW, ex.sheetH, ex.spacing);
      res.placed.forEach(function (pl) {
        var p = pieces[pl.id], w = p.bb.x1 - p.bb.x0, h = p.bb.y1 - p.bb.y0;
        p.polys.forEach(function (poly) {
          polys.push(poly.map(function (q) { var x = ex.mirror ? p.bb.x0 + p.bb.x1 - q[0] : q[0]; return [x - p.bb.x0 + pl.x, q[1] - p.bb.y0 + pl.y]; }));
        });
        if (ex.boxes) boxes.push({ x: pl.x, y: pl.y, w: w, h: h });
      });
      areaW = ex.sheetW; areaH = Math.max(ex.sheetH, res.usedH);
      message = res.fits ? 'nested ' + pieces.length + ' letters, ' + f2(res.usedH) + ' mm of the ' + ex.sheetH + ' mm sheet used' : 'WARNING: the letters do not fit the ' + ex.sheetW + ' x ' + ex.sheetH + ' mm sheet (need ' + f2(res.usedH) + ' mm height)';
    }
    return { text: core.epsFromPolys(polys, areaW, areaH, 'Casino plate letters', boxes), dxf: core.dxfFromPolys(polys, areaH, boxes), message: message, polys: polys.length };
  }
  function download(name, text, type) {
    $('exName').value = name; $('exText').value = text;
    var b = new Blob([text], { type: type }), a = document.createElement('a');
    a.href = URL.createObjectURL(b); a.download = name; document.body.appendChild(a); a.click();
    setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
  }

  // ------------------------------------------------------------------ start
  function init() {
    buildControls();
    var saved = lsGet(LS_AUTO);
    if (saved) { try { S = core.fromJson(saved); } catch (e) { S = core.defaultState(); } }
    ensureAllFonts(); fillLayouts(); syncControls(); fit(); commit();
    $('preview').style.fontWeight = S.ui.preview ? 'bold' : 'normal';
  }
  window.PlateApp = {
    state: function () { return S; }, scene: function () { return scene(PPM); }, drillEps: buildDrillEps, drillDxf: buildDrillDxf, drillPdf: buildDrillPdf, lettersEps: buildLettersEps,
    fontStatus: function (n) { return fontState[n] && fontState[n].status; }, useFont: useFont, commit: commit, view: view, selection: function () { return sel; }, hist: function () { return { n: hist.length, pos: histPos }; }
  };
  init();
})();
