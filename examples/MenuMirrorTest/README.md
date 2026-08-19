# MenuMirrorTest / FrameMirrorTest

Two regression tests for the one genuinely surprising thing about mirroring a
Clarion `MENUBAR`: **menu controls land in four different field-equate ranges,
and which one depends on the kind of window *and* on whether the control
carries a `USE`.**

Measured on Clarion 12:

| | carries a `USE` | no `USE` |
|---|---|---|
| **`WINDOW`** | `1, 2, 3 …` (upward) | `32768, 32769 …` (**upward**) |
| **`APPLICATION` frame** | `-1, -2, -3 …` (**negative**) | `32767, 32766 …` (**downward**) |

An `APPLICATION` frame also answers **`LASTFIELD() = 0`** — the ordinary field
range is not even walkable there.

`MenuMirrorTest.exe` is a `WINDOW`; `FrameMirrorTest.exe` is an MDI
`APPLICATION` frame. Both mix named and unnamed menus on purpose. Each reports
what `MirrorMenu` could see and puts it on the clipboard:

```
MirrorMenu returned 5 top-level menus.

PROP:MenuBar = -1  (APPLICATION frame)
FIRSTFIELD=0 LASTFIELD=0
  menu feq=-2      order=2    [&File]
  menu feq=-8      order=8    [&Edit]
  menu feq=-13     order=13   [&Window]
  menu feq=-15     order=15   [&Help]
  menu feq=32765   order=12   [&Trees]     <- a MENU with no USE
5 top-level menus.
```

## The bugs these caught

1. **An empty bar on a `WINDOW`.** The first version scanned a 1024-wide window
   around the menubar's own equate. With `PROP:MenuBar = 1` that never reaches
   `32768`, so a menubar whose menus have no `USE` mirrored to nothing.
2. **An empty bar on a FRAME** — the one reported from a real app. Every range
   was wrong: named controls are *negative*, unnamed ones count *down* from
   `32767`, and `LASTFIELD()` is `0`. All four ranges are scanned now.
3. **The wrong order.** An equate is not a position — the ranges are
   independent sequences, and on a frame they run backwards. Top-level menus
   are sorted on the lowest **rank** (`order=` above) of any *named* control in
   their subtree, because the items inside the first menu are always declared
   before the items inside the second whichever range they land in. Every menu
   worth mirroring has named items — an `ITEM` without a `USE` cannot have
   `ACCEPTED` code anyway.

4. **The frame's toolbar disappearing the moment a procedure opened.** Choosing
   **File → New** on `FrameMirrorTest` `START`s a real MDI child — on its own
   thread, carrying its own `MENUBAR` *and* `TOOLBAR`, so Clarion merges. What
   the merge actually does, measured by enumerating the frame's children before
   and after:

   ```
   before:  ClaCommandBar.Bar  0,0   910x26
            ClaToolBar         0,26  910x61
            MDIClient          0,87  910x496

   after:   ClaCommandBar.Bar  0,0   910x26
            ClaToolBar         0,26  910x61   hidden   <- the original
            MDIClient          0,87  910x496
            ClaToolBar         0,26  910x61            <- a NEW merged one
   ```

   Clarion **creates a second `ClaToolBar`**, hides the first and shows the new
   one. Forcing coordinates onto those show/hide messages, and remembering the
   collapsed rect that comes with one as the toolbar's canonical position, is
   what made the toolbar vanish. Only a real move or size is treated as a
   layout now, and the correction runs *after* the host's own window procedure
   — Clarion's MDI client re-imposes the frame's layout inside the same
   `SetWindowPos` call, so correcting first simply loses.

## What mirroring cannot do

The child's `&Child` menu **does not appear on the mirrored bar**, and cannot:

* it belongs to the child's window on the child's thread, so it is not in the
  frame's control list — `MenuReport()` with the child open still lists only
  the frame's five menus;
* the merge is real at the Win32 level (the frame's `HMENU` does grow a sixth
  popup), but Clarion owner-draws its menus, so every item reads back
  `MFT_OWNERDRAW` with `cch = 0` and no text.

Use `PROP:NoMerge` on children, or mirror with `HideOriginal` off so the real
menu bar keeps showing the merged menus.

## Running them

```
build.bat
MenuMirrorTest.exe
FrameMirrorTest.exe
```

The mirrored bar replaces the real menu in both. Open **File → Open Recent** (a
submenu) and **Trees** (a menu with no `USE`); the window reports which original
`ITEM` fired. That is the point of mirroring — `?MNew`, `?MR1` and `?MTrees` run
their ordinary `CASE ACCEPTED()` code, untouched.

**File → New** opens the MDI child. The toolbar must stay put, below the
mirrored menu row, and gain the child's *Child tool* button; the MDI client
must sit below both.

Set `CB_HOSTLOG=1` before running and every host-child move is traced to
`%TEMP%\cbhost.log` — `HOOK`, `CHANGING` and `APPLY` lines with the canonical
rect and what it was transformed to.

If a mirrored bar ever comes out empty in your own app, call
**`CB.MenuReport()`** and read it: it names the menubar equate it found, says
whether it is looking at a `WINDOW` or an `APPLICATION` frame, and lists every
top-level menu hanging off it.
