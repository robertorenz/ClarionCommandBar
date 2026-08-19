# Installing ClaCommandBar

Everything you need is committed pre-built. You do **not** need Visual Studio
unless you change `src\`.

| File | Goes where | What it is |
|---|---|---|
| `bin\commandbar.dll` | **beside every EXE that uses it** | the engine (32-bit) |
| `clarion\commandbar.lib` | anywhere the linker looks | the Clarion import library |
| `clarion\CommandBar.inc` | on the redirection path | the class header |
| `clarion\CommandBar.clw` | on the redirection path | the class body |
| `clarion\ClaCommandBar.tpl` | anywhere permanent | the template chain |

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

* the application's own folder, or
* `clarion12\accessory\libsrc\win` (available to every app on the machine).

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

**Use a full path, and run it from any folder except the template's own.**
This matters more than it looks:

| What you type | What happens |
|---|---|
| `ClarionCL -tr C:\path\ClaCommandBar.tpl` | correct |
| `cd C:\path` then `ClarionCL -tr ClaCommandBar.tpl` | registers the **relative** path. Afterwards **every** app in the IDE fails to open with `Could not open include file ClaCommandBar.tpl`, and re-registering does not undo it — you have to restore `clarion12\template\win\TemplateRegistry12.trf` from a backup |

Back the registry file up before you register anything, always. It is the only
clean way out.

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
* **Menu controls do not all live in the same equate range.** One carrying a
  `USE` gets an ordinary **low** equate — an AppGen frame answers
  `0{PROP:MenuBar} = 1` — and one without a `USE` (an unnamed `MENU`, every
  `ITEM,SEPARATOR`) gets a **synthetic** equate at `8000h` and up. A window
  mixes both freely, and they are numbered as two independent sequences, so
  **an equate is not a position**. `MirrorMenu` therefore scans both ranges,
  filters on `PROP:Parent`, and sorts the top-level menus on the lowest equate
  anywhere in each menu's subtree. See `examples\MenuMirrorTest`.
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
| The mirrored bar is **empty** | `MirrorMenu` found no MENU whose `PROP:Parent` is the menubar. Call `CB.MenuReport()` and read what it says: if `PROP:MenuBar` is 0 the window has no menubar of its own (mirror on the **FRAME**, not on an MDI child, or pass the equate to `MirrorMenuFrom`) |
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
