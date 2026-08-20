!=====================================================================
!  CommandBarShowcase.clw  -  everything ClaCommandBar can do, in four
!                             hand-coded Clarion windows.
!
!  1  Main      - a real Clarion MENUBAR MIRRORED onto a command bar,
!                 plus a toolbar carrying every item type.  Choosing a
!                 mirrored row fires the ORIGINAL menu ITEM, so the
!                 ordinary Clarion menu code underneath still runs.
!  2  Ribbon    - tabs, groups, large and small items, live theming.
!  3  Docking   - bars on all four edges at once, a second row, a
!                 floating bar, and the client area they leave behind.
!  4  On a REGION - a bar placed exactly on a REGION you positioned in
!                 the window designer, which is what the CONTROL
!                 template generates.
!
!  Build with build.bat, run CommandBarShowcase.exe.
!=====================================================================
  PROGRAM

  PRAGMA('link(commandbar.lib)')

  INCLUDE('EQUATES.CLW'),ONCE
  INCLUDE('KEYCODES.CLW'),ONCE
  INCLUDE('CommandBar.inc'),ONCE

  MAP
ShowcaseMain PROCEDURE()
RibbonDemo   PROCEDURE()
DockDemo     PROCEDURE()
RegionDemo   PROCEDURE()
LoadIcons    PROCEDURE(*CommandBarClass CB, *SIGNED iNew, *SIGNED iOpen, |
                       *SIGNED iSave, *SIGNED iPrint, *SIGNED iCut,      |
                       *SIGNED iCopy, *SIGNED iPaste, *SIGNED iFind,     |
                       *SIGNED iHelp, *SIGNED iUndo, *SIGNED iRedo)
  END

  CODE
  ShowcaseMain()

!=====================================================================
!  Shared: load the icon set once per window.
!=====================================================================
LoadIcons PROCEDURE(*CommandBarClass CB, *SIGNED iNew, *SIGNED iOpen,   |
                    *SIGNED iSave, *SIGNED iPrint, *SIGNED iCut,        |
                    *SIGNED iCopy, *SIGNED iPaste, *SIGNED iFind,       |
                    *SIGNED iHelp, *SIGNED iUndo, *SIGNED iRedo)
  CODE
  iNew   = CB.AddImage('images\new.ico')
  iOpen  = CB.AddImage('images\open.ico')
  iSave  = CB.AddImage('images\save.ico')
  iPrint = CB.AddImage('images\print.ico')
  iCut   = CB.AddImage('images\cut.ico')
  iCopy  = CB.AddImage('images\copy.ico')
  iPaste = CB.AddImage('images\paste.ico')
  iFind  = CB.AddImage('images\find.ico')
  iHelp  = CB.AddImage('images\help.ico')
  iUndo  = CB.AddImage('images\undo.ico')
  iRedo  = CB.AddImage('images\redo.ico')

!=====================================================================
!  1  MAIN - a real Clarion MENUBAR, mirrored
!=====================================================================
ShowcaseMain PROCEDURE()

CMD:Ribbon   EQUATE(201)
CMD:Dock     EQUATE(202)
CMD:Region   EQUATE(203)
CMD:Theme    EQUATE(300)      ! +0..+10
CMD:Bold     EQUATE(120)
CMD:Italic   EQUATE(121)
CMD:Colour   EQUATE(130)
CMD:Find     EQUATE(131)
CMD:Zoom     EQUATE(132)
CMD:Wrap     EQUATE(133)
CMD:Mirror   EQUATE(140)

CB           CommandBarClass
barMenu      SIGNED
barTools     SIGNED
mirrored     SIGNED
mbFeq        SIGNED
iNew         SIGNED
iOpen        SIGNED
iSave        SIGNED
iPrint       SIGNED
iCut         SIGNED
iCopy        SIGNED
iPaste       SIGNED
iFind        SIGNED
iHelp        SIGNED
iUndo        SIGNED
iRedo        SIGNED
it           SIGNED
i            SIGNED
seq          LONG
ThemeNames   STRING(160)
LogKind      STRING(16)
LogDetail    STRING(90)

LogQ         QUEUE,PRE(LOG)
Seq            LONG
Kind           STRING(16)
Detail         STRING(90)
             END

Window WINDOW('ClaCommandBar showcase - a mirrored Clarion menu'),AT(,,620,380),GRAY, |
         SYSTEM,MAX,RESIZE,FONT('Segoe UI',9),TIMER(10)
       MENUBAR
         MENU('&File')
           ITEM('&New'),USE(?MNew),KEY(CtrlN)
           ITEM('&Open...'),USE(?MOpen),KEY(CtrlO)
           MENU('Open &Recent')
             ITEM('ledger.tps'),USE(?MR1)
             ITEM('customers.tps'),USE(?MR2)
             ITEM('invoices.tps'),USE(?MR3)
           END
           ITEM('&Save'),USE(?MSave),KEY(CtrlS)
           ITEM,SEPARATOR
           ITEM('&Print...'),USE(?MPrint),KEY(CtrlP)
           ITEM,SEPARATOR
           ITEM('E&xit'),USE(?MExit)
         END
         MENU('&Edit')
           ITEM('&Undo'),USE(?MUndo),KEY(CtrlZ)
           ITEM('&Redo'),USE(?MRedo),KEY(CtrlY)
           ITEM,SEPARATOR
           ITEM('Cu&t'),USE(?MCut),KEY(CtrlX)
           ITEM('&Copy'),USE(?MCopy),KEY(CtrlC)
           ITEM('&Paste'),USE(?MPaste),KEY(CtrlV),DISABLE
         END
         MENU('&Demos')
           ITEM('&Ribbon bar...'),USE(?MRibbon)
           ITEM('&Docking on four edges...'),USE(?MDock)
           ITEM('Bar on a &REGION...'),USE(?MRegion)
         END
         MENU('&Help')
           ITEM('&About...'),USE(?MAbout)
         END
       END
       LIST,AT(4,4,612,372),USE(?Log),FROM(LogQ),HVSCROLL, |
         FORMAT('28R(2)|M~#~@n5@86L(2)|M~Event~@s16@300L(2)|M~Detail~@s90@')
     END

  CODE
  ThemeNames = 'Steel Blue|Office 2003|Office 2007|Office 2010|Office 2013|' & |
               'Office 2016|VS 2012 Light|VS 2012 Dark|Windows 11 Light|' &    |
               'Windows 11 Dark|Slate Dark'
  OPEN(Window)

  CB.TimerInterval = 10
  IF ~CB.Init(Window, CBS:Tooltips + CBS:Chevron + CBS:HotText + CBS:MenuIcons)
    MESSAGE('commandbar.dll could not start.', 'Showcase', ICON:Exclamation)
    CLOSE(Window)
    RETURN
  END
  CB.SetTheme(CBT:SteelBlue)
  CB.SetMetric(CBM:IconSize, 24)          ! the shipped art is 32x32
  LoadIcons(CB, iNew, iOpen, iSave, iPrint, iCut, iCopy, iPaste, iFind, |
            iHelp, iUndo, iRedo)

  !---- THE POINT OF THIS WINDOW ----------------------------------
  !  One call reads the MENUBAR above and rebuilds it as a command bar:
  !  same order, same nesting, same separators, the KEY() attributes
  !  turned into a shortcut column, and ?MPaste still disabled.  Picking
  !  a mirrored row POSTs EVENT:Accepted to the original ITEM, so the
  !  CASE ACCEPTED() further down runs exactly as it always did.
  barMenu  = CB.AddBar('Menu', CBD:Top, CBBS:MenuBar)
  CB.SetBarDock(barMenu, CBD:Top, 0, 0)
  mirrored = CB.MirrorMenu(barMenu, 1)     ! 1 = hide the original menu

  !---- a toolbar carrying every item type ----
  barTools = CB.AddBar('Standard', CBD:Top, CBBS:Gripper + CBBS:Floatable)
  CB.SetBarDock(barTools, CBD:Top, 1, 0)

  it = CB.AddButton(barTools, CB.MirrorBase + ?MNew, 'New', iNew)
  CB.SetItemTooltip(it, 'New (Ctrl+N) - fires the same ITEM as the menu')
  it = CB.AddButton(barTools, CB.MirrorBase + ?MOpen, 'Open', iOpen)
  CB.SetItemTooltip(it, 'Open (Ctrl+O)')
  it = CB.AddButton(barTools, CB.MirrorBase + ?MSave, 'Save', iSave)
  CB.SetItemTooltip(it, 'Save (Ctrl+S)')
  CB.AddSeparator(barTools)
  CB.AddButton(barTools, CB.MirrorBase + ?MUndo, 'Undo', iUndo)
  CB.AddButton(barTools, CB.MirrorBase + ?MRedo, 'Redo', iRedo)
  CB.AddSeparator(barTools)
  it = CB.AddToggle(barTools, CMD:Bold, '', 0)
  CB.SetItemText(it, 'B')
  CB.SetItemTooltip(it, 'Bold - a toggle button')
  it = CB.AddToggle(barTools, CMD:Italic, 'I', 0)
  CB.SetItemTooltip(it, 'Italic')
  it = CB.AddColorButton(barTools, CMD:Colour, '', COLOR:Maroon)
  CB.SetItemTooltip(it, 'Colour - the arrow opens the picker')
  CB.AddSeparator(barTools)
  CB.AddLabel(barTools, 'Find:')
  it = CB.AddEdit(barTools, CMD:Find, 'customer', 110)
  CB.SetItemTooltip(it, 'An in-bar edit - click, type, press Enter')
  it = CB.AddCombo(barTools, CMD:Zoom, '50%|75%|100%|150%|200%', 80)
  CB.SetComboSel(it, 2)
  it = CB.AddCheckBox(barTools, CMD:Wrap, 'Wrap')
  it = CB.AddButton(barTools, CMD:Mirror, 'Show the real menu again', 0)
  CB.SetItemStyle(it, CBIS:RightAlign)

  CB.Layout()
  CB.FitControl(?Log, 4, 4)

  LogKind   = 'Ready'
  LogDetail = 'Mirrored ' & mirrored & ' top-level menus.  Host menu visible now = ' & CB.HostMenuVisible()
  DO AddLog
  LogKind   = 'Try'
  LogDetail = 'File > Open Recent, the disabled Paste, Ctrl+N, and Demos.'
  DO AddLog

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      DO Pump
      CYCLE
    OF EVENT:Sized
      CB.Layout()
      CB.FitControl(?Log, 4, 4)
      DISPLAY
    END

    CASE ACCEPTED()
    OF ?MNew    ; LogKind = 'Menu ITEM' ; LogDetail = '?MNew - the ORIGINAL menu item ran'   ; DO AddLog
    OF ?MOpen   ; LogKind = 'Menu ITEM' ; LogDetail = '?MOpen'                                ; DO AddLog
    OF ?MSave   ; LogKind = 'Menu ITEM' ; LogDetail = '?MSave'                                ; DO AddLog
    OF ?MPrint  ; LogKind = 'Menu ITEM' ; LogDetail = '?MPrint'                               ; DO AddLog
    OF ?MUndo   ; LogKind = 'Menu ITEM' ; LogDetail = '?MUndo'                                ; DO AddLog
    OF ?MRedo   ; LogKind = 'Menu ITEM' ; LogDetail = '?MRedo'                                ; DO AddLog
    OF ?MCut    ; LogKind = 'Menu ITEM' ; LogDetail = '?MCut'                                 ; DO AddLog
    OF ?MCopy   ; LogKind = 'Menu ITEM' ; LogDetail = '?MCopy'                                ; DO AddLog
    OF ?MR1     ; LogKind = 'Menu ITEM' ; LogDetail = '?MR1 - from the Open Recent SUBMENU'    ; DO AddLog
    OF ?MR2     ; LogKind = 'Menu ITEM' ; LogDetail = '?MR2'                                  ; DO AddLog
    OF ?MR3     ; LogKind = 'Menu ITEM' ; LogDetail = '?MR3'                                  ; DO AddLog
    OF ?MRibbon ; RibbonDemo()
    OF ?MDock   ; DockDemo()
    OF ?MRegion ; RegionDemo()
    OF ?MAbout
      MESSAGE('ClaCommandBar showcase.||The bar at the top of this window ' & |
              'was built by CB.MirrorMenu() from the window''s own ' &        |
              'MENUBAR - order, nesting, separators, shortcut keys and ' &    |
              'the disabled Paste all came across, and every row still ' &    |
              'fires the original ITEM.', 'About', ICON:Asterisk)
    OF ?MExit
      POST(EVENT:CloseWindow)
    END
  END

  CB.Kill()
  CLOSE(Window)
  RETURN

!---------------------------------------------------------------------
Pump ROUTINE
  LOOP WHILE CB.TakeOne()
    LogDetail = ''
    CASE CB.LastEvent
    OF CBE:Command
      LogKind = 'Command'
      DO DoCommand
    OF CBE:Toggled
      LogKind   = 'Toggled'
      LogDetail = 'cmd ' & CB.LastCmd & ' now ' & CHOOSE(CB.LastParam = 1, 'ON', 'OFF')
    OF CBE:TextChanged
      LogKind   = 'TextChanged'
      LogDetail = 'text = "' & CLIP(CB.ItemValue(CB.LastItem)) & '"'
    OF CBE:SelChanged
      LogKind   = 'SelChanged'
      LogDetail = 'index ' & CB.LastParam & ' = "' & CLIP(CB.ItemValue(CB.LastItem)) & '"'
    OF CBE:ColorChanged
      LogKind   = 'ColorChanged'
      LogDetail = 'colour = ' & CB.LastParam
    OF CBE:RightClick
      LogKind   = 'RightClick'
      LogDetail = 'item ' & CB.LastItem
    OF CBE:Layout
      CB.FitControl(?Log, 4, 4)
      CYCLE
    OF CBE:DropDown
      CYCLE
    ELSE
      LogKind   = 'Event ' & CB.LastEvent
    END
    DO AddLog
  END

!---------------------------------------------------------------------
DoCommand ROUTINE
  CASE CB.LastCmd
  OF CMD:Mirror
    !  Put the real Clarion menu back and drop the mirrored bar.
    SETTARGET(Window)
    mbFeq = 0{PROP:MenuBar}
    IF mbFeq THEN mbFeq{PROP:Hide} = 0.
    SETTARGET()
    CB.SetBarVisible(barMenu, 0)
    LogDetail = 'original MENUBAR shown again, mirrored bar hidden'
  OF CMD:Bold OROF CMD:Italic
    LogDetail = 'formatting toggle'
  ELSE
    LogDetail = 'cmd ' & CB.LastCmd & ', item ' & CB.LastItem
  END

!---------------------------------------------------------------------
AddLog ROUTINE
  seq += 1
  CLEAR(LogQ)
  LOG:Seq    = seq
  LOG:Kind   = LogKind
  LOG:Detail = LogDetail
  ADD(LogQ)
  ?Log{PROP:Selected} = RECORDS(LogQ)
  DISPLAY

!=====================================================================
!  2  RIBBON
!=====================================================================
RibbonDemo PROCEDURE()

CMD:Paste  EQUATE(101)
CMD:Cut    EQUATE(102)
CMD:Copy   EQUATE(103)
CMD:Bold   EQUATE(110)
CMD:Ital   EQUATE(111)
CMD:Find   EQUATE(120)
CMD:Theme  EQUATE(300)

CB       CommandBarClass
ribbon   SIGNED
tHome    SIGNED
tInsert  SIGNED
tView    SIGNED
grp      SIGNED
it       SIGNED
i        SIGNED
iNew     SIGNED
iOpen    SIGNED
iSave    SIGNED
iPrint   SIGNED
iCut     SIGNED
iCopy    SIGNED
iPaste   SIGNED
iFind    SIGNED
iHelp    SIGNED
iUndo    SIGNED
iRedo    SIGNED
ThemeNames STRING(160)
Msg      STRING(120)

Window WINDOW('Ribbon - tabs of groups of items'),AT(,,640,300),GRAY,SYSTEM,MAX, |
         RESIZE,FONT('Segoe UI',9),TIMER(10)
       STRING(@s120),AT(12,140,600,12),USE(Msg),FONT(,10)
       BUTTON('Close'),AT(560,270,60,16),USE(?Close),STD(STD:Close)
     END

  CODE
  ThemeNames = 'Steel Blue|Office 2003|Office 2007|Office 2010|Office 2013|' & |
               'Office 2016|VS 2012 Light|VS 2012 Dark|Windows 11 Light|' &    |
               'Windows 11 Dark|Slate Dark'
  OPEN(Window)
  CB.TimerInterval = 10
  IF ~CB.Init(Window, CBS:Tooltips + CBS:MenuIcons + CBS:HotText)
    CLOSE(Window)
    RETURN
  END
  CB.SetTheme(CBT:Office2016)
  CB.SetMetric(CBM:IconSize, 20)
  CB.SetMetric(CBM:LargeIcon, 32)
  LoadIcons(CB, iNew, iOpen, iSave, iPrint, iCut, iCopy, iPaste, iFind, |
            iHelp, iUndo, iRedo)

  ribbon = CB.AddRibbon('Ribbon', CBD:Top)

  !---- Home ----
  tHome = CB.AddRibbonTab(ribbon, '&Home')

  grp = CB.AddRibbonGroup(tHome, 'Clipboard')
  CB.AddLargeButton(grp, CMD:Paste, 'Paste', iPaste)
  CB.AddButton(grp, CMD:Cut,  'Cut',  iCut)
  CB.AddButton(grp, CMD:Copy, 'Copy', iCopy)
  CB.AddButton(grp, 0, 'Format', 0)

  grp = CB.AddRibbonGroup(tHome, 'Font')
  it = CB.AddCombo(grp, 0, 'Segoe UI|Tahoma|Consolas|Times New Roman', 130)
  CB.SetComboSel(it, 0)
  it = CB.AddCombo(grp, 0, '8|9|10|12|14|18', 50)
  CB.SetComboSel(it, 1)
  CB.AddToggle(grp, CMD:Bold, 'B', 0)
  CB.AddToggle(grp, CMD:Ital, 'I', 0)
  CB.AddColorButton(grp, 0, '', COLOR:Navy)

  grp = CB.AddRibbonGroup(tHome, 'Editing')
  CB.AddLargeButton(grp, CMD:Find, 'Find', iFind)
  CB.AddButton(grp, 0, 'Replace', 0)
  CB.AddButton(grp, 0, 'Go To',   0)

  !---- Insert ----
  tInsert = CB.AddRibbonTab(ribbon, '&Insert')
  grp = CB.AddRibbonGroup(tInsert, 'Pages')
  CB.AddLargeButton(grp, 0, 'Cover', iNew)
  CB.AddLargeButton(grp, 0, 'Blank', iNew)
  grp = CB.AddRibbonGroup(tInsert, 'Illustrations')
  CB.AddLargeButton(grp, 0, 'Picture', iOpen)
  CB.AddLargeButton(grp, 0, 'Chart',   iPrint)
  grp = CB.AddRibbonGroup(tInsert, 'Links')
  CB.AddButton(grp, 0, 'Hyperlink', 0)
  CB.AddButton(grp, 0, 'Bookmark',  0)
  CB.AddButton(grp, 0, 'Reference', 0)

  !---- View, with the theme list ----
  tView = CB.AddRibbonTab(ribbon, '&View')
  grp = CB.AddRibbonGroup(tView, 'Show')
  CB.AddCheckBox(grp, 0, 'Ruler', 1)
  CB.AddCheckBox(grp, 0, 'Gridlines')
  CB.AddCheckBox(grp, 0, 'Navigation pane')
  grp = CB.AddRibbonGroup(tView, 'Theme')
  it = CB.AddCombo(grp, CMD:Theme, ThemeNames, 140)
  CB.SetComboSel(it, 5)                          ! Office 2016
  CB.AddLargeButton(grp, 0, 'Zoom', iFind)

  CB.Layout()
  Msg = 'Click the tabs.  Double-click a tab to collapse the ribbon.  ' & |
        'View > Theme changes the palette live.'
  DISPLAY

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      LOOP WHILE CB.TakeOne()
        CASE CB.LastEvent
        OF CBE:SelChanged
          IF CB.LastCmd = CMD:Theme
            CB.SetTheme(CB.LastParam + 1)
            Msg = 'Theme = ' & CLIP(CB.PipeItem(ThemeNames, CB.LastParam + 1))
          END
        OF CBE:Command
          Msg = 'Command ' & CB.LastCmd & ' from a ribbon item.'
        OF CBE:Toggled
          Msg = 'Toggle ' & CB.LastCmd & ' is now ' &                    |
                CHOOSE(CB.LastParam = 1, 'ON', 'OFF')
        END
        DISPLAY
      END
      CYCLE
    OF EVENT:Sized
      CB.Layout()
    END
  END
  CB.Kill()
  CLOSE(Window)

!=====================================================================
!  3  DOCKING on all four edges
!=====================================================================
DockDemo PROCEDURE()

CB       CommandBarClass
barTop   SIGNED
barTop2  SIGNED
barBot   SIGNED
itStatus SIGNED
itReady  SIGNED
barLeft  SIGNED
barRight SIGNED
barFloat SIGNED
it       SIGNED
iNew     SIGNED
iOpen    SIGNED
iSave    SIGNED
iPrint   SIGNED
iCut     SIGNED
iCopy    SIGNED
iPaste   SIGNED
iFind    SIGNED
iHelp    SIGNED
iUndo    SIGNED
iRedo    SIGNED

Window WINDOW('Docking - four edges, two rows, and a floating bar'),AT(,,600,380), |
         GRAY,SYSTEM,MAX,RESIZE,FONT('Segoe UI',9),TIMER(10)
       REGION,AT(4,4,592,372),USE(?Client),FILL(COLOR:White)
     END

  CODE
  OPEN(Window)
  CB.TimerInterval = 10
  IF ~CB.Init(Window, CBS:Tooltips + CBS:Chevron)
    CLOSE(Window)
    RETURN
  END
  CB.SetTheme(CBT:SlateDark)
  CB.SetMetric(CBM:IconSize, 24)
  LoadIcons(CB, iNew, iOpen, iSave, iPrint, iCut, iCopy, iPaste, iFind, |
            iHelp, iUndo, iRedo)

  barTop = CB.AddBar('Top row 0', CBD:Top, CBBS:Gripper + CBBS:Floatable)
  CB.SetBarDock(barTop, CBD:Top, 0, 0)
  CB.AddButton(barTop, 1, 'New',  iNew)
  CB.AddButton(barTop, 2, 'Open', iOpen)
  CB.AddButton(barTop, 3, 'Save', iSave)

  barTop2 = CB.AddBar('Top row 1', CBD:Top, CBBS:Gripper)
  CB.SetBarDock(barTop2, CBD:Top, 1, 0)
  CB.AddButton(barTop2, 4, 'Cut',   iCut)
  CB.AddButton(barTop2, 5, 'Copy',  iCopy)
  CB.AddButton(barTop2, 6, 'Paste', iPaste)
  it = CB.AddButton(barTop2, 7, 'Right aligned', iHelp)
  CB.SetItemStyle(it, CBIS:RightAlign)

  barLeft = CB.AddBar('Left', CBD:Left, CBBS:Gripper + CBBS:LargeIcons)
  it = CB.AddButton(barLeft, 10, 'Find',  iFind)
  CB.SetItemStyle(it, CBIS:TextBelow)
  it = CB.AddButton(barLeft, 11, 'Print', iPrint)
  CB.SetItemStyle(it, CBIS:TextBelow)

  barRight = CB.AddBar('Right', CBD:Right, CBBS:Gripper)
  CB.AddButton(barRight, 20, 'Undo', iUndo)
  CB.AddButton(barRight, 21, 'Redo', iRedo)

  barBot = CB.AddBar('Bottom', CBD:Bottom, CBBS:NoBorder)
  itStatus = CB.AddLabel(barBot, 'A bottom bar behaves like a status strip.')
  CB.AddSpace(barBot)
  itReady  = CB.AddLabel(barBot, 'Ready')

  barFloat = CB.AddBar('Floating', CBD:Float, CBBS:Floatable)
  CB.AddButton(barFloat, 30, 'Bold',   0)
  CB.AddButton(barFloat, 31, 'Italic', 0)
  CB.FloatBar(barFloat, 260, 300)

  CB.Layout()
  DO Fit

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      LOOP WHILE CB.TakeOne()
        IF CB.LastEvent = CBE:Layout
          DO Fit
        ELSIF CB.LastEvent = CBE:Command
          CB.SetItemText(itReady, 'Command ' & CB.LastCmd)
        END
      END
      CYCLE
    OF EVENT:Sized
      CB.Layout()
      DO Fit
    END
  END
  CB.Kill()
  CLOSE(Window)
  RETURN

Fit ROUTINE
  CB.FitControl(?Client, 2, 2)
  !  The white REGION is exactly what CB.ClientX/Y/Width/Height report,
  !  and the status text goes in the bottom BAR - a fixed STRING would
  !  sit behind the top bars.
  CB.SetItemText(itStatus, 'Client area left by five bars: ' &           |
        CB.ClientWidth() & ' x ' & CB.ClientHeight() &                   |
        ' px.  Drag the floating bar by its caption.')

!=====================================================================
!  4  A BAR ON A REGION - what the CONTROL template generates
!=====================================================================
RegionDemo PROCEDURE()

CB       CommandBarClass
bar      SIGNED
it       SIGNED
iNew     SIGNED
iOpen    SIGNED
iSave    SIGNED
iPrint   SIGNED
iCut     SIGNED
iCopy    SIGNED
iPaste   SIGNED
iFind    SIGNED
iHelp    SIGNED
iUndo    SIGNED
iRedo    SIGNED

BrowseQ  QUEUE,PRE(BRW)
Code       STRING(8)
Name       STRING(40)
Town       STRING(30)
         END
i        SIGNED

Window WINDOW('A bar placed on a REGION - the control template pattern'), |
         AT(,,520,320),GRAY,SYSTEM,MAX,RESIZE,FONT('Segoe UI',9),TIMER(10)
       REGION,AT(8,8,504,30),USE(?BarRegion)
       LIST,AT(8,44,504,240),USE(?Browse),FROM(BrowseQ),HVSCROLL, |
         FORMAT('50L(2)|M~Code~@s8@160L(2)|M~Name~@s40@120L(2)|M~Town~@s30@')
       BUTTON('&Close'),AT(452,292,60,16),USE(?Close),STD(STD:Close)
     END

  CODE
  OPEN(Window)
  LOOP i = 1 TO 12
    CLEAR(BrowseQ)
    BRW:Code = 'C' & FORMAT(i, @n03)
    BRW:Name = CHOOSE(i % 3 + 1, 'Acme Holdings', 'Bright Supplies', 'Corner Traders')
    BRW:Town = CHOOSE(i % 4 + 1, 'Bristol', 'Leeds', 'Cardiff', 'Derby')
    ADD(BrowseQ)
  END

  CB.TimerInterval = 10
  IF ~CB.Init(Window, CBS:Tooltips + CBS:MenuIcons)
    CLOSE(Window)
    RETURN
  END
  CB.SetTheme(CBT:Office2013)
  CB.SetMetric(CBM:IconSize, 20)
  LoadIcons(CB, iNew, iOpen, iSave, iPrint, iCut, iCopy, iPaste, iFind, |
            iHelp, iUndo, iRedo)

  bar = CB.AddBar('Browse tools', CBD:Top, CBBS:NoBorder)
  CB.AddButton(bar, 1, 'Insert', iNew)
  CB.AddButton(bar, 2, 'Change', iOpen)
  CB.AddButton(bar, 3, 'Delete', iCut)
  CB.AddSeparator(bar)
  CB.AddLabel(bar, 'Locate:')
  CB.AddEdit(bar, 4, '', 120)
  it = CB.AddButton(bar, 5, 'Print', iPrint)
  CB.SetItemStyle(it, CBIS:RightAlign)

  !  THE POINT: the bar lands exactly on ?BarRegion instead of docking
  !  to the window edge, so it takes nothing off the client area and the
  !  ABC resizer keeps moving it for you.
  CB.PlaceOnControl(bar, ?BarRegion)

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      LOOP WHILE CB.TakeOne()
      END
      CYCLE
    OF EVENT:Sized
      CB.PlaceOnControl(bar, ?BarRegion)
    END
  END
  CB.Kill()
  CLOSE(Window)
