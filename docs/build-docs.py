# Builds the four ClaCommandBar volumes:
#
#   getting-started.html    install it, first bar, the demos
#   programmers-guide.html  the concepts, the recipes, the Clarion notes
#   template-guide.html     every template, tab, prompt and embed point
#   reference.html          the class, the DLL, the equates
#
# The reference volume is READ OUT OF THE SOURCES - commandbar.h,
# commandbar.def and CommandBar.inc - so a signature or an ordinal here is
# the one in the build.  Run this after changing the API.
import io, re, html

# ---------------------------------------------------------------- extract
def _extract_api():
    h = io.open('src/commandbar.h', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    d = io.open('src/commandbar.def', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    ordn = {m.group(1): int(m.group(2)) for m in re.finditer(r'^\s*(CB_\w+)\s+@(\d+)', d, re.M)}
    strip = lambda s: re.sub(r'/\*.*?\*/', '', s).rstrip()
    secs, cur, pend, lines, i = [], None, [], h.split('\n'), 0
    while i < len(lines):
        raw = lines[i]
        m = re.match(r'/\* ---- (.+?) -+ \*/', raw)
        if m:
            cur = {'name': m.group(1).strip(), 'funcs': [], 'consts': []}
            secs.append(cur); pend = []; i += 1; continue
        m = re.match(r'#define\s+(CB\w+)\s+([^/]+?)\s*(?:/\*(.*?)\*/)?\s*$', raw)
        if m and cur is not None:
            cur['consts'].append({'name': m.group(1), 'value': m.group(2).strip(),
                                  'note': (m.group(3) or '').strip()})
            pend = []; i += 1; continue
        if 'CBAPI' in raw:
            codeline, trail = strip(raw), re.findall(r'/\*(.*?)\*/', raw)
            while not codeline.endswith(';') and i + 1 < len(lines):
                i += 1
                codeline += ' ' + strip(lines[i]).strip()
                trail += re.findall(r'/\*(.*?)\*/', lines[i])
            nm = re.search(r'CB_\w+', codeline)
            if nm and cur is not None:
                cur['funcs'].append({'name': nm.group(0), 'sig': re.sub(r'\s+', ' ', codeline).strip(),
                                     'ord': ordn.get(nm.group(0)),
                                     'trail': ' '.join(t.strip() for t in trail).strip(),
                                     'doc': [p for p in pend if p]})
            pend = []; i += 1; continue
        c = raw.strip()
        if c.startswith('/*') or c.startswith('*'):
            pend.append(re.sub(r'^/?\*+', '', c).replace('*/', '').strip())
        elif c == '':
            pend = []
        i += 1
    return secs

def _class_block():
    inc = io.open('clarion/CommandBar.inc', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    lines = inc.split('\n')
    a = next(i for i, l in enumerate(lines) if l.startswith('CommandBarClass CLASS'))
    b = next(i for i, l in enumerate(lines) if i > a and l.strip() == 'END')
    return lines[a + 1:b]

def _extract_class():
    out, pend = [], []
    for s in _class_block():
        t = s.strip()
        if t.startswith('!'):
            pend.append(re.sub(r'^!-*\s?', '', t)); continue
        m = re.match(r'^(\w+)\s+PROCEDURE\((.*?)\)(.*)$', s)
        if m:
            out.append({'name': m.group(1), 'parms': m.group(2), 'attrs': m.group(3).strip(', '),
                        'doc': [p for p in pend if p.strip()]})
            pend = []
        elif t == '':
            pend = []
    return out

def _extract_props():
    out, pend = [], []
    for ln in _class_block():
        t = ln.strip()
        if t.startswith('!'):
            pend.append(re.sub(r'^!-*\s?', '', t)); continue
        if not t or re.match(r'^\w+\s+PROCEDURE\(', ln):
            pend = []; continue
        m = re.match(r'^(\w+)\s+(\S.*?)(?:\s{2,}!\s*(.*))?$', ln)
        if m:
            out.append({'name': m.group(1), 'type': m.group(2).strip(),
                        'note': (m.group(3) or '').strip(),
                        'doc': [x for x in pend if x.strip()]})
        pend = []
    return out

API      = _extract_api()
CLASS    = _extract_class()
PROPLIST = _extract_props()

import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from usage import USAGE, PROPS

MISSING = []

# ---------------------------------------------------------------- helpers
def esc(s): return html.escape(s or '')
def slug(s): return re.sub(r'[^a-z0-9]+', '-', s.lower()).strip('-')

def code(txt, lang='clarion'):
    return '<pre class="code" data-lang="%s"><code>%s</code></pre>' % (lang, esc(txt.strip('\n')))

def usecode(txt):
    return '<pre class="code code--use" data-lang="use"><code>%s</code></pre>' % esc(txt)

def note(kind, title, body):
    return ('<aside class="note note--%s"><p class="note__t">%s</p><div class="note__b">%s</div></aside>'
            % (kind, esc(title), body))

def table(head, rows, cls=''):
    th = ''.join('<th>%s</th>' % h for h in head)
    tr = ''.join('<tr>%s</tr>' % ''.join('<td>%s</td>' % c for c in r) for r in rows)
    return ('<div class="tw"><table class="%s"><thead><tr>%s</tr></thead><tbody>%s</tbody></table></div>'
            % (cls, th, tr))

# ---------------------------------------------------------------- volumes
VOLUMES = [
 ('getting-started.html',   'Getting Started',      'Install it and get a bar on screen'),
 ('programmers-guide.html', "Programmer's Guide",   'How it works, and how to make it do things'),
 ('template-guide.html',    'Template Guide',       'Every template, tab, prompt and embed'),
 ('reference.html',         'Reference',            'The class, the DLL and the equates'),
]

#  Published, each volume is its own page on its own address, so a relative
#  filename does not reach the next one.  Cross-volume links are written as
#  these absolute addresses; the local copies in docs/ therefore point at the
#  published set, which is the only thing that works from both places.
PUBLISHED = {
 'getting-started.html':   'https://claude.ai/code/artifact/9f7423f8-9a58-4c90-83fb-bc902fce7cfc',
 'programmers-guide.html': 'https://claude.ai/code/artifact/7f6f84a6-3bea-4d92-948c-480f3d6a583d',
 'template-guide.html':    'https://claude.ai/code/artifact/dc72dc88-b5af-41b4-a3b0-8f73cb308ae9',
 'reference.html':         'https://claude.ai/code/artifact/bfef6768-0e5d-45e6-9a8e-9f053eb6dbbb',
}

def href(target, current):
    #  never link a volume to itself
    return '#' if target == current else PUBLISHED.get(target, target)

CSS = """
:root{
  --paper:#fbfcfd; --surface:#f1f5f9; --sunken:#e9eff5; --rule:#d6e0ea;
  --ink:#0f1720; --soft:#4e5f70; --faint:#7d8ea0;
  --accent:#2f6a9a; --accent-bg:#e4eef6; --accent-rule:#b9d3e6;
  --clarion:#166f69; --clarion-bg:#e2f1ef;
  --warn:#8a5a12; --warn-bg:#f7eeda;
}
@media (prefers-color-scheme:dark){
  :root:not([data-theme="light"]){
    --paper:#0d131a; --surface:#151d26; --sunken:#111922; --rule:#25313d;
    --ink:#e2eaf2; --soft:#9aabbc; --faint:#6f8092;
    --accent:#6fadde; --accent-bg:#152738; --accent-rule:#28455f;
    --clarion:#4fb5ab; --clarion-bg:#102a29;
    --warn:#d8a545; --warn-bg:#2b2413;
  }
}
:root[data-theme="dark"]{
  --paper:#0d131a; --surface:#151d26; --sunken:#111922; --rule:#25313d;
  --ink:#e2eaf2; --soft:#9aabbc; --faint:#6f8092;
  --accent:#6fadde; --accent-bg:#152738; --accent-rule:#28455f;
  --clarion:#4fb5ab; --clarion-bg:#102a29;
  --warn:#d8a545; --warn-bg:#2b2413;
}
*{box-sizing:border-box}
body{margin:0; background:var(--paper); color:var(--ink);
  font-family:"IBM Plex Serif",Georgia,serif; font-size:16px; line-height:1.62;
  -webkit-font-smoothing:antialiased}
h1,h2,h3,h4,.ui{font-family:"IBM Plex Sans",system-ui,-apple-system,Segoe UI,sans-serif}
code,pre,.mono{font-family:"IBM Plex Mono",ui-monospace,Consolas,monospace}
a{color:var(--accent)}
.wrap{display:grid; grid-template-columns:270px minmax(0,1fr); align-items:start}
.side{position:sticky; top:0; height:100vh; overflow-y:auto; padding:24px 20px 60px;
  border-right:1px solid var(--rule); background:var(--surface)}
.brand{font-family:"IBM Plex Sans",sans-serif; font-weight:600; font-size:15px; margin:0 0 14px}
.brand b{color:var(--accent)}
/* ---- the four volumes ---- */
.vols{list-style:none; margin:0 0 18px; padding:0; display:flex; flex-direction:column; gap:3px;
  border-bottom:1px solid var(--rule); padding-bottom:16px}
.vols a{display:block; padding:7px 10px; border-radius:6px; text-decoration:none;
  font:500 13px/1.35 "IBM Plex Sans",sans-serif; color:var(--soft); border:1px solid transparent}
.vols a:hover{background:var(--sunken); color:var(--ink)}
.vols a.here{background:var(--accent-bg); border-color:var(--accent-rule); color:var(--accent)}
.vols small{display:block; font:400 11px/1.35 "IBM Plex Sans",sans-serif; color:var(--faint);
  margin-top:2px}
.vols a.here small{color:var(--accent)}
.filter{width:100%; margin:0 0 16px; padding:7px 10px; font:13px/1.4 "IBM Plex Sans",sans-serif;
  color:var(--ink); background:var(--paper); border:1px solid var(--rule); border-radius:6px}
.filter:focus{outline:2px solid var(--accent); outline-offset:1px}
.nav__g{font:600 10.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.11em; text-transform:uppercase;
  color:var(--faint); margin:18px 0 8px}
.nav__l{list-style:none; margin:0; padding:0; display:flex; flex-direction:column; gap:1px}
.nav__l a{display:block; padding:4px 8px; border-radius:5px; text-decoration:none; color:var(--soft);
  font:400 13.5px/1.45 "IBM Plex Sans",sans-serif}
.nav__l a:hover{background:var(--sunken); color:var(--ink)}
.nav__l a.on{background:var(--accent-bg); color:var(--accent); font-weight:500}
.main{padding:0 0 120px; min-width:0}
.inner{max-width:980px; padding:0 40px}
.hero{padding:52px 40px 30px; border-bottom:1px solid var(--rule);
  background:linear-gradient(180deg,var(--accent-bg),transparent)}
.hero .inner{padding:0}
.eyebrow{font:600 11px/1 "IBM Plex Sans",sans-serif; letter-spacing:.14em; text-transform:uppercase;
  color:var(--accent); margin:0 0 12px}
h1{font-size:37px; line-height:1.1; margin:0 0 10px; letter-spacing:-.015em; text-wrap:balance}
.sub{font-size:17px; color:var(--soft); margin:0 0 18px; max-width:62ch}
.chips{display:flex; flex-wrap:wrap; gap:8px}
.chip{font:500 11.5px/1 "IBM Plex Sans",sans-serif; padding:6px 10px; border-radius:99px;
  border:1px solid var(--rule); background:var(--paper); color:var(--soft)}
.chip b{color:var(--ink); font-weight:600}
h2{font-size:26px; margin:60px 0 6px; letter-spacing:-.01em; scroll-margin-top:18px; text-wrap:balance}
h2 .k{font:600 10.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.12em; text-transform:uppercase;
  color:var(--accent); display:block; margin-bottom:9px}
h3{font-size:17.5px; margin:36px 0 10px; scroll-margin-top:18px}
h4{font-size:14.5px; margin:24px 0 6px; color:var(--soft); font-weight:600}
p{margin:0 0 14px; max-width:70ch}
.lead{color:var(--soft); max-width:70ch}
ul.b,ol.b{max-width:70ch; padding-left:20px; margin:0 0 16px}
ul.b li,ol.b li{margin:0 0 7px}
.code{background:var(--sunken); border:1px solid var(--rule); border-left:3px solid var(--accent-rule);
  border-radius:0 7px 7px 0; padding:14px 16px; overflow-x:auto; margin:0 0 18px;
  font-size:12.9px; line-height:1.62; position:relative}
.code code{white-space:pre; color:var(--ink)}
.code::after{content:attr(data-lang); position:absolute; top:0; right:0; padding:3px 9px;
  font:500 9.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.1em; text-transform:uppercase;
  color:var(--faint); background:var(--surface); border-left:1px solid var(--rule);
  border-bottom:1px solid var(--rule); border-radius:0 0 0 6px}
.code--use{margin:7px 0 2px; border-left-color:var(--clarion); background:var(--paper);
  font-size:12.4px; padding:9px 12px}
.code--use::after{content:'Clarion'; color:var(--clarion)}
p code,li code,td code{background:var(--sunken); border:1px solid var(--rule); border-radius:4px;
  padding:.06em .34em; font-size:.86em}
.tw{overflow-x:auto; margin:0 0 20px; border:1px solid var(--rule); border-radius:8px;
  background:var(--paper)}
table{border-collapse:collapse; width:100%; font-size:13.5px;
  font-family:"IBM Plex Sans",sans-serif}
th{text-align:left; font-weight:600; font-size:11px; letter-spacing:.08em; text-transform:uppercase;
  color:var(--faint); padding:9px 14px; border-bottom:1px solid var(--rule); background:var(--surface)}
td{padding:10px 14px; border-bottom:1px solid var(--rule); vertical-align:top}
tr:last-child td{border-bottom:0}
.fns .fn__n{width:210px; white-space:nowrap}
.fns .fn__n code{font-size:12.6px; color:var(--accent); font-weight:500; background:none;
  border:0; padding:0}
.fns .fn__s code{font-size:12.4px; color:var(--soft); background:none; border:0; padding:0;
  white-space:pre-wrap}
.fn__d{margin:5px 0 0; font-family:"IBM Plex Serif",serif; font-size:13.5px; color:var(--ink);
  max-width:74ch}
.ord{margin-left:7px; font:500 10px/1 "IBM Plex Mono",monospace; color:var(--faint);
  border:1px solid var(--rule); border-radius:4px; padding:2px 4px; vertical-align:1px}
.eq td:first-child code{color:var(--accent); background:none; border:0; padding:0}
.eq .v{font-variant-numeric:tabular-nums; color:var(--soft)}
.note{border:1px solid var(--rule); border-left:3px solid var(--accent); background:var(--surface);
  border-radius:0 7px 7px 0; padding:13px 16px; margin:0 0 18px; max-width:74ch}
.note--warn{border-left-color:var(--warn); background:var(--warn-bg)}
.note--cla{border-left-color:var(--clarion); background:var(--clarion-bg)}
.note__t{font:600 12px/1.3 "IBM Plex Sans",sans-serif; letter-spacing:.03em; margin:0 0 5px;
  text-transform:uppercase; color:var(--soft)}
.note--warn .note__t{color:var(--warn)} .note--cla .note__t{color:var(--clarion)}
.note__b p{margin:0 0 8px; font-size:14.5px} .note__b p:last-child{margin:0}
.stack{display:flex; flex-direction:column; gap:9px; margin:0 0 22px; max-width:640px}
.layer{border:1px solid var(--rule); border-radius:8px; padding:12px 15px; background:var(--surface);
  display:grid; grid-template-columns:118px 1fr; gap:14px; align-items:baseline}
.layer b{font:600 11px/1.4 "IBM Plex Sans",sans-serif; letter-spacing:.06em; text-transform:uppercase;
  color:var(--accent)}
.layer.cla b{color:var(--clarion)}
.layer p{margin:0; font-size:14px; color:var(--soft); max-width:none}
.arrow{text-align:center; color:var(--faint); font-size:12px; margin:-4px 0}
/* ---- where to next ---- */
.next{display:grid; grid-template-columns:repeat(auto-fit,minmax(210px,1fr)); gap:12px;
  margin:26px 0 0}
.next a{display:block; padding:14px 16px; border:1px solid var(--rule); border-radius:9px;
  background:var(--surface); text-decoration:none}
.next a:hover{border-color:var(--accent-rule); background:var(--accent-bg)}
.next b{display:block; font:600 14px/1.3 "IBM Plex Sans",sans-serif; color:var(--accent);
  margin-bottom:3px}
.next span{font:400 13px/1.45 "IBM Plex Serif",serif; color:var(--soft)}
.hide{display:none !important}
footer{margin-top:70px; padding:22px 40px 0; border-top:1px solid var(--rule); color:var(--faint);
  font:400 13px/1.6 "IBM Plex Sans",sans-serif}
@media (max-width:900px){
  .wrap{grid-template-columns:1fr}
  .side{position:static; height:auto; border-right:0; border-bottom:1px solid var(--rule)}
  .inner,.hero{padding-left:22px; padding-right:22px}
}
@media (prefers-reduced-motion:reduce){*{animation:none !important; transition:none !important}}
"""

JS = """
const q = document.getElementById('filter');
if (q) {
  const rows = [...document.querySelectorAll('tr.fn')];
  q.addEventListener('input', () => {
    const t = q.value.trim().toLowerCase();
    rows.forEach(r => r.classList.toggle('hide', t && !r.dataset.k.includes(t)));
    document.querySelectorAll('.tw').forEach(w => {
      const body = w.querySelector('tbody');
      if (!body) return;
      const any = [...body.querySelectorAll('tr')].some(r => !r.classList.contains('hide'));
      const h = w.previousElementSibling;
      w.classList.toggle('hide', !any);
      if (h && h.tagName === 'H3') h.classList.toggle('hide', !any);
    });
  });
}
const links = [...document.querySelectorAll('.nav__l a')];
if (links.length) {
  const spy = new IntersectionObserver(es => {
    es.forEach(e => {
      if (!e.isIntersecting) return;
      links.forEach(a => a.classList.toggle('on', a.getAttribute('href') === '#' + e.target.id));
    });
  }, {rootMargin:'0px 0px -78% 0px'});
  document.querySelectorAll('h2[id],h3[id]').forEach(h => spy.observe(h));
}
"""

def volnav(current):
    out = ['<ul class="vols">']
    for i, (hrefname, name, blurb) in enumerate(VOLUMES):
        here = ' class="here"' if hrefname == current else ''
        out.append('<li><a href="%s"%s>%s. %s<small>%s</small></a></li>'
                   % (href(hrefname, current), here, i + 1, esc(name), esc(blurb)))
    out.append('</ul>')
    return ''.join(out)

def secnav(groups):
    out = []
    for group, items in groups:
        if group: out.append('<p class="nav__g">%s</p>' % esc(group))
        out.append('<ul class="nav__l">')
        for aid, label in items:
            out.append('<li><a href="#%s">%s</a></li>' % (aid, esc(label)))
        out.append('</ul>')
    return ''.join(out)

def nextcards(names):
    cards = []
    for h in names:
        for hrefname, name, blurb in VOLUMES:
            if hrefname == h:
                cards.append('<a href="%s"><b>%s &rarr;</b><span>%s</span></a>'
                             % (PUBLISHED.get(hrefname, hrefname), esc(name), esc(blurb)))
    return '<div class="next">%s</div>' % ''.join(cards)

def page(filename, title, eyebrow, heading, sub, chips, groups, body, showfilter=False):
    nav = volnav(filename) + \
          ('<label class="ui" style="font-size:11px;color:var(--faint);letter-spacing:.08em;'
           'text-transform:uppercase" for="filter">Filter</label>'
           '<input id="filter" class="filter" type="search" placeholder="AddButton, theme&hellip;" '
           'autocomplete="off">' if showfilter else '') + secnav(groups)
    chiphtml = ''.join('<span class="chip">%s</span>' % c for c in chips)
    doc = ('<title>%s</title>\n'
           '<meta name="viewport" content="width=device-width,initial-scale=1">\n'
           '<link rel="preconnect" href="https://fonts.googleapis.com">\n'
           '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>\n'
           '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?'
           'family=IBM+Plex+Mono:wght@400;500&family=IBM+Plex+Sans:wght@400;500;600&'
           'family=IBM+Plex+Serif:wght@400;600&display=swap">\n'
           '<style>%s</style>\n'
           '<div class="wrap">\n<nav class="side">\n'
           '  <p class="brand"><b>ClaCommandBar</b></p>\n%s\n</nav>\n'
           '<main class="main">\n'
           '  <header class="hero"><div class="inner">\n'
           '    <p class="eyebrow">%s</p>\n    <h1>%s</h1>\n    <p class="sub">%s</p>\n'
           '    <div class="chips">%s</div>\n'
           '  </div></header>\n  <div class="inner">%s\n'
           '    <footer>ClaCommandBar &mdash; four volumes. The reference is generated from '
           '<code>commandbar.h</code>, <code>commandbar.def</code> and <code>CommandBar.inc</code>, '
           'so its signatures and ordinals are the ones in the build.</footer>\n'
           '  </div>\n</main>\n</div>\n<script>%s</script>\n'
           % (esc(title), CSS, nav, esc(eyebrow), esc(heading), sub, chiphtml, body, JS))
    io.open('docs/' + filename, 'w', encoding='utf-8', newline='\n').write(doc)
    return len(doc)

# =====================================================================
#  1  GETTING STARTED
# =====================================================================
S_HELLO = """
  PROGRAM
  PRAGMA('link(commandbar.lib)')
  INCLUDE('EQUATES.CLW'),ONCE
  INCLUDE('CommandBar.inc'),ONCE
  MAP
  END

CB    CommandBarClass
bar   SIGNED

CMD:New   EQUATE(1001)
CMD:Open  EQUATE(1002)

Win WINDOW('Hello'),AT(,,320,200),SYSTEM,GRAY,RESIZE,FONT('Segoe UI',9),TIMER(10)
    END

  CODE
  OPEN(Win)
  IF ~CB.Init(Win, CBS:Tooltips)             ! starts the DLL, makes a manager
    MESSAGE('commandbar.dll did not start.')
    RETURN
  END
  CB.SetTheme(CBT:SteelBlue)

  bar = CB.AddBar('Standard', CBD:Top)
  CB.AddButton(bar, CMD:New,  '&New')
  CB.AddButton(bar, CMD:Open, '&Open')
  CB.AddSeparator(bar)
  CB.AddButton(bar, 0, 'Does nothing')       ! command id 0 raises nothing

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      LOOP WHILE CB.TakeOne()                ! drain the queue
        CASE CB.LastCmd
        OF CMD:New  ; Win{PROP:Text} = 'New'
        OF CMD:Open ; Win{PROP:Text} = 'Open'
        END
      END
      CYCLE
    OF EVENT:Sized
      CB.Layout()
    END
  END
  CB.Kill()
  CLOSE(Win)
"""

def build_getting_started():
    B = []
    add = B.append

    add('''<h2 id="what"><span class="k">Start</span>What this is</h2>
<p>ClaCommandBar puts Codejock-style command bars, ribbons and popup menus on a Clarion window. It is
three layers, and you can stop at whichever one suits you.</p>
<div class="stack">
  <div class="layer cla"><b>Templates</b><p>Fill in prompts in AppGen; the wiring is generated. Most people never go below this.</p></div>
  <div class="arrow">&#8595;&nbsp; generates calls to</div>
  <div class="layer cla"><b>CommandBarClass</b><p>An ABC-style Clarion class. Hand-written code talks to this.</p></div>
  <div class="arrow">&#8595;&nbsp; calls, by pinned ordinal</div>
  <div class="layer"><b>commandbar.dll</b><p>A flat <code>__stdcall</code> C API over a Direct2D engine. 32-bit, static CRT, no VC++ redistributable.</p></div>
</div>
<p>The engine draws everything itself, so a bar looks the same on Windows 7 and Windows 11 rather than
inheriting whatever the OS thinks a toolbar should be.</p>''')

    add('<h2 id="install"><span class="k">Start</span>Installing</h2>')
    add('<p>Run <code>clarion\\install.bat</code>. It copies five files and registers the template:</p>')
    add(code('''ClaCommandBar.tpl   ->  accessory\\template\\win
CommandBar.inc      ->  accessory\\libsrc\\win
CommandBar.clw      ->  accessory\\libsrc\\win
commandbar.lib      ->  accessory\\lib  AND  accessory\\libsrc\\win
commandbar.dll      ->  accessory\\bin''', 'files'))
    add(note('warn', 'Close the IDE first, and reopen it after',
       '<p>AppGen caches a <b>parsed</b> copy of the template. An IDE that was open when you installed '
       'keeps serving the old one, which looks exactly like the install not working.</p>'))
    add('<p>Ship <code>commandbar.dll</code> beside your EXE. That is the only runtime file.</p>')

    add('<h2 id="hello"><span class="k">Start</span>Your first bar, by hand</h2>')
    add('<p>No templates involved &mdash; this is the whole program:</p>')
    add(code(S_HELLO))
    add(note('cla', 'Three things that are not optional',
       "<p><code>PRAGMA('link(commandbar.lib)')</code> binds the imports.</p>"
       '<p>A <code>TIMER</code> on the window drives the pump. Without one, nothing you click is ever '
       'delivered &mdash; the bars draw and respond to the mouse, but your code never hears about it.</p>'
       '<p><code>EVENT:Sized</code> has to reach <code>CB.Layout()</code>, or the bars keep the size they '
       'had when the window opened.</p>'))

    add('<h2 id="first-template"><span class="k">Start</span>The same thing, with the template</h2>')
    add('''<ol class="b">
<li>Open the procedure in AppGen and add the extension <b>ClaCommandBar &mdash; command bars and menus on
this window</b>. (On an <code>APPLICATION</code> frame, use the FRAME one instead.)</li>
<li>Go to the <b>Bars</b> tab, pick a toolbar from the drop and press <b>Add this toolbar</b>. That fills
in the bar, its items, the icons, the tooltips and the shortcuts.</li>
<li>Generate and compile. You have a working toolbar before writing a line.</li>
<li>Open the embed point for a command id and put your code there.</li>
</ol>''')
    add(note('cla', 'Icons: name them plainly, add them to the project',
       '<p>Write <code>NEW.ICO</code>, not a path. An image added to the application&rsquo;s project is '
       'linked into the EXE, and Clarion names that resource after the file &mdash; <code>NEW.ICO</code> '
       'becomes <code>NEW_ICO</code> &mdash; which is where the manager looks first. A loose copy beside '
       'the EXE, or in an <code>images</code> folder next to it, is found too.</p>'))

    add('<h2 id="demos"><span class="k">Start</span>The demos</h2>')
    add('<p>Each builds with its own <code>build.bat</code>.</p>')
    add(table(['Folder', 'What to look at'], [
      ['<code>CommandBarShowcase</code>',
       '<b>start here.</b> A mirrored Clarion menu on the main window; <b>Demos &rarr; Ribbon bar</b> for a '
       'ribbon with a Styles <b>gallery</b> and a Zoom group holding a slider, a spin box and a progress '
       'bar; <b>Docking on four edges</b>; <b>Bar on a REGION</b>'],
      ['<code>CommandBarDemo</code>', 'One window with every item type and all eleven themes on a menu'],
      ['<code>MenuMirrorTest</code>',
       'The awkward cases &mdash; mirroring on a plain <code>WINDOW</code>, and on an MDI '
       '<code>APPLICATION</code> frame that opens a merging child. Its README explains what each caught'],
      ['<code>bin\\testhost.exe</code>',
       'The engine with no Clarion at all: every item type, the ribbon, every theme. '
       '<code>testhost.exe 7</code> starts on theme 7'],
    ]))

    add('<h2 id="next"><span class="k">Start</span>Where to go next</h2>')
    add('<p>Three more volumes, depending on what you are doing.</p>')
    add(nextcards(['programmers-guide.html', 'template-guide.html', 'reference.html']))

    return page('getting-started.html',
                'ClaCommandBar Getting Started',
                'Volume 1', 'Getting Started',
                'Install it, put a bar on screen by hand, then do the same thing from AppGen in four '
                'steps. Twenty minutes, no prior knowledge assumed.',
                ['<b>5</b> files to install', '<b>1</b> DLL to ship', '<b>4</b> demos'],
                [('This volume', [('what', 'What this is'), ('install', 'Installing'),
                                  ('hello', 'Your first bar'), ('first-template', 'With the template'),
                                  ('demos', 'The demos'), ('next', 'Where to go next')])],
                ''.join(B))


# =====================================================================
#  2  PROGRAMMER'S GUIDE
# =====================================================================
S_PUMP = """
  OF EVENT:Timer
    LOOP WHILE CB.TakeOne()          ! returns 0 when the queue is empty
      CASE CB.LastCmd
      OF CMD:Save ; DO SaveRoutine
      END
    END
    CYCLE
"""

S_DERIVE = """
MyBar  CLASS(CommandBarClass)
TakeCommand PROCEDURE(LONG cmd, SIGNED item),BYTE,DERIVED
       END

MyBar.TakeCommand PROCEDURE(LONG cmd, SIGNED item)
  CODE
  CASE cmd
  OF CMD:Save ; DO SaveRoutine
  OF CMD:Exit ; POST(EVENT:CloseWindow)
  END
  RETURN PARENT.TakeCommand(cmd, item)
"""

S_MIRROR2 = """
Win WINDOW('Mirrored'),AT(,,420,260),SYSTEM,GRAY,RESIZE,TIMER(10)
      MENUBAR
        MENU('&File')
          ITEM('&New'),USE(?MNew),KEY(CtrlN)
          ITEM('E&xit'),USE(?MExit)
        END
      END
    END

  CODE
  OPEN(Win)
  CB.Init(Win, CBS:Tooltips + CBS:MenuIcons)
  CB.MirrorMenu(CB.AddMenuBar(), 1)      ! 1 = take the real menu off the window

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer ; LOOP WHILE CB.TakeOne() . ; CYCLE
    END
    CASE ACCEPTED()
    OF ?MNew                             ! your ORIGINAL embed code still runs
      Win{PROP:Text} = 'New, from the mirrored row'
    OF ?MExit
      POST(EVENT:CloseWindow)
    END
  END
"""

S_TB2 = """
tb = CB.AddBar('Tools', CBD:Top)
CB.SetBarDock(tb, CBD:Top, 1, 0)         ! its own row, under the menu
n  = CB.MirrorToolbar(tb, 1)             ! 1 = hide the real TOOLBAR
"""

S_RIBBON2 = """
rib = CB.AddRibbon('Ribbon', CBD:Top)
tab = CB.AddRibbonTab(rib, '&Home')

grp = CB.AddRibbonGroup(tab, 'Clipboard')
CB.AddLargeButton(grp, CMD:Paste, 'Paste', iPaste)   ! the big one
CB.AddButton(grp, CMD:Cut,  'Cut',  iCut)            ! small ones stack beside it
CB.AddButton(grp, CMD:Copy, 'Copy', iCopy)

grp = CB.AddRibbonGroup(tab, 'Styles')
gal = CB.AddGallery(grp, CMD:Style, 4, 62, 54)       ! columns, cell w, cell h
CB.AddGalleryCell(gal, iNew,  'Normal')
CB.AddGalleryCell(gal, iOpen, 'Heading')
CB.SetGallerySel(gal, 0)

CB.MinimizeRibbon(rib, 1)                            ! start collapsed
"""

S_HOST2 = """
!  On an APPLICATION FRAME the frame's own toolbar and MDI client are moved
!  clear of the bars automatically.  A plain WINDOW positions its own
!  controls, so re-fit whatever should fill what the bars left:
  OF EVENT:Sized
    CB.Layout()
    CB.FitControl(?Browse:1, 4, 4)       ! 4px margin

!  A WINDOW with a STATUS bar has no MDI client to measure from, so say
!  how tall it is and no bar will cover it:
  CB.HostReserveBottom(23)
"""

S_LAYOUT2 = """
!  after the bars are built
CB.RestoreLayoutFrom('.\\MyApp.INI', 'CommandBars')

!  before the window closes
CB.SaveLayoutTo('.\\MyApp.INI', 'CommandBars')
"""

S_VALUES2 = """
zoom = CB.AddSlider(bar, CMD:Zoom, 25, 400, 100, 130)   ! lo, hi, value, width
page = CB.AddSpin(bar, CMD:Page, 1, 9999, 1, 60)
busy = CB.AddProgress(bar, 0, 100, 0, 130)

!  in the pump - the value arrives in LastParam
  OF CBE:ValueChanged
    CASE CB.LastCmd
    OF CMD:Zoom ; Scale = CB.LastParam
    END

!  the progress bar raises nothing; drive it yourself
CB.SetItemNumber(busy, done * 100 / total)
"""

S_THEME2 = """
CB.SetTheme(CBT:Office2016)              ! start from one of eleven
CB.SetAccent(COLOR:Navy)                 ! re-derive the palette from an accent
CB.SetColor(CBC:BarBack, 00F5F5F5h)      ! or override single slots
CB.SetMetric(CBM:IconSize, 20)
CB.SetMetric(CBM:LargeIcon, 32)
CB.Redraw()
"""

def build_programmers_guide():
    B = []
    add = B.append

    add('''<h2 id="model"><span class="k">Concepts</span>Managers, containers, items</h2>
<p>One <b>manager</b> per window &mdash; <code>CB.Init</code> makes it, <code>CB.Kill</code> takes it
down. A <b>container</b> is anything that holds items: a docked bar, a floating bar, a popup menu, or a
ribbon group. An <b>item</b> is a button, a toggle, a separator, an edit box. Both come back as integer
ids, and you keep the ones you will need again.</p>
<h3>Command ids are yours</h3>
<p>Every item carries a <b>command id</b> you choose. Clicking it queues an event carrying that id; an id
of <code>0</code> means the item raises nothing, which is what separators and labels use. Two items may
share an id &mdash; a toolbar button and a menu row for the same command usually should, and then
<code>EnableCmd</code> greys out both at once.</p>''')

    add('<h2 id="pump"><span class="k">Concepts</span>Nothing is dispatched behind your back</h2>')
    add('<p>The engine never calls into your code. It queues events, and you drain the queue:</p>')
    add(code(S_PUMP))
    add('''<p><code>TakeOne</code> fills <code>LastItem</code>, <code>LastCmd</code>,
<code>LastEvent</code> and <code>LastParam</code>, then calls the virtual <code>TakeCommand</code> /
<code>TakeToggled</code> methods. Deriving is the tidier way once a window has more than a handful of
commands:</p>''')
    add(code(S_DERIVE))
    add(note('warn', 'The pump needs a TIMER',
       '<p>Bars with no timer draw, hot-track and open their menus &mdash; and never deliver a thing. '
       'It is the single most common reason a bar &ldquo;does nothing&rdquo;.</p>'))

    add('<h2 id="docking"><span class="k">Concepts</span>Rows, offsets and docking</h2>')
    add('''<p>Bars dock to an edge on a <b>row</b>: row 0 is nearest the edge, row 1 sits under it. Two
bars on the same row sit side by side in <b>offset</b> order. A user with a gripper can drag a bar to
another edge, into another row, or tear it off into a floating caption frame.</p>
<p>A <b>ribbon always takes a row to itself</b>. It is a full-width band with tabs across it, so sharing a
row would leave nothing for its neighbour &mdash; put a menu bar and a ribbon both on row 0 and the ribbon
moves down to row 1 rather than swallowing the menu.</p>''')

    add('<h2 id="ribbon"><span class="k">How to</span>Building a ribbon</h2>')
    add('<p>A bar of <b>tabs</b>; each tab a row of <b>groups</b>; each group full of ordinary items.</p>')
    add(code(S_RIBBON2))
    add('''<p><code>AddLargeButton</code> (or the <code>CBIS:TextBelow</code> style) makes the big
image-over-text button a group leads with; everything beside it stacks three-deep in small rows. A
<b>gallery</b> takes a column of its own, and is shrunk to fit if it asks for more height than the group
has.</p>
<p>Collapsing is built in: a chevron at the end of the tab strip, a double-click on a tab, or
<code>MinimizeRibbon</code>.</p>''')

    add('<h2 id="mirror"><span class="k">How to</span>Taking over the menu</h2>')
    add('''<p><code>MirrorMenu</code> reads the window&rsquo;s <code>MENUBAR</code> at run time and
rebuilds it as a command bar &mdash; same order, same nesting, separators kept, <code>KEY()</code>
attributes turned into a shortcut column, disabled items still disabled. Choosing a mirrored row POSTs
<code>EVENT:Accepted</code> to the <b>original</b> <code>ITEM</code>, so every menu embed you already
wrote goes on running.</p>''')
    add(code(S_MIRROR2))
    add('<h3>And the toolbar</h3>')
    add('''<p>The same trick on the row of buttons. Every <code>BUTTON</code>, <code>CHECK</code>,
<code>ENTRY</code>, <code>COMBO</code> and <code>PROMPT</code> in the <code>TOOLBAR</code> becomes a bar
item carrying its <code>ICON()</code>, its <code>TIP()</code> and its disabled state.</p>''')
    add(code(S_TB2))
    add(note('cla', 'Why this matters on an MDI frame',
       '<p>Opening a child window makes Clarion hide the frame&rsquo;s toolbar, build a <b>second</b> '
       '<code>ClaToolBar</code> for the merged one, and swap them. Mirror the toolbar, hide the real one, '
       'and there is nothing left on screen for that to disturb.</p>'))
    add(note('warn', 'What mirroring cannot see',
       '<p>On an MDI frame it mirrors the <b>frame&rsquo;s own</b> menu. A menu an MDI child merges in is '
       'not reachable: the child&rsquo;s controls live on the child&rsquo;s thread and never enter the '
       'frame&rsquo;s control list, and although the frame&rsquo;s <code>HMENU</code> really does grow, '
       'Clarion owner-draws its menus so the items come back with no text at all.</p>'
       '<p>Use <code>NOMERGE</code> on the children, or leave the real menu attached.</p>'))

    add('<h2 id="host"><span class="k">How to</span>Leaving room for the bars</h2>')
    add(code(S_HOST2))
    add('''<p><code>CB_GetClientRect</code> &mdash; <code>ClientX/Y/Width/Height</code> on the class
&mdash; reports what the bars left over, in <b>pixels</b>. On a frame the host&rsquo;s own children are
moved for you; on a plain window you place your own controls.</p>''')

    add('<h2 id="values"><span class="k">How to</span>Sliders, spin boxes and progress bars</h2>')
    add('<p>One idea wearing three faces: a value between two bounds.</p>')
    add(code(S_VALUES2))
    add(note('warn', 'A drag fires on every step',
       '<p>Keep that handler cheap &mdash; store the value and do the heavy work afterwards, or a slider '
       'repaints your report once per pixel.</p>'))

    add('<h2 id="layout"><span class="k">How to</span>Remembering where the user put them</h2>')
    add('''<p>One INI entry holds which edge each bar is on, which row, the order within it, whether it is
showing, where a floating one sits, and whether a ribbon is collapsed.</p>''')
    add(code(S_LAYOUT2))
    add('''<p>Bars are matched by <b>name</b>, so adding or removing one in a later release never hands an
old position to the wrong bar; a name it does not recognise is ignored. <code>LayoutText()</code> and
<code>RestoreLayout()</code> hand you the blob directly if you would rather keep it in a user record.</p>''')

    add('<h2 id="theme"><span class="k">How to</span>Theming</h2>')
    add(code(S_THEME2))
    add('''<p>Eleven themes, each derived from a handful of seed colours; 38 colour slots, four font roles
and thirteen metrics on top. The full tables are in the <a href="https://claude.ai/code/artifact/bfef6768-0e5d-45e6-9a8e-9f053eb6dbbb#eq">Reference</a>.</p>''')

    add('<h2 id="clarion"><span class="k">Clarion notes</span>Why Clarion behaves as it does</h2>')
    add('''<p>Most of the surprises in this library are not the library. They are worth knowing before you
go looking for a bug that is not there.</p>''')
    add(table(['What you see', 'What is actually happening'], [
      ['A mirrored bar comes out empty',
       'Menu controls live in <b>four</b> equate ranges. A <code>WINDOW</code> numbers named ones upward '
       'from 1 and unnamed ones upward from 32768; an <code>APPLICATION</code> frame numbers named ones '
       '<b>downward from -1</b> and unnamed ones downward from 32767, and answers '
       '<code>LASTFIELD() = 0</code>. Call <code>MenuReport()</code> &mdash; it says which it found'],
      ['The frame&rsquo;s toolbar disappears when a procedure opens',
       '<code>NOMERGE</code> on the frame&rsquo;s own <code>TOOLBAR</code> means &ldquo;do not carry this '
       'into the merge&rdquo;. Put <code>NOMERGE</code> on <b>children</b>, never on the frame&rsquo;s '
       'toolbar'],
      ['A child&rsquo;s merged menu never appears on the mirrored bar',
       'It cannot. The child&rsquo;s controls are on the child&rsquo;s thread, and the merged '
       '<code>HMENU</code> is owner-drawn &mdash; every item reads back <code>MFT_OWNERDRAW</code> with '
       '<code>cch = 0</code> and no text'],
      ['A toolbar flickers when an MDI child opens or closes',
       'Clarion swaps the frame&rsquo;s toolbar for a merged one and back, so that strip is painted twice '
       'either way. Mirroring the toolbar and hiding the real one removes it'],
      ['<code>PROP:Hide</code> does nothing to a <code>MENUBAR</code>',
       'It is a real Win32 menu on the frame, not a control. <code>SetHostMenu(0)</code> detaches it with '
       '<code>SetMenu()</code>, and puts it back when the window closes'],
    ]))
    add(note('cla', 'When a frame child sits in the wrong place',
       '<p>Set <code>CB_HOSTLOG=1</code> in the environment and run again. Every move of the host&rsquo;s '
       'own children is traced to <code>%TEMP%\\cbhost.log</code> &mdash; <code>HOOK</code>, '
       '<code>CHANGING</code> and <code>APPLY</code> lines with the rect asked for and the rect given.</p>'))

    add('<h2 id="trouble"><span class="k">Clarion notes</span>When it misbehaves</h2>')
    add(table(['What you see', 'What it is'], [
      ['No bars at all', 'The DLL is not beside the EXE, or <code>Init</code> returned 0 &mdash; check its result'],
      ['Bars appear, clicks do nothing', 'Nothing is pumping. The window needs a <code>TIMER</code> and the timer needs to reach <code>TakeOne</code>'],
      ['Bars do not resize with the window', '<code>EVENT:Sized</code> is not reaching <code>Layout()</code>'],
      ['Icons missing', 'The images are not in the project and not beside the EXE. <code>NEW.ICO</code> is looked up as the resource <code>NEW_ICO</code>'],
      ['A slider, spin, progress or gallery draws as a plain button with its text on it',
       'An old <code>commandbar.dll</code>: <code>CB_AddItem</code> used to reject any type above '
       '<code>CBI_SPACE</code> and hand back a button. The DLL alone fixes it &mdash; nothing to regenerate'],
      ['A slider moves but nothing happens in a generated app',
       'Older generated code: the pump only handled <code>CBE:Command</code>. It takes '
       '<code>CBE:ValueChanged</code> too now, so regenerate'],
      ['<code>Unresolved External CB_...</code>',
       'A second <code>commandbar.lib</code> is shadowing the installed one &mdash; check your own '
       'application folder, and <code>accessory\\libsrc\\win</code>'],
      ['AppGen hangs on a template button',
       'A template group returned text where a number was expected &mdash; <code>&#39;0&#39;</code> is '
       'true to <code>#IF</code> and to <code>#LOOP,WHILE</code>'],
    ]))

    add('<h2 id="next2"><span class="k">Guide</span>Where to go next</h2>')
    add(nextcards(['template-guide.html', 'reference.html', 'getting-started.html']))

    return page('programmers-guide.html',
                "ClaCommandBar Programmer's Guide",
                'Volume 2', "Programmer's Guide",
                'How the pieces fit, how to make each one do what you want, and why Clarion behaves the '
                'way it does when it surprises you.',
                ['<b>3</b> layers', '<b>16</b> item types', '<b>11</b> themes'],
                [('Concepts', [('model', 'Managers and items'), ('pump', 'The event pump'),
                               ('docking', 'Rows and docking')]),
                 ('How to', [('ribbon', 'Building a ribbon'), ('mirror', 'Taking over the menu'),
                             ('host', 'Room for the bars'), ('values', 'Sliders and progress'),
                             ('layout', 'Remembering positions'), ('theme', 'Theming')]),
                 ('Clarion notes', [('clarion', 'Why Clarion does that'), ('trouble', 'When it misbehaves'),
                                    ('next2', 'Where to go next')])],
                ''.join(B))

# =====================================================================
#  3  TEMPLATE GUIDE
# =====================================================================
S_GEN = """
!  what the template writes into your procedure, trimmed:
CBBar:1:Standard = CommandBar.AddBar('Standard',CBD:Top,6)
CommandBar.SetBarDock(CBBar:1:Standard,CBD:Top,0,0)
CBItm:1:1 = CommandBar.AddItem(CBBar:1:Standard,CBI:Button,1000,'&New',CBImg:1:New)
CommandBar.SetItemTooltip(CBItm:1:1,'New (Ctrl+N)')

!  and the dispatch, one branch per distinct command id:
LOOP WHILE CommandBar.TakeOne()
  CASE CommandBar.LastEvent
  OF CBE:Command
  OROF CBE:ValueChanged
    CASE CommandBar.LastCmd
    OF 1000
      !  <-- your embed for command 1000 runs here
    END
  END
END
"""

def build_template_guide():
    B = []
    add = B.append

    add('<h2 id="four"><span class="k">Templates</span>The four templates</h2>')
    add(table(['Template', 'Kind', 'Put it on', 'What it adds'], [
      ['<code>CommandBarGlobal</code>', 'Extension', 'the application, once',
       'defaults every window inherits &mdash; theme, colours, icon sizes'],
      ['<code>CommandBarOnWindow</code>', 'Extension', 'any procedure with a window',
       'bars, ribbons, menus and items on that window'],
      ['<code>CommandBarFrame</code>', 'Extension', 'an <code>APPLICATION</code> frame',
       "everything above, plus mirroring the frame's <code>MENUBAR</code> and <code>TOOLBAR</code>"],
      ['<code>CommandBarControl</code>', 'Control', 'a window, from the toolbox',
       'drops a <code>REGION</code> and lands a bar exactly on it'],
    ]))
    add('''<p>They share their tabs, so everything below applies wherever you find it. Only the frame
template has <b>Menu and toolbar</b>; only the control template can land a bar on its own region.</p>''')

    add('<h2 id="tabs"><span class="k">Templates</span>The tabs</h2>')
    add(table(['Tab', 'What lives there'], [
      ['<b>General</b>', 'whether to build at all, the timer interval, which control fills the space the '
       'bars leave, and <i>Remember where the user puts the bars</i>'],
      ['<b>Menu and toolbar</b>', '<i>frame only</i> &mdash; mirror the <code>MENUBAR</code>, mirror the '
       '<code>TOOLBAR</code>, and whether to take each original off the frame'],
      ['<b>Appearance</b>', 'theme, small and large icon sizes, item height, corner radius, menu width'],
      ['<b>Colours</b>', 'override any of the 38 slots on top of the theme'],
      ['<b>Images</b>', 'name the icon files once; items refer to the names'],
      ['<b>Bars</b>', 'one entry per bar &mdash; name, dock, row, offset, gripper, floatable, ribbon, '
       'starts collapsed. And the five toolbar presets'],
      ['<b>Ribbon</b>', 'tabs, and the groups inside them. And the ribbon preset'],
      ['<b>Menus</b>', 'popup menus by name. And five menu presets'],
      ['<b>Items</b>', 'everything that goes in a bar, a menu or a ribbon group'],
      ['<b>Keys</b>', 'a Clarion key equate to a command id'],
    ]))
    add('''<p><b>Names tie it together.</b> A bar is named on the Bars tab; an item names that bar in
<i>Put it in</i>. The same goes for menus and ribbon groups. Each name becomes a variable in the generated
source &mdash; <code>CBBar:1:Standard</code>, <code>CBMnu:1:RowMenu</code> &mdash; which is what you pass
when you call the class yourself from an embed.</p>''')
    add(note('warn', 'A name that resolves to nothing is a generate-time error',
       '<p>Not a silently missing item. If you rename a bar, the items pointing at it stop the generate '
       'until you rename them too.</p>'))

    add('<h2 id="presets"><span class="k">Templates</span>Presets</h2>')
    add('''<p>Three buttons fill the lists in for you. What they make is ordinary entries afterwards
&mdash; rename them, reorder them, delete the ones you do not want.</p>''')
    add('<h4>Bars tab &mdash; five classic toolbars</h4>')
    add(table(['Preset', 'What is on it'], [
      ['<b>Standard</b>', 'New, Open, Save &#124; Print &#124; Cut, Copy, Paste &#124; Undo, Redo &#124; '
       'a search box, Find &#124; Help at the far end. <code>Ctrl+N/O/S/P/F</code>, <code>F1</code>'],
      ['<b>Formatting</b>', 'font and size combos &#124; <b>B</b> <i>I</i> U and a text-colour button '
       '&#124; left / centre / right &#124; bulleted and numbered lists. <code>Ctrl+B/I/U</code>'],
      ['<b>Browse and records</b>', 'the VCR keys &#124; Insert, Change, Delete &#124; a locator box, '
       'Locate, Sort, Mark &#124; Refresh &#124; Print'],
      ['<b>Navigation</b>', 'Back, Forward, Stop, Refresh, Home &#124; an address box that '
       '<b>stretches</b> &#124; Go, Search &#124; a zoom <b>slider</b>'],
      ['<b>Print and export</b>', 'Print, Preview, a page <b>spin box</b> &#124; an <b>Export drop '
       'button</b> carrying PDF, Excel, CSV, HTML, XML and text &#124; Refresh &#124; a <b>progress '
       'bar</b> and Close'],
    ]))
    add('<h4>Ribbon tab</h4>')
    add('''<p>The ribbon from <code>CommandBarShowcase</code>, plus the two things only a ribbon can do:
<b>Home</b> (Clipboard, Font, a <b>Styles gallery</b>, Editing), <b>Insert</b> (Pages, Illustrations,
Links) and <b>View</b> (Show, and Zoom with a <b>slider</b>).</p>''')
    add('<h4>Menus tab</h4>')
    add('<p>One of <b>File</b>, <b>Edit</b>, <b>Browse row</b>, <b>View</b> or <b>Help</b>.</p>')
    add('''<p>Each press starts a new bar on the first row of that edge nobody is using, and command ids
carry on from the highest already in use &mdash; so nothing a preset makes collides with what you added
yourself. Press one twice and you get <code>Standard2</code>.</p>''')

    add('<h2 id="items"><span class="k">Templates</span>The Items dialog</h2>')
    add('<p>Five tabs, because sixteen item types will not fit in one column.</p>')
    add(table(['Tab', 'What is on it'], [
      ['<b>General</b>', 'where it goes, type, text, command id, image, tooltip, width'],
      ['<b>Action</b>', 'what it does, and only the fields that action needs'],
      ['<b>Settings</b>', '<b>one</b> box, chosen by type &mdash; the menu it opens, combo choices, edit '
       'value, colour, slider range, gallery grid and cells'],
      ['<b>In a menu</b>', 'shortcut text, bold (the default row), radio dot'],
      ['<b>State and layout</b>', 'checked, disabled, hidden, auto-check, right-align, wrap, '
       'image-above-text, icon-only, text-only, stretch'],
    ]))
    add('<h3>What an item does</h3>')
    add(table(['Action', 'Generates'], [
      ['Embed code only', 'nothing &mdash; you write it'],
      ['Call a procedure', '<code>ThatProcedure(parms)</code>'],
      ['Do a routine', '<code>DO ThatRoutine</code>'],
      ['Emulate a control', '<code>POST(EVENT:Accepted, ?ThatControl)</code>'],
      ['Post an event', '<code>POST(EVENT:Whatever, ?Control)</code>'],
      ['Close the window', '<code>POST(EVENT:CloseWindow)</code>'],
    ]))
    add('''<p><b>Emulate a control</b> saves the most work: point a bar button at a <code>BUTTON</code> or
a menu <code>ITEM</code> already on the window and its existing embed code runs, untouched.</p>''')
    add('<h3>A gallery&rsquo;s cells</h3>')
    add('''<p>One line, <code>Text=Image</code> separated by pipes, in the order they read &mdash; the
image names come from the Images tab:</p>''')
    add(code('Normal=New|Heading=Open|Title=Save|Quote=Print', 'cells'))
    add('''<p>Leave the <code>=Image</code> off a cell that has no picture. Clicking a cell raises the
item&rsquo;s command with the cell number (0 first) in <code>LastParam</code>.</p>''')

    add('<h2 id="embeds"><span class="k">Templates</span>Embed points</h2>')
    add('''<p>Ten, each existing in all three extensions. The per-command one is the one you will use
most: <b>every distinct command id gets its own embed</b>, so you are not writing a <code>CASE</code> by
hand.</p>''')
    add(table(['Embed', 'When it runs'], [
      ['<b>this command was chosen</b>', 'one per command id &mdash; the workhorse'],
      ['before the bars are created', 'anything that must happen before <code>Init</code>&rsquo;s work'],
      ['after the bars are built', 'add items the prompts cannot describe'],
      ['a toggle or check box flipped', '<code>LastParam</code> is 1 when on'],
      ['a menu is about to open', 'rebuild a Recent list before it shows'],
      ['an edit box was committed', '<code>ItemValue(item)</code> has the text'],
      ['a combo selection changed', '<code>LastParam</code> is the 0-based index'],
      ['a colour button changed', '<code>LastParam</code> is the colour'],
      ['a ribbon tab was switched', '<code>LastParam</code> is the tab'],
      ['an item was right-clicked', 'hang a context menu off it'],
    ]))
    add('''<p>The generated object is called <code>CommandBar</code>, so inside an embed you have the
whole class: <code>CommandBar.SetItemEnabled(CBItm:1:7, 0)</code>.</p>''')

    add('<h2 id="generated"><span class="k">Templates</span>What gets generated</h2>')
    add(code(S_GEN))
    add(note('cla', 'Regenerate versus rebuild',
       '<p>Template changes only reach you on the next <b>generate</b>. Engine changes are in the DLL and '
       'reach you on the next <b>run</b>. When something is fixed &ldquo;in the DLL alone&rdquo;, copying '
       'the new <code>commandbar.dll</code> beside your EXE is the whole fix.</p>'))

    add('<h2 id="next3"><span class="k">Templates</span>Where to go next</h2>')
    add(nextcards(['reference.html', 'programmers-guide.html', 'getting-started.html']))

    return page('template-guide.html',
                'ClaCommandBar Template Guide',
                'Volume 3', 'Template Guide',
                'The four templates, every tab and prompt, the presets, the embed points, and what the '
                'generator actually writes into your procedure.',
                ['<b>4</b> templates', '<b>10</b> tabs', '<b>10</b> embed points', '<b>11</b> presets'],
                [('This volume', [('four', 'The four templates'), ('tabs', 'The tabs'),
                                  ('presets', 'Presets'), ('items', 'The Items dialog'),
                                  ('embeds', 'Embed points'), ('generated', 'What gets generated'),
                                  ('next3', 'Where to go next')])],
                ''.join(B))


# =====================================================================
#  4  REFERENCE
# =====================================================================
CAPI_BLURB = {
 'lifetime': 'Start the engine once, make a manager per window, take it down again. Everything else hangs '
             'off the <code>HCB</code> handle <code>CB_Create</code> returns.',
 'theming': 'Eleven built-in themes, each derived from a handful of seed colours. Override any of the 38 '
            'slots, the four font roles or the thirteen metrics on top of one.',
 'images': 'An image list per manager. Every item that shows a picture refers to an index in it.',
 'containers: bars and menus': 'A <em>container</em> holds items: a docked bar, a floating bar, or a popup '
            'menu with no bar of its own.',
 'ribbons': 'A ribbon is a bar with <code>CBBS_RIBBON</code>: it holds tabs, a tab holds groups, and a '
            'group holds ordinary items.',
 'items': 'Everything that can sit in a container, and everything you can do to one afterwards.',
 'edit / combo / colour item values': 'Reading and writing the value of the items that carry one.',
 'items that carry a number': 'A slider, a spin box and a progress bar: a value between two bounds.',
 'galleries': 'A grid of picture choices, sized in cells.',
 'keyboard': 'Give a key an id and it raises that command, wherever the focus is.',
 'events': 'Nothing is dispatched behind your back: clicks queue up and you drain the queue.',
 'odds and ends': 'Small things with nowhere better to live.',
 'reserving space from the host': "On an APPLICATION frame the host lays its own toolbar and MDI client "
            "out against the full client area. This moves them clear of the bars.",
 'saving where the user put the bars': 'One small text blob in, one out.',
}

GROUPS = [
 ('Lifetime and layout',      ['Construct','Destruct','Init','Kill','Layout','Redraw','FitControl',
                               'ClientX','ClientY','ClientWidth','ClientHeight','PlaceOnControl',
                               'ReserveSpace','ReserveMode','HostReserveBottom','HostReserveHeight']),
 ('Bars',                     ['AddBar','AddMenuBar','AddRibbon','SetBarDock','BarDock','SetBarVisible',
                               'BarVisible','FloatBar','SetBarRect']),
 ('Ribbons',                  ['AddRibbonTab','AddRibbonGroup','AddLargeButton','SetActiveTab','ActiveTab',
                               'TabCount','TabAt','MinimizeRibbon','RibbonMinimized']),
 ('Menus and popups',         ['CreateMenu','AddMenuTitle','AddSubMenu','AddMenuRow','SetItemMenu',
                               'ItemMenu','PopupMenu','TrackMenu','ClearContainer','DestroyContainer']),
 ('Items',                    ['AddItem','InsertItem','AddButton','AddToggle','AddDropButton',
                               'AddSplitButton','AddSeparator','AddLabel','AddEdit','AddCombo',
                               'AddCheckBox','AddColorButton','AddSpace','RemoveItem','ItemCount',
                               'ItemAt','FindItem']),
 ('Item state and looks',     ['SetItemText','ItemText','SetItemImage','SetItemEnabled','ItemEnabled',
                               'SetItemChecked','ItemChecked','SetItemVisible','ItemVisible',
                               'SetItemStyle','ItemStyle','SetItemWidth','SetItemTooltip',
                               'SetItemShortcut','SetItemCmd','ItemCmd','EnableCmd','CheckCmd']),
 ('Values',                   ['SetItemValue','ItemValue','AddComboItem','SetComboList','ClearComboItems',
                               'SetComboSel','ComboSel','SetItemColor','ItemColor',
                               'AddSlider','AddSpin','AddProgress','SetItemRange','SetItemNumber',
                               'ItemNumber']),
 ('Galleries',                ['AddGallery','AddGalleryCell','SetGalleryGrid','GallerySel','SetGallerySel']),
 ('Images',                   ['AddImage','AddImageStrip','AddImageHandle','ImageCount']),
 ('Theme and appearance',     ['SetTheme','SetAccent','SetColor','GetColor','SetFont','SetMetric',
                               'GetMetric']),
 ('Keyboard',                 ['AddAccelerator','AddClarionKey','ClearAccelerators','TakeKey',
                               'TakeAlertKey','KeyText']),
 ('The event pump',           ['TakeOne','TakeEvent','TakeCommand','TakeToggled','TakeDropDown',
                               'TakeTextChanged','TakeSelChanged','TakeColorChanged','TakeLayoutChanged',
                               'TakeRightClick']),
 ('Mirroring a Clarion menu', ['MirrorMenu','MirrorMenuFrom','RefreshMirror','FindMenuBar','MenuReport',
                               'MirrorInto','MinFeqIn','MenuRank','SetHostMenu','HostMenuVisible']),
 ('Mirroring the toolbar',    ['MirrorToolbar','MirrorToolbarFrom','FindToolbar','FindControlOfType',
                               'ShowHostToolbar']),
 ('Remembering the layout',   ['LayoutText','RestoreLayout','SaveLayoutTo','RestoreLayoutFrom']),
]

def api_section(sec):
    rows = []
    for f in sec['funcs']:
        doc = ' '.join(f['doc']) if f['doc'] else (f['trail'] or '')
        rows.append('<tr class="fn" data-k="%s"><td class="fn__n"><code>%s</code>%s</td>'
                    '<td class="fn__s"><code>%s</code>%s</td></tr>'
                    % (esc((f['name'] + ' ' + doc).lower()), esc(f['name']),
                       '<span class="ord" title="export ordinal - pinned, never reused">@%s</span>'
                       % f['ord'] if f['ord'] else '',
                       esc(f['sig'].replace('CBAPI ', '')),
                       '<p class="fn__d">%s</p>' % esc(doc) if doc else ''))
    return ('<h3 id="capi-%s">%s</h3>%s<div class="tw"><table class="fns"><tbody>%s</tbody></table></div>'
            % (slug(sec['name']), esc(sec['name'][0].upper() + sec['name'][1:]),
               '<p class="lead">%s</p>' % CAPI_BLURB.get(sec['name'], ''), ''.join(rows)))

def const_section(sec):
    rows = ''.join('<tr><td><code>%s</code></td><td class="v"><code>%s</code></td><td>%s</td></tr>'
                   % (esc(c['name']), esc(c['value']), esc(c['note'])) for c in sec['consts'])
    return ('<h3 id="eq-%s">%s</h3><div class="tw"><table class="eq"><thead><tr>'
            '<th>C name</th><th>Value</th><th>Meaning</th></tr></thead><tbody>%s</tbody></table></div>'
            % (slug(sec['name']), esc(sec['name'][0].upper() + sec['name'][1:]), rows))

def methrow(name, sig, doc):
    u = USAGE.get(name)
    if not u: MISSING.append(name)
    return ('<tr class="fn" data-k="%s"><td class="fn__n"><code>%s</code></td>'
            '<td class="fn__s"><code>%s</code>%s%s</td></tr>'
            % (esc((name + ' ' + doc).lower()), esc(name), esc(sig),
               '<p class="fn__d">%s</p>' % esc(doc) if doc else '',
               usecode(u) if u else ''))

def class_tables():
    seen, out = set(), []
    byname = {m['name']: m for m in CLASS}
    for title, names in GROUPS:
        rows = []
        for n in names:
            m = byname.get(n)
            if not m: continue
            seen.add(n)
            sig = '%s(%s)%s' % (m['name'], m['parms'], (',' + m['attrs']) if m['attrs'] else '')
            rows.append(methrow(n, sig, ' '.join(m['doc'])))
        if rows:
            out.append('<h3 id="cls-%s">%s</h3><div class="tw"><table class="fns"><tbody>%s</tbody>'
                       '</table></div>' % (slug(title), esc(title), ''.join(rows)))
    rest = [m for m in CLASS if m['name'] not in seen]
    if rest:
        rows = ''.join(methrow(m['name'], '%s(%s)' % (m['name'], m['parms']), ' '.join(m['doc']))
                       for m in rest)
        out.append('<h3 id="cls-more">Everything else</h3><div class="tw"><table class="fns"><tbody>'
                   '%s</tbody></table></div>' % rows)
    return ''.join(out)

def build_reference():
    B = []
    add = B.append
    funcsecs  = [s for s in API if s['funcs']]
    constsecs = [s for s in API if s['consts']]

    add('<h2 id="cls"><span class="k">Clarion</span>CommandBarClass</h2>')
    add('''<p>Declared in <code>CommandBar.inc</code>, implemented in <code>CommandBar.clw</code>. Every
method is <code>VIRTUAL</code>, so deriving works the ABC way. Each entry carries a worked line of
Clarion, and the tables are read out of the header &mdash; they cannot drift from the code.</p>''')
    add(code('CB   CommandBarClass                     ! that is the whole declaration'))

    add('<h3 id="cls-props">Properties</h3>')
    add('''<p>Public members you read or set directly. The four <code>Last&hellip;</code> ones are what an
event leaves behind, and are the reason most windows never need more than <code>TakeOne</code>.</p>''')
    add('<div class="tw"><table class="fns"><tbody>' + ''.join(
        '<tr class="fn" data-k="%s"><td class="fn__n"><code>%s</code></td>'
        '<td class="fn__s"><code>%s</code>%s%s</td></tr>'
        % (esc((pr['name'] + ' ' + ' '.join(pr['doc'])).lower()), esc(pr['name']), esc(pr['type']),
           '<p class="fn__d">%s</p>' % esc(' '.join(pr['doc']) or pr['note'])
           if (pr['doc'] or pr['note']) else '',
           usecode(PROPS[pr['name']]) if pr['name'] in PROPS else '')
        for pr in PROPLIST) + '</tbody></table></div>')
    add(class_tables())

    add('<h2 id="capi"><span class="k">The DLL</span>commandbar.dll</h2>')
    add('''<p>Plain <code>__stdcall</code>, no C++ in the signatures &mdash; usable from anything that can
call a DLL. The Clarion class is only the first caller.</p>''')
    add(code('''CB_Initialize();
HCB cb = CB_Create(hwnd, CBS_TOOLTIPS);
CB_SetTheme(cb, CBT_STEELBLUE);

int bar = CB_AddBar(cb, "Standard", CBD_TOP, CBBS_GRIPPER | CBBS_FLOATABLE);
int img = CB_AddImage(cb, "NEW.ICO");
CB_AddButton(cb, bar, 1001, "&New", img);
CB_Layout(cb);

int item, ev; long cmd, parm;
while (CB_PollEvent(cb, &item, &cmd, &ev, &parm))
    if (ev == CBE_COMMAND && cmd == 1001) OnNew();

CB_Destroy(cb);
CB_Shutdown();''', 'c'))
    add(note('warn', 'Ordinals are a contract',
       '<p>The Clarion import library binds <b>by ordinal</b>, not by name. The numbers in '
       '<code>commandbar.def</code> are never renumbered and never reused; new exports get the next free '
       'number, appended. Renumbering one silently points every existing EXE at the wrong function.</p>'))
    for s in funcsecs:
        add(api_section(s))

    add('<h2 id="eq"><span class="k">Equates</span>Tables</h2>')
    add('''<p>The C names are below; the Clarion equates are the same with a colon &mdash;
<code>CBI_BUTTON</code> is <code>CBI:Button</code>, <code>CBD_TOP</code> is <code>CBD:Top</code>,
<code>CBT_STEELBLUE</code> is <code>CBT:SteelBlue</code>.</p>''')
    for s in constsecs:
        add(const_section(s))

    add('<h2 id="next4"><span class="k">Reference</span>Where to go next</h2>')
    add(nextcards(['programmers-guide.html', 'template-guide.html', 'getting-started.html']))

    nav = [('Clarion class', [('cls', 'CommandBarClass'), ('cls-props', 'Properties')] +
                             [('cls-' + slug(t), t) for t, _ in GROUPS]),
           ('The DLL', [('capi', 'commandbar.dll')] +
                       [('capi-' + slug(s['name']), s['name'][0].upper() + s['name'][1:])
                        for s in funcsecs]),
           ('Equates', [('eq', 'Tables')] +
                       [('eq-' + slug(s['name']), s['name'][0].upper() + s['name'][1:])
                        for s in constsecs])]

    ntypes = len([c for x in API for c in x['consts']
                  if c['name'].startswith('CBI_') and c['name'] != 'CBI_LAST'])
    return page('reference.html',
                'ClaCommandBar Reference',
                'Volume 4', 'Reference',
                'Every property, method, export and equate &mdash; generated from the sources, with a '
                'worked line of Clarion against each one.',
                ['<b>%d</b> class methods' % len(CLASS),
                 '<b>%d</b> properties' % len(PROPLIST),
                 '<b>%d</b> exports' % sum(len(x['funcs']) for x in API),
                 '<b>%d</b> item types' % ntypes],
                nav, ''.join(B), showfilter=True)


# =====================================================================
if __name__ == '__main__':
    total = 0
    for fn, kb in (('getting-started.html',   build_getting_started()),
                   ('programmers-guide.html', build_programmers_guide()),
                   ('template-guide.html',    build_template_guide()),
                   ('reference.html',         build_reference())):
        print('  docs/%-24s %6.1f KB' % (fn, kb / 1024.0))
        total += kb
    print('  %-26s %6.1f KB' % ('four volumes', total / 1024.0))
    if MISSING:
        print('  !! no usage snippet for: ' + ', '.join(sorted(set(MISSING))))
    else:
        print('  every class method has a worked example')
