# Builds docs/reference.html - the ClaCommandBar programmer's reference.
# The API and class tables come straight out of commandbar.h, commandbar.def
# and CommandBar.inc, so the reference cannot drift from the code.
import io, json, re, html

# ---------------------------------------------------------------- extract
# Everything here is read out of the sources, so re-running this script
# after an API change is all it takes to refresh the guide.
import io as _io, re as _re

def _extract_api():
    h = _io.open('src/commandbar.h', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    d = _io.open('src/commandbar.def', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    ordn = {m.group(1): int(m.group(2)) for m in _re.finditer(r'^\s*(CB_\w+)\s+@(\d+)', d, _re.M)}
    strip = lambda s: _re.sub(r'/\*.*?\*/', '', s).rstrip()
    secs, cur, pend, lines, i = [], None, [], h.split('\n'), 0
    while i < len(lines):
        raw = lines[i]
        m = _re.match(r'/\* ---- (.+?) -+ \*/', raw)
        if m:
            cur = {'name': m.group(1).strip(), 'funcs': [], 'consts': []}
            secs.append(cur); pend = []; i += 1; continue
        m = _re.match(r'#define\s+(CB\w+)\s+([^/]+?)\s*(?:/\*(.*?)\*/)?\s*$', raw)
        if m and cur is not None:
            cur['consts'].append({'name': m.group(1), 'value': m.group(2).strip(),
                                  'note': (m.group(3) or '').strip()})
            pend = []; i += 1; continue
        if 'CBAPI' in raw:
            codeline, trail = strip(raw), _re.findall(r'/\*(.*?)\*/', raw)
            while not codeline.endswith(';') and i + 1 < len(lines):
                i += 1
                codeline += ' ' + strip(lines[i]).strip()
                trail += _re.findall(r'/\*(.*?)\*/', lines[i])
            nm = _re.search(r'CB_\w+', codeline)
            if nm and cur is not None:
                cur['funcs'].append({'name': nm.group(0),
                                     'sig': _re.sub(r'\s+', ' ', codeline).strip(),
                                     'ord': ordn.get(nm.group(0)),
                                     'trail': ' '.join(t.strip() for t in trail).strip(),
                                     'doc': [p for p in pend if p]})
            pend = []; i += 1; continue
        c = raw.strip()
        if c.startswith('/*') or c.startswith('*'):
            pend.append(_re.sub(r'^/?\*+', '', c).replace('*/', '').strip())
        elif c == '':
            pend = []
        i += 1
    return secs

def _extract_class():
    inc = _io.open('clarion/CommandBar.inc', encoding='utf-8', newline='').read().replace('\r\n', '\n')
    lines = inc.split('\n')
    a = next(i for i, l in enumerate(lines) if l.startswith('CommandBarClass CLASS'))
    b = next(i for i, l in enumerate(lines) if i > a and l.strip() == 'END')
    out, pend = [], []
    for s in lines[a + 1:b]:
        t = s.strip()
        if t.startswith('!'):
            pend.append(_re.sub(r'^!-*\s?', '', t)); continue
        m = _re.match(r'^(\w+)\s+PROCEDURE\((.*?)\)(.*)$', s)
        if m:
            out.append({'name': m.group(1), 'parms': m.group(2),
                        'attrs': m.group(3).strip(', '),
                        'doc': [p for p in pend if p.strip()]})
            pend = []
        elif t == '':
            pend = []
    return out

API   = _extract_api()
CLASS = _extract_class()

def esc(s): return html.escape(s or '')

# ---------------------------------------------------------------- helpers
def code(txt, lang='clarion'):
    return '<pre class="code" data-lang="%s"><code>%s</code></pre>' % (lang, esc(txt.strip('\n')))

def note(kind, title, body):
    return ('<aside class="note note--%s"><p class="note__t">%s</p><div class="note__b">%s</div></aside>'
            % (kind, esc(title), body))

# ---------------------------------------------------------------- C API
CONST_SECTIONS = [s for s in API if s['consts']]
FUNC_SECTIONS  = [s for s in API if s['funcs']]

CAPI_BLURB = {
 'lifetime': 'Start the engine once, make a manager per window, and take it down again. '
             'Everything else hangs off the <code>HCB</code> handle <code>CB_Create</code> returns.',
 'theming': 'Eleven built-in themes, each derived from a handful of seed colours. Override any of '
            'the 38 slots, the four font roles or the thirteen metrics on top of one.',
 'images': 'An image list per manager. Every item that shows a picture refers to an index in it.',
 'containers: bars and menus': 'A <em>container</em> holds items: a docked bar, a floating bar, or a '
            'popup menu with no bar of its own. Containers are addressed by id.',
 'ribbons': 'A ribbon is a bar with <code>CBBS_RIBBON</code>: it holds tabs, a tab holds groups, and a '
            'group holds ordinary items.',
 'items': 'Everything that can sit in a container, and everything you can do to one afterwards.',
 'edit / combo / colour item values': 'Reading and writing the value of the three items that carry one.',
 'keyboard': 'Give a key an id and it raises that command, wherever the focus is.',
 'events': 'Nothing is dispatched behind your back: clicks queue up and you drain the queue.',
 'odds and ends': 'Small things with nowhere better to live.',
 'reserving space from the host': 'On an APPLICATION frame the host lays its own toolbar and MDI client '
            'out against the full client area. This moves them clear of the bars.',
}

def api_section(sec):
    rows = []
    for f in sec['funcs']:
        doc = ' '.join(f['doc']) if f['doc'] else (f['trail'] or '')
        sig = f['sig'].replace('CBAPI ', '')
        rows.append(
          '<tr class="fn" data-k="%s">'
          '<td class="fn__n"><code>%s</code>%s</td>'
          '<td class="fn__s"><code>%s</code>%s</td></tr>'
          % (esc((f['name'] + ' ' + doc).lower()),
             esc(f['name']),
             '<span class="ord" title="export ordinal - pinned, never reused">@%s</span>' % f['ord'] if f['ord'] else '',
             esc(sig),
             '<p class="fn__d">%s</p>' % esc(doc) if doc else ''))
    return ('<h3 id="capi-%s">%s</h3>%s<div class="tw"><table class="fns"><tbody>%s</tbody></table></div>'
            % (slug(sec['name']), esc(sec['name'].title()),
               '<p class="lead">%s</p>' % CAPI_BLURB.get(sec['name'], ''),
               ''.join(rows)))

def slug(s): return re.sub(r'[^a-z0-9]+', '-', s.lower()).strip('-')

def const_section(sec):
    rows = ''.join('<tr><td><code>%s</code></td><td class="v"><code>%s</code></td><td>%s</td></tr>'
                   % (esc(c['name']), esc(c['value']), esc(c['note']))
                   for c in sec['consts'])
    return ('<h3 id="eq-%s">%s</h3><div class="tw"><table class="eq"><thead><tr>'
            '<th>C</th><th>Value</th><th>Meaning</th></tr></thead><tbody>%s</tbody></table></div>'
            % (slug(sec['name']), esc(sec['name'][0].upper() + sec['name'][1:]), rows))

# ---------------------------------------------------------------- class
GROUPS = [
 ('Lifetime and layout',      ['Construct','Destruct','Init','Kill','Layout','FitControl','ClientX','ClientY',
                               'ClientWidth','ClientHeight','Handle','PlaceOnControl','ReserveSpace','ReserveMode',
                               'HostReserveBottom','HostReserveHeight']),
 ('Bars',                     ['AddBar','AddMenuBar','AddRibbon','SetBarDock','BarDock','SetBarVisible','BarVisible',
                               'SetBarTitle','SetBarStyle','BarStyle','SetBarRect','BarCount','BarAt','DestroyBar']),
 ('Ribbons',                  ['AddRibbonTab','AddRibbonGroup','AddLargeButton','SetActiveTab','ActiveTab',
                               'TabCount','TabAt','MinimizeRibbon','RibbonMinimized']),
 ('Menus and popups',         ['CreateMenu','AddMenuTitle','AddSubMenu','AddMenuRow','SetItemMenu','PopupMenu',
                               'PopupMenuAt','DestroyContainer','ClearContainer']),
 ('Items',                    ['AddButton','AddToggle','AddDropButton','AddSplitButton','AddSeparator','AddLabel',
                               'AddEdit','AddCombo','AddCheckBox','AddColorButton','AddSpace','AddItem',
                               'ItemCount','ItemAt','DestroyItem']),
 ('Item state and looks',     ['SetItemText','ItemText','SetItemImage','SetItemEnabled','ItemEnabled','SetItemChecked',
                               'ItemChecked','SetItemVisible','ItemVisible','SetItemStyle','ItemStyle','SetItemWidth',
                               'SetItemTooltip','SetItemShortcut','SetItemCmd','ItemCmd','SetItemRadio']),
 ('Values',                   ['SetEditText','EditText','SetComboList','SetComboSel','ComboSel','SetColorValue',
                               'ColorValue']),
 ('Images',                   ['AddImage','AddImageStrip','AddImageHandle','ImageCount']),
 ('Theme and appearance',     ['SetTheme','Theme','SetColor','Color','SetFont','SetMetric','Metric','SetSeed',
                               'Refresh','SetStyle','ManagerStyle']),
 ('Keyboard',                 ['AddAccelerator','ClearAccelerators','TakeAlertKey','KeyText']),
 ('Events',                   ['TakeOne','TakeEvent','TakeCommand','TakeToggled','TakeEventProc','Poll']),
 ('Mirroring a Clarion menu', ['MirrorMenu','MirrorMenuFrom','RefreshMirror','FindMenuBar','MenuReport','MinFeqIn',
                               'MenuRank','MirrorInto','SetHostMenu','HostMenuVisible']),
]

def class_tables():
    seen, out = set(), []
    byname = {m['name']: m for m in CLASS}
    for title, names in GROUPS:
        rows = []
        for n in names:
            m = byname.get(n)
            if not m: continue
            seen.add(n)
            doc = ' '.join(m['doc'])
            sig = '%s(%s)%s' % (m['name'], m['parms'], (',' + m['attrs']) if m['attrs'] else '')
            rows.append('<tr class="fn" data-k="%s"><td class="fn__n"><code>%s</code></td>'
                        '<td class="fn__s"><code>%s</code>%s</td></tr>'
                        % (esc((n + ' ' + doc).lower()), esc(n), esc(sig),
                           '<p class="fn__d">%s</p>' % esc(doc) if doc else ''))
        if rows:
            out.append('<h3 id="cls-%s">%s</h3><div class="tw"><table class="fns"><tbody>%s</tbody></table></div>'
                       % (slug(title), esc(title), ''.join(rows)))
    rest = [m for m in CLASS if m['name'] not in seen]
    if rest:
        rows = ''.join('<tr class="fn" data-k="%s"><td class="fn__n"><code>%s</code></td>'
                       '<td class="fn__s"><code>%s(%s)</code>%s</td></tr>'
                       % (esc(m['name'].lower()), esc(m['name']), esc(m['name']), esc(m['parms']),
                          '<p class="fn__d">%s</p>' % esc(' '.join(m['doc'])) if m['doc'] else '')
                       for m in rest)
        out.append('<h3 id="cls-more">Everything else</h3><div class="tw"><table class="fns"><tbody>%s</tbody></table></div>' % rows)
    return ''.join(out)

# ---------------------------------------------------------------- samples
S_MIN = """
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
  IF ~CB.Init(Win, CBS:Tooltips)             ! starts the DLL and makes a manager
    MESSAGE('commandbar.dll did not start.')
    RETURN
  END
  CB.SetTheme(CBT:SteelBlue)

  bar = CB.AddBar('Standard', CBD:Top)       ! a docked bar
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

S_MIRROR = """
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
  CB.MirrorMenu(CB.AddMenuBar(), 1)          ! 1 = take the real menu off the window

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer ; LOOP WHILE CB.TakeOne() . ; CYCLE
    END
    CASE ACCEPTED()
    OF ?MNew                                 ! your ORIGINAL embed code still runs
      Win{PROP:Text} = 'New, from the mirrored row'
    OF ?MExit
      POST(EVENT:CloseWindow)
    END
  END
"""

S_RIBBON = """
rib  = CB.AddRibbon('Ribbon', CBD:Top)
tab  = CB.AddRibbonTab(rib, '&Home')

grp  = CB.AddRibbonGroup(tab, 'Clipboard')
CB.AddLargeButton(grp, CMD:Paste, 'Paste', iPaste)   ! the big one, image above text
CB.AddButton(grp, CMD:Cut,  'Cut',  iCut)            ! small ones stack beside it
CB.AddButton(grp, CMD:Copy, 'Copy', iCopy)

grp  = CB.AddRibbonGroup(tab, 'Font')
it   = CB.AddCombo(grp, CMD:Font, 'Segoe UI|Tahoma|Consolas', 130)
CB.SetComboSel(it, 0)
CB.AddToggle(grp, CMD:Bold, 'B')
CB.AddColorButton(grp, CMD:Colour, '', COLOR:Navy)

CB.MinimizeRibbon(rib, 1)                            ! start collapsed to the tabs
"""

S_POPUP = """
!  build it once
mnu = CB.CreateMenu()
CB.AddMenuRow(mnu, CMD:Insert, '&Insert', iInsert, 'Ins')
CB.AddMenuRow(mnu, CMD:Change, '&Change', iChange, 'Enter')
CB.AddSeparator(mnu)
CB.AddMenuRow(mnu, CMD:Delete, '&Delete', iDelete, 'Del')

!  show it where the mouse is
  OF EVENT:AlertKey
    IF KEYCODE() = MouseRight
      CB.PopupMenu(mnu)
    END
"""

S_HOST = """
!  On an APPLICATION FRAME the frame's own toolbar and MDI client are moved
!  clear of the bars automatically.  A plain WINDOW positions its own
!  controls, so re-fit whatever should fill what the bars left:
  OF EVENT:Sized
    CB.Layout()
    CB.FitControl(?Browse:1, 4, 4)           ! 4px margin

!  A WINDOW with a STATUS bar has no MDI client to measure, so say how tall
!  it is and no bar will cover it:
  CB.HostReserveBottom(23)
"""

S_TOOLBAR = """
!  the frame's own TOOLBAR, rebuilt as a command bar
bar = CommandBar.AddBar('Tools', CBD:Top)
CommandBar.SetBarDock(bar, CBD:Top, 1, 0)         ! its own row, under the menu
n   = CommandBar.MirrorToolbar(bar, 1)            ! 1 = hide the real toolbar
"""

S_LAYOUT = """
!  after the bars are built
CommandBar.RestoreLayoutFrom('.\\MyApp.INI', 'CommandBars')

!  before the window closes
CommandBar.SaveLayoutTo('.\\MyApp.INI', 'CommandBars')
"""

S_VALUES = """
zoom = CommandBar.AddSlider(bar, CMD:Zoom, 25, 400, 100, 130)   ! lo, hi, value, width
page = CommandBar.AddSpin(bar, CMD:Page, 1, 9999, 1, 60)
prog = CommandBar.AddProgress(bar, 0, 100, 0, 130)

!  in the pump - the value arrives in LastParam
  OF CBE:ValueChanged
    CASE CommandBar.LastCmd
    OF CMD:Zoom ; Scale = CommandBar.LastParam
    END

!  drive the progress bar yourself
CommandBar.SetItemNumber(prog, done * 100 / total)
"""

S_GALLERY = """
gal = CommandBar.AddGallery(grp, CMD:Style, 4, 62, 54)   ! columns, cell w, cell h
CommandBar.AddGalleryCell(gal, iNew,  'Normal')
CommandBar.AddGalleryCell(gal, iOpen, 'Heading')
CommandBar.AddGalleryCell(gal, iSave, 'Title')
CommandBar.SetGallerySel(gal, 0)

!  a click raises the item's command with the cell number in LastParam
  OF CMD:Style
    Style = CommandBar.LastParam
"""

S_RIBMIN = """
CommandBar.MinimizeRibbon(rib, 1)              ! collapse it
IF CommandBar.RibbonMinimized(rib)
  !  it is showing its tabs only
END
"""

S_C = """
CB_Initialize();
HCB cb = CB_Create(hwnd, CBS_TOOLTIPS);
CB_SetTheme(cb, CBT_STEELBLUE);

int bar = CB_AddBar(cb, "Standard", CBD_TOP, CBBS_GRIPPER | CBBS_FLOATABLE);
int img = CB_AddImage(cb, "NEW.ICO");
CB_AddButton(cb, bar, 1001, "&New", img);
CB_Layout(cb);

/* in the message loop */
int item, ev; long cmd, parm;
while (CB_PollEvent(cb, &item, &cmd, &ev, &parm))
    if (ev == CBE_COMMAND && cmd == 1001) OnNew();

CB_Destroy(cb);
CB_Shutdown();
"""

# ---------------------------------------------------------------- page
NAV = [
 ('Start', [('overview','Overview'), ('install','Installing'), ('hello','Your first bar'), ('model','How it fits together')]),
 ('Templates', [('tpl','The four templates'), ('tpl-work','Working in AppGen'), ('tpl-presets','Presets'),
                ('tpl-actions','What an item does'), ('tpl-embeds','Embed points')]),
 ('Clarion class', [('cls','CommandBarClass')] + [('cls-' + slug(t), t) for t, _ in GROUPS]),
 ('C API', [('capi','commandbar.dll')] + [('capi-' + slug(s['name']), s['name'].title()) for s in FUNC_SECTIONS]),
 ('Equates', [('eq','Tables')] + [('eq-' + slug(s['name']), s['name'][0].upper() + s['name'][1:]) for s in CONST_SECTIONS]),
 ('Recipes', [('rec','Recipes'), ('trouble','When it misbehaves')]),
]

def nav_html():
    out = []
    for group, items in NAV:
        out.append('<p class="nav__g">%s</p><ul class="nav__l">' % esc(group))
        for aid, label in items:
            out.append('<li><a href="#%s">%s</a></li>' % (aid, esc(label)))
        out.append('</ul>')
    return ''.join(out)

CSS = """
:root{
  --paper:#fbfcfd; --surface:#f1f5f9; --sunken:#e9eff5; --rule:#d6e0ea;
  --ink:#0f1720; --soft:#4e5f70; --faint:#7d8ea0;
  --accent:#2f6a9a; --accent-bg:#e4eef6; --accent-rule:#b9d3e6;
  --clarion:#166f69; --clarion-bg:#e2f1ef;
  --warn:#8a5a12; --warn-bg:#f7eeda;
  --shadow:0 1px 2px rgba(15,23,32,.06),0 8px 24px -18px rgba(15,23,32,.35);
}
@media (prefers-color-scheme:dark){
  :root:not([data-theme="light"]){
    --paper:#0d131a; --surface:#151d26; --sunken:#111922; --rule:#25313d;
    --ink:#e2eaf2; --soft:#9aabbc; --faint:#6f8092;
    --accent:#6fadde; --accent-bg:#152738; --accent-rule:#28455f;
    --clarion:#4fb5ab; --clarion-bg:#102a29;
    --warn:#d8a545; --warn-bg:#2b2413;
    --shadow:0 1px 2px rgba(0,0,0,.5),0 10px 30px -20px rgba(0,0,0,.9);
  }
}
:root[data-theme="dark"]{
  --paper:#0d131a; --surface:#151d26; --sunken:#111922; --rule:#25313d;
  --ink:#e2eaf2; --soft:#9aabbc; --faint:#6f8092;
  --accent:#6fadde; --accent-bg:#152738; --accent-rule:#28455f;
  --clarion:#4fb5ab; --clarion-bg:#102a29;
  --warn:#d8a545; --warn-bg:#2b2413;
  --shadow:0 1px 2px rgba(0,0,0,.5),0 10px 30px -20px rgba(0,0,0,.9);
}
*{box-sizing:border-box}
body{
  margin:0; background:var(--paper); color:var(--ink);
  font-family:"IBM Plex Serif",Georgia,serif; font-size:16px; line-height:1.62;
  -webkit-font-smoothing:antialiased;
}
h1,h2,h3,h4,.ui{font-family:"IBM Plex Sans",system-ui,-apple-system,Segoe UI,sans-serif}
code,pre,.mono{font-family:"IBM Plex Mono",ui-monospace,Consolas,monospace}
a{color:var(--accent)}
.wrap{display:grid; grid-template-columns:264px minmax(0,1fr); gap:0; align-items:start}
/* ---- sidebar ---- */
.side{
  position:sticky; top:0; height:100vh; overflow-y:auto; padding:26px 20px 60px;
  border-right:1px solid var(--rule); background:var(--surface);
}
.brand{font-family:"IBM Plex Sans",sans-serif; font-weight:600; font-size:15px; letter-spacing:.01em;
  display:flex; align-items:baseline; gap:8px; margin:0 0 2px}
.brand b{color:var(--accent)}
.brand span{font-size:11px; color:var(--faint); letter-spacing:.08em; text-transform:uppercase}
.filter{width:100%; margin:16px 0 20px; padding:7px 10px; font:13px/1.4 "IBM Plex Sans",sans-serif;
  color:var(--ink); background:var(--paper); border:1px solid var(--rule); border-radius:6px}
.filter:focus{outline:2px solid var(--accent); outline-offset:1px; border-color:var(--accent)}
.nav__g{font:600 10.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.11em; text-transform:uppercase;
  color:var(--faint); margin:20px 0 8px}
.nav__l{list-style:none; margin:0; padding:0; display:flex; flex-direction:column; gap:1px}
.nav__l a{display:block; padding:4px 8px; border-radius:5px; text-decoration:none; color:var(--soft);
  font:400 13.5px/1.45 "IBM Plex Sans",sans-serif}
.nav__l a:hover{background:var(--sunken); color:var(--ink)}
.nav__l a.on{background:var(--accent-bg); color:var(--accent); font-weight:500}
/* ---- content ---- */
.main{padding:0 0 120px; min-width:0}
.inner{max-width:980px; padding:0 40px}
.hero{padding:56px 40px 34px; border-bottom:1px solid var(--rule); background:
  linear-gradient(180deg,var(--accent-bg),transparent)}
.hero .inner{padding:0}
h1{font-size:38px; line-height:1.1; margin:0 0 10px; letter-spacing:-.015em; text-wrap:balance}
.sub{font-size:17px; color:var(--soft); margin:0 0 20px; max-width:62ch}
.chips{display:flex; flex-wrap:wrap; gap:8px}
.chip{font:500 11.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.04em; padding:6px 10px; border-radius:99px;
  border:1px solid var(--rule); background:var(--paper); color:var(--soft)}
.chip b{color:var(--ink); font-weight:600}
h2{font-size:26px; margin:64px 0 6px; letter-spacing:-.01em; scroll-margin-top:18px; text-wrap:balance}
h2 .k{font:600 10.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.12em; text-transform:uppercase;
  color:var(--accent); display:block; margin-bottom:9px}
h3{font-size:17.5px; margin:38px 0 10px; scroll-margin-top:18px; color:var(--ink)}
h4{font-size:14.5px; margin:26px 0 6px; color:var(--soft); font-weight:600}
p{margin:0 0 14px; max-width:70ch}
.lead{color:var(--soft); max-width:70ch}
ul.b,ol.b{max-width:70ch; padding-left:20px; margin:0 0 16px}
ul.b li,ol.b li{margin:0 0 7px}
hr{border:0; border-top:1px solid var(--rule); margin:44px 0}
/* ---- code ---- */
.code{background:var(--sunken); border:1px solid var(--rule); border-left:3px solid var(--accent-rule);
  border-radius:0 7px 7px 0; padding:14px 16px; overflow-x:auto; margin:0 0 18px;
  font-size:12.9px; line-height:1.62; position:relative}
.code code{white-space:pre; color:var(--ink)}
.code::after{content:attr(data-lang); position:absolute; top:0; right:0; padding:3px 9px;
  font:500 9.5px/1 "IBM Plex Sans",sans-serif; letter-spacing:.1em; text-transform:uppercase;
  color:var(--faint); background:var(--surface); border-left:1px solid var(--rule);
  border-bottom:1px solid var(--rule); border-radius:0 0 0 6px}
p code,li code,td code{background:var(--sunken); border:1px solid var(--rule); border-radius:4px;
  padding:.06em .34em; font-size:.86em}
/* ---- tables ---- */
.tw{overflow-x:auto; margin:0 0 20px; border:1px solid var(--rule); border-radius:8px; background:var(--paper)}
table{border-collapse:collapse; width:100%; font-size:13.5px;
  font-family:"IBM Plex Sans",sans-serif}
th{text-align:left; font-weight:600; font-size:11px; letter-spacing:.08em; text-transform:uppercase;
  color:var(--faint); padding:9px 14px; border-bottom:1px solid var(--rule); background:var(--surface)}
td{padding:10px 14px; border-bottom:1px solid var(--rule); vertical-align:top}
tr:last-child td{border-bottom:0}
.fns .fn__n{width:210px; white-space:nowrap}
.fns .fn__n code{font-size:12.6px; color:var(--accent); font-weight:500; background:none; border:0; padding:0}
.fns .fn__s code{font-size:12.4px; color:var(--soft); background:none; border:0; padding:0; white-space:pre-wrap}
.fn__d{margin:5px 0 0; font-family:"IBM Plex Serif",serif; font-size:13.5px; color:var(--ink); max-width:74ch}
.ord{margin-left:7px; font:500 10px/1 "IBM Plex Mono",monospace; color:var(--faint);
  border:1px solid var(--rule); border-radius:4px; padding:2px 4px; vertical-align:1px}
.eq td:first-child code{color:var(--accent); background:none; border:0; padding:0}
.eq .v{font-variant-numeric:tabular-nums; color:var(--soft)}
.two{display:grid; grid-template-columns:1fr 1fr; gap:0 26px}
@media (max-width:1080px){.two{grid-template-columns:1fr}}
/* ---- notes ---- */
.note{border:1px solid var(--rule); border-left:3px solid var(--accent); background:var(--surface);
  border-radius:0 7px 7px 0; padding:13px 16px; margin:0 0 18px; max-width:74ch}
.note--warn{border-left-color:var(--warn); background:var(--warn-bg)}
.note--cla{border-left-color:var(--clarion); background:var(--clarion-bg)}
.note__t{font:600 12px/1.3 "IBM Plex Sans",sans-serif; letter-spacing:.03em; margin:0 0 5px;
  text-transform:uppercase; color:var(--soft)}
.note--warn .note__t{color:var(--warn)} .note--cla .note__t{color:var(--clarion)}
.note__b p{margin:0 0 8px; font-size:14.5px} .note__b p:last-child{margin:0}
/* ---- layer diagram ---- */
.stack{display:flex; flex-direction:column; gap:9px; margin:0 0 22px; max-width:640px}
.layer{border:1px solid var(--rule); border-radius:8px; padding:12px 15px; background:var(--surface);
  display:grid; grid-template-columns:118px 1fr; gap:14px; align-items:baseline}
.layer b{font:600 11px/1.4 "IBM Plex Sans",sans-serif; letter-spacing:.06em; text-transform:uppercase;
  color:var(--accent)}
.layer.cla b{color:var(--clarion)}
.layer p{margin:0; font-size:14px; color:var(--soft); max-width:none}
.arrow{text-align:center; color:var(--faint); font-size:12px; margin:-4px 0}
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
const links = [...document.querySelectorAll('.nav__l a')];
const spy = new IntersectionObserver(es => {
  es.forEach(e => {
    if (!e.isIntersecting) return;
    links.forEach(a => a.classList.toggle('on', a.getAttribute('href') === '#' + e.target.id));
  });
}, {rootMargin:'0px 0px -78% 0px'});
document.querySelectorAll('h2[id],h3[id]').forEach(h => spy.observe(h));
"""

# ---------------------------------------------------------------- prose
BODY = []
def add(s): BODY.append(s)

add('''
<h2 id="overview"><span class="k">Start</span>Overview</h2>
<p>ClaCommandBar puts Codejock-style command bars, ribbons and popup menus on a
Clarion window. It is three layers, and you can stop at whichever one suits you.</p>
<div class="stack">
  <div class="layer cla"><b>Templates</b><p>Fill in prompts in AppGen; the wiring is generated for you. Most people never go below this.</p></div>
  <div class="arrow">&#8595;&nbsp; generates calls to</div>
  <div class="layer cla"><b>CommandBarClass</b><p>An ABC-style Clarion class. Hand-written code talks to this: <code>CB.AddBar</code>, <code>CB.AddButton</code>, <code>CB.MirrorMenu</code>.</p></div>
  <div class="arrow">&#8595;&nbsp; calls, by pinned ordinal</div>
  <div class="layer"><b>commandbar.dll</b><p>A flat <code>__stdcall</code> C API over a Direct2D / DirectWrite engine. 32-bit, static CRT, no VC++ redistributable.</p></div>
</div>
<p>The engine draws everything itself, so a bar looks the same on Windows 7 and
Windows 11 and does not inherit whatever the OS thinks a toolbar should be. It
knows nothing about Clarion: the Clarion-shaped parts &mdash; mirroring a
<code>MENUBAR</code>, moving a frame's toolbar out of the way &mdash; live in the class
and the templates.</p>
''')

add('<h2 id="install"><span class="k">Start</span>Installing</h2>')
add('''<p>Run <code>clarion\\install.bat</code>. It copies five files into your Clarion
installation and registers the template:</p>''')
add(code('''ClaCommandBar.tpl   ->  accessory\\template\\win
CommandBar.inc      ->  accessory\\libsrc\\win
CommandBar.clw      ->  accessory\\libsrc\\win
commandbar.lib      ->  accessory\\lib  AND  accessory\\libsrc\\win
commandbar.dll      ->  accessory\\bin''', 'files'))
add(note('warn', 'Two copies of the import library, on purpose',
   '<p>The linker takes the first <code>commandbar.lib</code> it finds on the redirection path, and '
   '<code>libsrc\\win</code> comes before <code>lib</code>. Refreshing only one leaves the other shadowing it, '
   'and new exports come back as <code>Unresolved External</code> however often you rebuild. If you see that, '
   'look for a third copy in your own application folder.</p>'))
add('''<p>Ship <code>commandbar.dll</code> beside your EXE. Close and reopen the IDE after
installing &mdash; it caches a parsed copy of the template.</p>''')

add('<h2 id="hello"><span class="k">Start</span>Your first bar</h2>')
add('<p>By hand, without the templates, this is the whole thing:</p>')
add(code(S_MIN))
add(note('cla', 'Three things that are not optional',
   '<p><code>PRAGMA(\'link(commandbar.lib)\')</code> binds the imports. A <code>TIMER</code> on the window '
   'drives the pump &mdash; without one, nothing you click is ever delivered. And <code>EVENT:Sized</code> '
   'has to reach <code>CB.Layout()</code>, or the bars keep the size they had when the window opened.</p>'))

add('<h2 id="model"><span class="k">Start</span>How it fits together</h2>')
add('''
<h3>Managers, containers, items</h3>
<p>One <b>manager</b> per window &mdash; <code>CB.Init</code> makes it, <code>CB.Kill</code> takes it down.
A <b>container</b> is anything that holds items: a docked bar, a floating bar, a popup menu, or a ribbon
group. An <b>item</b> is a button, a toggle, a separator, an edit box. Both come back as integer ids,
and you keep the ones you will need again.</p>
<h3>Command ids are yours</h3>
<p>Every item carries a <b>command id</b> you choose. Clicking it queues an event carrying that id; an
id of <code>0</code> means the item raises nothing, which is what separators and labels use. Two items
may share an id &mdash; a toolbar button and a menu row for the same command usually should.</p>
<h3>Nothing is dispatched behind your back</h3>
<p>The engine never calls into your code. It queues events, and you drain the queue from the window's
<code>EVENT:Timer</code>:</p>
''')
add(code('''  OF EVENT:Timer
    LOOP WHILE CB.TakeOne()          ! returns 0 when the queue is empty
      CASE CB.LastCmd
      OF CMD:Save ; DO SaveRoutine
      END
    END
    CYCLE'''))
add('''<p><code>TakeOne</code> fills <code>LastItem</code>, <code>LastCmd</code>, <code>LastEvent</code> and
<code>LastParam</code>, then calls the virtual <code>TakeCommand</code> / <code>TakeToggled</code> methods,
which is where a derived class hooks in. The template generates that loop for you.</p>
<h3>Rows and offsets</h3>
<p>Bars dock to an edge on a <b>row</b>: row 0 is nearest the edge, row 1 sits under it. Two bars on the
same row sit side by side in <b>offset</b> order. A ribbon always takes a row to itself &mdash; it is a
full-width band, so sharing one would leave nothing for its neighbour.</p>''')

# ---------------------------------------------------------------- templates
add('<h2 id="tpl"><span class="k">Templates</span>The four templates</h2>')
add('''<div class="tw"><table><thead><tr><th>Template</th><th>Kind</th><th>Put it on</th><th>What it adds</th></tr></thead><tbody>
<tr><td><code>CommandBarGlobal</code></td><td>Extension</td><td>the application, once</td><td>defaults every window inherits &mdash; theme, colours, icon sizes</td></tr>
<tr><td><code>CommandBarOnWindow</code></td><td>Extension</td><td>any procedure with a window</td><td>bars, ribbons, menus and items on that window</td></tr>
<tr><td><code>CommandBarFrame</code></td><td>Extension</td><td>an <code>APPLICATION</code> frame</td><td>everything above, plus mirroring or replacing the frame's <code>MENUBAR</code></td></tr>
<tr><td><code>CommandBarControl</code></td><td>Control</td><td>a window, from the control toolbox</td><td>drops a <code>REGION</code> and lands a bar exactly on it</td></tr>
</tbody></table></div>
<p>The extensions and the control share the same tabs, so anything below applies wherever you find it.
Only the frame template has the <b>Menu</b> tab, and only the control template can land a bar on its own
region.</p>''')

add('<h2 id="tpl-work"><span class="k">Templates</span>Working in AppGen</h2>')
add('''<p>Add the extension, then fill in the tabs. They are in the order you will use them:</p>
<div class="tw"><table><thead><tr><th>Tab</th><th>What lives there</th></tr></thead><tbody>
<tr><td><b>General</b></td><td>whether to build at all, the timer interval, which control fills the space the bars leave</td></tr>
<tr><td><b>Menu</b></td><td><i>frame only</i> &mdash; mirror the <code>MENUBAR</code>, and whether to take the real menu off the frame</td></tr>
<tr><td><b>Appearance</b></td><td>theme, small and large icon sizes, item height, corner radius, menu width</td></tr>
<tr><td><b>Colours</b></td><td>override any of the 38 slots on top of the theme</td></tr>
<tr><td><b>Images</b></td><td>name the icon files once; items refer to the names</td></tr>
<tr><td><b>Bars</b></td><td>one entry per bar &mdash; name, dock, row, offset, gripper, floatable, ribbon</td></tr>
<tr><td><b>Ribbon</b></td><td>tabs, and the groups inside them</td></tr>
<tr><td><b>Menus</b></td><td>popup menus, by name</td></tr>
<tr><td><b>Items</b></td><td>everything that goes in a bar, a menu or a ribbon group</td></tr>
<tr><td><b>Keys</b></td><td>a Clarion key equate to a command id</td></tr>
</tbody></table></div>
<p>Two more worth knowing: <b>General</b> carries <i>Remember where the user puts the bars</i>, and
on a frame the <b>Menu and toolbar</b> tab mirrors the frame's <code>TOOLBAR</code> as well as its
<code>MENUBAR</code>.</p>
<p>Names tie it together. A bar is named on the Bars tab; an item names that bar in <b>Put it in</b>.
The same goes for menus and ribbon groups. Each name becomes a variable in the generated source
&mdash; <code>CBBar:1:Standard</code>, <code>CBMnu:1:RowMenu</code> &mdash; which is what you pass when you
call the class yourself.</p>''')
add(note('cla', 'Icons: name them plainly, add them to the project',
   '<p>Write <code>NEW.ICO</code>, not a path. An image added to the application\'s project is linked into '
   'the EXE, and Clarion names the resource after the file &mdash; <code>NEW.ICO</code> becomes '
   '<code>NEW_ICO</code> &mdash; which is where the manager looks first. A loose copy beside the EXE, or in '
   'an <code>images</code> folder next to it, is found too.</p>'))

add('<h2 id="tpl-presets"><span class="k">Templates</span>Presets</h2>')
add('''<p>Three buttons fill the lists in for you. What they make is ordinary entries afterwards
&mdash; rename them, reorder them, delete the ones you do not want.</p>
<h4>Bars tab &mdash; five classic toolbars</h4>
<div class="tw"><table><thead><tr><th>Preset</th><th>What is on it</th></tr></thead><tbody>
<tr><td><b>Standard</b></td><td>New, Open, Save &#124; Print &#124; Cut, Copy, Paste &#124; Undo, Redo &#124; a search box, Find &#124; Help at the far end</td></tr>
<tr><td><b>Formatting</b></td><td>font and size combos &#124; B, I, U and a text-colour button &#124; left / centre / right &#124; bulleted and numbered lists</td></tr>
<tr><td><b>Browse and records</b></td><td>the VCR keys &#124; Insert, Change, Delete &#124; a locator box, Locate, Sort, Mark &#124; Refresh &#124; Print</td></tr>
<tr><td><b>Navigation</b></td><td>Back, Forward, Stop, Refresh, Home &#124; an address box that stretches &#124; Go, Search</td></tr>
<tr><td><b>Print and export</b></td><td>Print, Preview &#124; an Export drop button carrying a menu of PDF, Excel, CSV, HTML, XML, text</td></tr>
</tbody></table></div>
<h4>Ribbon tab</h4>
<p>Builds the ribbon from <code>CommandBarShowcase</code>, plus the two things only a ribbon can do:
<b>Home</b> (Clipboard, Font, a <b>Styles gallery</b>, Editing), <b>Insert</b> (Pages, Illustrations,
Links) and <b>View</b> (Show, and Zoom with a <b>slider</b>) &mdash; a big button leading each group and
real controls, combos, toggles and a colour button, among them.</p>
<h4>Menus tab</h4>
<p>One of <b>File</b>, <b>Edit</b>, <b>Browse row</b>, <b>View</b> or <b>Help</b>, chosen in the drop
beside the button.</p>
<p>Each press starts a new bar on the first row of that edge nobody is using, and command ids carry on
from the highest already in use, so nothing a preset makes collides with what you added yourself.</p>''')

add('<h2 id="tpl-actions"><span class="k">Templates</span>What an item does</h2>')
add('''<p>Every item has an <b>Action</b>, run just before its embed point:</p>
<div class="tw"><table><thead><tr><th>Action</th><th>Generates</th></tr></thead><tbody>
<tr><td>Embed code only</td><td>nothing &mdash; you write it</td></tr>
<tr><td>Call a procedure</td><td><code>ThatProcedure(parms)</code></td></tr>
<tr><td>Do a routine</td><td><code>DO ThatRoutine</code></td></tr>
<tr><td>Emulate a control</td><td><code>POST(EVENT:Accepted, ?ThatControl)</code></td></tr>
<tr><td>Post an event</td><td><code>POST(EVENT:Whatever, ?Control)</code></td></tr>
<tr><td>Close the window</td><td><code>POST(EVENT:CloseWindow)</code></td></tr>
</tbody></table></div>
<p><b>Emulate a control</b> is the one that saves the most work: point a bar button at a
<code>BUTTON</code> or a menu <code>ITEM</code> that is already on the window and its existing embed code
runs, untouched.</p>''')

add('<h2 id="tpl-embeds"><span class="k">Templates</span>Embed points</h2>')
add('''<p>Thirty embeds, the useful ones being:</p>
<ul class="b">
<li><b>After the bars are built</b> &mdash; the place to add anything the prompts cannot describe.</li>
<li><b>One per command id</b> &mdash; each distinct id gets its own embed, so you are not writing a
<code>CASE</code> by hand.</li>
<li><b>Before the manager is destroyed</b> &mdash; for anything you allocated yourself.</li>
</ul>
<p>The generated object is called <code>CommandBar</code> by default, so inside an embed you have the
whole class: <code>CommandBar.SetItemEnabled(CBItm:1:7, 0)</code>.</p>''')

# ---------------------------------------------------------------- class
add('<h2 id="cls"><span class="k">Clarion class</span>CommandBarClass</h2>')
add('''<p>Declared in <code>CommandBar.inc</code>, implemented in <code>CommandBar.clw</code>. Every method
is <code>VIRTUAL</code>, so deriving works the ABC way. The tables below come straight out of the header,
so they cannot drift from the code.</p>''')
add(code('''CB   CommandBarClass                     ! that is the whole declaration'''))
add(class_tables())

# ---------------------------------------------------------------- C API
add('<h2 id="capi"><span class="k">C API</span>commandbar.dll</h2>')
add('''<p>Plain <code>__stdcall</code>, no C++ in the signatures. Usable from anything that can
call a DLL &mdash; the Clarion class is only the first caller.</p>''')
add(code(S_C, 'c'))
add(note('warn', 'Ordinals are a contract',
   '<p>The Clarion import library binds <b>by ordinal</b>, not by name. The numbers in '
   '<code>commandbar.def</code> are never renumbered and never reused; new exports get the next free '
   'number, appended. Renumbering one silently points every existing EXE at the wrong function.</p>'))
for s in FUNC_SECTIONS:
    add(api_section(s))

# ---------------------------------------------------------------- equates
add('<h2 id="eq"><span class="k">Equates</span>Tables</h2>')
add('''<p>The C names are below; the Clarion equates are the same with a colon &mdash;
<code>CBI_BUTTON</code> is <code>CBI:Button</code>, <code>CBD_TOP</code> is <code>CBD:Top</code>,
<code>CBT_STEELBLUE</code> is <code>CBT:SteelBlue</code>.</p>''')
for s in CONST_SECTIONS:
    add(const_section(s))

# ---------------------------------------------------------------- recipes
add('<h2 id="rec"><span class="k">Recipes</span>Recipes</h2>')
add('<h3>Take over the window\'s own menu</h3>')
add('''<p><code>MirrorMenu</code> reads the <code>MENUBAR</code> at run time and rebuilds it as a command
bar &mdash; same order, same nesting, separators kept, <code>KEY()</code> attributes turned into a shortcut
column, disabled items still disabled. Choosing a mirrored row POSTs <code>EVENT:Accepted</code> to the
<b>original</b> <code>ITEM</code>.</p>''')
add(code(S_MIRROR))
add(note('warn', 'What mirroring cannot see',
   '<p>On an MDI frame it mirrors the <b>frame\'s own</b> menu. A menu an MDI child merges in is not '
   'reachable: the child\'s controls live on the child\'s thread and never enter the frame\'s control '
   'list, and although the frame\'s <code>HMENU</code> really does grow, Clarion owner-draws its menus so '
   'the items come back with no text at all. Use <code>NOMERGE</code> on the children, or leave the real '
   'menu attached.</p><p>Put <code>NOMERGE</code> on a <b>child window</b>, never on the frame\'s own '
   '<code>TOOLBAR</code> &mdash; there it means "do not carry this toolbar into the merge", and the '
   'toolbar disappears the moment any MDI procedure opens.</p>'))
add('<h3>A ribbon</h3>')
add(code(S_RIBBON))
add('''<p>Collapsing is built in: a small chevron sits at the end of the tab strip, double-clicking a tab
does the same thing, and clicking a tab while collapsed opens it there.</p>''')
add('<h3>A context menu</h3>')
add(code(S_POPUP))
add('<h3>Leaving room for the bars</h3>')
add(code(S_HOST))
add('<h3>Recolouring one thing</h3>')
add(code('''CB.SetTheme(CBT:Office2016)                  ! start from a theme
CB.SetColor(CBC:Accent, COLOR:Navy)          ! then override slots
CB.SetColor(CBC:BarBack, 00F5F5F5h)
CB.SetMetric(CBM:IconSize, 20)
CB.SetMetric(CBM:LargeIcon, 32)
CB.Refresh()'''))

add('<h3>Take over the frame\'s toolbar too</h3>')
add('''<p>The same trick on the row of buttons. Every <code>BUTTON</code>, <code>CHECK</code>,
<code>ENTRY</code>, <code>COMBO</code> and <code>PROMPT</code> in the <code>TOOLBAR</code> becomes a bar
item carrying its <code>ICON()</code>, its <code>TIP()</code> and its disabled state, and choosing one
POSTs <code>EVENT:Accepted</code> to the <b>original</b> control.</p>''')
add(code(S_TOOLBAR))
add(note('cla', 'Why this matters on an MDI frame',
   '<p>Opening a child window makes Clarion hide the frame\'s toolbar, build a <b>second</b> '
   '<code>ClaToolBar</code> for the merged one and swap them. Mirror the toolbar, hide the real one, and '
   'there is nothing left on screen for that to disturb.</p>'))

add('<h3>Remember where the user put them</h3>')
add('''<p>One INI entry holds the lot &mdash; which edge each bar is on, which row, the order within it,
whether it is showing, where a floating one sits, and whether a ribbon is collapsed.</p>''')
add(code(S_LAYOUT))
add('''<p>Bars are matched by <b>name</b>, so adding or removing one in a later release never hands an old
position to the wrong bar; a name it does not recognise is ignored. <code>LayoutText()</code> and
<code>RestoreLayout()</code> hand you the blob directly if you would rather keep it in a user record or a
settings table.</p>''')

add('<h3>A slider, a spin box, a progress bar</h3>')
add('''<p>One idea wearing three faces: a value between two bounds. The first two raise
<code>CBE:ValueChanged</code> as the user moves them; a progress bar is yours to drive.</p>''')
add(code(S_VALUES))
add(note('warn', 'A drag fires on every step',
   '<p>Keep that handler cheap &mdash; store the value and do the heavy work afterwards, or a slider will '
   'repaint your report once per pixel.</p>'))

add('<h3>A gallery</h3>')
add('''<p>A grid of picture choices &mdash; what a ribbon group wants when a row of buttons will not do.</p>''')
add(code(S_GALLERY))
add('''<p>A ribbon group is a fixed height, so a gallery too tall for it is <b>shrunk to fit</b> rather
than dropped. Set the cell height to what you actually want, then look at it.</p>''')

add('<h3>Collapse the ribbon</h3>')
add('''<p>A small chevron sits at the end of the tab strip &mdash; up while the ribbon is open, down once
it is collapsed. Double-clicking a tab does the same thing, and clicking a tab while collapsed opens it
there.</p>''')
add(code(S_RIBMIN))

add('<h2 id="trouble"><span class="k">Recipes</span>When it misbehaves</h2>')
add('''<div class="tw"><table><thead><tr><th>What you see</th><th>What it is</th></tr></thead><tbody>
<tr><td>No bars at all</td><td>the DLL is not beside the EXE, or <code>Init</code> returned 0 &mdash; check its result</td></tr>
<tr><td>Bars appear, clicks do nothing</td><td>nothing is pumping. The window needs a <code>TIMER</code> and the timer needs to reach <code>TakeOne</code></td></tr>
<tr><td>Bars do not resize with the window</td><td><code>EVENT:Sized</code> is not reaching <code>Layout()</code></td></tr>
<tr><td>A mirrored bar comes out empty</td><td>call <code>MenuReport()</code> &mdash; it names the menubar equate it found and lists what hung off it</td></tr>
<tr><td>Icons missing</td><td>the images are not in the project and not beside the EXE. Names are matched as <code>NEW.ICO</code> &rarr; <code>NEW_ICO</code></td></tr>
<tr><td>The frame's toolbar vanishes when a procedure opens</td><td><code>NOMERGE</code> on the frame's <code>TOOLBAR</code>. Clear it</td></tr>
<tr><td><code>Unresolved External CB_...</code></td><td>a second <code>commandbar.lib</code> is shadowing the installed one</td></tr>
<tr><td>AppGen hangs on a template button</td><td>a group returned text where a number was expected &mdash; <code>&#39;0&#39;</code> is true to <code>#IF</code></td></tr>
<tr><td>A slider, spin, progress or gallery draws as a plain button with its text on it</td><td>an old <code>commandbar.dll</code>. <code>CB_AddItem</code> used to reject any type above <code>CBI_SPACE</code> and hand back a button instead &mdash; generated code fine, picture wrong. The DLL alone fixes it</td></tr>
<tr><td>A slider moves but nothing happens in a generated app</td><td>older generated code: the pump only handled <code>CBE:Command</code>. It takes <code>CBE:ValueChanged</code> too now, so regenerate</td></tr>
<tr><td>AppGen hangs on a template button</td><td>a group returned text where a number was expected &mdash; <code>&#39;0&#39;</code> is true to <code>#IF</code> and to <code>#LOOP,WHILE</code></td></tr>
<tr><td>A frame child sits in the wrong place</td><td>set <code>CB_HOSTLOG=1</code> and run again; every host-child move is traced to <code>%TEMP%\\cbhost.log</code></td></tr>
</tbody></table></div>''')

# ---------------------------------------------------------------- assemble
PAGE = '''<title>ClaCommandBar Reference</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=IBM+Plex+Sans:wght@400;500;600&family=IBM+Plex+Serif:wght@400;600&display=swap">
<style>__CSS__</style>
<div class="wrap">
<nav class="side">
  <p class="brand"><b>ClaCommandBar</b><span>Reference</span></p>
  <label class="ui" style="font-size:11px;color:var(--faint);letter-spacing:.08em;text-transform:uppercase" for="filter">Filter the API</label>
  <input id="filter" class="filter" type="search" placeholder="AddButton, theme, ribbon&hellip;" autocomplete="off">
  __NAV__
</nav>
<main class="main">
  <header class="hero"><div class="inner">
    <h1>ClaCommandBar</h1>
    <p class="sub">Command bars, ribbons and popup menus for Clarion &mdash; a Direct2D engine, an ABC-style
    class, and the templates that wire them up. This is the programmer&rsquo;s reference for all three.</p>
    <div class="chips">
      <span class="chip"><b>__NEXPORT__</b> exports</span>
      <span class="chip"><b>__NMETH__</b> class methods</span>
      <span class="chip"><b>4</b> templates</span>
      <span class="chip"><b>11</b> themes</span>
      <span class="chip"><b>__NTYPES__</b> item types</span>
    </div>
  </div></header>
  <div class="inner">__BODY__
    <footer>Generated from <code>commandbar.h</code>, <code>commandbar.def</code> and
    <code>CommandBar.inc</code> &mdash; the signatures and ordinals here are the ones in the build.</footer>
  </div>
</main>
</div>
<script>__JS__</script>
'''

out = (PAGE.replace('__CSS__', CSS)
           .replace('__NAV__', nav_html())
           .replace('__BODY__', ''.join(BODY))
           .replace('__NMETH__', str(len(CLASS)))
           .replace('__NEXPORT__', str(sum(len(x['funcs']) for x in API)))
           .replace('__NTYPES__', str(len([c for x in API for c in x['consts']
                                           if c['name'].startswith('CBI_')
                                           and c['name'] != 'CBI_LAST'])))
           .replace('__JS__', JS))
io.open('docs/reference.html', 'w', encoding='utf-8', newline='\n').write(out)
print('docs/reference.html  %.1f KB' % (len(out) / 1024.0))
