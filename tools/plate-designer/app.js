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
  // The font that is wanted for a text (what the controls say) ...
  function wantedFont(key) {
    var t = S.texts[key];
    if (key !== 'casino' && t.sameFont) return { font: S.texts.casino.font, weight: S.texts.casino.weight };
    return { font: t.font, weight: t.weight };
  }
  // ... and the font that is drawn: the new font only after it has really loaded, so the text does not flash a fallback font.
  var shown = {};
  function effFont(key) {
    var w = wantedFont(key);
    if (!fontState[w.font]) ensureFont(w.font);
    if (fontState[w.font].status === 'loading' && shown[key]) return shown[key];
    shown[key] = w;
    return w;
  }
  // Run a change that resizes a text box (height, cell width, gap) and keep the centre of its bounding box where it was.
  function keepCentre(key, change) {
    var b = textScene(key, PPM), cx = b.x + b.w / 2, cy = b.y + b.h / 2;
    change();
    var a = textScene(key, PPM), t = S.texts[key];
    t.x = Math.round((cx - a.w / 2) * 100) / 100; t.y = Math.round((cy - a.h / 2) * 100) / 100;
  }
  // Uniform scaling of a whole text (height, cell width, gap, hole offsets; for CLOSED also its hole offset) around the
  // centre of its box. base = snapshotText(key) taken before the change, so repeated drag steps do not drift.
  function snapshotText(key) {
    var t = S.texts[key], b = textScene(key, PPM);
    return { h: t.h, cellW: t.cellW, spacing: t.spacing, holeOffsets: JSON.parse(JSON.stringify(t.holeOffsets || [])), extraHoles: JSON.parse(JSON.stringify(t.extraHoles || [])),
      rd: { dx: S.closedRound.dx, dy: S.closedRound.dy }, sd: { dx: S.closedSlot.dx, dy: S.closedSlot.dy }, cx: b.x + b.w / 2, cy: b.y + b.h / 2, w: b.w, boxH: b.h };
  }
  function scaleText(key, base, s) {
    var lim = core.scaleLimits(base), t = S.texts[key];
    s = cl(s, lim.min, lim.max);
    var n = core.scaledText(base, s);
    t.h = n.h; t.cellW = n.cellW; t.spacing = n.spacing; t.holeOffsets = n.holeOffsets; t.extraHoles = n.extraHoles;
    if (key === 'closed') {
      S.closedRound.dx = Math.round(base.rd.dx * s * 1000) / 1000; S.closedRound.dy = Math.round(base.rd.dy * s * 1000) / 1000;
      S.closedSlot.dx = Math.round(base.sd.dx * s * 1000) / 1000; S.closedSlot.dy = Math.round(base.sd.dy * s * 1000) / 1000;
    }
    var a = textScene(key, PPM);
    t.x = Math.round((base.cx - a.w / 2) * 100) / 100; t.y = Math.round((base.cy - a.h / 2) * 100) / 100;
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
    var sw = S.frame.inside ? 0 : S.frame.stripWidth, W = S.plate.w + 2 * sw + 60, H = S.plate.h + 2 * sw + 60;
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
    var W = S.plate.w, H = S.plate.h, sw = S.frame.stripWidth, unit = S.frame.unit, fc = core.frameCounts(W, H, S.frame.layout, unit, sw, S.frame.inside), s = '';
    var geo = core.frameSides(W, H, S.frame.layout, sw, S.frame.inside), infos = [fc.top, fc.bottom, fc.left, fc.right];
    var sides = geo.map(function (g, i) { return { x: g.x, y: g.y, len: g.len, horiz: g.horiz, info: infos[i] }; });
    sides.forEach(function (sd, si) {
      var w = sd.horiz ? sd.len : sw, h = sd.horiz ? sw : sd.len;
      var ghost = S.frame.inside;
      s += '<rect x="' + sd.x + '" y="' + sd.y + '" width="' + w + '" height="' + h + '" fill="' + (ghost ? (preview ? '#000' : '#e8a000') : (preview ? '#000' : '#4a4a4a')) + '"' + (ghost ? ' fill-opacity="' + (preview ? 0.5 : 0.3) + '" stroke="#e8a000" stroke-dasharray="' + 4 / view.k + ' ' + 2 / view.k + '"' : ' stroke="#222"') + ' stroke-width="' + 0.4 / view.k * 2 + '"/>';
      var n = sd.info.n, i, j;
      for (i = 0; i < n; i++) {
        // each 100 mm unit (3 LEDs) is one group: alternate tint plus a clear divider
        var ux = sd.horiz ? sd.x + i * unit : sd.x, uy = sd.horiz ? sd.y : sd.y + i * unit, uw = sd.horiz ? unit : sw, uh = sd.horiz ? sw : unit;
        if (i % 2) s += '<rect x="' + ux + '" y="' + uy + '" width="' + uw + '" height="' + uh + '" fill="#fff" fill-opacity="' + (ghost ? 0.25 : 0.12) + '"/>';
        var hue = (si * 70 + i * 47) % 360, col = 'hsl(' + hue + ',90%,58%)';
        for (j = 0; j < 3; j++) {
          var t = i * unit + unit * (1 + 2 * j) / 6;
          var cx = sd.horiz ? sd.x + t : sd.x + sw / 2, cy = sd.horiz ? sd.y + sw / 2 : sd.y + t;
          s += '<circle cx="' + f2(cx) + '" cy="' + f2(cy) + '" r="2.4" fill="' + col + '"' + (preview ? ' filter="url(#glow)"' : '') + '/>';
        }
        if (i > 0) { var b = i * unit; s += sd.horiz ? '<line x1="' + (sd.x + b) + '" y1="' + sd.y + '" x2="' + (sd.x + b) + '" y2="' + (sd.y + sw) + '" stroke="#ddd" stroke-width="' + 1.5 / view.k + '"/>' : '<line x1="' + sd.x + '" y1="' + (sd.y + b) + '" x2="' + (sd.x + sw) + '" y2="' + (sd.y + b) + '" stroke="#ddd" stroke-width="' + 1.5 / view.k + '"/>'; }
      }
      if (sd.info.left > 0.05) {
        var o = n * unit;
        s += sd.horiz ? '<rect x="' + (sd.x + o) + '" y="' + sd.y + '" width="' + sd.info.left + '" height="' + sw + '" fill="url(#hatch)"/>' : '<rect x="' + sd.x + '" y="' + (sd.y + o) + '" width="' + sw + '" height="' + sd.info.left + '" fill="url(#hatch)"/>';
      }
    });
    return s;
  }
  // The centre of the plate: a vertical and a horizontal centre line and a marker at the crossing. Things snap to them.
  function centreSvg() {
    var W = S.plate.w, H = S.plate.h, px = 1 / view.k, cx = W / 2, cy = H / 2, c = '#0a8f8f', d = ' stroke-dasharray="' + 10 * px + ' ' + 3 * px + ' ' + 2 * px + ' ' + 3 * px + '"';
    return '<g pointer-events="none"><line x1="' + cx + '" y1="0" x2="' + cx + '" y2="' + H + '" stroke="' + c + '" stroke-width="' + px + '"' + d + '/>' +
      '<line x1="0" y1="' + cy + '" x2="' + W + '" y2="' + cy + '" stroke="' + c + '" stroke-width="' + px + '"' + d + '/>' +
      '<circle cx="' + cx + '" cy="' + cy + '" r="' + 5 * px + '" fill="none" stroke="' + c + '" stroke-width="' + 1.5 * px + '"/><circle cx="' + cx + '" cy="' + cy + '" r="' + 1.5 * px + '" fill="' + c + '"/>' +
      '<text x="' + (cx + 8 * px) + '" y="' + (cy - 6 * px) + '" font-size="' + 9 * px + '" fill="' + c + '">centre ' + f2(cx) + ', ' + f2(cy) + '</text></g>';
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
    if (!S.frame.inside) s += frameSvg(pv);
    s += '<rect x="0" y="0" width="' + W + '" height="' + H + '" fill="' + (pv ? '#101010' : '#f3efe0') + '" stroke="#000" stroke-width="' + px + '"/>';
    if (S.ui.grid && !pv) s += gridSvg();
    if (!pv) s += centreSvg();
    if (S.frame.inside) s += '<g pointer-events="none">' + frameSvg(pv) + '</g>';
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
          [c.hole].concat(c.extras || []).forEach(function (h, hi) {
            var extra = hi > 0, on = extra && selX && selX.key === key && selX.k === h.k;
            var ok = core.holeStatus(c, S.holeD / 2, S.texts[key].thicken, h).covered, col = ok ? '#1b9a35' : '#e00';
            s += '<circle cx="' + f2(h.x) + '" cy="' + f2(h.y) + '" r="' + S.holeD / 2 + '" fill="' + (extra ? '#eaf3ff' : '#fff') + '" fill-opacity="0.9" stroke="' + col + '" stroke-width="' + (on ? 3 : 1.5) * px + '"' + (extra ? ' stroke-dasharray="' + 4 * px + ' ' + 2 * px + '"' : '') + '/>' +
              '<path d="M' + (h.x - 2 * px * 3) + ' ' + h.y + 'H' + (h.x + 6 * px) + 'M' + h.x + ' ' + (h.y - 6 * px) + 'V' + (h.y + 6 * px) + '" stroke="' + col + '" stroke-width="' + px + '"/>';
          });
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
      // handles of the selected text box: corners and side midpoints scale it uniformly around its centre, the centre dot moves it
      if (sel.type && sel.type !== 'arrow' && sel.type !== 'strip') {
        var hb = boxOf(sc, sel.type), hs = 4 * px;
        handlePoints(hb).forEach(function (hp) {
          if (hp.hx === 0 && hp.hy === 0) s += '<circle cx="' + f2(hp.x) + '" cy="' + f2(hp.y) + '" r="' + 3 * px + '" fill="#0057d8"/>';
          else s += '<rect x="' + f2(hp.x - hs) + '" y="' + f2(hp.y - hs) + '" width="' + 2 * hs + '" height="' + 2 * hs + '" fill="#fff" stroke="#0057d8" stroke-width="' + 1.5 * px + '"/>';
        });
      }
      // dimension arrows
      var pts = [];
      function boxPts(b) { return core.boxPoints(b); }
      if (S.ui.dims === 'selected') {
        if (sel.type === 'strip' && S.strips[sel.idx]) pts = boxPts(core.stripRect(S.strips[sel.idx]));
        else if (sel.type) {
          pts = boxPts(boxOf(sc, sel.type));
          if (sel.type === 'casino' || sel.type === 'open') sc[sel.type].cells.forEach(function (c) { if (c.hole) pts.push(c.hole); (c.extras || []).forEach(function (e) { pts.push(e); }); });
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
  function handlePoints(b) {
    var pts = [], hx, hy;
    for (hy = -1; hy <= 1; hy++) for (hx = -1; hx <= 1; hx++) pts.push({ hx: hx, hy: hy, x: b.x + b.w / 2 + hx * b.w / 2, y: b.y + b.h / 2 + hy * b.h / 2 });
    return pts;
  }

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
    renderAllExtras();
    KEYS.forEach(function (key) {
      var t = S.texts[key], f = effFont(key), wf = wantedFont(key), el = $('info-' + key), cells = sc[key].cells, thin = [], cov = 0, holes = 0, st = fontState[wf.font];
      cells.forEach(function (c, i) {
        if (!c.glyph) return;
        if (c.hole) [c.hole].concat(c.extras || []).forEach(function (h) { holes++; if (core.holeStatus(c, r, t.thicken, h).covered) cov++; });
        var a = thinOf(c.glyph, t.thicken);
        if (a > 1) thin.push(t.text[i] + ' ' + Math.round(a) + ' mm2');
      });
      var h = 'Box ' + f2(sc[key].w) + ' x ' + f2(sc[key].h) + ' mm. ';
      if (holes) h += '<span class="' + (cov === holes ? 'ok' : 'warn') + '">Holes covered by the letters: ' + cov + '/' + holes + '.</span> ';
      h += thin.length ? '<span class="warn">Thin parts (below ' + S.minStroke + ' mm): ' + thin.join(', ') + '.</span> ' : '<span class="ok">No thin parts.</span> ';
      if (st && st.status === 'missing') h += '<br><span class="warn">Font "' + wf.font + '" not available (no network or wrong name) - showing the browser default.</span>';
      else if (st && st.status === 'loading') h += '<br>Loading font ' + wf.font + '...';
      el.innerHTML = h;
    });
    updateFrameInfo();
  }
  function updateFrameInfo() {
    var fc = core.frameCounts(S.plate.w, S.plate.h, S.frame.layout, S.frame.unit, S.frame.stripWidth, S.frame.inside);
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
  var holdCommits = false;
  function scheduleCommit() { if (holdCommits) return; clearTimeout(commitTimer); commitTimer = setTimeout(commit, 350); }
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
      if (b.el._dd) b.el._dd.refresh();
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
        '<div class="row"><span>Width mm</span><input type="number" id="t-' + k + '-w" min="5" max="3000" step="1"><span style="width:auto;flex:1;color:#666">height and width: aspect ratio locked</span></div>' +
        '<div class="row"><span>Height mm</span><input type="number" id="t-' + k + '-h" min="5" max="600" step="1"><span style="width:auto">cell</span><input type="number" id="t-' + k + '-cw" min="5" max="600" step="1"><span style="width:auto">gap</span><input type="number" id="t-' + k + '-sp" min="-50" max="100" step="0.5"></div>' +
        '<div class="row"><span>Thicken mm</span><input type="range" id="t-' + k + '-th" min="-3" max="8" step="0.1"><input type="number" id="t-' + k + '-thn" min="-5" max="10" step="0.1"></div>' +
        '<div class="row"><span>Position x, y</span><input type="number" id="t-' + k + '-x" step="1"><input type="number" id="t-' + k + '-y" step="1"></div>' +
        (k === 'closed' ? '<div class="row"><span>Hole offset</span><input type="number" id="t-closed-hdx" step="0.5"><input type="number" id="t-closed-hdy" step="0.5"></div>' :
          '<div class="row"><button id="t-' + k + '-centre">Centre holes in cells</button><button id="t-' + k + '-fit">Fit holes into thick parts</button></div>' +
          '<div class="xbox" id="t-' + k + '-xbox"></div>') +
        '<div class="note" id="info-' + k + '"></div></div>';
    });
    $('texts').innerHTML = h;
    $('texts').addEventListener('click', function (e) {
      var b = e.target.closest ? e.target.closest('button') : null; if (!b) return;
      var box = b.closest('.xbox'); if (!box) return;
      var key = box.id.replace(/^t-/, '').replace(/-xbox$/, '');
      if (b.classList.contains('xadd')) addExtraHole(key, parseInt(b.getAttribute('data-i'), 10));
      else if (b.classList.contains('xdel')) removeExtraHole(key, parseInt(b.getAttribute('data-k'), 10));
    });
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
    bind('frameInside', function () { return S.frame.inside; }, function (v) { S.frame.inside = v; fit(); });
    bind('exPlateCut', function () { return S.export.plateCut; }, function (v) { S.export.plateCut = v; });
    bind('frameLayout', function () { return S.frame.layout; }, function (v) { S.frame.layout = v; });
    bind('dims', function () { return S.ui.dims; }, function (v) { S.ui.dims = v; });
    bind('snap', function () { return S.ui.snap; }, function (v) { S.ui.snap = v; });
    bind('grid', function () { return S.ui.grid; }, function (v) { S.ui.grid = v; });
    bind('closedHole', function () { return S.export.closedHole; }, function (v) { S.export.closedHole = v; });
    bind('slotW', function () { return S.closedSlot.w; }, function (v) { S.closedSlot.w = cl(v, 1, 300); });
    bind('slotH', function () { return S.closedSlot.h; }, function (v) { S.closedSlot.h = cl(v, 1, 100); });
    bind('exOutline', function () { return S.export.plateOutline; }, function (v) { S.export.plateOutline = v; });
    bind('exPdfRef', function () { return S.export.pdfRef; }, function (v) { S.export.pdfRef = v; });
    bind('exPdfDims', function () { return S.export.pdfDims; }, function (v) { S.export.pdfDims = v; });
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
      bind('t-' + k + '-h', function () { return T().h; }, function (v) { var b0 = snapshotText(k); scaleText(k, b0, v / b0.h); });
      bind('t-' + k + '-w', function () { return Math.round(textScene(k, PPM).w * 100) / 100; }, function (v) { var b0 = snapshotText(k); scaleText(k, b0, v / b0.w); });
      bind('t-' + k + '-cw', function () { return T().cellW; }, function (v) { keepCentre(k, function () { T().cellW = cl(v, 5, 600); }); });
      bind('t-' + k + '-sp', function () { return T().spacing; }, function (v) { keepCentre(k, function () { T().spacing = cl(v, -50, 100); }); });
      bind('t-' + k + '-th', function () { return T().thicken; }, function (v) { T().thicken = cl(v, -5, 10); });
      bind('t-' + k + '-thn', function () { return T().thicken; }, function (v) { T().thicken = cl(v, -5, 10); });
      bind('t-' + k + '-x', function () { return T().x; }, function (v) { T().x = v; });
      bind('t-' + k + '-y', function () { return T().y; }, function (v) { T().y = v; });
      $('t-' + k + '-use').addEventListener('click', function () { useFont(k, $('t-' + k + '-other').value); });
      $('t-' + k + '-other').addEventListener('keydown', function (e) { if (e.key === 'Enter') useFont(k, e.target.value); });
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
    $('jsonApply').onclick = function () { importJson($('json').value); };
    $('exCopy').onclick = function () { $('exText').select(); try { document.execCommand('copy'); statusMsg = 'copied ' + $('exName').value; updateStatus(); } catch (e) { } };
    $('jsonCopy').onclick = function () {
      var text = core.toJson(S);
      function ok() { $('jsonMsg').textContent = 'JSON copied to the clipboard.'; $('jsonMsg').className = 'note ok'; }
      function fallback() { $('json').value = text; $('json').select(); try { if (document.execCommand('copy')) ok(); else throw 0; } catch (e) { $('jsonMsg').textContent = 'Copy was blocked: the JSON is selected, press Ctrl+C.'; $('jsonMsg').className = 'note warn'; } }
      if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(text).then(ok, fallback); else fallback();
    };
    $('jsonPaste').onclick = function () {
      function blocked() { $('json').focus(); $('json').select(); $('jsonMsg').textContent = 'Reading the clipboard is blocked here: press Ctrl+V now and the layout loads.'; $('jsonMsg').className = 'note warn'; }
      if (navigator.clipboard && navigator.clipboard.readText) navigator.clipboard.readText().then(importJson, blocked); else blocked();
    };
    $('json').addEventListener('paste', function (e) {
      var txt = e.clipboardData && e.clipboardData.getData('text'); e.preventDefault();
      if (txt) { $('json').value = txt; importJson(txt); }
    });
    document.addEventListener('paste', function (e) {
      var t = e.target, tag = (t.tagName || '').toLowerCase();
      if (t.id === 'json' || tag === 'textarea' || (tag === 'input' && t.type === 'text')) return;
      var txt = e.clipboardData && e.clipboardData.getData('text');
      if (txt && txt.trim().charAt(0) === '{') { e.preventDefault(); importJson(txt); }
    });
    $('reset').onclick = function () { loadState(core.defaultState()); fit(); commit(); };
    $('layoutSave').onclick = function () {
      var name = $('layoutName').value.trim(); if (!name) return;
      var all = layouts(); all[name] = core.toJson(S); lsSet(LS_LAYOUTS, JSON.stringify(all)); fillLayouts(name);
    };
    $('layoutLoad').onclick = function () { var all = layouts(), n = $('layoutList').value; if (all[n]) { loadState(core.fromJson(all[n])); fit(); commit(); } };
    $('layoutDel').onclick = function () { var all = layouts(); delete all[$('layoutList').value]; lsSet(LS_LAYOUTS, JSON.stringify(all)); fillLayouts(); };
  }
  function importJson(text) {
    try { var st = core.fromJson(text); loadState(st); fit(); commit(); $('json').value = core.toJson(S); $('jsonMsg').textContent = 'Layout loaded.'; $('jsonMsg').className = 'note ok'; }
    catch (e) { $('jsonMsg').textContent = 'Not loaded: ' + e.message; $('jsonMsg').className = 'note warn'; }
  }
  function layouts() { try { return JSON.parse(lsGet(LS_LAYOUTS) || '{}'); } catch (e) { return {}; } }
  function fillLayouts(pick) {
    var all = layouts(), names = Object.keys(all).sort();
    $('layoutList').innerHTML = names.map(function (n) { return '<option>' + n.replace(/</g, '&lt;') + '</option>'; }).join('');
    if (pick) $('layoutList').value = pick;
    if ($('layoutList')._dd) $('layoutList')._dd.refresh();
  }

  // ------------------------------------------------------------------ dropdowns (click opens the list, arrows preview live, Enter accepts, Esc reverts)
  function enhanceSelect(sel) {
    if (sel._dd) return;
    var wrap = document.createElement('div'); wrap.className = 'dd';
    sel.parentNode.insertBefore(wrap, sel); wrap.appendChild(sel);
    var btn = document.createElement('button'); btn.type = 'button'; btn.className = 'dd-btn'; wrap.insertBefore(btn, sel);
    var list = null, items = [], hi = -1, orig = null, typed = '', typedTimer = null;
    function label() { var o = sel.options[sel.selectedIndex]; btn.textContent = (o ? o.textContent : '') + ' ▾'; }
    sel._dd = { btn: btn, refresh: label };
    sel.addEventListener('change', label);
    function highlight(i) {
      if (hi >= 0 && items[hi]) items[hi].el.classList.remove('hi');
      hi = i; items[hi].el.classList.add('hi'); items[hi].el.scrollIntoView({ block: 'nearest' });
    }
    function apply(v) { if (sel.value !== v) { sel.value = v; sel.dispatchEvent(new Event('change', { bubbles: true })); } label(); }
    function outside(e) { if (list && !list.contains(e.target) && e.target !== btn) close(false); }
    function close(accept) {
      if (!list) return;
      list.remove(); list = null; document.removeEventListener('mousedown', outside, true);
      if (!accept) apply(orig);
      holdCommits = false; commit(); btn.focus();
    }
    function open() {
      if (list) return;
      orig = sel.value; holdCommits = true; items = []; hi = -1;
      list = document.createElement('div'); list.className = 'dd-list';
      function addOpt(o) {
        var d = document.createElement('div'); d.className = 'dd-item' + (o.value === orig ? ' cur' : ''); d.textContent = o.textContent;
        var idx = items.length; items.push({ el: d, value: o.value });
        d.addEventListener('mousemove', function () { if (hi !== idx) highlight(idx); });
        d.addEventListener('mousedown', function (e) { e.preventDefault(); apply(o.value); close(true); });
        list.appendChild(d);
      }
      Array.prototype.forEach.call(sel.children, function (c) {
        if (c.tagName === 'OPTGROUP') { var g = document.createElement('div'); g.className = 'dd-group'; g.textContent = c.label; list.appendChild(g); Array.prototype.forEach.call(c.children, addOpt); }
        else addOpt(c);
      });
      var r = btn.getBoundingClientRect(), below = window.innerHeight - r.bottom, up = r.top;
      list.style.left = r.left + 'px'; list.style.minWidth = Math.max(r.width, 160) + 'px';
      if (below >= 220 || below >= up) { list.style.top = r.bottom + 'px'; list.style.maxHeight = Math.min(300, below - 8) + 'px'; }
      else { list.style.bottom = (window.innerHeight - r.top) + 'px'; list.style.maxHeight = Math.min(300, up - 8) + 'px'; }
      document.body.appendChild(list);
      document.addEventListener('mousedown', outside, true);
      var cur = items.findIndex(function (it) { return it.value === orig; }); highlight(cur < 0 ? 0 : cur);
    }
    function move(i) { i = cl(i, 0, items.length - 1); highlight(i); apply(items[i].value); }
    btn.addEventListener('mousedown', function (e) { e.preventDefault(); btn.focus(); if (list) close(false); else open(); });
    btn.addEventListener('keydown', function (e) {
      if (!list) {
        if (e.key === 'ArrowDown' || e.key === 'ArrowUp' || e.key === 'Enter' || e.key === ' ') { e.preventDefault(); e.stopPropagation(); open(); }
        return;
      }
      var k = e.key, handled = true;
      if (k === 'ArrowDown') move(hi + 1); else if (k === 'ArrowUp') move(hi - 1); else if (k === 'Home') move(0); else if (k === 'End') move(items.length - 1);
      else if (k === 'PageDown') move(hi + 8); else if (k === 'PageUp') move(hi - 8);
      else if (k === 'Enter') close(true); else if (k === 'Escape') close(false);
      else if (k === 'Tab') { close(true); handled = false; }
      else if (k.length === 1 && !e.ctrlKey && !e.altKey) {
        typed += k.toLowerCase(); clearTimeout(typedTimer); typedTimer = setTimeout(function () { typed = ''; }, 800);
        var n = items.length, i, j;
        for (j = 1; j <= n; j++) { i = (hi + (typed.length > 1 ? j - 1 : j)) % n; if (items[i].el.textContent.toLowerCase().indexOf(typed) === 0) { move(i); break; } }
      } else handled = false;
      if (handled) { e.preventDefault(); e.stopPropagation(); }
    });
    btn.addEventListener('blur', function () { if (list) close(true); });
    label();
  }
  function enhanceAllSelects() { Array.prototype.forEach.call(document.querySelectorAll('select'), enhanceSelect); }

  // ------------------------------------------------------------------ drag a number to change it
  // Drag sideways: 1 step per 2 px. Drag up/down: 4 times faster (up = more). A click without moving puts the cursor in the field.
  function enableScrub(el) {
    var st = null;
    function decimals(step) { var t = String(step), i = t.indexOf('.'); return i < 0 ? 0 : t.length - i - 1; }
    function inSpinner(e) { var r = el.getBoundingClientRect(); return e.clientX > r.right - 20; }
    function move(e) {
      if (!st) return;
      var dx = e.clientX - st.x, dy = e.clientY - st.y;
      if (!st.moved) { if (Math.abs(dx) + Math.abs(dy) < 4) return; st.moved = true; document.body.style.userSelect = 'none'; document.body.style.cursor = 'ew-resize'; }
      e.preventDefault();
      var step = parseFloat(el.step) || 1, steps = Math.round((dx - dy * 4) / 2), v = st.v + steps * step;
      if (el.min !== '') v = Math.max(parseFloat(el.min), v);
      if (el.max !== '') v = Math.min(parseFloat(el.max), v);
      v = Math.round(v / step) * step;
      var t = v.toFixed(decimals(step));
      if (el.value !== t) { el.value = t; el.dispatchEvent(new Event('input', { bubbles: true })); }
    }
    function up() {
      if (!st) return;
      var was = st; st = null;
      window.removeEventListener('pointermove', move, true); window.removeEventListener('pointerup', up, true); window.removeEventListener('pointercancel', up, true);
      document.body.style.userSelect = ''; document.body.style.cursor = '';
      if (!was.moved) { el.focus(); el.select(); }
    }
    el.addEventListener('mousedown', function (e) { if (!el.readOnly && !inSpinner(e)) e.preventDefault(); });
    el.addEventListener('pointerdown', function (e) {
      if (el.readOnly || e.button !== 0 || inSpinner(e)) return;
      st = { x: e.clientX, y: e.clientY, v: parseFloat(el.value) || 0, moved: false };
      window.addEventListener('pointermove', move, true); window.addEventListener('pointerup', up, true); window.addEventListener('pointercancel', up, true);
    });
  }
  function enableAllScrubs() { Array.prototype.forEach.call(document.querySelectorAll('input[type=number]'), enableScrub); }

  // ------------------------------------------------------------------ tooltips
  var TIPS = {
    undo: 'Undo the last change (Ctrl+Z).', redo: 'Redo the change you undid (Ctrl+Y).', zin: 'Zoom in.', zout: 'Zoom out.', zfit: 'Zoom and centre so that the whole plate and the LED strip frame are visible.',
    preview: 'Switch between the design view and a preview: black plate, glowing red letters, glowing LEDs. Holes, boxes and dimensions are hidden in the preview.',
    plateW: 'Width of the vinyl plate in mm (100-980). This is the final cut size; the dimension arrows measure to its edges.', plateH: 'Height of the vinyl plate in mm (100-300). This is the final cut size; the dimension arrows measure to its edges.',
    holeD: 'Diameter in mm of the round holes: one behind every CASINO and OPEN letter, and the round CLOSED hole. One value for all (5-30 mm). 10 mm leaves room for about 6 wires.',
    minStroke: 'Minimum stroke thickness in mm. Parts of a letter thinner than this (hairlines, thin serifs, narrow joins) are listed as "thin parts" because they can break or burn away when cut. It is only a warning, nothing is changed. Default 3 mm.',
    frameInside: 'Put the LED strip frame on the plate, against its edges, instead of outside the plate edge. The strips then cover the outer 10 mm of the plate.',
    exPlateCut: 'Also draw the outer edge of the plate as a cut line in the letter cut exports (EPS and DXF; DXF layer PLATE_CUT).',
    frameLayout: 'How the 10 mm wide LED strip frame meets at the corners. "Top and bottom over the corners": the top and bottom strips are as long as the plate plus both side strips. "Sides over the corners": the side strips are longer instead. Only whole 100 mm units (3 LEDs each) are counted; the leftover is hatched.',
    stripAdd: 'Add a 100 x 10 mm LED strip piece (one 3-LED unit) in the middle of the plate. Drag it behind a letter.', stripRot: 'Rotate the selected strip piece between horizontal and vertical (key R).', stripDel: 'Delete the selected strip piece (key Delete).',
    dims: 'Which mm dimension arrows to draw to the final plate edge: from the centre, corners and hole centres of the selected object, from every object, or none.',
    snap: 'When on, dragging a box snaps its vertical and horizontal centre line to the centre lines of the other boxes and of the plate when they are within 3 mm (pink guide line); dragged holes and LED strip pieces snap to the plate centre lines. Hold Ctrl while dragging to move without snapping.',
    grid: 'Show the grid lines and the ruler numbers (mm) on the plate.',
    closedHole: 'What the drill pattern contains for CLOSED: a round hole for the wires, a slot for the LED stick window, or nothing. Both are drawn on screen; the unused one is dashed.',
    slotW: 'Width in mm of the CLOSED slot. Default 74 mm = LED window of a Grove stick (an assumption: check it on the real stick).', slotH: 'Height in mm of the CLOSED slot. Default 4 mm = LED window of a Grove stick (an assumption).',
    exOutline: 'Include the plate outline (blue) in the drill pattern exports.', exArrow: 'Include the 10 arrow cable holes in the drill pattern exports.',
    exEps: 'Download the drill pattern as EPS (1 unit = 1 mm, 5 mm margin).', exA4: 'Download the drill pattern as a 1:1 PDF on A4 sheets (tiles). Print at 100 %, no scaling, and tape the tiles together.', exA3: 'Download the drill pattern as a 1:1 PDF on A3 sheets (tiles). Print at 100 %, no scaling.',
    exDxf: 'Download the drill pattern as DXF (R12, mm, layers PLATE, HOLES, SLOT) for a cutter or CAD program.',
    exPdfRef: 'Draw the letters and the arrow sticks (with letter numbers such as C1, A2 and stick names such as L-CAP D2) in light grey under the holes of the drill PDF, so the sheet is also a layout drawing. Print it twice: drill through one copy and read the other to see what goes where.',
    exPdfDims: 'Write the distance from every hole centre to all four plate edges around the hole in the drill PDF (numbers in mm, rounded to 0.1, with thin dotted lines to the edges). For the CLOSED slot the distances are to its edges. Not in the EPS or DXF files.',
    exMirror: 'Mirror the letter cut left-to-right, for cutting from the back of the material.', exBoxes: 'Draw dashed bounding boxes in the letter cut export (the text boxes, or the pieces when nested).',
    exNest: 'Pack the letters tightly on a raw sheet of the size below instead of keeping them at their place on the plate.',
    exKerf: 'Laser kerf in mm (width of the cut). Every outline is grown by half of this so that the cut-out letter ends up the right size. Use 0 if unsure.',
    exSpacing: 'Gap in mm between letters when nesting them on the raw sheet.', exSheetW: 'Width in mm of the raw sheet used for nesting.', exSheetH: 'Height in mm of the raw sheet used for nesting. A warning appears if the letters do not fit.',
    exLetters: 'Download the letter outlines as EPS (as placed on the plate, or nested, mirrored etc. as set above).', exLettersDxf: 'Download the letter outlines as DXF (layers LETTERS and BOXES).',
    exName: 'File name of the last export.', exCopy: 'Copy the last export text. Use it if the download is blocked: paste into a file with the name shown on the left.', exText: 'Content of the last export as text.',
    layoutName: 'Name for saving the current layout in this browser.', layoutSave: 'Save the current layout under this name (stored in this browser only).', layoutList: 'Layouts saved in this browser.', layoutLoad: 'Load the selected saved layout.', layoutDel: 'Delete the selected saved layout.',
    reset: 'Throw away the current layout and start from the defaults (undo can bring it back).',
    json: 'The whole layout as JSON, updated live. Copy it to keep it, or paste a layout here and press "Apply JSON". Fonts are stored by name only.', jsonApply: 'Load the layout from the JSON text box (after you edited it by hand).', jsonCopy: 'Copy the current layout as JSON from the app to the clipboard.', jsonPaste: 'Read a layout (JSON) from the clipboard into the app. If reading the clipboard is blocked, press Ctrl+V anywhere on the page (outside the text fields) instead.'
  };
  function textTips(k) {
    var n = NAMES[k], t = {};
    t['t-' + k + '-text'] = 'The text of the ' + n + ' box (up to 20 characters). Each letter gets its own cell' + (k === 'closed' ? '.' : ' and its own hole.') + ' A space leaves an empty cell.';
    t['t-' + k + '-font'] = 'Font of the ' + n + ' text. Click to open the list; the arrow keys preview the font live, Enter accepts, Esc goes back to the previous font. Fonts are loaded by name from Google Fonts (needs network). "(thin)" fonts have hairline strokes that are hard to cut.';
    t['t-' + k + '-same'] = 'Use the font and bold setting of CASINO for this text, so the three words match.';
    t['t-' + k + '-other'] = 'Any font name from fonts.google.com, typed exactly (for example Bungee Hairline), then Enter or "Use".';
    t['t-' + k + '-use'] = 'Use the typed Google font name for this text.';
    t['t-' + k + '-bold'] = 'Use the bold (700) weight if the font has it. Otherwise the browser fakes it.';
    t['t-' + k + '-mode'] = 'Uniform cells: every letter is centred in a cell of the same width ("cell"). Natural widths: each letter takes the width the font gives it plus the gap.';
    t['t-' + k + '-h'] = 'Height in mm of the ' + n + ' text: the cap height (height of the letter H). The box has this height. Height and width scale the whole text uniformly (cells, gaps, hole positions; the aspect ratio is locked) around the centre of the box, which stays where it is. You can also drag the handles of the selected box.';
    t['t-' + k + '-w'] = 'Width in mm of the whole ' + n + ' box. Scales the whole text uniformly like the height (the aspect ratio is locked) around the centre of the box, which stays where it is.';
    t['t-' + k + '-cw'] = 'Width in mm of one cell in uniform mode (the letter is centred in it). Not used in natural mode.';
    t['t-' + k + '-sp'] = 'Gap in mm between the letters in natural mode (added to each letter width; half on each side). Not used in uniform mode.';
    t['t-' + k + '-th'] = 'Thicken in mm: grows (+) or shrinks (-) every letter outline evenly. Use it to make thin parts stronger or to make room for a hole. Affects the drawing, the hole check and the letter cut.';
    t['t-' + k + '-thn'] = t['t-' + k + '-th'];
    t['t-' + k + '-x'] = 'Left edge of the ' + n + ' box in mm from the left edge of the plate. Dragging the box changes it.';
    t['t-' + k + '-y'] = 'Top edge of the ' + n + ' box in mm from the top edge of the plate. Dragging the box changes it.';
    if (k === 'closed') { t['t-closed-hdx'] = 'Sideways shift in mm of the CLOSED hole/slot from the centre of the CLOSED box (right is positive). You can also drag it.'; t['t-closed-hdy'] = 'Vertical shift in mm of the CLOSED hole/slot from the centre of the CLOSED box (down is positive). You can also drag it.'; }
    else {
      t['t-' + k + '-centre'] = 'Put every hole in the middle of its letter cell.';
      t['t-' + k + '-fit'] = 'Move every hole to the nearest spot where the whole circle (plus 0.5 mm) is behind the letter, so it is hidden. If none fits, it goes to the thickest part.';
    }
    return t;
  }
  function applyTooltips() {
    var tips = Object.assign({}, TIPS);
    KEYS.forEach(function (k) { Object.assign(tips, textTips(k)); });
    tips.plateWn = tips.plateW; tips.plateHn = tips.plateH; tips.holeDn = tips.holeD; tips.minStrokeN = tips.minStroke;
    Object.keys(tips).forEach(function (id) {
      var el = $(id); if (!el) return;
      el.title = tips[id] + (el.type === 'number' && !el.readOnly ? ' (Drag the number left/right to change it; up/down is 4 times faster.)' : '');
      if (el._dd) el._dd.btn.title = tips[id];
      var par = el.parentElement;
      if (par && par.classList.contains('dd')) par = par.parentElement;
      if (par && par.tagName === 'LABEL') par.title = tips[id];
      var sp = el.previousElementSibling;
      if (sp && sp.classList.contains('dd')) sp = sp.previousElementSibling;
      while (sp && sp.tagName !== 'SPAN') sp = sp.previousElementSibling;
      if (sp && par && par.classList.contains('row') && !sp.title) sp.title = tips[id];
    });
  }
  window.PlateTips = function () { return TIPS; };

  // ------------------------------------------------------------------ mouse and keyboard
  var drag = null;
  function hit(p) {
    var sc = cur, r = S.holeD / 2, tol = 4 / view.k, i, k, key;
    if (sel.type && sel.type !== 'arrow' && sel.type !== 'strip') {
      var hp = handlePoints(boxOf(sc, sel.type)), hr = 7 / view.k;
      for (i = 0; i < hp.length; i++) if ((hp[i].hx !== 0 || hp[i].hy !== 0) && Math.abs(p.x - hp[i].x) <= hr && Math.abs(p.y - hp[i].y) <= hr) return { kind: 'handle', key: sel.type, hx: hp[i].hx, hy: hp[i].hy };
    }
    for (k = 0; k < 2; k++) {
      key = KEYS[k];
      for (i = 0; i < sc[key].cells.length; i++) {
        var cx0 = sc[key].cells[i], xe = cx0.extras || [], q;
        for (q = 0; q < xe.length; q++) if (Math.hypot(p.x - xe[q].x, p.y - xe[q].y) <= Math.max(r, tol)) return { kind: 'xhole', key: key, k: xe[q].k, idx: i };
      }
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
    var p = toMm(e), now = Date.now(), dbl = e.button === 0 && lastDown && now - lastDown.t < 400 && Math.hypot(e.clientX - lastDown.x, e.clientY - lastDown.y) < 5;
    lastDown = dbl ? null : { t: now, x: e.clientX, y: e.clientY };
    if (dbl && onDouble(p)) { drag = null; return; }
    var h = e.button === 1 ? null : hit(p);
    svg.setPointerCapture(e.pointerId); selX = null;
    if (!h) { drag = { kind: 'pan', sx: e.clientX, sy: e.clientY, cx: view.cx, cy: view.cy, moved: false }; return; }
    var sc = cur;
    if (h.kind === 'handle') { var bs = snapshotText(h.key); drag = { kind: 'scale', key: h.key, hx: h.hx, hy: h.hy, base: bs }; }
    else if (h.kind === 'hole') { sel = { type: h.key }; var c = sc[h.key].cells[h.idx]; drag = { kind: 'hole', key: h.key, idx: h.idx, gx: c.hole.x - p.x, gy: c.hole.y - p.y }; }
    else if (h.kind === 'xhole') { sel = { type: h.key }; selX = { key: h.key, k: h.k }; var xe0 = S.texts[h.key].extraHoles[h.k], c0 = sc[h.key].cells[h.idx]; drag = { kind: 'xhole', key: h.key, k: h.k, idx: h.idx, gx: c0.cx + xe0.dx - p.x, gy: c0.cy + xe0.dy - p.y }; }
    else if (h.kind === 'closedRound') { sel = { type: 'closed' }; drag = { kind: 'closedRound', gx: sc.closedRound.x - p.x, gy: sc.closedRound.y - p.y }; }
    else if (h.kind === 'closedSlot') { sel = { type: 'closed' }; drag = { kind: 'closedSlot', gx: sc.closedSlot.x - p.x, gy: sc.closedSlot.y - p.y }; }
    else if (h.kind === 'strip') { sel = { type: 'strip', idx: h.idx }; var s0 = S.strips[h.idx]; drag = { kind: 'strip', idx: h.idx, ox: s0.x, oy: s0.y, sx: p.x, sy: p.y }; }
    else { sel = { type: h.key }; var o = h.key === 'arrow' ? S.arrow : S.texts[h.key]; drag = { kind: 'box', key: h.key, ox: o.x, oy: o.y, sx: p.x, sy: p.y }; }
    drag.moved = false; render(); updateInfo();
  });
  svg.addEventListener('pointermove', function (e) {
    if (!drag) {
      var hv = cur && e.buttons === 0 ? hit(toMm(e)) : null, cur2 = '';
      if (hv && hv.kind === 'handle') cur2 = hv.hy === 0 ? 'ew-resize' : hv.hx === 0 ? 'ns-resize' : (hv.hx * hv.hy > 0 ? 'nwse-resize' : 'nesw-resize');
      if (svg.style.cursor !== cur2) svg.style.cursor = cur2;
      return;
    }
    if (drag.kind === 'pan') {
      var dx = e.clientX - drag.sx, dy = e.clientY - drag.sy;
      if (Math.abs(dx) + Math.abs(dy) > 2) drag.moved = true;
      view.cx = drag.cx - dx / view.k; view.cy = drag.cy - dy / view.k; render(); return;
    }
    var p = toMm(e); drag.moved = true; guides = { v: null, h: null };
    var snapOn = S.ui.snap && !(e.ctrlKey || e.metaKey);   // hold Ctrl while dragging to move freely
    // snaps a point (hole, strip centre) to the plate centre lines within 3 mm; returns the corrected point
    function snapPt(x, y) {
      if (!snapOn) return { x: x, y: y };
      var sn = core.snapCentres({ x: x, y: y, w: 0, h: 0 }, [{ cx: S.plate.w / 2, cy: S.plate.h / 2 }], 3);
      guides = { v: sn.vertical, h: sn.horizontal }; return { x: x + sn.dx, y: y + sn.dy };
    }
    if (drag.kind === 'scale') {
      var bs2 = drag.base, sf = core.handleScale(drag.hx, drag.hy, bs2.cx, bs2.cy, bs2.w, bs2.boxH, p.x, p.y);
      if (snapOn) {
        var xs = [0, S.plate.w / 2, S.plate.w], ys = [0, S.plate.h / 2, S.plate.h];
        KEYS.concat(['arrow']).forEach(function (kk) { if (kk !== drag.key) { var q = boxOf(cur, kk); xs.push(q.x, q.x + q.w / 2, q.x + q.w); ys.push(q.y, q.y + q.h / 2, q.y + q.h); } });
        var sr = core.snapHandleScale(sf, drag.hx, drag.hy, bs2.cx, bs2.cy, bs2.w, bs2.boxH, xs, ys, 6 / view.k);
        sf = sr.s; guides = { v: sr.vertical, h: sr.horizontal };
      }
      scaleText(drag.key, bs2, sf); syncControls(); render(); return;
    }
    if (drag.kind === 'box') {
      var o = drag.key === 'arrow' ? S.arrow : S.texts[drag.key], b = boxOf(cur, drag.key);
      var nx = drag.ox + p.x - drag.sx, ny = drag.oy + p.y - drag.sy;
      if (snapOn) {
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
      var hp = snapPt(p.x + drag.gx, p.y + drag.gy);
      t.holeOffsets[drag.idx] = { dx: Math.round((hp.x - c.cx) * 100) / 100, dy: Math.round((hp.y - c.cy) * 100) / 100 };
    } else if (drag.kind === 'xhole') {
      var cx1 = cur[drag.key].cells[drag.idx], xe1 = S.texts[drag.key].extraHoles[drag.k];
      var xp = snapPt(p.x + drag.gx, p.y + drag.gy);
      xe1.dx = Math.round((xp.x - cx1.cx) * 100) / 100; xe1.dy = Math.round((xp.y - cx1.cy) * 100) / 100;
    } else if (drag.kind === 'closedRound' || drag.kind === 'closedSlot') {
      var cb = cur.closed, cp = snapPt(p.x + drag.gx, p.y + drag.gy), dx2 = Math.round((cp.x - (cb.x + cb.w / 2)) * 100) / 100, dy2 = Math.round((cp.y - (cb.y + cb.h / 2)) * 100) / 100;
      S.closedRound.dx = S.closedSlot.dx = dx2; S.closedRound.dy = S.closedSlot.dy = dy2; syncControls();
    } else if (drag.kind === 'strip') {
      var s1 = S.strips[drag.idx]; s1.x = drag.ox + p.x - drag.sx; s1.y = drag.oy + p.y - drag.sy;
      var sr1 = core.stripRect(s1), sp = snapPt(sr1.x + sr1.w / 2, sr1.y + sr1.h / 2);
      s1.x += sp.x - (sr1.x + sr1.w / 2); s1.y += sp.y - (sr1.y + sr1.h / 2);
      s1.x = Math.round(s1.x * 100) / 100; s1.y = Math.round(s1.y * 100) / 100;
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
  var selX = null;   // selected extra hole {key, k}
  function addExtraHole(key, i, at) {
    var t = S.texts[key], sc = scene(PPM), c = sc[key].cells[i], r = S.holeD / 2;
    if (!c || !c.glyph || t.extraHoles.length >= 40) return false;
    var o;
    if (at) o = { dx: Math.round((at.x - c.cx) * 100) / 100, dy: Math.round((at.y - c.cy) * 100) / 100 };
    else o = core.suggestHole(c, r, t.thicken, [c.hole].concat(c.extras || []));
    t.extraHoles.push({ i: i, dx: o.dx, dy: o.dy }); selX = { key: key, k: t.extraHoles.length - 1 }; sel = { type: key };
    changed(); updateInfo(); return true;
  }
  function removeExtraHole(key, k) {
    var t = S.texts[key]; if (!t.extraHoles[k]) return;
    t.extraHoles.splice(k, 1); selX = null; changed(); updateInfo();
  }
  // buttons "add a hole to letter X" and the list of the extra holes of one text
  function renderExtras(key) {
    var t = S.texts[key], box = $('t-' + key + '-xbox'); if (!box) return;
    var h = '<div class="xrow"><span class="xl">Add a hole to</span>', i, sc = cur || scene(PPM);
    for (i = 0; i < t.text.length; i++) if (t.text.charAt(i) !== ' ') h += '<button class="xadd" data-i="' + i + '" title="Add one more hole to letter ' + (i + 1) + ' (' + t.text.charAt(i) + ') at the roomiest spot, then drag it where you want it.">' + t.text.charAt(i) + (i + 1) + ' +</button>';
    h += '</div>';
    var chips = '';
    t.extraHoles.forEach(function (e, k) {
      if (e.i >= t.text.length) return;
      chips += '<span class="xchip' + (selX && selX.key === key && selX.k === k ? ' on' : '') + '" data-k="' + k + '">' + t.text.charAt(e.i) + (e.i + 1) + ' hole ' + (k + 1) + ' <button class="xdel" data-k="' + k + '" title="Remove this extra hole">&times;</button></span>';
    });
    h += '<div class="xlist">' + (chips || '<span class="xnone">No extra holes. Double-click inside a letter to add one, double-click a hole to remove it.</span>') + '</div>';
    box.innerHTML = h;
  }
  function renderAllExtras() { KEYS.forEach(function (k) { if (k !== 'closed') renderExtras(k); }); }
  function deleteStrip() { if (selX) { removeExtraHole(selX.key, selX.k); return; } if (sel.type === 'strip' && S.strips[sel.idx]) { S.strips.splice(sel.idx, 1); sel = { type: null }; changed(); updateInfo(); } }
  document.addEventListener('keydown', function (e) {
    var tag = (e.target.tagName || '').toLowerCase();
    if ((e.ctrlKey || e.metaKey) && (e.key === 'z' || e.key === 'Z')) { if (tag === 'textarea' || (tag === 'input' && e.target.type === 'text')) return; e.preventDefault(); e.shiftKey ? redo() : undo(); return; }
    if ((e.ctrlKey || e.metaKey) && (e.key === 'y' || e.key === 'Y')) { if (tag === 'textarea' || (tag === 'input' && e.target.type === 'text')) return; e.preventDefault(); redo(); return; }
    if (tag === 'input' || tag === 'select' || tag === 'textarea' || (e.target.classList && e.target.classList.contains('dd-btn'))) return;
    if (e.key === 'r' || e.key === 'R') { rotateStrip(); return; }
    if (e.key === 'Delete' || e.key === 'Backspace') { deleteStrip(); return; }
    if (e.key === 'Escape') { sel = { type: null }; selX = null; render(); updateInfo(); return; }
    var step = e.altKey ? 0.1 : e.shiftKey ? 10 : 1, dx = 0, dy = 0;
    if (e.key === 'ArrowLeft') dx = -step; else if (e.key === 'ArrowRight') dx = step; else if (e.key === 'ArrowUp') dy = -step; else if (e.key === 'ArrowDown') dy = step; else return;
    e.preventDefault();
    var o = sel.type === 'strip' ? S.strips[sel.idx] : sel.type === 'arrow' ? S.arrow : sel.type ? S.texts[sel.type] : null;
    if (!o) return;
    o.x = Math.round((o.x + dx) * 100) / 100; o.y = Math.round((o.y + dy) * 100) / 100; syncControls(); changed();
  });
  // Double click (own detection: the SVG is rebuilt on every pointer press, so the browser's dblclick event never fires).
  // On an extra hole: remove it. Inside a letter: add an extra hole at that spot. Returns true when it did something.
  var lastDown = null;
  function onDouble(p) {
    var h = hit(p), done = false;
    if (h && h.kind === 'xhole') { removeExtraHole(h.key, h.k); return true; }
    ['casino', 'open'].forEach(function (key) {
      if (h && ['hole', 'handle', 'closedRound', 'closedSlot'].indexOf(h.kind) >= 0) return;
      cur[key].cells.forEach(function (c, i) {
        if (c.glyph && c.glyph.insideDist(p.x - c.gox, p.y - c.goy, S.texts[key].thicken) >= 0) done = addExtraHole(key, i, p) || done;
      });
    });
    return done;
  }
  window.addEventListener('resize', render);

  // ------------------------------------------------------------------ exports
  function drillScene() { return scene(PPM); }
  function buildDrillEps() { return core.epsFromItems(core.drillItems(S, drillScene()), S.plate, 'Casino plate drill pattern'); }
  // Light grey letters and arrow sticks with labels under the drill pattern: the PDF is then also a layout drawing.
  function pdfReference() {
    var sc = scene(PPM_EXPORT), ref = [], r = S.holeD / 2;
    ARROW.sticks.forEach(function (st) {
      var pts = st.pts.map(function (q) { return [q[0] + S.arrow.x, q[1] + S.arrow.y]; }), mx = 0, my = 0;
      pts.forEach(function (q) { mx += q[0] / pts.length; my += q[1] / pts.length; });
      ref.push({ kind: 'poly', loops: [pts], fill: 0.93, stroke: 0.5 });
      ref.push({ kind: 'text', x: mx - 24, y: my + 3, s: st.name + ' ' + st.connector, size: 5 });
    });
    KEYS.forEach(function (key) {
      var off = S.texts[key].thicken;
      sc[key].cells.forEach(function (c) {
        if (!c.glyph) return;
        var loops = contours(c.glyph, off, 0.03).map(function (p) { return p.map(function (q) { return [q[0] + c.gox, q[1] + c.goy]; }); });
        ref.push({ kind: 'poly', loops: loops, fill: 0.85, stroke: 0.4 });
        if (c.hole) ref.push({ kind: 'text', x: c.hole.x + r + 4, y: c.hole.y + r + 3, s: S.texts[key].text.charAt(c.index) + (c.index + 1), size: 7 });
        (c.extras || []).forEach(function (e, q) { ref.push({ kind: 'text', x: e.x + r + 4, y: e.y + r + 3, s: S.texts[key].text.charAt(c.index) + (c.index + 1) + String.fromCharCode(98 + q), size: 7 }); });
      });
    });
    ref.push({ kind: 'text', x: sc.closedRound.x + r + 8, y: sc.closedRound.y + r + 3, s: 'CLOSED', size: 6 });
    return ref;
  }
  function buildDrillPdf(paper) {
    var items = core.drillItems(S, drillScene()), ref = (S.export.pdfRef ? pdfReference() : []).concat(S.export.pdfDims ? core.holeDimItems(items, S.plate) : []);
    return core.pdfTiles(items, S.plate, paper, 'Casino plate drill pattern', ref);
  }
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
    var lp = letterPieces(PPM_EXPORT), pieces = lp.pieces, ex = S.export, W = S.plate.w, H = S.plate.h, polys = [], boxes = [], plateCuts = [], message = '', areaW = W, areaH = H;
    var plateRect = [[0, 0], [W, 0], [W, H], [0, H]];
    if (ex.plateCut && !ex.nest) { plateCuts.push(plateRect); polys.push(plateRect); }
    if (!ex.nest) {
      pieces.forEach(function (p) { p.polys.forEach(function (poly) { polys.push(ex.mirror ? poly.map(function (q) { return [W - q[0], q[1]]; }) : poly); }); });
      if (ex.boxes) KEYS.forEach(function (k) { var b = boxOf(lp.sc, k); boxes.push({ x: ex.mirror ? W - b.x - b.w : b.x, y: b.y, w: b.w, h: b.h }); });
      message = (pieces.length) + ' letters' + (ex.plateCut ? ' + plate edge' : '') + ' as placed on the plate' + (ex.mirror ? ' (mirrored)' : '');
    } else {
      if (ex.plateCut) pieces = pieces.concat([{ polys: [plateRect], bb: { x0: 0, y0: 0, x1: W, y1: H }, plate: true }]);
      var items = pieces.map(function (p, i) { return { id: i, w: p.bb.x1 - p.bb.x0, h: p.bb.y1 - p.bb.y0 }; });
      var res = core.nestShelves(items, ex.sheetW, ex.sheetH, ex.spacing);
      res.placed.forEach(function (pl) {
        var p = pieces[pl.id], w = p.bb.x1 - p.bb.x0, h = p.bb.y1 - p.bb.y0;
        p.polys.forEach(function (poly) {
          var moved = poly.map(function (q) { var x = ex.mirror ? p.bb.x0 + p.bb.x1 - q[0] : q[0]; return [x - p.bb.x0 + pl.x, q[1] - p.bb.y0 + pl.y]; });
          polys.push(moved); if (p.plate) plateCuts.push(moved);
        });
        if (ex.boxes) boxes.push({ x: pl.x, y: pl.y, w: w, h: h });
      });
      areaW = ex.sheetW; areaH = Math.max(ex.sheetH, res.usedH);
      message = res.fits ? 'nested ' + (pieces.length - (ex.plateCut ? 1 : 0)) + ' letters' + (ex.plateCut ? ' + plate edge' : '') + ', ' + f2(res.usedH) + ' mm of the ' + ex.sheetH + ' mm sheet used' : 'WARNING: the letters do not fit the ' + ex.sheetW + ' x ' + ex.sheetH + ' mm sheet (need ' + f2(res.usedH) + ' mm height)';
    }
    return { text: core.epsFromPolys(polys, areaW, areaH, 'Casino plate letters', boxes), dxf: core.dxfFromPolys(polys.filter(function (p) { return plateCuts.indexOf(p) < 0; }), areaH, boxes, plateCuts), message: message, polys: polys.length };
  }
  function download(name, text, type) {
    $('exName').value = name; $('exText').value = text;
    var b = new Blob([text], { type: type }), a = document.createElement('a');
    a.href = URL.createObjectURL(b); a.download = name; document.body.appendChild(a); a.click();
    setTimeout(function () { URL.revokeObjectURL(a.href); a.remove(); }, 1000);
  }

  // ------------------------------------------------------------------ start
  function init() {
    buildControls(); enhanceAllSelects(); enableAllScrubs(); applyTooltips();
    var saved = lsGet(LS_AUTO);
    if (saved) { try { S = core.fromJson(saved); } catch (e) { S = core.defaultState(); } }
    ensureAllFonts(); fillLayouts(); syncControls(); fit(); commit();
    $('preview').style.fontWeight = S.ui.preview ? 'bold' : 'normal';
  }
  window.PlateApp = {
    state: function () { return S; }, scene: function () { return scene(PPM); }, drillEps: buildDrillEps, drillDxf: buildDrillDxf, drillPdf: buildDrillPdf, lettersEps: buildLettersEps,
    shownFont: function (k) { return effFont(k).font; }, fontStatus: function (n) { return fontState[n] && fontState[n].status; }, useFont: useFont, commit: commit, view: view, selection: function () { return sel; }, hist: function () { return { n: hist.length, pos: histPos }; }
  };
  init();
})();
