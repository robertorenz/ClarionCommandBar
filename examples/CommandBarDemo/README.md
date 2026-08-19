# CommandBarDemo

A hand-coded Clarion application that drives `CommandBarClass` directly — no
templates involved. It is the quickest way to see everything the engine does,
and the reference for wiring the class into a window by hand.

## Build and run

```
..\..\src\build.bat     :: only if you changed src\ - the DLL is committed
build.bat
CommandBarDemo.exe
```

`build.bat` stages `CommandBar.inc`, `CommandBar.clw` and `commandbar.lib` into
this folder (the class's `LINK()` attribute needs them on the redirection path),
runs MSBuild, then copies `commandbar.dll` in **after** the link — the Clarion
build copies any DLL it finds via redirection into the output folder, and would
otherwise drop a stale one on top.

## What to try

| | |
|---|---|
| **Menu bar** | open *File*, then slide sideways across *Edit* / *View* / *Help* — the menus follow, the way a real menu bar behaves. Arrow keys and `&` accelerator letters work too |
| **Open ▾** | a split button: the left half runs the command, the arrow drops a menu |
| **Colour ▾** | the swatch applies the current colour; the arrow opens the picker |
| **Find:** | an in-bar edit — click it, type, press Enter |
| **Zoom** | an in-bar drop list |
| **View → Theme** | all eleven themes, live |
| **View → Re-accent** | rebuilds the whole palette around one accent colour |
| **View → Side bar** | hides the left dock; the list re-fits itself |
| **Drag either gripper** | the ribbed handle at the left of the Standard or Format bar. Drop it on an edge to dock it there, or in the middle to tear it off. `Esc` cancels |
| **View → Float the format bar** | drag its caption back to an edge to re-dock, or double-click it |
| **Right-click** the list | a context menu popped at the pointer |
| **Ctrl+N / Ctrl+O / Ctrl+S** | accelerators, through `ALRT()` and `TakeAlertKey` |

The list logs every event the bars raise — command, toggle, text, selection,
colour, right-click and layout — with its command id and detail.

## Things worth copying

* **`CB.FitControl(?Log, 4, 4)`** parks the LIST in whatever space the bars
  left. It is called on `EVENT:Sized` *and* on `CBE:Layout`, so hiding a bar or
  floating one re-fits it too.
* **`LOOP WHILE CB.TakeOne()`** drains the queue one event at a time and leaves
  it in `CB.LastEvent` / `LastCmd` / `LastItem` / `LastParam`. The alternative
  is `CB.TakeEvent()` with the `Take…` methods overridden on a derived class —
  never mix the two on one object, the first one to run drains the queue.
* **`CB.SetMetric(CBM:IconSize, 24)`** — the icons here are Clarion's own 32×32
  artwork. The engine pre-scales with WIC's Fant filter, but halving 32px line
  art into a 16px slot still greys out every one-pixel stroke. Match the slot to
  the art you have.
* **`CB.AddClarionKey(CMD:Save, CtrlS)`** takes a Clarion key equate directly;
  the window needs the matching `ALRT()` for `EVENT:AlertKey` to fire at all.

## Icons

`images\` holds a handful of Clarion's own toolbar icons, copied from
`clarion12\images`. Any format WIC reads works: `.ICO`, `.PNG`, `.GIF`, `.JPG`,
`.BMP`, alpha honoured.
