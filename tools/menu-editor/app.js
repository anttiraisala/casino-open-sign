/* Menu Editor UI: edits the menu JSON (format 1), validates it with MenuCore and previews the 2 x 16 display. */
(function () {
  'use strict';
  var C = window.MenuCore;
  var $ = function (id) { return document.getElementById(id); };
  var S = JSON.parse(JSON.stringify(window.MENU_V2));      // the menu file being edited
  var tab = 'tree', sel = null;                              // sel: {tab, path|id}
  var model = null, engine = null, eeprom = null, shiftOn = false, phys = { program: 0, casino: 0, arrow: 0 };
  var startMs = Date.now(), cbLog = [], eeCount = 0;
  var PROP_FIELDS = [['kind', 'sel', ['', 'int', 'float', 'choice', 'bool']], ['type', 'typesel'], ['name', 'text'], ['min', 'num'], ['max', 'num'], ['default', 'val'], ['step', 'num'], ['steps', 'num'],
    ['scale', 'sel', ['', 'linear', 'log']], ['wrap', 'bool'], ['shift', 'num'], ['readOnly', 'bool'], ['persist', 'bool'], ['bind', 'text'], ['unit', 'text'], ['decimals', 'num'],
    ['format', 'sel', ['', 'hex']], ['valueTexts', 'json'], ['choices', 'list'], ['trueText', 'text'], ['falseText', 'text'], ['callbacks', 'list']];
  function now() { return Date.now() - startMs; }
  function clone(o) { return JSON.parse(JSON.stringify(o)); }
  function ensureCommon() { S.common = S.common || {}; ['types', 'properties', 'menus'].forEach(function (k) { S.common[k] = S.common[k] || {}; }); }

  // ---------------------------------------------------------------- rebuild + preview
  function rebuild(keepEngine) {
    model = C.build(S);
    var msgs = $('msgs'); msgs.innerHTML = '';
    model.errors.forEach(function (e) { var d = document.createElement('div'); d.className = 'err'; d.textContent = 'Error: ' + e; msgs.appendChild(d); });
    model.warnings.forEach(function (e) { var d = document.createElement('div'); d.className = 'warn'; d.textContent = 'Warning: ' + e; msgs.appendChild(d); });
    if (model.ok && !model.errors.length && !model.warnings.length) { var g = document.createElement('div'); g.className = 'good'; g.textContent = 'Menu is valid.'; msgs.appendChild(g); }
    $('status-msg').textContent = model.ok ? 'valid' : model.errors.length + ' error(s)';
    $('status-msg').className = model.ok ? 'small good' : 'small err';
    if (model.ok) {
      var old = engine;
      engine = new C.Engine(model, { onCallback: function (cb, id, v) { cbLog.push(cb + '(' + id + ' = ' + v + ')'); if (cbLog.length > 40) cbLog.shift(); } });
      if (keepEngine && old && eeprom) engine.load(eeprom);
      Object.keys(phys).forEach(function (k) { if (model.props[k]) engine.physical(k, phys[k], now()); });
      engine.mode = 'status';
      buildPhys();
    } else engine = null;
    $('json').value = JSON.stringify(S, null, 2);
    renderList(); render();
  }
  function buildPhys() {
    var box = $('phys'); box.innerHTML = '';
    if (!model.status) { box.textContent = 'No status section: switches not shown.'; return; }
    ['program', 'casino', 'arrow'].forEach(function (k) {
      var p = model.props[model.status[k]], s = document.createElement('select'), lab = document.createElement('label');
      for (var i = 0; i < p.count; i++) { var o = document.createElement('option'); o.value = i; o.textContent = C.valueText(p, i); s.appendChild(o); }
      s.value = Math.min(phys[k] || 0, p.count - 1);
      s.onchange = function () { phys[k] = +s.value; engine.physical(model.status[k], +s.value, now()); render(); };
      s.id = 'phys-' + k; lab.className = 'grp'; lab.textContent = p.name + ' '; lab.appendChild(s); box.appendChild(lab);
    });
  }
  var lastSecs = null;
  function render() {
    if (!engine) { $('lcd0').textContent = '(menu has errors)'; $('lcd1').textContent = ''; return; }
    engine.setRuntime({ changeInSec: Math.max(0, +$('rt').value || 0) });
    var r = engine.rows(now());
    var map = function (s) { return s.replace(/\x7e/g, '→').replace(/\x7f/g, '←'); };
    $('lcd0').textContent = map(r[0]); $('lcd1').textContent = map(r[1]);
    $('log-cb').textContent = cbLog.join('\n');
    $('log-ee').textContent = engine.batches.map(function (b, i) { return (i + 1) + ': ' + b.join(', '); }).join('\n') + '\nimage: ' + JSON.stringify(engine.eeprom());
  }
  setInterval(function () { if (engine) { engine.tick(now()); render(); } }, 100);

  // ---------------------------------------------------------------- list (left)
  function itemLabel(it) {
    if (it.menu !== undefined) return ['menu', it.menu];
    if (it.menuRef !== undefined) return ['menu ref', it.menuRef];
    if (it.property !== undefined) return ['property', it.property];
    if (it.propertyRef !== undefined) return ['prop ref', it.propertyRef];
    if (it.preset !== undefined) return ['preset', it.preset];
    return ['?', JSON.stringify(it)];
  }
  function isSel(t, k) { return sel && sel.tab === t && String(sel.key) === String(k); }
  function renderList() {
    var box = $('list'), act = $('list-actions'); box.innerHTML = ''; act.innerHTML = '';
    $('json').hidden = tab !== 'json'; $('json-actions').hidden = tab !== 'json'; box.hidden = tab === 'json'; act.hidden = tab === 'json';
    document.querySelectorAll('.tabs button').forEach(function (b) { b.className = b.dataset.tab === tab ? 'sel' : ''; });
    if (tab === 'json') return;
    function line(text, kind, depth, t, key) {
      var d = document.createElement('div'); d.style.paddingLeft = (4 + depth * 14) + 'px';
      d.className = isSel(t, key) ? 'sel' : '';
      d.innerHTML = '<span class="k"></span>'; d.firstChild.textContent = kind; d.appendChild(document.createTextNode(text));
      d.onclick = function () { sel = { tab: t, key: key }; renderList(); renderForm(); };
      box.appendChild(d);
    }
    if (tab === 'tree') {
      (function walk(items, path, depth) {
        (items || []).forEach(function (it, i) {
          var p = path.concat([i]), l = itemLabel(it); line(l[1], l[0], depth, 'tree', p.join('.'));
          if (it.menu !== undefined) walk(it.items, p, depth + 1);
        });
      })(S.root.items, [], 0);
    } else {
      var reg = S.common[tab] || {};
      Object.keys(reg).forEach(function (id) { line(id, tab === 'menus' ? 'menu' : tab === 'types' ? 'type' : (reg[id].kind || reg[id].type || 'prop'), 0, tab, id); });
    }
    addActions(act);
  }
  // path of a tree item: indices joined by '.', each level is items[index]; submenu children are in .items
  function getItem(key) {
    var idx = String(key).split('.').map(Number), list = S.root.items, it = null, parent = S.root.items;
    for (var i = 0; i < idx.length; i++) { parent = list; it = list[idx[i]]; if (!it) return null; list = it.items; }
    return { item: it, list: parent, index: idx[idx.length - 1] };
  }
  function btn(text, fn) { var b = document.createElement('button'); b.textContent = text; b.onclick = fn; return b; }
  function uniqueId(base, reg) { var n = 1, id = base; while (reg[id]) id = base + (++n); return id; }
  function allPropIds() { var ids = Object.keys(S.common.properties || {}); (function w(items) { (items || []).forEach(function (it) { if (it.property) ids.push(it.property); if (it.items) w(it.items); }); })(S.root.items); return ids; }
  function addActions(act) {
    ensureCommon();
    if (tab === 'tree') {
      var cur = sel && sel.tab === 'tree' ? getItem(sel.key) : null;
      var target = function () { return cur && cur.item.menu !== undefined ? cur.item.items : (cur ? cur.list : S.root.items); };
      var addTo = function (it) { var t = target(); t.push(it); sel = null; rebuild(true); };
      act.appendChild(btn('+ submenu', function () { addTo({ menu: 'New menu', items: [] }); }));
      act.appendChild(btn('+ property ref', function () { var ids = Object.keys(S.common.properties); if (!ids.length) { alert('Define a property under Properties first.'); return; } addTo({ propertyRef: ids[0] }); }));
      act.appendChild(btn('+ inline property', function () { addTo({ property: uniqueId('newProp', S.common.properties), kind: 'int', min: 0, max: 10, name: 'New' }); }));
      act.appendChild(btn('+ menu ref', function () { var ids = Object.keys(S.common.menus); if (!ids.length) { alert('Define a menu under Menus first.'); return; } addTo({ menuRef: ids[0] }); }));
      act.appendChild(btn('+ preset', function () { addTo({ preset: 'New preset', set: {} }); }));
      if (cur) {
        act.appendChild(btn('up', function () { if (cur.index > 0) { var l = cur.list, i = cur.index; l.splice(i - 1, 0, l.splice(i, 1)[0]); sel.key = String(sel.key).split('.').slice(0, -1).concat([i - 1]).join('.'); rebuild(true); } }));
        act.appendChild(btn('down', function () { var l = cur.list, i = cur.index; if (i < l.length - 1) { l.splice(i + 1, 0, l.splice(i, 1)[0]); sel.key = String(sel.key).split('.').slice(0, -1).concat([i + 1]).join('.'); rebuild(true); } }));
        act.appendChild(btn('delete', function () { cur.list.splice(cur.index, 1); sel = null; rebuild(true); renderForm(); }));
      }
    } else {
      var reg = S.common[tab];
      act.appendChild(btn('+ new', function () {
        var id = uniqueId(tab === 'menus' ? 'newMenu' : tab === 'types' ? 'newType' : 'newProp', reg);
        reg[id] = tab === 'menus' ? { name: 'New menu', items: [{ propertyRef: Object.keys(S.common.properties)[0] || 'x' }] } : { kind: 'int', min: 0, max: 10, name: 'New' };
        sel = { tab: tab, key: id }; rebuild(true); renderForm();
      }));
      if (sel && sel.tab === tab && reg[sel.key]) {
        act.appendChild(btn('rename id', function () {
          var n = prompt('New id (letters, digits, _):', sel.key); if (!n || n === sel.key || reg[n] || !/^[A-Za-z][A-Za-z0-9_]*$/.test(n)) return;
          var out = {}; Object.keys(reg).forEach(function (k) { out[k === sel.key ? n : k] = reg[k]; }); S.common[tab] = out;
          renameRefs(tab, sel.key, n); sel.key = n; rebuild(true); renderForm();
        }));
        act.appendChild(btn('delete', function () { delete reg[sel.key]; sel = null; rebuild(true); renderForm(); }));
      }
    }
  }
  function renameRefs(t, a, b) {
    (function w(items) { (items || []).forEach(function (it) {
      if (t === 'properties' && it.propertyRef === a) it.propertyRef = b;
      if (t === 'menus' && it.menuRef === a) it.menuRef = b;
      if (t === 'properties' && it.set && it.set[a] !== undefined) { it.set[b] = it.set[a]; delete it.set[a]; }
      if (it.items) w(it.items); }); })(S.root.items);
    Object.keys(S.common.menus).forEach(function (k) { (function w(items) { (items || []).forEach(function (it) {
      if (t === 'properties' && it.propertyRef === a) it.propertyRef = b; if (t === 'menus' && it.menuRef === a) it.menuRef = b;
      if (t === 'properties' && it.set && it.set[a] !== undefined) { it.set[b] = it.set[a]; delete it.set[a]; } if (it.items) w(it.items); }); })(S.common.menus[k].items); });
    if (t === 'types') Object.keys(S.common.properties).concat(Object.keys(S.common.types)).forEach(function (k) { var d = S.common.properties[k] || S.common.types[k]; if (d.type === a) d.type = b; });
    if (t === 'properties' && S.status) ['program', 'casino', 'arrow'].forEach(function (k) { if (S.status[k] === a) S.status[k] = b; });
  }

  // ---------------------------------------------------------------- form (middle)
  function row(label, el) { var r = document.createElement('div'); r.className = 'row'; var l = document.createElement('label'); l.textContent = label; r.appendChild(l); r.appendChild(el); return r; }
  function textInput(val, fn, id) { var i = document.createElement('input'); i.type = 'text'; i.value = val === undefined ? '' : val; if (id) i.id = id; i.onchange = function () { fn(i.value); }; return i; }
  function selectInput(val, opts, fn, id) {
    var s = document.createElement('select'); if (id) s.id = id;
    opts.forEach(function (o) { var e = document.createElement('option'); e.value = Array.isArray(o) ? o[0] : o; e.textContent = Array.isArray(o) ? o[1] : (o === '' ? '(not set)' : o); s.appendChild(e); });
    s.value = val === undefined ? '' : val; s.onchange = function () { fn(s.value); }; return s;
  }
  function checkInput(val, fn) { var c = document.createElement('input'); c.type = 'checkbox'; c.checked = !!val; c.onchange = function () { fn(c.checked); }; return c; }
  function parseVal(t) { if (t === '') return undefined; if (t === 'true') return true; if (t === 'false') return false; var n = Number(t); return isNaN(n) ? t : n; }
  function setField(def, f, v) { if (v === undefined || v === '' || (Array.isArray(v) && !v.length)) delete def[f]; else def[f] = v; rebuild(true); }
  function propForm(box, def, withId) {
    PROP_FIELDS.forEach(function (pf) {
      var f = pf[0], kind = pf[1], v = def[f], el;
      if (kind === 'text') el = textInput(v, function (x) { setField(def, f, x); }, 'f-' + f);
      else if (kind === 'num') el = textInput(v, function (x) { setField(def, f, x === '' ? undefined : Number(x)); }, 'f-' + f);
      else if (kind === 'val') el = textInput(v, function (x) { setField(def, f, parseVal(x)); }, 'f-' + f);
      else if (kind === 'sel') el = selectInput(v, pf[2], function (x) { setField(def, f, x); }, 'f-' + f);
      else if (kind === 'typesel') el = selectInput(v, [''].concat(Object.keys(S.common.types)), function (x) { setField(def, f, x); }, 'f-' + f);
      else if (kind === 'bool') { el = checkInput(v, function (x) { if (x) def[f] = true; else def[f] = false; rebuild(true); }); el.id = 'f-' + f; }
      else if (kind === 'list') el = textInput(v ? v.join(', ') : '', function (x) { setField(def, f, x.split(',').map(function (s) { return s.trim(); }).filter(Boolean)); }, 'f-' + f);
      else if (kind === 'json') el = textInput(v ? JSON.stringify(v) : '', function (x) { try { setField(def, f, x.trim() ? JSON.parse(x) : undefined); } catch (e) { alert('Not valid JSON: ' + e.message); } }, 'f-' + f);
      box.appendChild(row(f, el));
    });
  }
  function renderForm() {
    var box = $('form'); box.innerHTML = ''; ensureCommon();
    if (!sel) { box.innerHTML = '<div class="small">Select an item on the left.</div>'; return; }
    if (sel.tab === 'tree') {
      var cur = getItem(sel.key); if (!cur) { box.textContent = ''; return; }
      var it = cur.item;
      if (it.menu !== undefined) { box.appendChild(row('name', textInput(it.menu, function (x) { it.menu = x; rebuild(true); }, 'f-name'))); }
      else if (it.menuRef !== undefined) { box.appendChild(row('menu', selectInput(it.menuRef, Object.keys(S.common.menus), function (x) { it.menuRef = x; rebuild(true); }, 'f-ref'))); }
      else if (it.propertyRef !== undefined) { box.appendChild(row('property', selectInput(it.propertyRef, Object.keys(S.common.properties), function (x) { it.propertyRef = x; rebuild(true); }, 'f-ref'))); }
      else if (it.property !== undefined) {
        box.appendChild(row('id', textInput(it.property, function (x) { if (/^[A-Za-z][A-Za-z0-9_]*$/.test(x) && allPropIds().indexOf(x) < 0) { renameRefs('properties', it.property, x); it.property = x; rebuild(true); renderList(); } }, 'f-id')));
        propForm(box, it, false);
      } else if (it.preset !== undefined) {
        box.appendChild(row('name', textInput(it.preset, function (x) { it.preset = x; rebuild(true); }, 'f-name')));
        box.appendChild(row('confirm', checkInput(it.confirm, function (x) { if (x) it.confirm = true; else delete it.confirm; rebuild(true); })));
        box.appendChild(row('confirm text', textInput(it.confirmText, function (x) { if (x) it.confirmText = x; else delete it.confirmText; rebuild(true); })));
        box.appendChild(row('setAll default', checkInput(it.setAll === 'default', function (x) { if (x) it.setAll = 'default'; else delete it.setAll; rebuild(true); })));
        it.set = it.set || {};
        Object.keys(it.set).forEach(function (id) {
          var r = document.createElement('div'); r.className = 'row';
          r.appendChild(selectInput(id, Object.keys(S.common.properties).concat(allPropIds().filter(function (x) { return !S.common.properties[x]; })), function (x) { if (x !== id && it.set[x] === undefined) { var o = {}; Object.keys(it.set).forEach(function (k) { o[k === id ? x : k] = it.set[k]; }); it.set = o; rebuild(true); renderForm(); } }));
          var w = document.createElement('div'); w.style.display = 'flex'; w.style.gap = '4px';
          var vi = textInput(String(it.set[id]), function (x) { it.set[id] = parseVal(x); rebuild(true); }); w.appendChild(vi);
          w.appendChild(btn('x', function () { delete it.set[id]; rebuild(true); renderForm(); })); r.appendChild(w); box.appendChild(r);
        });
        box.appendChild(btn('+ value', function () { var ids = Object.keys(S.common.properties).concat(allPropIds()); var free = ids.filter(function (x) { return it.set[x] === undefined; })[0]; if (free) { it.set[free] = 'default'; rebuild(true); renderForm(); } }));
        box.appendChild(row('', document.createElement('span')));
      }
      if (it.text !== undefined || it.menu !== undefined || it.propertyRef !== undefined || it.preset !== undefined || it.menuRef !== undefined) {
        box.appendChild(row('row text', textInput(it.text, function (x) { if (x) it.text = x; else delete it.text; rebuild(true); }, 'f-text')));
      }
      return;
    }
    var reg = S.common[sel.tab], def = reg[sel.key]; if (!def) return;
    var h = document.createElement('div'); h.className = 'small'; h.textContent = sel.tab.slice(0, -1) + ' id: ' + sel.key; box.appendChild(h);
    if (sel.tab === 'menus') {
      box.appendChild(row('name', textInput(def.name, function (x) { def.name = x; rebuild(true); }, 'f-name')));
      var n = document.createElement('div'); n.className = 'small'; n.textContent = 'Items of this menu are edited in the JSON tab.'; box.appendChild(n);
    } else propForm(box, def, true);
  }

  // ---------------------------------------------------------------- events
  document.querySelectorAll('.tabs button').forEach(function (b) { b.onclick = function () { tab = b.dataset.tab; sel = null; renderList(); renderForm(); }; });
  $('btn-json-apply').onclick = function () {
    try { var j = JSON.parse($('json').value); S = j; ensureCommon(); S.root = S.root || { items: [] }; sel = null; rebuild(false); renderForm(); }
    catch (e) { alert('Not valid JSON: ' + e.message); }
  };
  $('btn-new').onclick = function () { S = { format: 1, name: 'New menu', common: { types: {}, properties: { demo: { name: 'Demo', kind: 'int', min: 0, max: 10, default: 5 } }, menus: {} }, root: { name: 'Menu', items: [{ propertyRef: 'demo' }] } }; sel = null; rebuild(false); renderForm(); };
  $('btn-sample').onclick = function () { S = clone(window.MENU_V2); sel = null; rebuild(false); renderForm(); };
  $('btn-open').onclick = function () { $('file').click(); };
  $('file').onchange = function () {
    var f = $('file').files[0]; if (!f) return; var r = new FileReader();
    r.onload = function () { try { S = JSON.parse(r.result); ensureCommon(); sel = null; rebuild(false); renderForm(); } catch (e) { alert('Not valid JSON: ' + e.message); } };
    r.readAsText(f);
  };
  $('btn-save').onclick = function () {
    var a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([JSON.stringify(S, null, 2) + '\n'], { type: 'application/json' }));
    a.download = 'menu.json'; document.body.appendChild(a); a.click(); a.remove();
  };
  $('btn-copy').onclick = function () {
    var t = JSON.stringify(S, null, 2), done = function () { $('status-msg').textContent = 'copied'; },
      fallback = function () { tab = 'json'; sel = null; renderList(); renderForm(); $('json').value = t; $('json').select(); $('status-msg').textContent = 'select all and copy from the JSON tab'; };
    try { navigator.clipboard.writeText(t).then(done, fallback); } catch (e) { fallback(); }
  };
  $('k-left').onclick = function () { if (engine) { engine.turn(-1, shiftOn, now()); render(); } };
  $('k-right').onclick = function () { if (engine) { engine.turn(1, shiftOn, now()); render(); } };
  $('k-ok').onclick = function () { if (engine) { engine.key('OK', now()); render(); } };
  $('k-back').onclick = function () { if (engine) { engine.key('BACK', now()); render(); } };
  $('k-shift').onclick = function () { shiftOn = !shiftOn; $('k-shift').className = shiftOn ? 'on' : ''; };
  $('btn-restart').onclick = function () { if (engine) { eeprom = engine.eeprom(); rebuild(true); } };
  $('btn-wipe').onclick = function () { eeprom = null; rebuild(false); };
  document.addEventListener('keydown', function (e) {
    if (/INPUT|TEXTAREA|SELECT/.test(e.target.tagName) || !engine) return;
    if (e.key === 'ArrowUp' || e.key === 'ArrowLeft') engine.turn(-1, e.shiftKey, now());
    else if (e.key === 'ArrowDown' || e.key === 'ArrowRight') engine.turn(1, e.shiftKey, now());
    else if (e.key === 'Enter') engine.key('OK', now());
    else if (e.key === 'Escape' || e.key === 'Backspace') engine.key('BACK', now());
    else return;
    e.preventDefault(); render();
  });
  window.MenuEditor = { get S() { return S; }, rebuild: rebuild };
  rebuild(false);
})();
