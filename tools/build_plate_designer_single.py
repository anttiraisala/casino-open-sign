#!/usr/bin/env python3
"""Builds tools/plate-designer/plate-designer-single.html: index.html with plate-core.js, arrow-data.js and app.js inlined
(one self-contained page, used for the published artifact)."""
import os, re
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'plate-designer')
html = open(os.path.join(D, 'index.html')).read()
for name in ['plate-core.js', 'arrow-data.js', 'app.js']:
    js = open(os.path.join(D, name)).read().replace('</script', '<\\/script')
    html = html.replace('<script src="%s"></script>' % name, '<script>\n' + js + '\n</script>')
assert 'src=' not in html
open(os.path.join(D, 'plate-designer-single.html'), 'w').write(html)
print('Wrote plate-designer-single.html (%d bytes)' % len(html))
