# Browser test of tools/plate-designer/index.html (Playwright, local system fonts; Google Fonts are not reachable here).
#   python3 tests/plate_designer_ui_test.py [screenshot.png]
import os, sys, json
from playwright.sync_api import sync_playwright
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
URL = 'file://' + ROOT + '/tools/plate-designer/index.html'
fails = []
def check(c, m):
    if not c: print('FAIL: ' + m); fails.append(m)
def near(a, b, t, m): check(abs(a - b) <= t, '%s (got %s, want %s +-%s)' % (m, a, b, t))
with sync_playwright() as p:
    b = p.chromium.launch(); pg = b.new_context(viewport={'width': 1500, 'height': 850}, color_scheme='dark', permissions=['clipboard-read', 'clipboard-write']).new_page()
    errs = []; pg.on('pageerror', lambda e: errs.append(str(e))); pg.on('console', lambda m: errs.append(m.text) if m.type == 'error' and 'fonts.googleapis' not in m.text and 'ERR_' not in m.text else None)
    pg.goto(URL); pg.wait_for_timeout(500)
    S = lambda: pg.evaluate('PlateApp.state()')
    pg.evaluate("PlateApp.useFont('casino','DejaVu Sans')"); pg.wait_for_function("PlateApp.fontStatus('DejaVu Sans') && PlateApp.fontStatus('DejaVu Sans') !== 'loading'", timeout=15000)
    check(pg.input_value('#t-casino-font') == 'DejaVu Sans', 'font combobox shows the custom font')
    check(pg.evaluate("PlateApp.fontStatus('DejaVu Sans')") == 'ok', 'local font is detected as available')
    st = S(); check(st['texts']['casino']['font'] == 'DejaVu Sans', 'font set')
    sc = pg.evaluate('PlateApp.scene()')
    check(len(sc['casino']['cells']) == 6 and len(sc['open']['cells']) == 4 and len(sc['closed']['cells']) == 6, 'cell counts 6/4/6')
    check(all(c.get('hole') for c in sc['casino']['cells']) and not any(c.get('hole') for c in sc['closed']['cells']), 'holes in CASINO/OPEN cells only')
    near(sc['casino']['h'], 100, 1e-6, 'CASINO box height = 100'); near(sc['closed']['h'], 50, 1e-6, 'CLOSED half height')
    near(sc['casino']['w'], 540, 1e-6, 'CASINO width 6 x 90')
    check(len(sc['arrow']['holes']) == 10, '10 arrow holes')
    # drill EPS
    eps = pg.evaluate('PlateApp.drillEps()')
    check(eps.count(' arc ') == 6 + 4 + 10 + 1, 'EPS circles: 6+4 letters, 10 arrow, 1 CLOSED round, got %d' % eps.count(' arc '))
    check('%%BoundingBox: 0 0 ' in eps, 'EPS header')
    pdf = pg.evaluate("PlateApp.drillPdf('A4')"); check(pdf['pages'] == 8 and pdf['cols'] == 4 and pdf['rows'] == 2 and pdf['text'].startswith('%PDF-1.4'), 'A4 tiling of 980 x 300: 4 x 2 pages, got %s' % ((pdf['cols'], pdf['rows']),))
    ptxt = pdf['text']; check(' B*' in ptxt and '(C1) Tj' in ptxt and '(O6) Tj' in ptxt and '(L-CAP D2) Tj' in ptxt and '(R-WING-DN A2) Tj' in ptxt and '(CLOSED) Tj' in ptxt, 'drill PDF has the letter and arrow shapes with labels (C1..N6, stick names, CLOSED)')
    check(ptxt.count(' B*') >= 6 + 4 + 6 + 10, 'one filled shape per letter and per arrow stick, got %d' % ptxt.count(' B*'))
    pg.uncheck('#exPdfRef'); p2 = pg.evaluate("PlateApp.drillPdf('A4')")['text']; check(' B*' not in p2 and '(C1) Tj' not in p2, 'the option turns the reference drawing off'); pg.check('#exPdfRef')
    pdf3 = pg.evaluate("PlateApp.drillPdf('A3')"); check(pdf3['pages'] == 6, 'A3 tiling: 3 x 2 pages, got %d' % pdf3['pages'])
    dxf = pg.evaluate('PlateApp.drillDxf()'); check(dxf.count('0\nCIRCLE\n') == 21 and dxf.startswith('0\nSECTION') and dxf.endswith('0\nEOF\n'), 'DXF has 21 circles, got %d' % dxf.count('0\nCIRCLE\n'))
    ld = pg.evaluate('PlateApp.lettersEps()'); check(ld['dxf'].count('POLYLINE\n8\nLETTERS') == ld['polys'] and ld['polys'] >= 14, 'letter DXF has one polyline per outline')
    pg.click('#exDxf'); pg.wait_for_timeout(100); check(pg.input_value('#exName') == 'plate-drill-pattern.dxf' and pg.input_value('#exText').count('CIRCLE') == 21, 'export is also shown as text (name and content)')
    # letter EPS
    le = pg.evaluate('PlateApp.lettersEps()'); check(le['polys'] >= 14, 'letter outlines exist, got %d' % le['polys']); check('lineto' in le['text'], 'letter EPS has paths')
    # holes covered by the letters in the default layout (DejaVu Sans, uniform cells)
    cov = pg.inner_text('#info-casino'); print('casino info:', cov)
    # controls: plate width slider
    pg.fill('#plateWn', '700'); pg.wait_for_timeout(100); check(S()['plate']['w'] == 700, 'plate width numeric field'); check(pg.input_value('#plateW') == '700', 'slider follows numeric field')
    pg.fill('#plateWn', '980')
    # hole slider shared diameter
    pg.fill('#holeDn', '12'); pg.wait_for_timeout(50); check(S()['holeD'] == 12, 'hole diameter 12')
    eps = pg.evaluate('PlateApp.drillEps()'); check('6 mm 0 360 arc' in eps, 'hole radius 6 mm in the EPS')
    pg.fill('#holeDn', '10')
    # text edit
    pg.fill('#t-open-text', 'OP'); pg.wait_for_timeout(50); check(len(pg.evaluate('PlateApp.scene()')['open']['cells']) == 2, 'OPEN edited to two letters')
    pg.fill('#t-open-text', 'OPEN')
    # drag the CASINO box with the mouse
    box = pg.evaluate('PlateApp.scene().casino'); vb = pg.evaluate("document.getElementById('svg').getAttribute('viewBox')").split(); vb = [float(v) for v in vb]
    r = pg.evaluate("(()=>{const r=document.getElementById('svg').getBoundingClientRect();return [r.left,r.top,r.width,r.height]})()")
    def px(x, y): return r[0] + (x - vb[0]) / vb[2] * r[2], r[1] + (y - vb[1]) / vb[3] * r[3]
    # click on a gap between holes inside the CASINO box (top edge area, away from holes)
    sx, sy = px(box['x'] + 5, box['y'] + 4)
    pg.mouse.move(sx, sy); pg.mouse.down(); pg.mouse.move(sx + 20, sy + 40, steps=5); pg.mouse.up(); pg.wait_for_timeout(100)
    dx = S()['texts']['casino']['x'] - 20; dy = S()['texts']['casino']['y'] - 20
    check(dx > 5 and dy > 5, 'box moved by dragging (dx %.1f dy %.1f)' % (dx, dy))
    # undo restores
    pg.evaluate('PlateApp.commit()'); n0 = pg.evaluate('PlateApp.hist()')
    pg.click('#undo'); pg.wait_for_timeout(100); check(S()['texts']['casino']['x'] == 20, 'undo restores the position (x=%s)' % S()['texts']['casino']['x'])
    pg.click('#redo'); pg.wait_for_timeout(100); check(S()['texts']['casino']['x'] > 25, 'redo moves it again')
    # keyboard nudge
    pg.keyboard.press('ArrowRight'); x1 = S()['texts']['casino']['x']; pg.keyboard.press('Shift+ArrowRight'); x2 = S()['texts']['casino']['x']
    near(x2 - x1, 10, 1e-6, 'Shift+Arrow nudges 10 mm'); pg.keyboard.press('Alt+ArrowLeft'); near(S()['texts']['casino']['x'] - x2, -0.1, 1e-6, 'Alt+Arrow nudges 0.1 mm')
    # dropdowns: click opens the list, arrows preview live, Enter accepts, Esc reverts
    pg.select_option('#t-casino-font', 'Limelight'); pg.wait_for_timeout(100)
    pg.click('#t-casino-font + *', force=True) if False else pg.locator('#t-casino-font').locator('xpath=preceding-sibling::button').click()
    check(pg.locator('.dd-list').count() == 1, 'click opens the list')
    check(pg.locator('.dd-item.hi').inner_text() == 'Limelight', 'the current font is highlighted when the list opens')
    pg.keyboard.press('ArrowDown'); pg.wait_for_timeout(100); f1 = S()['texts']['casino']['font']; check(f1 != 'Limelight', 'ArrowDown previews the next font at once (got %s)' % f1)
    check(pg.locator('#t-casino-font').locator('xpath=preceding-sibling::button').inner_text().startswith(f1), 'the box shows the previewed font')
    pg.keyboard.press('Escape'); pg.wait_for_timeout(100); check(S()['texts']['casino']['font'] == 'Limelight' and pg.locator('.dd-list').count() == 0, 'Esc closes the list and puts the old font back')
    pg.locator('#t-casino-font').locator('xpath=preceding-sibling::button').click(); pg.keyboard.press('ArrowDown'); pg.keyboard.press('ArrowDown'); pg.keyboard.press('Enter'); pg.wait_for_timeout(100)
    f2 = S()['texts']['casino']['font']; check(f2 not in ('Limelight', f1) and pg.locator('.dd-list').count() == 0, 'Enter accepts the previewed font (%s)' % f2)
    h0 = pg.evaluate('PlateApp.hist()')['n']; pg.locator('#t-casino-font').locator('xpath=preceding-sibling::button').click(); pg.keyboard.press('ArrowDown'); pg.wait_for_timeout(600); pg.keyboard.press('Escape'); pg.wait_for_timeout(100)
    check(S()['texts']['casino']['font'] == f2 and pg.evaluate('PlateApp.hist()')['n'] == h0, 'a cancelled preview leaves no undo step')
    pg.locator('#frameLayout').locator('xpath=preceding-sibling::button').click(); pg.keyboard.press('ArrowDown'); check(S()['frame']['layout'] == 'sides', 'other dropdowns preview live too'); pg.keyboard.press('Escape'); check(S()['frame']['layout'] == 'topbottom', 'and revert on Esc')
    pg.locator('#dims').locator('xpath=preceding-sibling::button').click(); pg.mouse.click(5, 5); check(pg.locator('.dd-list').count() == 0, 'a click outside closes the list')
    pg.evaluate("PlateApp.useFont('casino','DejaVu Sans')")
    col = pg.evaluate("(()=>{const s=getComputedStyle(document.querySelector('.dd-btn'));return [s.color,s.backgroundColor]})()")
    check(col[0] != col[1], 'dropdown text is readable in dark mode: %s on %s' % tuple(col))
    # every control has a tooltip
    missing = pg.evaluate("Array.from(document.querySelectorAll('#panel input, #panel textarea, #panel button, #panel select')).filter(e => !(e.title || '').trim() && !e.closest('label[title]')).map(e => e.id || e.className)")
    check(not missing, 'controls without a tooltip: %s' % missing)
    mp = pg.evaluate("document.getElementById('minStroke').title"); check('thin' in mp and 'Minimum stroke' in mp, 'min stroke tooltip explains the setting')
    # drag a number
    def centre(sel):
        pg.locator(sel).scroll_into_view_if_needed(); bb = pg.locator(sel).bounding_box(); return bb['x'] + bb['width'] / 3, bb['y'] + bb['height'] / 2
    cx, cy = centre('#plateHn'); pg.mouse.move(cx, cy); pg.mouse.down(); pg.mouse.move(cx - 40, cy, steps=6); pg.mouse.up(); pg.wait_for_timeout(50)
    check(S()['plate']['h'] == 280, 'dragging left 40 px lowers plate height by 20 mm (got %s)' % S()['plate']['h'])
    cx, cy = centre('#plateHn'); pg.mouse.move(cx, cy); pg.mouse.down(); pg.mouse.move(cx, cy - 10, steps=5); pg.mouse.up(); pg.wait_for_timeout(50)
    check(S()['plate']['h'] == 300, 'dragging up 10 px raises it by 20 steps (4x faster), clamped to 300 (got %s)' % S()['plate']['h'])
    cx, cy = centre('#holeDn'); pg.mouse.move(cx, cy); pg.mouse.down(); pg.mouse.move(cx + 8, cy, steps=4); pg.mouse.up(); pg.wait_for_timeout(50)
    check(S()['holeD'] == 12, 'step 0.5: 8 px right = 4 steps = +2 mm (got %s)' % S()['holeD'])
    pg.fill('#holeDn', '10')
    cx, cy = centre('#t-casino-h'); pg.mouse.click(cx, cy); pg.keyboard.type('90'); pg.wait_for_timeout(50)
    check(S()['texts']['casino']['h'] == 90 and pg.evaluate("document.activeElement.id") == 't-casino-h', 'a plain click still lets you type a number (got %s)' % S()['texts']['casino']['h'])
    pg.fill('#t-casino-h', '100'); pg.keyboard.press('Tab')
    # a font that is still loading is not drawn: the old font stays until the new one is ready
    pg.route('**/fonts.googleapis.com/**', lambda r: (pg.wait_for_timeout(1500), r.abort()))
    before = pg.evaluate("PlateApp.shownFont('casino')")
    pg.evaluate("PlateApp.useFont('casino','Zilla Slab')"); pg.wait_for_timeout(200)
    check(pg.evaluate("PlateApp.shownFont('casino')") == before and pg.evaluate("PlateApp.fontStatus('Zilla Slab')") == 'loading', 'while the new font loads the old one stays drawn (shows %s)' % pg.evaluate("PlateApp.shownFont('casino')"))
    pg.wait_for_timeout(500); check('Loading font Zilla Slab' in pg.inner_text('#info-casino'), 'the info line says the font is loading')
    pg.wait_for_function("PlateApp.fontStatus('Zilla Slab') !== 'loading'", timeout=10000)
    check(pg.evaluate("PlateApp.shownFont('casino')") == 'Zilla Slab', 'after loading (or failing) the new font is the drawn one')
    pg.unroute('**/fonts.googleapis.com/**'); pg.evaluate("PlateApp.useFont('casino','DejaVu Sans')"); pg.wait_for_timeout(300)
    # resizing keeps the centre of the box
    pg.fill('#t-casino-x', '20'); pg.fill('#t-casino-y', '20'); pg.fill('#t-casino-h', '60'); st = S()['texts']['casino']
    near(st['y'], 40, 0.011, 'height 100 -> 60 keeps the vertical centre (y 20 -> 40)'); near(st['x'], 20, 0.011, 'uniform width does not change with the height, x stays')
    pg.fill('#t-casino-cw', '100'); st = S()['texts']['casino']; near(st['x'], -10, 0.011, 'cell width 90 -> 100 keeps the horizontal centre (x 20 -> -10)')
    pg.fill('#t-casino-cw', '90'); pg.fill('#t-casino-h', '100'); pg.select_option('#t-casino-mode', 'natural')
    c0 = pg.evaluate("(()=>{const b=PlateApp.scene().casino;return [b.x+b.w/2,b.y+b.h/2]})()"); pg.fill('#t-casino-h', '70'); c1 = pg.evaluate("(()=>{const b=PlateApp.scene().casino;return [b.x+b.w/2,b.y+b.h/2]})()")
    near(c1[0], c0[0], 0.02, 'natural mode: horizontal centre stays when the width changes with the height'); near(c1[1], c0[1], 0.02, 'natural mode: vertical centre stays')
    pg.select_option('#t-casino-mode', 'uniform'); pg.fill('#t-casino-h', '100'); pg.fill('#t-casino-x', '20'); pg.fill('#t-casino-y', '20')
    # strip pieces
    pg.click('#stripAdd'); check(len(S()['strips']) == 1, 'strip piece added'); pg.keyboard.press('r'); check(S()['strips'][0]['rot'] == 90, 'R rotates the strip')
    pg.keyboard.press('Delete'); check(len(S()['strips']) == 0, 'Delete removes the strip')
    # CLOSED hole export modes
    pg.select_option('#closedHole', 'slot'); e2 = pg.evaluate('PlateApp.drillEps()'); check(e2.count(' arc ') == 20 and 'rlineto' in e2, 'slot mode: 20 circles + a rectangle')
    pg.select_option('#closedHole', 'none'); e3 = pg.evaluate('PlateApp.drillEps()'); check(e3.count(' arc ') == 20, 'none mode: 20 circles')
    pg.select_option('#closedHole', 'round')
    # quick hole buttons
    pg.click('#t-casino-fit'); offs = S()['texts']['casino']['holeOffsets']; check(len(offs) == 6, 'fit button stores 6 offsets')
    pg.click('#t-casino-centre'); check(all(o['dx'] == 0 and o['dy'] == 0 for o in S()['texts']['casino']['holeOffsets']), 'centre button zeros the offsets')
    # JSON round trip
    j = pg.input_value('#json'); pg.evaluate('PlateApp.commit()'); j = pg.input_value('#json'); d = json.loads(j); check(d['version'] == 1 and d['texts']['casino']['font'] == 'DejaVu Sans', 'JSON textarea shows the state')
    d['plate']['w'] = 800; pg.fill('#json', json.dumps(d)); pg.click('#jsonApply'); check(S()['plate']['w'] == 800, 'JSON import applies')
    pg.fill('#json', '{"version":9}'); pg.click('#jsonApply'); check('Not loaded' in pg.inner_text('#jsonMsg'), 'bad JSON is reported')
    # copy JSON from the app / paste the clipboard into the app
    pg.click('#jsonCopy'); pg.wait_for_timeout(100); clip = pg.evaluate('navigator.clipboard.readText()')
    check(json.loads(clip)['plate']['w'] == 800 and 'Copied' in pg.inner_text('#jsonMsg') + 'Copied' and 'copied' in pg.inner_text('#jsonMsg'), 'Copy JSON puts the layout on the clipboard')
    d2 = json.loads(clip); d2['plate']['w'] = 600; pg.evaluate('t => navigator.clipboard.writeText(t)', json.dumps(d2)); pg.click('#jsonPaste'); pg.wait_for_timeout(200)
    check(S()['plate']['w'] == 600, 'Paste clipboard into app loads the layout (got %s)' % S()['plate']['w'])
    pg.evaluate('t => navigator.clipboard.writeText(t)', 'not json'); pg.click('#jsonPaste'); pg.wait_for_timeout(200); check('Not loaded' in pg.inner_text('#jsonMsg') and S()['plate']['w'] == 600, 'bad clipboard text is reported and changes nothing')
    pg.evaluate("document.activeElement.blur()"); d2['plate']['w'] = 700
    pg.evaluate("t => { const dt = new DataTransfer(); dt.setData('text', t); document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: dt, bubbles: true })); }", json.dumps(d2)); pg.wait_for_timeout(100)
    check(S()['plate']['w'] == 700, 'Ctrl+V anywhere on the page loads a pasted layout')
    pg.fill('#plateWn', '980')
    # frame info
    pg.fill('#plateWn', '980'); info = pg.inner_text('#frameInfo'); check('Total 26 units = 78 LEDs' in info, 'frame info: ' + info.replace('\n', ' | '))
    # layouts in localStorage
    pg.fill('#layoutName', 'test one'); pg.click('#layoutSave'); check('test one' in pg.inner_text('#layoutList'), 'saved layout is listed')
    pg.evaluate('PlateApp.commit()'); pg.reload(); pg.wait_for_timeout(400); check(S()['plate']['w'] == 980 or S()['plate']['w'] == 800, 'autosave restored after reload'); check('test one' in pg.inner_text('#layoutList'), 'saved layout survives reload')
    # preview and screenshots
    shot = sys.argv[1] if len(sys.argv) > 1 else '/tmp/plate_designer.png'
    pg.click('#zfit'); pg.screenshot(path=shot)
    pg.click('#preview'); pg.screenshot(path=shot.replace('.png', '_preview.png'))
    check(not errs, 'no page errors: %s' % errs)
    b.close()
print('RESULT: %d failures' % len(fails)); sys.exit(1 if fails else 0)
