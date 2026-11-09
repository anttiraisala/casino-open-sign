# Mutation test of tools/plate-designer/plate-core.js: every mutant must make
# tests/plate_designer_core_test.js fail.   python3 tests/mutate_plate_designer.py
import subprocess, os, concurrent.futures as cf
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = open(os.path.join(ROOT, 'tools', 'plate-designer', 'plate-core.js')).read()
muts = [
 ("edt swap term", "(f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);\n      while", "(f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k] + 1);\n      while"),
 ("sdf half pixel in", "dIn[i] - 0.5 :", "dIn[i] - 0.4 :"),
 ("sdf half pixel out", "-(dOut[i] - 0.5)", "-(dOut[i] - 0.6)"),
 ("raster top edge", "if (yc < lo || yc >= hi) continue;", "if (yc < lo || yc > hi) continue;"),
 ("raster winding", "b[1] > a[1] ? 1 : -1", "1"),
 ("raster x round", "Math.ceil(xs[k][0] - 0.5), x1 = Math.ceil(xs[k + 1][0] - 0.5)", "Math.ceil(xs[k][0]), x1 = Math.ceil(xs[k + 1][0])"),
 ("insideDist half px", "* this.ppm + this.ox - 0.5, py", "* this.ppm + this.ox, py"),
 ("insideDist offset", "return v / this.ppm + (offsetMm || 0);", "return v / this.ppm - (offsetMm || 0);"),
 ("insideDist bounds", "i >= this.w - 1 || j >= this.h - 1) return -1e3", "i >= this.w || j >= this.h) return -1e3"),
 ("inscribed offset", "r: best / this.ppm + (offsetMm || 0) };", "r: best / this.ppm };"),
 ("bestHole margin", "(r + (marginMm || 0) - (offsetMm || 0))", "(r - (offsetMm || 0))"),
 ("bestHole offset", "(r + (marginMm || 0) - (offsetMm || 0))", "(r + (marginMm || 0) + (offsetMm || 0))"),
 ("bestHole nearest", "if (dd < best) { best = dd; bi = i; }", "if (dd > best || bi < 0) { best = dd; bi = i; }"),
 ("bestHole fits flag", "r: f[bi] / this.ppm + (offsetMm || 0), fits: true", "r: f[bi] / this.ppm + (offsetMm || 0), fits: false"),
 ("thin core radius", "r = minStrokeMm / 2 * this.ppm", "r = minStrokeMm * this.ppm"),
 ("thin slack", "d[i] > r + 1) thin++", "d[i] > r + 3) thin++"),
 ("thin offset", "if (f[i] + off >= r) { core[i] = 1; any = true; }", "if (f[i] >= r) { core[i] = 1; any = true; }"),
 ("natural spacing half", "gx = x + def.spacing / 2;", "gx = x + def.spacing;"),
 ("natural cell width", "cw = adv + def.spacing;", "cw = adv;"),
 ("uniform centring", "gx = x + cw / 2 - (ink ? (ink.x0 + ink.x1) / 2 : 0);", "gx = x + cw / 2;"),
 ("baseline", "goy: def.y + def.h }", "goy: def.y }"),
 ("cell centre y", "cell.cy = cell.y + def.h / 2;", "cell.cy = cell.y;"),
 ("hole offset dy", "cy + (o.dy || 0)", "cy"),
 ("hole offset dx", "cx + (o.dx || 0)", "cx"),
 ("hole covered tolerance", "d >= r - 1e-6", "d > r + 1"),
 ("hole clearance", "clearance: d - r }", "clearance: d }"),
 ("hole centre mode", "if (mode === 'centre') return { dx: 0, dy: 0 };", ""),
 ("hole fit margin default", "marginMm === undefined ? 0.5 : marginMm", "marginMm === undefined ? 0 : marginMm"),
 ("frame floor", "Math.floor(len / unit + 1e-9)", "Math.round(len / unit)"),
 ("frame sides layout", "topLen = layout === 'sides' ? W : W + 2 * sw", "topLen = layout === 'sides' ? W + 2 * sw : W"),
 ("frame leds", "total * 3", "total * 2"),
 ("frame exact fit", "unit + 1e-9)", "unit)"),
 ("dims right", "right: W - x", "right: W + x"),
 ("dims down", "down: H - y", "down: H + y"),
 ("snap threshold", "Math.abs(dx) <= thr &&", "Math.abs(dx) < thr &&"),
 ("snap threshold y", "Math.abs(dy) <= thr &&", "Math.abs(dy) < thr &&"),
 ("snap nearest", "Math.abs(dx) < Math.abs(bestX.d)", "Math.abs(dx) > Math.abs(bestX.d)"),
 ("snap nearest y", "Math.abs(dy) < Math.abs(bestY.d)", "Math.abs(dy) > Math.abs(bestY.d)"),
 ("strip rot", "s.rot === 90 ? { x: s.x, y: s.y, w: 10, h: 100 }", "s.rot === 90 ? { x: s.x, y: s.y, w: 100, h: 10 }"),
 ("eps y flip", "function Y(y) { return plate.h - y + m; }", "function Y(y) { return y + m; }"),
 ("eps polys y flip", "function Y(y) { return h - y + m; }", "function Y(y) { return y + m; }"),
 ("nest tallest first", "return b.h - a.h; }", "return a.h - b.h; }"),
 ("nest wrap", "x + it.w > sheetW + 1e-9", "x + it.w > sheetW + 50"),
 ("nest height", "y + it.h > sheetH + 1e-9", "y + it.h > sheetH + 50"),
 ("dxf circle y", "dxfCircle(it.x, plate.h - it.y,", "dxfCircle(it.x, it.y,"),
 ("dxf radius", "it.d / 2, 'HOLES'", "it.d, 'HOLES'"),
 ("dxf slot y", "plate.h - it.y - it.h, it.w, it.h, 'SLOT'", "plate.h - it.y, it.w, it.h, 'SLOT'"),
 ("dxf poly closed", "'\\n66\\n1\\n70\\n1\\n'", "'\\n66\\n1\\n70\\n0\\n'"),
 ("dxf letters flip", "[q[0], h - q[1]]", "[q[0], q[1]]"),
 ("dxf min points", "if (p.length >= 3) t += dxfPoly", "if (p.length >= 2) t += dxfPoly"),
 ("dxf box y", "dxfRect(b.x, h - b.y - b.h,", "dxfRect(b.x, h - b.y,"),
 ("dxf version", "AC1009", "AC1015"),
 ("json clamp", "Math.min(hi, Math.max(lo, v))", "v"),
 ("json strips rot", "t.rot === 90 ? 90 : 0", "t.rot"),
 ("json dims", "o.ui.dims : 'selected'", "o.ui.dims : 'all'"),
]
def run(i):
    name, old, new = muts[i]
    if SRC.count(old) < 1: return name, 'BAD MUTANT (snippet missing)'
    d = '/tmp/mut_plate/%d' % i; os.makedirs(d, exist_ok=True)
    open(d + '/plate-core.js', 'w').write(SRC.replace(old, new, 1))
    env = dict(os.environ, PLATE_CORE=d + '/plate-core.js')
    r = subprocess.run(['node', ROOT + '/tests/plate_designer_core_test.js'], capture_output=True, text=True, env=env, timeout=120)
    return name, ('DETECTED' if r.returncode != 0 or 'all checks passed' not in r.stdout else 'SURVIVED')
with cf.ThreadPoolExecutor(6) as ex:
    res = list(ex.map(run, range(len(muts))))
for n, r in res: print(r + ' ' + n)
print('RESULT: %d mutants, %d survived/bad' % (len(res), sum(1 for n, r in res if r != 'DETECTED')))
