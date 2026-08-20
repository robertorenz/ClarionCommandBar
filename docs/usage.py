# One worked line of Clarion for every property and method on
# CommandBarClass.  Kept beside the generator rather than inside it so the
# reference guide can show how a call is actually used, not just its
# prototype.  A name missing from here is reported when the guide is built.

PROPS = {
 'CB':            ("!  the DLL's handle - 0 until Init succeeds, and what every\n"
                   "!  method checks before it does anything\n"
                   "IF ~CB.CB\n"
                   "  MESSAGE('commandbar.dll did not start.')\n"
                   "END"),
 'Win':           "!  the window the bars live on, set by Init\n"
                  "IF ~CB.Win &= NULL THEN SETTARGET(CB.Win).",
 'Style':         "!  the CBS: flags Init was given\n"
                  "IF BAND(CB.Style, CBS:Tooltips) THEN DO ShowTipHint.",
 'Theme':         "!  the theme in force, as a CBT: number\n"
                  "IF CB.Theme = CBT:SlateDark THEN Msg = 'dark today'.",
 'Initialized':   "!  1 once Init has run, so a shared routine can tell\n"
                  "IF CB.Initialized THEN CB.Layout().",
 'TimerInterval': "!  the TIMER Init forces on the window (0 = leave it alone).\n"
                  "!  Set it BEFORE Init.\n"
                  "CB.TimerInterval = 5                     ! a livelier pump",
 'MenuBar':       "!  the bar AddMenuBar made, 0 if there is none\n"
                  "IF CB.MenuBar THEN CB.SetBarVisible(CB.MenuBar, 0).",
 'MirrorBase':    "!  mirrored rows carry MirrorBase + the original ITEM's equate,\n"
                  "!  and TakeOne turns them back into a POST on that ITEM.\n"
                  "CB.MirrorBase = 0                        ! see the raw ids instead",
 'MirrorBar':     "!  the bar the last MirrorMenu built into\n"
                  "IF CB.MirrorBar THEN CB.SetBarVisible(CB.MirrorBar, 1).",
 'MirrorTBFeq':   "!  the TOOLBAR the last MirrorToolbar read\n"
                  "IF CB.MirrorTBFeq THEN CB.ShowHostToolbar(CB.MirrorTBFeq, 1).",
 'MirrorHide':    "!  whether that mirror took the original menu off the window\n"
                  "IF CB.MirrorHide THEN Msg = 'the real menu is detached'.",
 'MirrorMenus':   "!  PRIVATE - the popups the mirror built, so RefreshMirror can\n"
                  "!  throw them away.  Nothing to do with it from outside.",
 'LastItem':      "!  filled by TakeOne, with LastCmd / LastEvent / LastParam\n"
                  "CB.SetItemText(CB.LastItem, 'Just clicked')",
 'LastCmd':       "LOOP WHILE CB.TakeOne()\n"
                  "  CASE CB.LastCmd\n"
                  "  OF CMD:Save ; DO SaveRoutine\n"
                  "  END\n"
                  "END",
 'LastEvent':     "CASE CB.LastEvent\n"
                  "OF CBE:Command      ; DO RunCommand\n"
                  "OF CBE:ValueChanged ; Zoom = CB.LastParam\n"
                  "END",
 'LastParam':     "!  what the event carries: a toggle's state, a slider's value,\n"
                  "!  a combo's index, a gallery's cell number\n"
                  "Zoom = CB.LastParam",
}

USAGE = {
 # ---- lifetime and layout ----
 'Construct': "!  runs by itself when the object comes into scope - nothing to call.",
 'Destruct':  "!  runs by itself.  Kill() is the one to call when you want the\n"
              "!  bars gone before the object goes out of scope.",
 'Init':      "IF ~CB.Init(Window, CBS:Tooltips + CBS:MenuIcons)\n"
              "  MESSAGE('commandbar.dll did not start.')\n"
              "  RETURN\n"
              "END",
 'Kill':      "CB.Kill()                                ! before CLOSE(Window)\n"
              "CLOSE(Window)",
 'Layout':    "  OF EVENT:Sized\n"
              "    CB.Layout()                          ! bars re-flow to the new size",
 'Redraw':    "CB.SetColor(CBC:Accent, COLOR:Navy)\n"
              "CB.Redraw()                              ! repaint with the new colour",
 'ClientX':      "X = CB.ClientX()                         ! pixels the bars left free",
 'ClientY':      "Y = CB.ClientY()",
 'ClientWidth':  "W = CB.ClientWidth()",
 'ClientHeight': "H = CB.ClientHeight()\n"
                 "?Browse:1{PROP:Height} = H               ! in PIXELS, so set PROP:Pixels",
 'FitControl':   "  OF EVENT:Sized\n"
                 "    CB.Layout()\n"
                 "    CB.FitControl(?Browse:1, 4, 4)       ! fill what is left, 4px margin",

 # ---- theme ----
 'SetTheme':  "CB.SetTheme(CBT:Office2016)              ! one of eleven",
 'SetAccent': "CB.SetAccent(COLOR:Navy)                 ! re-derives the palette",
 'SetColor':  "CB.SetColor(CBC:BarBack, 00F5F5F5h)      ! override one slot",
 'GetColor':  "c = CB.GetColor(CBC:Accent)",
 'SetFont':   "CB.SetFont(CBF:Item, 'Segoe UI', 9)\n"
              "CB.SetFont(CBF:Menu, 'Segoe UI', 9, 1)   ! bold",
 'SetMetric': "CB.SetMetric(CBM:IconSize, 20)\n"
              "CB.SetMetric(CBM:LargeIcon, 32)",
 'GetMetric': "h = CB.GetMetric(CBM:ItemHeight)",

 # ---- images ----
 'AddImage':       "iNew = CB.AddImage('NEW.ICO')            ! add it to the PROJECT too\n"
                   "CB.AddButton(bar, CMD:New, '&New', iNew)",
 'AddImageStrip':  "n = 0\n"
                   "first = CB.AddImageStrip('TOOLBAR.BMP', 16, n)\n"
                   "CB.AddButton(bar, CMD:New, '&New', first)        ! + 1, + 2 ... for the rest",
 'AddImageHandle': "hIco = LoadIcon(0, IDI_INFORMATION)\n"
                   "iInf = CB.AddImageHandle(hIco, 1)",
 'ImageCount':     "n = CB.ImageCount()",

 # ---- bars ----
 'AddBar':      "bar = CB.AddBar('Standard', CBD:Top)     ! gripper + floatable by default\n"
                "plain = CB.AddBar('Fixed', CBD:Top, 0)   ! no gripper, cannot float",
 'AddMenuBar':  "mbar = CB.AddMenuBar()                   ! a bar styled as THE menu bar\n"
                "CB.MirrorMenu(mbar, 1)",
 'CreateMenu':  "mnu = CB.CreateMenu()                    ! a popup with no bar of its own\n"
                "CB.AddMenuRow(mnu, CMD:Cut, 'Cu&t', iCut, 'Ctrl+X')",
 'ClearContainer':   "CB.ClearContainer(mnu)                   ! empty it, keep the container",
 'DestroyContainer': "CB.DestroyContainer(mnu)                 ! and the container itself",
 'SetBarDock':  "CB.SetBarDock(bar, CBD:Top, 1, 0)        ! edge, row, offset in the row",
 'BarDock':     "IF CB.BarDock(bar) = CBD:Float THEN DO SaveFloatPos.",
 'SetBarVisible': "CB.SetBarVisible(bar, 0)                 ! hide it",
 'BarVisible':  "IF ~CB.BarVisible(bar) THEN CB.SetBarVisible(bar, 1).",
 'FloatBar':    "CB.FloatBar(bar, 200, 160)               ! tear it off at a screen point",
 'SetBarRect':  "CB.SetBarRect(bar, 0, 0, 400, 26)        ! place it yourself, in pixels",
 'PlaceOnControl': "CB.PlaceOnControl(bar, ?BarRegion)       ! land it on a REGION you drew",

 # ---- ribbon ----
 'AddRibbon':      "rib = CB.AddRibbon('Ribbon', CBD:Top)    ! a bar of tabs",
 'AddRibbonTab':   "tHome = CB.AddRibbonTab(rib, '&Home')",
 'AddRibbonGroup': "grp = CB.AddRibbonGroup(tHome, 'Clipboard')\n"
                   "CB.AddLargeButton(grp, CMD:Paste, 'Paste', iPaste)",
 'SetActiveTab':   "CB.SetActiveTab(rib, tHome)",
 'ActiveTab':      "IF CB.ActiveTab(rib) = tView THEN DO RefreshView.",
 'TabCount':       "n = CB.TabCount(rib)",
 'TabAt':          "LOOP i = 0 TO CB.TabCount(rib) - 1\n"
                   "  tab = CB.TabAt(rib, i)\n"
                   "END",
 'MinimizeRibbon': "CB.MinimizeRibbon(rib, 1)                ! collapse to the tab strip",
 'RibbonMinimized':"IF CB.RibbonMinimized(rib) THEN Msg = 'tabs only'.",

 # ---- menus ----
 'TrackMenu':   "CB.TrackMenu(mnu, 300, 240)              ! at a screen point",
 'PopupMenu':   "  OF EVENT:AlertKey\n"
                "    IF KEYCODE() = MouseRight\n"
                "      CB.PopupMenu(rowMenu)              ! where the mouse is\n"
                "    END",
 'AddMenuTitle':"CB.AddMenuTitle(mbar, '&File', fileMenu)  ! a title on a menu bar",
 'AddSubMenu':  "sub = CB.CreateMenu()\n"
                "CB.AddSubMenu(fileMenu, 'Open &Recent', sub)",

 # ---- items ----
 'AddItem':     "it = CB.AddItem(bar, CBI:Button, CMD:New, '&New', iNew)\n"
                "!  the generic one - AddButton and friends are shorthands",
 'InsertItem':  "CB.InsertItem(bar, it, CBI:Separator, 0, '')   ! put it BEFORE 'it'",
 'AddButton':   "CB.AddButton(bar, CMD:Save, '&Save', iSave)",
 'AddToggle':   "bold = CB.AddToggle(bar, CMD:Bold, 'B')       ! stays down until clicked again",
 'AddDropButton':  "CB.AddDropButton(bar, 0, '&Export', expMenu, iSave)",
 'AddSplitButton': "CB.AddSplitButton(bar, CMD:Print, '&Print', printMenu, iPrint)\n"
                   "!  left half runs the command, the arrow opens the menu",
 'AddSeparator':"CB.AddSeparator(bar)",
 'AddLabel':    "CB.AddLabel(bar, 'Find:')",
 'AddEdit':     "find = CB.AddEdit(bar, CMD:Find, '', 160)",
 'AddCombo':    "fnt = CB.AddCombo(bar, CMD:Font, 'Segoe UI|Tahoma|Consolas', 130)\n"
                "CB.SetComboSel(fnt, 0)",
 'AddCheckBox': "CB.AddCheckBox(bar, CMD:Wrap, 'Wrap', 1)      ! starts ticked",
 'AddColorButton': "col = CB.AddColorButton(bar, CMD:Colour, '', COLOR:Navy)",
 'AddSpace':    "CB.AddSpace(bar)                             ! flexible gap - pushes the rest right\n"
                "CB.AddButton(bar, CMD:Help, '&Help', iHelp)  ! ... so Help sits at the far end",
 'AddLargeButton': "CB.AddLargeButton(grp, CMD:Paste, 'Paste', iPaste)\n"
                   "!  the big image-over-text button a ribbon group leads with",
 'AddMenuRow':  "CB.AddMenuRow(mnu, CMD:Save, '&Save', iSave, 'Ctrl+S')",
 'RemoveItem':  "CB.RemoveItem(it)",

 # ---- item state ----
 'SetItemText':    "CB.SetItemText(it, 'Saved')",
 'ItemText':       "s = CB.ItemText(it)",
 'SetItemImage':   "CB.SetItemImage(it, iSaveDirty)",
 'SetItemStyle':   "CB.SetItemStyle(it, CBIS:TextBelow + CBIS:RightAlign)\n"
                   "!  REPLACES the style word - include the bits you want to keep",
 'ItemStyle':      "IF BAND(CB.ItemStyle(it), CBIS:TextBelow) THEN Msg = 'big button'.",
 'SetItemEnabled': "CB.SetItemEnabled(it, 0)                 ! grey it out",
 'ItemEnabled':    "IF CB.ItemEnabled(it) THEN DO RunIt.",
 'SetItemChecked': "CB.SetItemChecked(bold, 1)",
 'ItemChecked':    "IF CB.ItemChecked(bold) THEN Style = 'bold'.",
 'SetItemVisible': "CB.SetItemVisible(it, 0)",
 'ItemVisible':    "IF ~CB.ItemVisible(it) THEN CB.SetItemVisible(it, 1).",
 'SetItemTooltip': "CB.SetItemTooltip(it, 'Save (Ctrl+S)')",
 'SetItemShortcut':"CB.SetItemShortcut(row, 'Ctrl+S')        ! the grey text on a menu row",
 'SetItemWidth':   "CB.SetItemWidth(find, 200)               ! pixels",
 'SetItemMenu':    "CB.SetItemMenu(it, recentMenu)",
 'ItemMenu':       "m = CB.ItemMenu(it)",
 'SetItemCmd':     "CB.SetItemCmd(it, CMD:SaveAs)",
 'ItemCmd':        "c = CB.ItemCmd(it)",
 'FindItem':       "it = CB.FindItem(CMD:Save)               ! the FIRST item with that id",
 'EnableCmd':      "CB.EnableCmd(CMD:Save, 0)                ! every item with that id at once",
 'CheckCmd':       "CB.CheckCmd(CMD:Wrap, 1)",
 'ItemCount':      "n = CB.ItemCount(bar)",
 'ItemAt':         "LOOP i = 0 TO CB.ItemCount(bar) - 1\n"
                   "  it = CB.ItemAt(bar, i)\n"
                   "END",

 # ---- values ----
 'SetItemValue':   "CB.SetItemValue(find, 'customer')        ! an EDIT item's text",
 'ItemValue':      "What = CB.ItemValue(find)",
 'AddComboItem':   "CB.AddComboItem(fnt, 'Times New Roman')",
 'SetComboList':   "CB.SetComboList(fnt, 'Segoe UI|Tahoma|Consolas')",
 'ClearComboItems':"CB.ClearComboItems(fnt)",
 'SetComboSel':    "CB.SetComboSel(fnt, 0)                   ! 0 is the first",
 'ComboSel':       "i = CB.ComboSel(fnt)",
 'SetItemColor':   "CB.SetItemColor(col, COLOR:Maroon)",
 'ItemColor':      "c = CB.ItemColor(col)",
 'AddSlider':      "zoom = CB.AddSlider(bar, CMD:Zoom, 25, 400, 100, 130)\n"
                   "!  lo, hi, starting value, width",
 'AddSpin':        "page = CB.AddSpin(bar, CMD:Page, 1, 9999, 1, 60)",
 'AddProgress':    "busy = CB.AddProgress(bar, 0, 100, 0, 130)",
 'SetItemRange':   "CB.SetItemRange(zoom, 10, 800)",
 'SetItemNumber':  "CB.SetItemNumber(busy, done * 100 / total)",
 'ItemNumber':     "Scale = CB.ItemNumber(zoom)",

 # ---- gallery ----
 'AddGallery':     "gal = CB.AddGallery(grp, CMD:Style, 4, 62, 54)\n"
                   "!  columns, cell width, cell height",
 'AddGalleryCell': "CB.AddGalleryCell(gal, iNew, 'Normal')",
 'SetGalleryGrid': "CB.SetGalleryGrid(gal, 5, 60, 48)        ! for one made by AddItem",
 'GallerySel':     "Style = CB.GallerySel(gal)               ! -1 if nothing is chosen",
 'SetGallerySel':  "CB.SetGallerySel(gal, 0)",

 # ---- mirroring ----
 'MirrorMenu':      "CB.MirrorMenu(CB.AddMenuBar(), 1)        ! 1 = take the real menu off",
 'MirrorMenuFrom':  "CB.MirrorMenuFrom(bar, ?MenuBar, 1)      ! when you know the equate",
 'FindMenuBar':     "feq = CB.FindMenuBar()                   ! 0 if the window has no MENUBAR",
 'MenuReport':      "SETCLIPBOARD(CLIP(CB.MenuReport()))\n"
                    "!  says which equate it found and what hung off it - read this first\n"
                    "!  when a mirrored bar comes out empty",
 'RefreshMirror':   "n = CB.RefreshMirror()                   ! read the MENUBAR again",
 'MirrorToolbar':   "tb = CB.AddBar('Tools', CBD:Top)\n"
                    "CB.SetBarDock(tb, CBD:Top, 1, 0)\n"
                    "CB.MirrorToolbar(tb, 1)                  ! 1 = hide the real TOOLBAR",
 'MirrorToolbarFrom':"CB.MirrorToolbarFrom(tb, ?Toolbar, 1)",
 'FindToolbar':     "feq = CB.FindToolbar()",
 'FindControlOfType':"feq = CB.FindControlOfType(CREATE:toolbar)",
 'ShowHostToolbar': "CB.ShowHostToolbar(CB.MirrorTBFeq, 1)    ! put the real one back",
 'MirrorInto':      "CB.MirrorInto(mnu, ?FileMenu)            ! one MENU's rows into a container",
 'MinFeqIn':        "!  used by MirrorMenu to sort menus into declaration order.\n"
                    "rank = CB.MinFeqIn(?FileMenu)",
 'MenuRank':        "!  where this control sits among the NAMED controls.\n"
                    "r = CB.MenuRank(?MNew)",
 'SetHostMenu':     "CB.SetHostMenu(0)                        ! detach the real MENUBAR\n"
                    "CB.SetHostMenu(1)                        ! and put it back",
 'HostMenuVisible': "IF CB.HostMenuVisible() THEN CB.SetHostMenu(0).",

 # ---- host space ----
 'ReserveSpace':      "CB.ReserveSpace(0)                       ! 1 always, 0 never, -1 auto",
 'ReserveMode':       "m = CB.ReserveMode()",
 'HostReserveBottom': "CB.HostReserveBottom(23)                 ! a WINDOW's STATUS bar height",
 'HostReserveHeight': "px = CB.HostReserveHeight()",

 # ---- layout memory ----
 'LayoutText':        "Blob = CB.LayoutText()                   ! keep it wherever you like",
 'RestoreLayout':     "CB.RestoreLayout(Blob)",
 'SaveLayoutTo':      "CB.SaveLayoutTo('.\\MyApp.INI', 'CommandBars')   ! before the window closes",
 'RestoreLayoutFrom': "CB.RestoreLayoutFrom('.\\MyApp.INI', 'CommandBars') ! after the bars are built",

 # ---- keyboard ----
 'AddAccelerator':   "CB.AddAccelerator(CMD:Save, 083h, 2)     ! 'S' + Ctrl, in Win32 terms",
 'AddClarionKey':    "CB.AddClarionKey(CMD:Save, CtrlS)        ! the Clarion equate - easier",
 'ClearAccelerators':"CB.ClearAccelerators()",
 'TakeKey':          "IF CB.TakeKey(070h, 0) THEN CYCLE.       ! F1, no modifiers",
 'TakeAlertKey':     "  OF EVENT:AlertKey\n"
                     "    IF CB.TakeAlertKey(KEYCODE()) THEN CYCLE.",

 # ---- the pump ----
 'TakeOne':   "  OF EVENT:Timer\n"
              "    LOOP WHILE CB.TakeOne()               ! 0 when the queue is empty\n"
              "      CASE CB.LastCmd\n"
              "      OF CMD:Save ; DO SaveRoutine\n"
              "      END\n"
              "    END\n"
              "    CYCLE",
 'TakeEvent': "IF CB.TakeEvent()                        ! one event, no dispatch\n"
              "  Msg = 'event ' & CB.LastEvent\n"
              "END",
 'TakeCommand':    "!  DERIVE this instead of writing a CASE in every window:\n"
                   "MyBar.TakeCommand PROCEDURE(LONG cmd, SIGNED item)\n"
                   "  CODE\n"
                   "  CASE cmd\n"
                   "  OF CMD:Save ; DO SaveRoutine\n"
                   "  END\n"
                   "  RETURN PARENT.TakeCommand(cmd, item)",
 'TakeToggled':    "MyBar.TakeToggled PROCEDURE(LONG cmd, SIGNED item, BYTE checked)\n"
                   "  CODE\n"
                   "  IF cmd = CMD:Bold THEN Bold = checked.\n"
                   "  RETURN PARENT.TakeToggled(cmd, item, checked)",
 'TakeDropDown':   "!  derived: a drop button is about to open its menu - a chance to\n"
                   "!  rebuild the rows first (a Recent list, say).",
 'TakeTextChanged':"!  derived: an EDIT item's text changed.  CB.ItemValue(item) has it.",
 'TakeSelChanged': "!  derived: a COMBO's selection changed; the index is in LastParam.",
 'TakeColorChanged':"!  derived: a colour button was set; the colour is in LastParam.",
 'TakeLayoutChanged':"!  derived: bars moved, so the space left over changed - re-fit\n"
                    "!  whatever fills it.",
 'TakeRightClick': "!  derived: an item was right-clicked - hang a context menu off it.",

 # ---- odds ----
 'KeyText':  "s = CB.KeyText(CtrlS)                    ! 'Ctrl+S', for a menu row",
 'PipeItem': "s = CB.PipeItem('Red|Green|Blue', 2)     ! 'Green' - 1 is the first",
}
