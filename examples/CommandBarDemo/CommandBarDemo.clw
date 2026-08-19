!=====================================================================
!  CommandBarDemo.clw  -  a hand-coded Clarion app that drives
!                         CommandBarClass directly.
!
!  Nothing here needs the templates: it is the class, an ordinary
!  WINDOW, and an ACCEPT loop.  Build it with build.bat.
!
!  What it shows:
!    * a menu bar with submenus, shortcut text, check marks and
!      '&' accelerators
!    * a toolbar with every item type - button, split, toggle, colour,
!      edit, combo, checkbox, label, right-aligned button
!    * a second toolbar row and a left-docked bar with large icons
!    * icons loaded from .ICO files
!    * all eleven themes, and SetAccent recolouring one of them
!    * FitControl keeping the LIST in whatever space the bars leave
!    * a context menu at the mouse pointer
!    * Ctrl+N / Ctrl+O / Ctrl+S through the DLL's accelerator table
!=====================================================================
  PROGRAM

  PRAGMA('link(commandbar.lib)')

  INCLUDE('EQUATES.CLW'),ONCE
  INCLUDE('KEYCODES.CLW'),ONCE
  INCLUDE('CommandBar.inc'),ONCE

  MAP
MainWindow  PROCEDURE()
  END

  CODE
  MainWindow()

!=====================================================================
MainWindow PROCEDURE()

!---- command ids ----------------------------------------------------
CMD:New      EQUATE(101)
CMD:Open     EQUATE(102)
CMD:Save     EQUATE(103)
CMD:Print    EQUATE(104)
CMD:Exit     EQUATE(105)
CMD:Cut      EQUATE(110)
CMD:Copy     EQUATE(111)
CMD:Paste    EQUATE(112)
CMD:Undo     EQUATE(113)
CMD:Redo     EQUATE(114)
CMD:Bold     EQUATE(120)
CMD:Italic   EQUATE(121)
CMD:Colour   EQUATE(130)
CMD:Find     EQUATE(131)
CMD:Zoom     EQUATE(132)
CMD:Wrap     EQUATE(133)
CMD:SideBar  EQUATE(140)
CMD:FloatFmt EQUATE(141)
CMD:Accent   EQUATE(142)
CMD:Clear    EQUATE(143)
CMD:About    EQUATE(150)
CMD:Theme    EQUATE(200)          ! +0 .. +10, one per built-in theme

CB           CommandBarClass

barMenu      SIGNED
barMain      SIGNED
barFmt       SIGNED
barSide      SIGNED
mFile        SIGNED
mRecent      SIGNED
mEdit        SIGNED
mView        SIGNED
mTheme       SIGNED
mHelp        SIGNED
mOpen        SIGNED
mCtx         SIGNED
itFind       SIGNED
itZoom       SIGNED
itColour     SIGNED

imgNew       SIGNED
imgOpen      SIGNED
imgSave      SIGNED
imgPrint     SIGNED
imgCut       SIGNED
imgCopy      SIGNED
imgPaste     SIGNED
imgFind      SIGNED
imgHelp      SIGNED
imgExit      SIGNED
imgBold      SIGNED
imgItalic    SIGNED
imgUndo      SIGNED
imgRedo      SIGNED

ThemeNames   STRING(160)
i            SIGNED
seq          LONG

!---- what DO AddLog writes ------------------------------------------
LogKind      STRING(14)
LogCmd       LONG
LogDetail    STRING(80)

LogQ         QUEUE,PRE(LOG)
Seq            LONG
Kind           STRING(14)
Cmd            LONG
Detail         STRING(80)
             END

Window WINDOW('ClaCommandBar for Clarion - demo'),AT(,,600,360),GRAY,SYSTEM,MAX,RESIZE, |
         FONT('Segoe UI',9),TIMER(10),ALRT(CtrlN),ALRT(CtrlO),ALRT(CtrlS),ALRT(MouseRight)
       LIST,AT(4,60,592,296),USE(?Log),FROM(LogQ),HVSCROLL, |
         FORMAT('28R(2)|M~#~@n5@76L(2)|M~Event~@s14@48R(2)|M~Command~@n6@280L(2)|M~Detail~@s80@')
     END

  CODE
  ThemeNames = 'Steel Blue|Office 2003|Office 2007|Office 2010|Office 2013|' & |
               'Office 2016|VS 2012 Light|VS 2012 Dark|Windows 11 Light|' &    |
               'Windows 11 Dark|Slate Dark'
  OPEN(Window)
  DO BuildBars
  DO FitList

  LogKind   = 'Ready'
  LogCmd    = 0
  LogDetail = 'Click things - every event the bars raise is logged here.'
  DO AddLog

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      DO Pump
      CYCLE
    OF EVENT:Sized
      CB.Layout()
      DO FitList
    OF EVENT:AlertKey
      IF KEYCODE() = MouseRight
        CB.PopupMenu(mCtx)                    ! blocks until picked/cancelled
        DO Pump
        CYCLE
      END
      IF CB.TakeAlertKey(KEYCODE())
        DO Pump
        CYCLE
      END
    END
  END

  CB.Kill()
  CLOSE(Window)
  RETURN

!---------------------------------------------------------------------
BuildBars ROUTINE
  DATA
it   SIGNED
  CODE
  CB.TimerInterval = 10
  IF ~CB.Init(Window, CBS:Tooltips + CBS:Chevron + CBS:HotText + CBS:MenuIcons)
    MESSAGE('COMMANDBAR.DLL could not start.  Is commandbar.dll beside the EXE?', |
            'ClaCommandBar', ICON:Exclamation)
    EXIT
  END
  CB.SetTheme(CBT:SteelBlue)
  !  Clarion's shipped artwork is 32x32.  The engine pre-scales with a
  !  proper Fant filter, but halving 32px line art to 16 still greys out
  !  every one-pixel stroke - so give it a slot close to its own size.
  !  Rule of thumb: make CBM:IconSize match the art you actually have.
  CB.SetMetric(CBM:IconSize, 24)

  !---- images.  WIC reads .ICO, .PNG, .GIF, .JPG and .BMP. ----
  imgNew    = CB.AddImage('images\new.ico')
  imgOpen   = CB.AddImage('images\open.ico')
  imgSave   = CB.AddImage('images\save.ico')
  imgPrint  = CB.AddImage('images\print.ico')
  imgCut    = CB.AddImage('images\cut.ico')
  imgCopy   = CB.AddImage('images\copy.ico')
  imgPaste  = CB.AddImage('images\paste.ico')
  imgFind   = CB.AddImage('images\find.ico')
  imgHelp   = CB.AddImage('images\help.ico')
  imgExit   = CB.AddImage('images\exit.ico')
  imgBold   = CB.AddImage('images\bold.ico')
  imgItalic = CB.AddImage('images\italic.ico')
  imgUndo   = CB.AddImage('images\undo.ico')
  imgRedo   = CB.AddImage('images\redo.ico')

  !---- the menu bar ----
  barMenu = CB.AddMenuBar()
  mFile   = CB.CreateMenu()
  mRecent = CB.CreateMenu()
  mEdit   = CB.CreateMenu()
  mView   = CB.CreateMenu()
  mTheme  = CB.CreateMenu()
  mHelp   = CB.CreateMenu()

  CB.AddMenuTitle(barMenu, '&File', mFile)
  CB.AddMenuTitle(barMenu, '&Edit', mEdit)
  CB.AddMenuTitle(barMenu, '&View', mView)
  CB.AddMenuTitle(barMenu, '&Help', mHelp)

  it = CB.AddButton(mFile, CMD:New, '&New', imgNew)
  CB.SetItemShortcut(it, 'Ctrl+N')
  CB.SetItemStyle(it, CBIS:Default)
  it = CB.AddButton(mFile, CMD:Open, '&Open...', imgOpen)
  CB.SetItemShortcut(it, 'Ctrl+O')
  CB.AddSubMenu(mFile, 'Open &Recent', mRecent)
  it = CB.AddButton(mFile, CMD:Save, '&Save', imgSave)
  CB.SetItemShortcut(it, 'Ctrl+S')
  CB.AddSeparator(mFile)
  CB.AddButton(mFile, CMD:Print, '&Print...', imgPrint)
  CB.AddSeparator(mFile)
  CB.AddButton(mFile, CMD:Exit, 'E&xit', imgExit)

  CB.AddButton(mRecent, 301, 'ledger.tps')
  CB.AddButton(mRecent, 302, 'customers.tps')
  CB.AddButton(mRecent, 303, 'invoices.tps')

  it = CB.AddButton(mEdit, CMD:Undo, '&Undo', imgUndo)
  CB.SetItemShortcut(it, 'Ctrl+Z')
  it = CB.AddButton(mEdit, CMD:Redo, '&Redo', imgRedo)
  CB.SetItemShortcut(it, 'Ctrl+Y')
  CB.AddSeparator(mEdit)
  it = CB.AddButton(mEdit, CMD:Cut, 'Cu&t', imgCut)
  CB.SetItemShortcut(it, 'Ctrl+X')
  it = CB.AddButton(mEdit, CMD:Copy, '&Copy', imgCopy)
  CB.SetItemShortcut(it, 'Ctrl+C')
  it = CB.AddButton(mEdit, CMD:Paste, '&Paste', imgPaste)
  CB.SetItemShortcut(it, 'Ctrl+V')
  CB.SetItemEnabled(it, 0)                    ! nothing on the clipboard

  CB.AddSubMenu(mView, '&Theme', mTheme)
  LOOP i = 1 TO 11
    it = CB.AddButton(mTheme, CMD:Theme + i - 1, CB.PipeItem(ThemeNames, i))
    CB.SetItemStyle(it, CBIS:Radio)
    IF i = 1 THEN CB.SetItemChecked(it, 1).
  END
  CB.AddSeparator(mView)
  it = CB.AddButton(mView, CMD:SideBar, '&Side bar')
  CB.SetItemStyle(it, CBIS:AutoCheck)
  CB.SetItemChecked(it, 1)
  it = CB.AddButton(mView, CMD:Wrap, '&Word wrap')
  CB.SetItemStyle(it, CBIS:AutoCheck)
  CB.AddSeparator(mView)
  CB.AddButton(mView, CMD:FloatFmt, '&Float the format bar')
  CB.AddButton(mView, CMD:Accent, 'Re-&accent this theme (teal)')
  CB.AddSeparator(mView)
  CB.AddButton(mView, CMD:Clear, 'C&lear the log')

  CB.AddButton(mHelp, CMD:About, '&About ClaCommandBar', imgHelp)

  !---- the standard toolbar ----
  barMain = CB.AddBar('Standard', CBD:Top, CBBS:Gripper + CBBS:Floatable)
  CB.SetBarDock(barMain, CBD:Top, 1, 0)

  it = CB.AddButton(barMain, CMD:New, 'New', imgNew)
  CB.SetItemTooltip(it, 'New document (Ctrl+N)')

  mOpen = CB.CreateMenu()
  CB.AddButton(mOpen, 301, 'ledger.tps')
  CB.AddButton(mOpen, 302, 'customers.tps')
  CB.AddSeparator(mOpen)
  CB.AddButton(mOpen, CMD:Open, '&Browse...', imgOpen)
  it = CB.AddSplitButton(barMain, CMD:Open, 'Open', mOpen, imgOpen)
  CB.SetItemTooltip(it, 'Open - the arrow lists recent files')

  it = CB.AddButton(barMain, CMD:Save, 'Save', imgSave)
  CB.SetItemTooltip(it, 'Save (Ctrl+S)')
  it = CB.AddButton(barMain, CMD:Print, 'Print', imgPrint)
  CB.SetItemTooltip(it, 'Print')

  CB.AddSeparator(barMain)

  it = CB.AddToggle(barMain, CMD:Bold, '', imgBold)
  CB.SetItemTooltip(it, 'Bold')
  it = CB.AddToggle(barMain, CMD:Italic, '', imgItalic)
  CB.SetItemTooltip(it, 'Italic')

  CB.AddSeparator(barMain)

  itColour = CB.AddColorButton(barMain, CMD:Colour, '', COLOR:Maroon)
  CB.SetItemTooltip(itColour, 'Text colour - the arrow opens the picker')

  CB.AddSeparator(barMain)
  CB.AddLabel(barMain, 'Find:')
  itFind = CB.AddEdit(barMain, CMD:Find, 'customer', 110)
  CB.SetItemTooltip(itFind, 'Type here, then Enter or Tab away')

  it = CB.AddCheckBox(barMain, CMD:Wrap, 'Wrap')
  CB.SetItemTooltip(it, 'Word wrap')

  it = CB.AddButton(barMain, CMD:About, 'About', imgHelp)
  CB.SetItemStyle(it, CBIS:RightAlign)

  !---- the format toolbar, second row ----
  barFmt = CB.AddBar('Format', CBD:Top, CBBS:Gripper + CBBS:Floatable)
  CB.SetBarDock(barFmt, CBD:Top, 2, 0)

  CB.AddLabel(barFmt, 'Zoom:')
  itZoom = CB.AddCombo(barFmt, CMD:Zoom, '50%|75%|100%|150%|200%', 80)
  CB.SetComboSel(itZoom, 2)
  CB.AddSeparator(barFmt)
  CB.AddButton(barFmt, CMD:Undo, 'Undo', imgUndo)
  CB.AddButton(barFmt, CMD:Redo, 'Redo', imgRedo)
  CB.AddSeparator(barFmt)
  CB.AddButton(barFmt, CMD:Cut, 'Cut', imgCut)
  CB.AddButton(barFmt, CMD:Copy, 'Copy', imgCopy)
  CB.AddButton(barFmt, CMD:Paste, 'Paste', imgPaste)

  !---- a left-docked bar with large icons over text ----
  barSide = CB.AddBar('Tools', CBD:Left, CBBS:Gripper + CBBS:LargeIcons)
  it = CB.AddButton(barSide, 601, 'Find', imgFind)
  CB.SetItemStyle(it, CBIS:TextBelow)
  it = CB.AddButton(barSide, 602, 'Print', imgPrint)
  CB.SetItemStyle(it, CBIS:TextBelow)
  it = CB.AddButton(barSide, 603, 'Help', imgHelp)
  CB.SetItemStyle(it, CBIS:TextBelow)

  !---- a context menu for the right mouse button ----
  mCtx = CB.CreateMenu()
  CB.AddButton(mCtx, CMD:Copy, '&Copy', imgCopy)
  CB.AddSeparator(mCtx)
  CB.AddButton(mCtx, CMD:Clear, 'C&lear the log')
  CB.AddButton(mCtx, CMD:About, '&About', imgHelp)

  !---- keyboard: the window ALRTs the keys, we forward them ----
  CB.AddClarionKey(CMD:New,  CtrlN)
  CB.AddClarionKey(CMD:Open, CtrlO)
  CB.AddClarionKey(CMD:Save, CtrlS)

  CB.Layout()

!---------------------------------------------------------------------
!  Park the LIST in whatever the bars did not take.
FitList ROUTINE
  CB.FitControl(?Log, 4, 4)
  DISPLAY

!---------------------------------------------------------------------
!  Drain the queue one event at a time and log each one.
Pump ROUTINE
  LOOP WHILE CB.TakeOne()
    LogCmd    = CB.LastCmd
    LogDetail = ''
    CASE CB.LastEvent
    OF CBE:Command
      LogKind = 'Command'
      DO DoCommand
    OF CBE:Toggled
      LogKind   = 'Toggled'
      LogDetail = 'now ' & CHOOSE(CB.LastParam = 1, 'ON', 'OFF')
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
      LogDetail = 'on item ' & CB.LastItem
    OF CBE:Layout
      LogKind   = 'Layout'
      LogDetail = 'client area changed - refitting the list'
      DO FitList
    OF CBE:DropDown
      CYCLE                                   ! too chatty to log
    ELSE
      LogKind = 'Event ' & CB.LastEvent
    END
    DO AddLog
  END

!---------------------------------------------------------------------
DoCommand ROUTINE
  DATA
n SIGNED
  CODE
  CASE CB.LastCmd
  OF CMD:Exit
    POST(EVENT:CloseWindow)
    LogDetail = 'closing'
  OF CMD:SideBar
    CB.SetBarVisible(barSide, 1 - CB.BarVisible(barSide))
    LogDetail = 'side bar ' & CHOOSE(CB.BarVisible(barSide) = 1, 'shown', 'hidden')
    DO FitList
  OF CMD:FloatFmt
    CB.FloatBar(barFmt, 420, 380)
    LogDetail = 'format bar floated - drag its caption, double-click it to re-dock'
    DO FitList
  OF CMD:Accent
    CB.SetAccent(0808040h)                    ! COLORREF is 0BBGGRRh: a deep teal
    LogDetail = 'whole palette rebuilt around one accent colour'
  OF CMD:Clear
    FREE(LogQ)
    seq = 0
    DISPLAY
    LogDetail = 'log cleared'
  OF CMD:About
    MESSAGE('ClaCommandBar - Direct2D command bars for Clarion.||' &          |
            'Every bar, menu and popup you see is drawn by COMMANDBAR.DLL ' & |
            'and driven from Clarion through CommandBarClass.',               |
            'About', ICON:Asterisk)
    LogDetail = 'about box'
  ELSE
    IF CB.LastCmd >= CMD:Theme AND CB.LastCmd <= CMD:Theme + 10
      n = CB.LastCmd - CMD:Theme
      CB.SetTheme(n + 1)
      LOOP i = 0 TO 10                        ! keep the radio marks honest
        CB.CheckCmd(CMD:Theme + i, CHOOSE(i = n, 1, 0))
      END
      LogDetail = 'theme = ' & CLIP(CB.PipeItem(ThemeNames, n + 1))
    ELSE
      LogDetail = 'item ' & CB.LastItem
    END
  END

!---------------------------------------------------------------------
AddLog ROUTINE
  seq += 1
  CLEAR(LogQ)
  LOG:Seq    = seq
  LOG:Kind   = LogKind
  LOG:Cmd    = LogCmd
  LOG:Detail = LogDetail
  ADD(LogQ)
  LOOP WHILE RECORDS(LogQ) > 500
    GET(LogQ, 1)
    DELETE(LogQ)
  END
  ?Log{PROP:Selected} = RECORDS(LogQ)
  DISPLAY
