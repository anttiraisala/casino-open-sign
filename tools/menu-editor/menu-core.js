/*
 * Menu core: validation of the menu JSON (format 1) and the menu engine (keys, editing, EEPROM image, 2 x 16 rows).
 * Pure logic, no DOM: works in the browser (window.MenuCore) and in Node (require). Time is always passed in as `now` (ms).
 * The same rules are implemented on the Arduino by the generated tables; this file is the reference and the preview.
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.MenuCore = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  var COLS = 16;
  var LEFT_ARROW = '\x7f', RIGHT_ARROW = '\x7e';          // HD44780 ROM characters
  var LOCK_CHAR = '\u0001', TIMEOUT_MS = 60000, MARQUEE_PAUSE_MS = 1000, MARQUEE_STEP_MS = 400, STATUS_ALT_MS = 3000;
  var MAX_PRESET_VALUES = 16;
  var KINDS = { int: 1, float: 1, choice: 1, bool: 1 };
  var BUILTIN = {
    int: { min: 0, max: 100, step: 1, wrap: false, shift: 10, readOnly: false, persist: true, decimals: 0 },
    float: { min: 0, max: 1, scale: 'linear', steps: 50, wrap: false, shift: 10, readOnly: false, persist: true, decimals: 2 },
    choice: { wrap: false, shift: 1, readOnly: false, persist: true },
    bool: { wrap: false, shift: 1, readOnly: false, persist: true, trueText: 'On', falseText: 'Off' }
  };
  var FIELDS = ['kind', 'name', 'type', 'min', 'max', 'default', 'step', 'steps', 'scale', 'wrap', 'shift', 'readOnly', 'persist', 'bind',
    'unit', 'decimals', 'format', 'valueTexts', 'choices', 'trueText', 'falseText', 'callbacks', 'text'];

  function isNum(v) { return typeof v === 'number' && isFinite(v); }
  function clone(o) { return JSON.parse(JSON.stringify(o)); }

  // ------------------------------------------------------------------ property maths (positions)
  // A property has N positions 0..N-1; stepping moves the position; wrap or stop at the ends.
  function count(p) {
    if (p.kind === 'int') return Math.floor((p.max - p.min) / p.step + 1e-9) + 1;
    if (p.kind === 'float') return p.steps + 1;
    if (p.kind === 'choice') return p.choices.length;
    return 2;
  }
  function valueAt(p, i) {
    if (p.kind === 'int') return p.min + i * p.step;
    if (p.kind === 'float') {
      if (p.scale === 'log') return p.min * Math.pow(p.max / p.min, i / p.steps);
      return p.min + i * (p.max - p.min) / p.steps;
    }
    return i;
  }
  function posOf(p, v) {
    var n = count(p), best = 0, bd = Infinity;
    if (p.kind === 'choice' || p.kind === 'bool') { v = +v; return Math.max(0, Math.min(n - 1, Math.round(v))); }
    for (var i = 0; i < n; i++) {
      var d = p.scale === 'log' ? Math.abs(Math.log(valueAt(p, i)) - Math.log(Math.max(v, 1e-300))) : Math.abs(valueAt(p, i) - v);
      if (d < bd - 1e-12) { bd = d; best = i; }
    }
    return best;
  }
  function movePos(p, pos, delta) {
    var n = count(p), q = pos + delta;
    if (p.wrap) return ((q % n) + n) % n;
    return Math.max(0, Math.min(n - 1, q));
  }
  function fmtNum(x, dec) { return dec > 0 ? x.toFixed(dec) : String(Math.round(x)); }
  function valueText(p, pos) {
    var v = valueAt(p, pos), t;
    if (p.kind === 'bool') return v ? p.trueText : p.falseText;
    if (p.kind === 'choice') return String(p.choices[pos]);
    if (p.valueTexts && p.valueTexts[String(Math.round(v * 1e6) / 1e6)] !== undefined) return String(p.valueTexts[String(Math.round(v * 1e6) / 1e6)]);
    if (p.format === 'hex') t = Math.round(v).toString(16).toUpperCase();
    else t = fmtNum(v, p.decimals);
    return t + (p.unit || '');
  }

  // ------------------------------------------------------------------ build (resolve + validate)
  function resolveProp(id, def, types, errors) {
    var chain = [], seen = {}, cur = def, kind = def.kind;
    while (cur) {
      chain.unshift(cur);
      if (!cur.type) break;
      if (seen[cur.type]) { errors.push('property ' + id + ': type loop at ' + cur.type); return null; }
      seen[cur.type] = 1;
      cur = types[cur.type];
      if (!cur) { errors.push('property ' + id + ': unknown type ' + chain[0].type); return null; }
    }
    chain.forEach(function (c) { if (c.kind) kind = c.kind; });
    if (!KINDS[kind]) { errors.push('property ' + id + ': kind missing or unknown (' + kind + ')'); return null; }
    var p = clone(BUILTIN[kind]);
    chain.forEach(function (c) { FIELDS.forEach(function (f) { if (c[f] !== undefined && f !== 'type') p[f] = clone(c[f]); }); });
    p.id = id; p.kind = kind;
    if (!p.name) p.name = id;
    if (!Array.isArray(p.callbacks)) p.callbacks = p.callbacks ? [p.callbacks] : [];
    return p;
  }
  function checkProp(p, errors, warnings) {
    var e = function (m) { errors.push('property ' + p.id + ': ' + m); };
    if (p.kind === 'int') {
      if (!isNum(p.min) || !isNum(p.max) || p.min > p.max) { e('min/max invalid'); return false; }
      if (!isNum(p.step) || p.step < 1 || Math.floor(p.step) !== p.step) { e('step must be an integer >= 1'); return false; }
    } else if (p.kind === 'float') {
      if (!isNum(p.min) || !isNum(p.max) || p.min >= p.max) { e('min/max invalid'); return false; }
      if (p.scale !== 'linear' && p.scale !== 'log') { e('scale must be linear or log'); return false; }
      if (p.scale === 'log' && p.min <= 0) { e('log scale needs min > 0'); return false; }
      if (!isNum(p.steps) || p.steps < 1 || Math.floor(p.steps) !== p.steps) { e('steps must be an integer >= 1'); return false; }
    } else if (p.kind === 'choice') {
      if (!Array.isArray(p.choices) || p.choices.length < 2) { e('choice needs at least 2 choices'); return false; }
    }
    if (!isNum(p.shift) || p.shift < 1) { e('shift must be >= 1'); return false; }
    if (p.default === undefined) p.default = p.kind === 'int' || p.kind === 'float' ? p.min : (p.kind === 'bool' ? false : 0);
    if (p.kind === 'choice' && typeof p.default === 'string') {
      var ci = p.choices.indexOf(p.default);
      if (ci < 0) { e('default is not one of the choices'); return false; }
      p.default = ci;
    }
    if (p.kind === 'bool') p.default = p.default ? 1 : 0;
    var d = +p.default;
    if (p.kind === 'int' || p.kind === 'float') {
      if (d < p.min - 1e-9 || d > p.max + 1e-9) { e('default outside min..max'); return false; }
    } else if (d < 0 || d >= count(p) || Math.floor(d) !== d) { e('default invalid'); return false; }
    p.defaultPos = posOf(p, d);
    if (p.kind === 'int' || p.kind === 'float') {
      var n = count(p), g = valueAt(p, p.defaultPos), sp = n > 1 ? Math.abs(valueAt(p, Math.min(n - 1, p.defaultPos + 1)) - valueAt(p, Math.max(0, p.defaultPos - 1))) / 2 : 1;
      if (Math.abs(g - d) > 0.25 * sp + 1e-9) warnings.push('property ' + p.id + ': default ' + d + ' snapped to ' + g);
    }
    p.count = count(p);
    return true;
  }

  function build(src) {
    var errors = [], warnings = [], m = { errors: errors, warnings: warnings, ok: false, props: {}, order: [], source: src, root: null, status: null };
    if (!src || typeof src !== 'object') { errors.push('menu file is not an object'); return m; }
    if (src.format !== 1) { errors.push('format must be 1'); return m; }
    var common = src.common || {}, types = common.types || {}, defs = {}, menus = common.menus || {}, i;
    var idRe = /^[A-Za-z][A-Za-z0-9_]*$/;
    Object.keys(common.properties || {}).forEach(function (id) {
      if (!idRe.test(id)) errors.push('bad property id ' + id);
      defs[id] = common.properties[id];
    });
    var seenMenus = {};
    function walkItems(items, path, stack, collect) {
      var out = [];
      if (!Array.isArray(items) || !items.length) { errors.push('menu ' + path + ' has no items'); return out; }
      items.forEach(function (it, k) {
        var where = path + '[' + k + ']';
        if (it.menu !== undefined) {
          out.push({ type: 'menu', name: String(it.menu), items: walkItems(it.items, where + '/' + it.menu, stack, collect), text: it.text });
        } else if (it.menuRef !== undefined) {
          var mr = menus[it.menuRef];
          if (!mr) { errors.push(where + ': unknown menuRef ' + it.menuRef); return; }
          if (stack.indexOf(it.menuRef) >= 0) { errors.push(where + ': menu loop via ' + it.menuRef); return; }
          out.push({ type: 'menu', name: String(it.name || mr.name || it.menuRef), ref: it.menuRef,
            items: walkItems(mr.items, where + '/' + it.menuRef, stack.concat([it.menuRef]), collect), text: it.text });
        } else if (it.property !== undefined) {
          if (!idRe.test(it.property)) errors.push(where + ': bad property id ' + it.property);
          if (defs[it.property]) errors.push(where + ': property id ' + it.property + ' defined twice');
          var d = clone(it); delete d.property; defs[it.property] = d; collect.push(it.property);
          out.push({ type: 'prop', id: it.property, text: it.text });
        } else if (it.propertyRef !== undefined) {
          out.push({ type: 'prop', id: it.propertyRef, text: it.text, line: where });
        } else if (it.preset !== undefined) {
          out.push({ type: 'preset', name: String(it.preset), text: it.text, set: it.set, setAll: it.setAll, confirm: !!it.confirm, confirmText: it.confirmText, where: where });
        } else errors.push(where + ': item is not menu, menuRef, property, propertyRef or preset');
      });
      return out;
    }
    var inline = [];
    var rootSrc = src.root;
    if (!rootSrc || !Array.isArray(rootSrc.items)) { errors.push('root.items missing'); return m; }
    m.root = { type: 'menu', name: String(rootSrc.name || 'Menu'), items: walkItems(rootSrc.items, 'root', [], inline) };
    Object.keys(menus).forEach(function (id) {
      if (!idRe.test(id)) errors.push('bad menu id ' + id);
      if (menus[id] && Array.isArray(menus[id].items)) walkItems(menus[id].items, 'menus.' + id, [id], inline);   // detects loops inside unused menus too
      else errors.push('menu ' + id + ' has no items');
    });
    // resolve properties in declaration order: common first, then inline in tree order
    Object.keys(defs).forEach(function (id) {
      var p = resolveProp(id, defs[id], types, errors);
      if (p && checkProp(p, errors, warnings)) { m.props[id] = p; m.order.push(id); }
    });
    (function check(menu) {
      menu.items.forEach(function (it) {
        if (it.type === 'prop' && !defs[it.id]) errors.push((it.line || 'menu') + ': unknown property ' + it.id);
        if (it.type === 'menu') check(it);
        if (it.type === 'preset') {
          var n = 0, w = it.where;
          if (it.set === undefined && it.setAll === undefined) errors.push(w + ': preset has neither set nor setAll');
          if (it.setAll !== undefined && it.setAll !== 'default') errors.push(w + ': setAll must be "default"');
          if (it.set) it.set = Object.assign({}, it.set);
          Object.keys(it.set || {}).forEach(function (id) {
            n++;
            var p = m.props[id], v = it.set[id];
            if (!p) { errors.push(w + ': preset sets unknown property ' + id); return; }
            if (p.readOnly) { warnings.push(w + ': preset sets read-only property ' + id + ' (it is skipped)'); delete it.set[id]; return; }
            if (v === 'default') return;
            if (p.kind === 'int' || p.kind === 'float') {
              if (!isNum(v) || v < p.min - 1e-9 || v > p.max + 1e-9) errors.push(w + ': value ' + v + ' for ' + id + ' outside ' + p.min + '..' + p.max);
              else if (Math.abs(valueAt(p, posOf(p, v)) - v) > 0.25 * Math.abs(valueAt(p, Math.min(p.count - 1, posOf(p, v) + 1)) - valueAt(p, Math.max(0, posOf(p, v) - 1))) / 2 + 1e-9) warnings.push(w + ': value ' + v + ' for ' + id + ' snapped to ' + valueAt(p, posOf(p, v)));
            } else if (p.kind === 'bool') { if (typeof v !== 'boolean') errors.push(w + ': value for ' + id + ' must be true or false'); }
            else if (p.choices.indexOf(v) < 0 && !(isNum(v) && v >= 0 && v < p.count && Math.floor(v) === v)) errors.push(w + ': value ' + v + ' is not a choice of ' + id);
          });
          if (n > MAX_PRESET_VALUES) errors.push(w + ': preset sets ' + n + ' values (max ' + MAX_PRESET_VALUES + ')');
        }
      });
    })(m.root);
    var st = src.status;
    if (st) {
      ['program', 'casino', 'arrow'].forEach(function (k) { if (!m.props[st[k]]) errors.push('status.' + k + ' must name a property'); });
      m.status = { program: st.program, casino: st.casino, arrow: st.arrow, randomValue: st.randomValue === undefined ? 15 : st.randomValue, arrowAscii: !!st.arrowAscii };
    }
    m.ok = errors.length === 0;
    return m;
  }

  // ------------------------------------------------------------------ EEPROM image
  function fnv(s) {
    var h = 0x811c9dc5;
    for (var i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = (h * 0x01000193) >>> 0; }
    return ('00000000' + h.toString(16)).slice(-8);
  }
  function persistIds(m) { return m.order.filter(function (id) { return m.props[id].persist && !m.props[id].readOnly; }); }
  function layoutTag(m) {
    return fnv(persistIds(m).map(function (id) { var p = m.props[id]; return [id, p.kind, p.count].join(':'); }).join('|'));
  }

  // ------------------------------------------------------------------ engine
  function fit(left, right, now, since) {
    right = right || '';
    var avail = COLS - right.length - (right ? 1 : 0), out;
    if (avail < 0) avail = 0;
    if (left.length <= avail) out = left;
    else {
      var maxOff = left.length - avail, t = Math.max(0, now - since), total = MARQUEE_PAUSE_MS + maxOff * MARQUEE_STEP_MS + MARQUEE_PAUSE_MS, off;
      t = t % total;
      off = t < MARQUEE_PAUSE_MS ? 0 : Math.min(maxOff, Math.floor((t - MARQUEE_PAUSE_MS) / MARQUEE_STEP_MS));
      out = left.substr(off, avail);
    }
    while (out.length < avail) out += ' ';
    var s = right ? out + ' ' + right : out;
    return (s + '                ').substr(0, COLS);
  }

  function Engine(model, opts) {
    if (!model.ok) throw new Error('menu model has errors');
    opts = opts || {};
    this.m = model; this.pos = {}; this.opts = opts;
    this.mode = 'status'; this.stack = []; this.edit = null; this.confirmItem = null;
    this.lastInput = 0; this.since = 0; this.runtime = { changeInSec: null };
    this.callLog = []; this.batches = [];
    var self = this;
    model.order.forEach(function (id) { self.pos[id] = model.props[id].defaultPos; });
    this.tag = layoutTag(model);
  }
  Engine.prototype._setPos = function (id, pos, silent) {
    var p = this.m.props[id];
    if (pos === this.pos[id]) return false;
    this.pos[id] = pos;
    if (!silent) this._call(id);
    return true;
  };
  Engine.prototype._call = function (id) {
    var p = this.m.props[id], v = valueAt(p, this.pos[id]);
    for (var i = 0; i < p.callbacks.length; i++) {
      this.callLog.push({ id: id, value: v, pos: this.pos[id], callback: p.callbacks[i] });
      if (this.opts.onCallback) this.opts.onCallback(p.callbacks[i], id, v);
    }
  };
  Engine.prototype._save = function (ids) {
    var m = this.m, list = ids.filter(function (id) { return m.props[id].persist && !m.props[id].readOnly; });
    if (list.length) this.batches.push(list.slice());
  };
  Engine.prototype.get = function (id) { return valueAt(this.m.props[id], this.pos[id]); };
  Engine.prototype.getPos = function (id) { return this.pos[id]; };
  Engine.prototype.text = function (id) { return valueText(this.m.props[id], this.pos[id]); };
  Engine.prototype.set = function (id, value) {   // outside change (bound control, code): same path as the menu, no EEPROM write
    this._setPos(id, posOf(this.m.props[id], value));
  };
  Engine.prototype.setRuntime = function (r) { for (var k in r) this.runtime[k] = r[k]; };

  Engine.prototype.eeprom = function () {
    var self = this;
    return { tag: this.tag, slots: persistIds(this.m).map(function (id) { return self.pos[id]; }) };
  };
  Engine.prototype.load = function (img) {            // start-up: stored image or defaults
    var ids = persistIds(this.m), m = this.m, ok = !!img && img.tag === this.tag && Array.isArray(img.slots) && img.slots.length === ids.length;
    if (ok) ids.forEach(function (id, i) { var s = img.slots[i]; if (!(Number.isInteger(s) && s >= 0 && s < m.props[id].count)) ok = false; });
    for (var i = 0; i < ids.length; i++) this.pos[ids[i]] = ok ? img.slots[i] : m.props[ids[i]].defaultPos;
    return ok;
  };

  Engine.prototype._level = function () { return this.stack[this.stack.length - 1]; };
  Engine.prototype._item = function () { var l = this._level(); return l.menu.items[l.idx]; };
  Engine.prototype._touch = function (now, newView) { this.lastInput = now; if (newView) this.since = now; };
  Engine.prototype._toStatus = function (now) { this.mode = 'status'; this.stack = []; this.edit = null; this.confirmItem = null; this.since = now; };

  // physical control changed (arrow switch, casino switch, program switch): value follows; an open menu is closed
  Engine.prototype.physical = function (id, value, now) {
    var st = this.m.status, isStatus = !!st && (id === st.program || id === st.casino || id === st.arrow);
    if (isStatus && this.mode === 'edit' && this.edit) this._setPos(this.edit.id, this.edit.start);
    this._setPos(id, posOf(this.m.props[id], value));
    if (isStatus && this.mode !== 'status') this._toStatus(now);
  };
  Engine.prototype.turn = function (steps, shift, now) {
    if (!steps) return;
    if (this.mode === 'menu') {
      var l = this._level(), n = l.menu.items.length, k = Math.max(0, Math.min(n - 1, l.idx + steps));
      if (k !== l.idx) { l.idx = k; this._touch(now, true); } else this._touch(now, false);
    } else if (this.mode === 'edit') {
      var p = this.m.props[this.edit.id];
      this._setPos(this.edit.id, movePos(p, this.pos[this.edit.id], steps * (shift ? p.shift : 1)));
      this._touch(now, false);
    }
  };
  Engine.prototype._apply = function (it) {
    var m = this.m, self = this, targets = [], want = {};
    if (it.setAll === 'default') m.order.forEach(function (id) { var p = m.props[id]; if (!p.readOnly) want[id] = p.defaultPos; });
    Object.keys(it.set || {}).forEach(function (id) {
      var p = m.props[id], v = it.set[id];
      want[id] = v === 'default' ? p.defaultPos : (p.kind === 'choice' && typeof v === 'string' ? p.choices.indexOf(v) : (p.kind === 'bool' ? (v ? 1 : 0) : posOf(p, v)));
    });
    var changed = [];
    Object.keys(want).forEach(function (id) { if (self.pos[id] !== want[id]) { self.pos[id] = want[id]; changed.push(id); } });
    changed.forEach(function (id) { self._call(id); });
    this._save(changed);
  };
  Engine.prototype.key = function (name, now) {
    if (this.mode === 'status') { if (name === 'OK') { this.mode = 'menu'; this.stack = [{ menu: this.m.root, idx: 0 }]; this._touch(now, true); } return; }
    if (this.mode === 'menu') {
      if (name === 'BACK') { if (this.stack.length > 1) { this.stack.pop(); this._touch(now, true); } else this._toStatus(now); return; }
      var it = this._item();
      if (it.type === 'menu') { this.stack.push({ menu: it, idx: 0 }); this._touch(now, true); }
      else if (it.type === 'prop') { var p = this.m.props[it.id]; this.mode = p.readOnly ? 'view' : 'edit'; this.edit = { id: it.id, start: this.pos[it.id] }; this._touch(now, true); }
      else if (it.type === 'preset') {
        if (it.confirm) { this.mode = 'confirm'; this.confirmItem = it; this._touch(now, true); }
        else { this._apply(it); this._touch(now, false); }
      }
    } else if (this.mode === 'edit') {
      if (name === 'OK') this._save([this.edit.id]); else this._setPos(this.edit.id, this.edit.start);
      this.mode = 'menu'; this.edit = null; this._touch(now, true);
    } else if (this.mode === 'view') {                 // a read-only property: shown like an edit page with a lock, nothing to change
      this.mode = 'menu'; this.edit = null; this._touch(now, true);
    } else if (this.mode === 'confirm') {
      if (name === 'OK') this._apply(this.confirmItem);
      this.mode = 'menu'; this.confirmItem = null; this._touch(now, true);
    }
  };
  Engine.prototype.tick = function (now) {
    if (this.mode === 'menu' && this.stack.length === 1 && now - this.lastInput >= TIMEOUT_MS) this._toStatus(now);
  };
  Engine.prototype.rows = function (now) {
    var m = this.m, since = this.since, st = m.status, r0, r1;
    if (this.mode === 'status') {
      if (!st) return ['', ''].map(function () { return fit('', '', now, since); });
      var prog = this.get(st.program), isRandom = prog === st.randomValue;
      r0 = '#' + Math.round(prog).toString(16).toUpperCase() + ' Casino ' + this.text(st.casino);
      var left = this.getPos(st.arrow) === 0;
      var dir = st.arrowAscii ? (left ? '<-Customer right' : '->Customer left') : (left ? LEFT_ARROW + ' Customer right' : RIGHT_ARROW + ' Customer left');
      if (isRandom && this.runtime.changeInSec !== null && Math.floor(Math.max(0, now - since) / STATUS_ALT_MS) % 2 === 1) {
        var s = Math.max(0, Math.floor(this.runtime.changeInSec));
        dir = 'Change in ' + Math.floor(s / 60) + ':' + ('0' + (s % 60)).slice(-2);
      }
      return [fit(r0, '', now, since), fit(dir, '', now, since)];
    }
    if (this.mode === 'edit' || this.mode === 'view') {
      var p = m.props[this.edit.id];
      return [fit(p.name, '', now, since), fit((this.mode === 'view' ? LOCK_CHAR : '>') + ' ' + this.text(p.id), '', now, since)];
    }
    if (this.mode === 'confirm') {
      var c = this.confirmItem;
      return [fit(c.confirmText || (c.text || c.name) + '?', '', now, since), fit('OK=yes BACK=no', '', now, since)];
    }
    var l = this._level(), it = l.menu.items[l.idx];
    if (this.stack.length === 1) {
      var sec = Math.max(0, Math.ceil((TIMEOUT_MS - (now - this.lastInput)) / 1000));
      r0 = fit(l.menu.name, sec + 's', now, since);
    } else r0 = fit(l.menu.name, '', now, since);
    if (it.type === 'menu') r1 = fit(it.text || it.name, '>', now, since);
    else if (it.type === 'prop') {
      var pn = it.text || m.props[it.id].name, pv = this.text(it.id);
      // name and value that do not fit together scroll as one text "Name: value" (pause, scroll, pause)
      r1 = pn.length + 1 + pv.length <= COLS ? fit(pn, pv, now, since) : fit(pn + ': ' + pv, '', now, since);
    }
    else r1 = fit(it.text || it.name, '', now, since);
    return [r0, r1];
  };

  return {
    COLS: COLS, TIMEOUT_MS: TIMEOUT_MS, MAX_PRESET_VALUES: MAX_PRESET_VALUES, LEFT_ARROW: LEFT_ARROW, RIGHT_ARROW: RIGHT_ARROW, LOCK_CHAR: LOCK_CHAR,
    build: build, Engine: Engine, count: count, valueAt: valueAt, posOf: posOf, movePos: movePos, valueText: valueText,
    fit: fit, layoutTag: layoutTag, persistIds: persistIds
  };
});
