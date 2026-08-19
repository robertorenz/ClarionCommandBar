# MenuMirrorTest

A regression test for the one thing that is genuinely surprising about
mirroring a Clarion `MENUBAR`: **menu controls do not all live in the same
field-equate range.**

## The trap

Measured on Clarion 12:

| The control | What equate it gets |
|---|---|
| a `MENUBAR` / `MENU` / `ITEM` **with** a `USE` | an ordinary **low** equate — an AppGen frame answers `0{PROP:MenuBar} = 1` |
| one **without** a `USE` (an unnamed `MENU`, every `ITEM,SEPARATOR`) | a **synthetic** equate at `8000h` and up |

A window mixes the two freely, and the two sequences are numbered
independently. This test's window is deliberately the awkward shape: the
`MENUBAR` carries a `USE`, `&Edit` carries one, and `&File` and `&Help` do not.
It reports what `MirrorMenu` could see:

```
PROP:MenuBar = 1
FIRSTFIELD=12 LASTFIELD=12
  menu feq=7      order=7    [&Edit]
  menu feq=32768  order=2    [&File]
  menu feq=32771  order=11   [&Help]
3 top-level menus.
```

Two bugs came out of exactly this:

1. **An empty bar.** The first version scanned a 1024-wide window around the
   menubar's own equate. With `PROP:MenuBar = 1` that scan never reaches
   `8000h`, so a menubar whose menus have no `USE` mirrored to **nothing**.
   Both ranges are scanned now.
2. **The wrong order.** Sorting the menus by equate gives *Edit, File, Help* —
   the ranges interleave, so an equate is not a position. They are sorted on
   the **lowest equate anywhere in the menu's subtree** instead (`order=` in
   the report above), because the items inside the first menu are always
   declared before the items inside the second whichever range they land in.
   That gives *File, Edit, Help*.

## Running it

```
build.bat
MenuMirrorTest.exe
```

The mirrored bar replaces the real menu. Open **File → Open Recent** (a submenu
of a menu with no `USE`) and **Help → About**; the window logs which original
`ITEM` fired. That is the point of mirroring: `?MFileNew`, `?MR1` and `?MAbout`
run their ordinary `CASE ACCEPTED()` code, untouched.

The full report is also put on the clipboard at startup. If a mirrored bar ever
comes out empty in your own app, call `CB.MenuReport()` and paste it — it names
the menubar equate it found and every top-level menu hanging off it.
