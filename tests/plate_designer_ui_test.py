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
    b = p.chromium.launch(); pg = b.new_page(viewport={'width': 1500, 'height': 850})
    errs = []; pg.on('pageerror', lambda e: errs.append(str(e))); pg.on('console', lambda m: errs.append(m.text) if m.type == 'error' and 'fonts.googleapis' not in m.text and 'ERR_' not in m.text else None)
    pg.goto(URL); pg.wait_for_timeout(500)
    S = lambda: pg.evaluate('PlateApp.state()')
    pg.evaluate("PlateApp.useFont('casino','DejaVu Sans')"); pg.wait_for_timeout(300)
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
    pdf3 = pg.evaluate("PlateApp.drillPdf('A3')"); check(pdf3['pages'] == 6, 'A3 tiling: 3 x 2 pages, got %d' % pdf3['pages'])
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
    # font combobox: arrows switch live, Esc reverts, Enter accepts
    pg.select_option('#t-casino-font', 'Limelight'); pg.wait_for_timeout(100); pg.focus('#t-casino-font')
    pg.keyboard.press('ArrowDown'); pg.wait_for_timeout(100); f1 = S()['texts']['casino']['font']; check(f1 != 'Limelight', 'ArrowDown switches the font live (got %s)' % f1)
    pg.keyboard.press('Escape'); pg.wait_for_timeout(100); check(S()['texts']['casino']['font'] == 'Limelight', 'Esc puts the old font back')
    pg.keyboard.press('ArrowDown'); pg.keyboard.press('Enter'); pg.wait_for_timeout(100); f2 = S()['texts']['casino']['font']; check(f2 != 'Limelight', 'Enter accepts the new font (%s)' % f2)
    pg.evaluate("PlateApp.useFont('casino','DejaVu Sans')")
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
    # frame info
    pg.fill('#plateWn', '980'); info = pg.inner_text('#frameInfo'); check('Total 26 units = 78 LEDs' in info, 'frame info: ' + info.replace('\n', ' | '))
    # layouts in localStorage
    pg.fill('#layoutName', 'test one'); pg.click('#layoutSave'); check('test one' in pg.inner_text('#layoutList'), 'saved layout is listed')
    pg.reload(); pg.wait_for_timeout(400); check(S()['plate']['w'] == 980 or S()['plate']['w'] == 800, 'autosave restored after reload'); check('test one' in pg.inner_text('#layoutList'), 'saved layout survives reload')
    # preview and screenshots
    shot = sys.argv[1] if len(sys.argv) > 1 else '/tmp/plate_designer.png'
    pg.click('#zfit'); pg.screenshot(path=shot)
    pg.click('#preview'); pg.screenshot(path=shot.replace('.png', '_preview.png'))
    check(not errs, 'no page errors: %s' % errs)
    b.close()
print('RESULT: %d failures' % len(fails)); sys.exit(1 if fails else 0)
