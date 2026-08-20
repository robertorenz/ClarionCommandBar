# Installing ClaCommandBar

Everything you need is committed pre-built. You do **not** need Visual Studio
unless you change `src\`.

## Install it into Clarion — five copies

Put everything where Clarion's own redirection already looks and you never have
to think about paths again:

| Copy this | To here |
|---|---|
| `clarion\ClaCommandBar.tpl` | `C:\clarion12\accessory\template\win` |
| `clarion\CommandBar.inc` | `C:\clarion12\accessory\libsrc\win` |
| `clarion\CommandBar.clw` | `C:\clarion12\accessory\libsrc\win` |
| `clarion\commandbar.lib` | `C:\clarion12\accessory\lib` |
| `bin\commandbar.dll` | `C:\clarion12\accessory\bin` |

then register it once:

```
ClarionCL -tr C:\clarion12\accessory\template\win\ClaCommandBar.tpl
```

Those five folders are on the search paths in `clarion12\bin\CLARION120.RED`
(`*.* = %ROOT%\Accessory\libsrc\win; … %ROOT%\Accessory\template\win`,
`*.lib = %ROOT%\Accessory\lib`, `*.dll = %ROOT%\Accessory\bin`), so any app
on the machine finds the class, links the lib, and gets `commandbar.dll`
**copied into its output folder automatically**. Nothing to stage per project.

> One thing to remember: if you ever rebuild the engine from `src\`, copy the
> new `commandbar.dll` to `accessory\bin` as well. The import library binds by
> ordinal, so an app built against a newer lib and loading the older DLL from
> `accessory\bin` fails with `Entry Point Not Found`. See §7.

The rest of this section explains what each piece is and what happens if you
put it somewhere else.

| File | Goes where | What it is |
|---|---|---|
| `bin\commandbar.dll` | `accessory\bin`, or beside every EXE | the engine (32-bit) |
| `clarion\commandbar.lib` | `accessory\lib`, or anywhere the linker looks | the Clarion import library |
| `clarion\CommandBar.inc` | `accessory\libsrc\win` | the class header |
| `clarion\CommandBar.clw` | `accessory\libsrc\win` | the class body |
| `clarion\ClaCommandBar.tpl` | `accessory\template\win` | the template chain |

---

## 1. The import library — nothing to do

`clarion\commandbar.lib` is generated from `src\commandbar.def` by
`src\make-clarion-lib.ps1`, in the TopSpeed OMF format Clarion's own LibMaker
produces. It imports **by ordinal**, and the ordinals in `commandbar.def` are a
pinned contract — they are never renumbered or reused, so a lib built today
keeps working against a DLL built later.

The format predates Clarion 9 and has not changed since, so the same
`commandbar.lib` works in Clarion 9, 10, 11 and 12.

> **The one failure this causes:** a DLL *older* than the lib is missing the
> newest ordinals, and you get
> `Entry Point Not Found: the procedure entry point CB_GetCursorPos could not
> be located`. It is always a stale `commandbar.dll` — see §7.

## 2. Ship `commandbar.dll` beside the EXE

It statically links the CRT, so the only things it needs are stock Windows
DLLs: `user32`, `gdi32`, `ole32`, `comdlg32`, `d2d1`, `dwrite`, `windowscodecs`.
No VC++ redistributable, no .NET. Windows 7 SP1 through Windows 11.

## 3. Put the class where redirection can find it

Copy `CommandBar.inc` and `CommandBar.clw` to either

* `clarion12\accessory\libsrc\win` — available to every app on the machine,
  and what the install block at the top of this file does, or
* the application's own folder, if you want one app pinned to its own copy.

Both files are ANSI with **CRLF** line endings. Clarion's parser needs CRLF —
a file saved with bare LF is read as one enormous line and fails with
`Expected: <directive> <LINEBREAK> PROGRAM PRAGMA` at line 1.

Do **not** add `CommandBar.clw` to the project's file list. The class's own
`LINK('CommandBar.clw', …)` attribute pulls it in; listing it as well gives
`Unresolved External _main in iexe32.obj`.

## 4. Register the template

```
ClarionCL -tr C:\full\path\to\ClaCommandBar.tpl
```

The registry records the template's **source file name**, and re-reads that
file every time an app is opened. So the name has to stay resolvable:

| What you do | What happens |
|---|---|
| the `.tpl` is in `accessory\template\win` (the recommended install) | the bare name always resolves through redirection — safest |
| the `.tpl` is elsewhere and you register it with a **full path** | fine, and what you want while developing the template itself |
| the `.tpl` is elsewhere and you `cd` to it and register the **bare name** | the bare name is stored, nothing can resolve it later, and **every** app in the IDE then fails to open with `Could not open include file ClaCommandBar.tpl`. Re-registering does not undo it — restore `clarion12\template\win\TemplateRegistry12.trf` from a backup |

Back the registry file up before you register anything, always. It is the only
clean way out of that last row.

The chain is **one file on purpose**. It started as a `.tpl` plus a `.tpw` of
shared `#GROUP`s, the way ClaPropGrid ships — but a template `#INCLUDE` is
resolved against the *current directory*, not against the folder the `.tpl`
lives in, so a two-file chain can only be registered one of the two broken ways
above. Inlining the groups at the end of the `.tpl` removes the choice.

Close the IDE before registering and reopen it afterwards. The registry holds a
**parsed** copy of the chain and an IDE that was already open keeps serving the
old one — which is why every template in the chain shows a version stamp on its
General tab. If the stamp in the prompt sheet is not the one at the top of
`ClaCommandBar.tpl`, you are looking at a stale copy.

## 5. Using the templates

Four templates ship in the one `.tpl`:

| Template | Kind | For |
|---|---|---|
| `CommandBarGlobal` | APPLICATION | add **once** per app |
| `CommandBarOnWindow` | PROCEDURE extension | any window |
| `CommandBarControl` | CONTROL, MULTI | dropped from the control palette; can land a bar on the REGION it places |
| `CommandBarFrame` | PROCEDURE extension | an application FRAME; can mirror or replace its `MENUBAR` |

### 5a. `CommandBarGlobal` — application extension, add once per app

*Application → Extensions → Insert → ClaCommandBar → CommandBarGlobal.*

It places `CommandBarClass` through the ABC class category `COMMANDBAR` — which
is what generates the `_CommandBarLinkMode_` / `_CommandBarDllMode_` project
defines — and adds `commandbar.lib` to the project.

In a single-EXE app the procedure extension works without it (with neither
define present the class defaults to "compile the code in"). **In a multi-DLL
suite every application in the suite needs it**, or that app quietly compiles a
private copy of the class.

### 5b. `CommandBarOnWindow` — procedure extension

*Procedure → Extensions → Insert → ClaCommandBar → CommandBarOnWindow.*

Nine tabs. The three that matter are **Bars**, **Menus** and **Items**:

1. **Bars** — one entry per bar. `Dock` picks the edge; bars on the same side
   stack by `Row` (0 nearest the edge); two bars sharing a row sit side by side
   in `Offset` order. Tick *Is the menu bar* for the one that carries menu
   titles. Each bar's **Name** is what the Items tab refers to.
2. **Menus** — a popup menu is a container with no bar. Name it here, fill it
   on the Items tab, then hang it off a *Menu title*, *Drop button* or
   *Split button* — or leave it loose and pop it yourself with
   `CommandBar.PopupMenu(CBMnu:<name>)` for a context menu.
3. **Items** — *Put it in* names the bar or menu, *Type* picks the widget, and
   *Command id* is the number you act on. **A name that resolves to nothing is
   a generate-time `#ERROR`**, not a silently missing item.

Every distinct command id gets **its own embed point**, so the generated
dispatch is:

```clarion
CBPump:CommandBar ROUTINE
  LOOP WHILE CommandBar.TakeOne()
    CASE CommandBar.LastEvent
    OF CBE:Command
      CASE CommandBar.LastCmd
      OF 101                       ! <- one embed here
      OF 102                       ! <- and here
      END
    OF CBE:Toggled                 ! <- one embed for all toggles
    ...
```

Read `CommandBar.LastItem`, `LastCmd` and `LastParam` inside any of them.

### 5b2. `CommandBarControl` — dropped from the control palette

*Window designer → the control palette → ClaCommandBar → CommandBarControl.*

It places a **REGION**. Position that region where you want the bar, then tick
**Land on the region** on that bar in the Bars list. The bar fills the region
exactly instead of docking to a window edge, so:

* it takes **nothing** off the client area — your LIST stays where you put it;
* the ABC resizer keeps moving the region, and the bar follows it.

The region is hidden at run time and goes on reporting its position. Bars in
the same instance *without* the tick dock to the window edges as usual, so you
can mix the two.

It is MULTI, so a window can carry several independent instances; every
generated label is keyed on the template instance.

### 5b3. `CommandBarFrame` — mirroring the frame's own menu

*Procedure (a FRAME) → Extensions → Insert → ClaCommandBar → CommandBarFrame.*

The **Menu** tab has three choices:

| Choice | What happens |
|---|---|
| *Leave the menu alone* | ordinary command bars, the Clarion menu untouched |
| *Mirror it onto a command bar* | the menu is rebuilt as a command bar **and** the original stays on the frame |
| *Mirror it and take the original menu off the frame* | the command bar **replaces** the menu |

Mirroring reads the `MENUBAR` at run time and reproduces it: same order, same
nesting, the separators, the `KEY()` attributes as a shortcut column, and
disabled items still disabled. Choosing a mirrored row POSTs `EVENT:Accepted`
to the **original `ITEM`**, so every `CASE ACCEPTED()` branch and every menu
embed you already wrote keeps running. You do not re-declare a single item.

By hand it is one call:

```clarion
bar = CB.AddBar('Menu', CBD:Top, CBBS:MenuBar)
CB.MirrorMenu(bar, 1)          ! 1 = take the original menu off the frame
```

`MirrorMenu` returns how many top-level menus it mirrored. `CB.SetHostMenu(1)`
puts the real menu back.

### 5b4. Ribbons

A ribbon is a bar with `CBBS:Ribbon`; it holds **tabs**, a tab holds
**groups**, and a group holds ordinary items — so a ribbon group is filled by
exactly the same `AddButton` calls as a toolbar.

In the templates: tick *Is a ribbon* on the bar, add rows to the **Ribbon** tab
(tabs, then groups), then on the **Items** tab name the *group* in
*Put it in*.

Mark an item **Image above the text** and it becomes the big button; everything
beside it stacks three-deep in small rows, which is how a ribbon lays a group
out. The large button uses *Large icon size*, the small ones *Small icon size*.

```clarion
rib = CB.AddRibbon('Ribbon', CBD:Top)
tab = CB.AddRibbonTab(rib, '&Home')
grp = CB.AddRibbonGroup(tab, 'Clipboard')
CB.AddLargeButton(grp, CMD:Paste, 'Paste', imgPaste)
CB.AddButton(grp, CMD:Cut, 'Cut', imgCut)
```

Switching tabs raises `CBE:TabChanged` with the tab id in `LastParam`.

### 5b5. What an item DOES

Every item on the Items tab has an **Action**, generated just before that
command's embed point:

| Action | Generated |
|---|---|
| Embed code only | nothing — the embed is all there is |
| Call a procedure | `MyProcedure(parms)` |
| Do a routine | `DO MyRoutine` — the ROUTINE must exist in that procedure |
| Emulate a control | `POST(EVENT:Accepted, ?Control)` |
| Post an event | `POST(EVENT:Whatever, ?Control)` |
| Close the window | `POST(EVENT:CloseWindow)` |

*Emulate a control* is the one to reach for first: point a bar button at a
BUTTON or a menu ITEM that is already on the window and its own embed code
runs, with nothing duplicated.

Where two items share one command id, the **first** of them decides the action.

### 5b6. Moving a bar at run time

A bar the user can move needs a handle, and the handle is the **gripper** —
tick *Drag gripper* on it. Then:

| Drag it to | What happens |
|---|---|
| near any edge of the client area | it docks there, re-laid out for that orientation: a wide toolbar becomes a narrow column down the side |
| the middle | it tears off into a floating frame with a caption (only if *User may float it* is ticked) |
| a floating bar's **caption**, back to an edge | it re-docks |

While the mouse is down nothing moves — a translucent hint shows the strip the
bar would land in. **`Esc` cancels** the drag. Dropping between two existing
rows inserts a row and pushes the others along.

Two style flags decide what is allowed:

* *Drag gripper* (`CBBS:Gripper`) — no gripper, no drag handle, so the bar
  cannot be moved. This is what keeps a menu bar put.
* *User may float it* (`CBBS:Floatable`) — without it, a drop in the middle
  leaves the bar where it was instead of floating it.
* `CBBS:Locked` refuses the drag outright.

Double-clicking a floating bar's caption sends it back to the top, and the
little **x** on the caption hides it (`CB.SetBarVisible(bar, 1)` brings it
back).

### 5b7. On an application FRAME the bars reserve their own space

A plain `WINDOW` positions its own controls, so you point *Control to fill the
space the bars leave* at a LIST and the template re-fits it. A **frame** does
not work like that: Clarion lays out its `ClaToolBar` and `MDIClient` itself,
against the **full** client area, and never reads `CB_GetClientRect`. Left
alone, a docked bar is simply drawn on top of the frame's toolbar.

So on a frame the manager takes over: it subclasses the host's own children and
corrects their layout in flight — a top band like a toolbar is pushed down
keeping its height, and a filler like the MDI client is pushed down and
shortened, keeping whatever inset the frame left for its status bar. Nothing to
configure; it turns itself on when it sees an `MDIClient` or a `ClaToolBar`.

`CommandBar.ReserveSpace(0)` turns it off, `(1)` forces it on, `(-1)` is the
auto default.

Three Win32 details make this less obvious than it sounds, and all three are in
the comments in `commandbar.cpp`:

* Clarion derives the MDI client's top from the toolbar's **height** rather than
  its position, so simply moving the windows starts a fight it wins.
* Clarion's MDI client procedure re-imposes the frame's own layout **inside the
  same `SetWindowPos` call**, so the correction has to run *after* the host's
  window procedure has had its say, not before. The hook calls the original
  procedure first and adjusts the answer.
* What the bars take off each edge is remembered as four **thicknesses**, not as
  the client rectangle's coordinates. A coordinate goes stale the instant the
  frame is resized, and the host moves its own children long before it asks us
  to lay out — which used to leave the MDI client short by whatever the window
  had grown.

**When an MDI child opens, Clarion creates a second `ClaToolBar`** for the
merged toolbar, hides the original and shows the new one. Both are picked up,
so the toolbar keeps its place; a bare show or hide is passed through
untouched, because forcing coordinates onto one is what used to make the
frame's toolbar disappear the moment a procedure opened.

### 5b8. MDI menu merging — what mirroring can and cannot see

**A mirrored bar shows the frame's own `MENUBAR`. Menus that an MDI child
merges into the frame are not mirrored.** That is a Clarion limitation, not a
choice, and it is worth knowing exactly where the wall is:

* The child's `MENU` controls belong to the **child's** window, on the child's
  own thread. They never appear in the frame's control list, so no amount of
  scanning the frame's field equates will find them — measured on Clarion 12,
  `MenuReport()` on a frame with a child open still reports only the frame's
  five menus.
* The merge really does happen, but at the **Win32** level: the frame's `HMENU`
  grows a sixth popup while the child is open. Reading it back does not help
  either — Clarion draws its menus itself, so every item comes back
  `MFT_OWNERDRAW` with `cch = 0` and no text at all.

So plan the frame's bar around the frame's own menu, and use one of these for
the rest:

* **`NOMERGE` on the child window** — stops Clarion merging that child in, so
  the frame's menu never changes and one mirror lasts the life of the app.
  This is the tidy answer when the frame's bar is meant to be the whole menu.

  > **Put it on the CHILD, never on the frame's own `TOOLBAR`.** `NOMERGE` on
  > an `APPLICATION` frame's `TOOLBAR` means "do not carry this toolbar into
  > the merge", and the frame's toolbar then **disappears the moment any MDI
  > procedure opens**. Measured on a real ABC app: with `NOMERGE` set on the
  > frame's `?TOOLBAR1` the toolbar was hidden and the MDI client took the
  > whole client area; clearing it put the toolbar back. Nothing to do with
  > the command bar — it happens with or without one.
* **Leave the child's menu alone** — a child that merges normally still shows
  its own menu through the frame's real menu bar, so keep `HideOriginal` off
  (`MirrorMenu(bar, 0)`) if the children need their merged menus.
* **`CommandBar.RefreshMirror()`** — throws the mirrored menu away and reads the
  frame's `MENUBAR` again onto the same bar, returning the new top-level count.
  Use it when the **frame's own** menu changes at runtime (items added, renamed
  or removed by your own code). It will not conjure up a child's merged menu,
  for the reasons above.

### 5b9. The host's status bar

Bars stop above the strip the host paints its **status bar** in, rather than
covering it — on the left, on the right and on the bottom.

There is no status-bar window to find: a Clarion frame paints that strip
itself. It is measured from the host's own intent instead — the frame stops its
MDI client short of the bottom, and that gap *is* the status bar. Measured on a
frame 1374x776, the client stops at 753, so 23px belong to the status bar.

A plain **WINDOW** with a `STATUS` bar has no such child to measure from, so
say how tall it is:

```clarion
CommandBar.HostReserveBottom(23)     ! pixels; -1 goes back to measuring
```

### 5b10. Presets - a bar, a ribbon or a menu in one click

Three buttons fill the lists in for you. Everything they make is an ordinary
entry afterwards: rename it, reorder it, delete what you do not want, and put
your own code in the embed points.

| Tab | Button | What you get |
|-----|--------|--------------|
| **Bars** | *Add this toolbar*, with a **Toolbar** drop beside it | one of five classics — see below |
| **Ribbon** | *Add a standard ribbon* | the ribbon from `CommandBarShowcase`, entry for entry: **Home** — Clipboard (big Paste, then Cut / Copy / Format), Font (font and size combos, **B** and *I* toggles, a colour button), Editing (big Find, then Replace / Go To); **Insert** — Pages, Illustrations, Links; **View** — Show (three tick boxes) and Zoom. Small icons go to 20px and large to 32px if they were still at their defaults, so the big buttons have room |
| **Menus** | *Add this preset menu* | the popup named by the **Preset** drop beside it: **File**, **Edit**, **Browse row**, **View** or **Help** — with icons, shortcut text, and check marks where they belong |

**The five toolbars:**

| Preset | What is on it |
|--------|---------------|
| **Standard** | New, Open, Save ǀ Print ǀ Cut, Copy, Paste ǀ Undo, Redo ǀ a search box, Find ǀ Help pushed to the far end. `Ctrl+N/O/S/P/F` and `F1` |
| **Formatting** | font and size combos ǀ **B** *I* <u>U</u> and a text-colour button ǀ left / centre / right ǀ bulleted and numbered lists. `Ctrl+B/I/U` |
| **Browse and records** | the VCR keys ǀ Insert, Change, Delete ǀ a locator box, Locate, Sort, Mark ǀ Refresh ǀ Print at the far end. `Ctrl+F`, `F5` |
| **Navigation** | Back, Forward, Stop, Refresh, Home ǀ an address box that **stretches** to eat the leftover width, Go ǀ Search. `F5` |
| **Print and export** | Print, Preview ǀ an **Export drop button** carrying a menu of PDF, Excel, CSV, HTML, XML and plain text ǀ Refresh ǀ Close at the far end. `Ctrl+P`, `F5` |

Each press starts a **new** bar on the first free row of the top edge, so you
can stack two of them without them fighting over a row.

Press one twice and you get `Standard2`, `Ribbon2`, `FileMenu2` — names become
variables in the generated source, so they are kept unique. Command ids carry
on from the highest already in use, so a preset never collides with items you
added yourself.

**Icons.** Presets name the stock Clarion icons by plain file name —
`NEW.ICO`, `OPEN.ICO`. **Add those icons to the application's project** and
Clarion links them into the EXE; the manager finds them there by resource name
(`NEW.ICO` becomes `NEW_ICO`, which is how Clarion stores them), so they work
whatever directory the program is started from. A copy sitting next to the EXE,
or in an `images` folder beside it, is found too.

### 5b11a. A ribbon always gets a row to itself

Bars sharing a row are handed out left to right, each taking the width it
asks for — and a **ribbon asks for the whole row**, because it is a full-width
band with tabs across it. Put one on the same row as a menu bar and the row
went to whichever came first: with the ribbon first the menu bar was squeezed
to a single pixel and disappeared.

So a ribbon sharing a row is moved to a row of its own **directly below**,
leaving the thin bars where they were. Set both to row 0 and you get the menu
bar on top with the ribbon under it — the only arrangement that shows both.
Nothing to configure.

### 5b11. A ribbon that collapses

Tick **Starts collapsed to its tabs** on a bar marked *Is a ribbon* and it
opens showing the tab strip alone, giving the window back the rest of the
height. There is a **small chevron button at the far end of the tab strip**, the one
older ribbons put there: it points up while the ribbon is open, down once it
is collapsed, and one click does either. The keyboard-free gestures work too:

* **the chevron button** — collapse, and open again
* **double-click a tab** — the same thing
* **click a tab while collapsed** — open it on that tab

or say so yourself:

```clarion
CommandBar.MinimizeRibbon(CBBar:1:Ribbon, 1)     ! collapse
IF CommandBar.RibbonMinimized(CBBar:1:Ribbon)
```

Measured on the showcase's ribbon: 109px open, 29px collapsed, and back to
109 on the next tab click.

### 5c. Putting your own controls under the bars

The bars take space off the top / bottom / sides of the window. Whatever is
left is reported by `CommandBar.ClientX/ClientY/ClientWidth/ClientHeight`, in
**pixels**.

Set *Control to fill the space the bars leave* on the General tab to a LIST,
SHEET or REGION and the template re-fits it on every resize, and whenever a bar
is shown, hidden, docked or floated. By hand it is one call:

```clarion
CommandBar.FitControl(?Browse:1, 4, 4)      ! 4px margin
```

### 5d. Keyboard shortcuts

The **Keys** tab takes a Clarion key equate (`CtrlS`, `F5Key`, `ShiftF3`) and a
command id. The template adds the `ALRT()` to the window for you and forwards
`EVENT:AlertKey` to the bars.

This works because a Clarion KEYCODE packs the Windows virtual key in the low
byte and the modifiers in the high byte (1 = shift, 2 = ctrl, 4 = alt), so
`CtrlS = 0253h` is `'S'` (53h) with ctrl. `AddClarionKey` and `TakeAlertKey`
do the translation — including swapping the two low bits, because the DLL's own
`CBK:` flags are ctrl = 1, shift = 2.

### 5e. Theming

Pick one of the eleven themes on the **Appearance** tab and you are done. All
37 colour slots are set from it.

To match a house style, tick *Re-colour the whole theme around one accent
colour* and pick the accent — every hover, pressed, checked, gutter and
highlight shade is **derived** from it, so the set stays coherent and the
light/dark character of the theme is kept.

For one specific colour, add an entry on the **Colours** tab. Those are applied
*after* the theme, so they always win.

Sizing note: set *Small icon size* to match the artwork you actually have.
Clarion ships 32×32 icons; the engine pre-scales with WIC's Fant filter, but
halving 32px line art into a 16px slot still greys out every one-pixel stroke.
A 24px slot for 32px art looks far better than a 16px one.

## 6. Verified runtime notes (why the class does what it does)

* **`W{PROP:Handle}` is the FRAME, not the client area.** A Clarion window puts
  a full-size `ClaChildClient` over its whole client area and every Clarion
  control lives inside that. Bars created as children of the frame are
  therefore *siblings* of `ClaChildClient` — and left where they were created
  they sit underneath it and never show. The DLL puts every bar at `HWND_TOP`
  on **every** layout, and sets `WS_CLIPCHILDREN` on the host so it stops
  erasing over them.
* **The DLL subclasses the host window** to catch `WM_SIZE` (and
  `WM_DPICHANGED`) so the bars stay put even if nothing calls `Layout()`. It
  restores the original window procedure on `CB_Destroy` and on `WM_NCDESTROY`.
* **`PROP:XPos` and friends are dialog units** unless the window is switched to
  pixel mode. `FitControl` saves `0{PROP:Pixels}`, sets it TRUE, positions, and
  puts it back.
* **The event queue is drained on `EVENT:Timer`,** so the window needs a timer.
  The class sets one only if the window has none of its own.
* **A popup menu runs its own modal message loop** (like `TrackPopupMenu`), so
  the calling ACCEPT loop is suspended while a menu is open. The chosen command
  arrives as a normal queued event afterwards, and `TrackMenu` / `PopupMenu`
  also return it directly.
* **A Clarion MENUBAR ignores `PROP:Hide`.** It is a real Win32 menu on the
  frame, so removing it means `SetMenu(hwnd, NULL)` — which the DLL does, and
  then **re-asserts**, because the Clarion runtime re-attaches the menu while
  the window finishes opening. The menu is kept, not destroyed, and put back on
  `CB_Destroy`; the field equates stay valid the whole time, which is what lets
  a mirrored row still POST to its `ITEM`.
* **Menu controls land in FOUR different equate ranges.** Which one depends on
  the kind of window *and* on whether the control carries a `USE`:

  | | carries a `USE` | no `USE` |
  |---|---|---|
  | `WINDOW` | `1, 2, 3 …` upward | `32768, 32769 …` upward |
  | `APPLICATION` frame | `-1, -2, -3 …` **negative** | `32767, 32766 …` **downward** |

  An `APPLICATION` frame also answers **`LASTFIELD() = 0`**, so the ordinary
  field range is not walkable there at all. `MirrorMenu` scans all four ranges
  and filters on `PROP:Parent`. **An equate is not a position** — the ranges
  are independent sequences and on a frame they run backwards — so top-level
  menus are sorted on the lowest *rank* of any named control in their subtree.
  See `examples\MenuMirrorTest`.
* A MENU answers `PROP:Child,n` with its immediate children **in declaration
  order**, submenus and separators included — but the MENUBAR itself does not,
  which is why the top level has to be found by scanning. A menu ITEM with
  empty `PROP:Text` is a separator.
* **`CBE:DropDown` is queued before a menu opens,** but a Clarion app polling
  on a timer only sees it *after* the menu has closed. Filling a menu on the
  fly needs the immediate C callback (`CB_SetCallback`), not the poll queue.
  Build menus up front instead.

## 7. Troubleshooting

| Symptom | Cause |
|---|---|
| Every app fails to open with `Could not open include file ClaCommandBar.tpl` | the template was registered with a **relative** path (§4). Restore `TemplateRegistry12.trf` from your backup and re-register with a full path |
| The new prompts do not appear | the IDE was open during `-tr`; close it, re-register, reopen. Check the version stamp on the General tab against the top of the `.tpl` |
| `Expected: <directive> <LINEBREAK> PROGRAM PRAGMA` at line 1 | `CommandBar.inc` / `.clw` were saved with LF line endings; Clarion needs CRLF |
| `Unresolved External CB_Create` | `commandbar.lib` is not in the project or on the linker path. In a **hand-coded** project the lib is linked by `PRAGMA('link(commandbar.lib)')` in the source — a `<LibraryFile>` project item does nothing |
| `Entry Point Not Found: … CB_GetCursorPos …` | a **stale `commandbar.dll`**. The import lib binds by ordinal, so a DLL older than the lib is missing the newest exports. Usually `accessory\bin\commandbar.dll`, which the Clarion build copies into the output folder on top of a freshly staged one — stage the DLL **after** the link |
| `Unresolved External _main in iexe32.obj` | `CommandBar.clw` was listed in the project. Its `LINK()` attribute already pulls it in; remove it |
| `Missing procedure definition: CB_CREATE(…)` | the `MODULE()` label in `CommandBar.clw` was changed to `COMMANDBAR.DLL`. Clarion strips the extension, compares it with the module being compiled, and decides the prototypes are defined there. The label is only a grouping name |
| `Label duplicated, second used: _COMMANDBARLINKMODE_` | the two mode defines were EQUATEd inside `CommandBar.inc`; they must arrive only as project defines |
| The bars are invisible but the client area shrank | the host window is not the one the bars were created on, or something re-ordered z-order after the last layout. Call `CommandBar.Layout()` |
| The bars never respond | the window has no timer — set *Timer interval* above 0, or give the window `TIMER()` |
| `ClaCommandBar: item 4 says "Put it in: Toolbar" but there is no bar or menu with that name` | exactly what it says — the Items tab names a container that is not on the Bars or Menus tab. Names are matched case-insensitively but otherwise exactly |
| Icons look pale and washed out | 32px artwork in a 16px slot. Raise *Small icon size*, or use art drawn at the size you are showing it |
| A plain menu row grows a check mark every time it is picked | *auto-check* is ticked on it. That setting is for menu rows that behave like a setting; toggle buttons and check boxes do it anyway |
| `Illegal data type: COMMANDBAR` on the generated object | a `#INSERT` that emits LABELS was indented. `#INSERT` carries the indentation of its own line into every line the group emits, and a Clarion label must start in column 1 — `#INSERT(%CBEmitData)` and `#INSERT(%CBEmitFitRoutine)` sit at column 0 for that reason |
| The mirrored bar appears but the Clarion menu is still above it | *Mirror it onto a command bar* was chosen instead of *…take the original menu off the frame*. If you picked the latter and it still shows, the host window is not the one the manager was created on |
| A bar will not drag | it has no *Drag gripper*, or it is `CBBS:Locked`. The gripper is the handle — the cursor turns into a move cursor over it |
| A bar will not float, only re-dock | *User may float it* is off |
| The mirrored bar is **empty** | `MirrorMenu` found no MENU whose `PROP:Parent` is the menubar. Call `CB.MenuReport()` and read what it says — it names the menubar equate, says whether it sees a `WINDOW` or an `APPLICATION` frame, and lists what it found. If `PROP:MenuBar` is 0 the window has no menubar of its own: mirror on the **FRAME**, not on an MDI child, or pass the equate to `MirrorMenuFrom` |
| A docked bar is drawn **on top of** the frame's toolbar | space reservation is off, or the host is not recognised as owning its layout. Call `CommandBar.ReserveSpace(1)` to force it |
| Mirrored rows appear but **clicking does nothing** on a FRAME | fixed in v1.2. A frame numbers its menu controls NEGATIVE, so a mirrored command id lands just *below* `MirrorBase`, and the old test only matched ids above it |
| The frame's menu gained items and the mirrored bar did not | an MDI child merged its menu in — that cannot be mirrored (see 5b8). Set `NOMERGE` on the child, or mirror with `HideOriginal` off |
| The frame's TOOLBAR disappears when a procedure opens | check the frame's `TOOLBAR` for **`NOMERGE`** and clear it — that attribute drops the toolbar out of the merge, so opening any MDI child takes it away. It is not caused by the bars |
| AppGen **hangs** on a template button or while generating | a template group hands its answer back as TEXT, and every string except the empty one is TRUE - `'0'` included. `#LOOP,WHILE(%SomeGroup())` therefore never ends. Spell the test out: `#LOOP,WHILE(%SomeGroup() = 1)`. It is a hang, not a crash: Windows logs it as `AppHangB1` against `Clarion.exe` |
| `Unresolved External CB_...` after an update | a **second** `commandbar.lib` is shadowing the installed one. The linker takes the first it finds on the redirection path, so a copy in `accessory\libsrc\win` — or one sitting in your own app folder — wins over `accessory\lib` and an old one leaves new exports unresolved however often you rebuild. `install.bat` refreshes both Clarion copies now; delete any copy in your app folder, or replace it with the new one |
| A docked bar covers the frame's status bar | fixed - bars stop above it. The strip is measured from how far short the host stops its MDI client. On a plain **WINDOW** with a `STATUS` bar there is no such child to measure, so tell it: `CommandBar.HostReserveBottom(23)` |
| A bar seems to appear twice after being dragged | old pixels nothing repainted, now cleaned up two ways: the host is told to erase the strip a bar vacated, and the host's own children are repainted rather than having their pixels blitted to a new position. The blit was the one that showed on the **toolbar** specifically - dragging a bar straight from one side to the other moves the toolbar across, and moving a window normally copies its pixels along, bar and all |
| The frame's toolbar flickers when an MDI child opens or closes | some of that is Clarion's own: merging swaps the frame's toolbar for a merged one and unmerging swaps it back, so that strip is painted twice either way, with or without a command bar. What is ours is not: the bars are only repainted by a layout that actually moved something |
| A frame child (toolbar, MDI client) sits in the wrong place | set `CB_HOSTLOG=1` in the environment and run again — every host-child move is traced to `%TEMP%\cbhost.log` |
| A mirrored row does nothing | its original `ITEM` has no `CASE ACCEPTED()` branch — mirroring only forwards the click, it does not invent behaviour |
| A ribbon group is empty | the item's *Put it in* names the TAB, not the GROUP. Items go in a group |
| A toggle button no longer stays down | its style was overwritten. `SetItemStyle` **replaces** the style word — include `CBIS:AutoCheck` when you set it by hand |

## 8. Verifying a change without opening the IDE

The whole chain can be exercised from the command line, which is how the
template in this repo was tested:

```
:: 1. register (full path, from anywhere but the template's folder)
ClarionCL -tr C:\path\ClaCommandBar.tpl

:: 2. attach the extension to a procedure by editing a TXA
ClarionCL -win -au -ax app.app a.txa
::    insert into the procedure you want:
::        [ADDITION]
::        NAME ClaCommandBar CommandBarOnWindow
::        [INSTANCE]
::        INSTANCE 99
ClarionCL -win -au -ai app.app a.txa

:: 3. export again - AppGen now writes EMPTY declarations for every
::    repeating prompt (%CBBarList MULTI LONG (), %CBBarName DEPEND … TIMES 0)
ClarionCL -win -au -ax app.app b.txa
::    populate those blocks IN PLACE, then
ClarionCL -win -au -ai app.app b.txa

:: 4. generate and read the .clw
ClarionCL -win -au -ag app.app
```

Two things bite here. Editing the declarations **in place** matters: `-ax`
puts the empty blocks at the end of the addition's prompt list, and a
hand-inserted copy earlier in the file is overwritten by the empty one that
follows — the rows vanish with no message. And when you search for a symbol to
replace, anchor the match: `%CBItemList` must not match `%CBItemListSomething`,
and a bare `%CBItem` would match `%CBItemHeight`. (The list symbols in this
chain are named `…List` precisely so no list name is a prefix of one of its own
children, but the general trap remains.)

Template-language traps around `#FOR(%SomeMultiList)`, measured in the sister
ClaPropGrid project and designed around here:

| Call in a generated line, inside `#FOR` over a MULTI list | Result |
|---|---|
| `%(%CBTypeEquate(%CBItemType))` — **one** argument | works |
| a group call with **two** arguments | expands to **nothing** |
| a group call with **no** arguments reading this template's MULTI children | expands to **nothing** |
| a group call reading another template's symbols | can silently kill the whole `#AT` block |

So every value a generated line needs is computed with `#SET` into a symbol
declared in `#ATSTART` and emitted as `%Symbol`. **If generated code disappears
entirely, suspect a `#GROUP` call** — AppGen reports nothing.

And note that numeric prompts arrive as **strings**: `#IF(%CBItemWidth)` is
TRUE when the value is `'0'`. Every numeric test in this chain is an explicit
comparison (`#IF(%CBItemWidth > 0)`).
