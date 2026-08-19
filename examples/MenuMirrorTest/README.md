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

## The three bugs these caught

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

If a mirrored bar ever comes out empty in your own app, call
**`CB.MenuReport()`** and read it: it names the menubar equate it found, says
whether it is looking at a `WINDOW` or an `APPLICATION` frame, and lists every
top-level menu hanging off it.
