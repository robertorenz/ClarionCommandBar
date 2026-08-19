!=====================================================================
! CommandBar.clw - implementation of CommandBarClass (see CommandBar.inc)
!
! Notes this code relies on:
!   * W{PROP:Handle} is the HWND the DLL docks its bars inside.  It is
!     only valid once the window is OPEN, so Init must be called after
!     the window opens (the template does this at PRIORITY 8600).
!   * The bars are real child HWNDs of the Clarion window.  The DLL
!     subclasses that window to catch WM_SIZE, so the bars stay put
!     even if nobody calls Layout() - but calling Layout() on
!     EVENT:Sized costs nothing and makes the intent obvious.
!   * PROP:XPos / PROP:Width and friends are DIALOG UNITS unless the
!     window is switched to pixel mode.  FitControl saves 0{PROP:Pixels},
!     switches to TRUE, positions, and puts it back - the same dance
!     ClaPropGrid uses.
!   * A Clarion KEYCODE packs the Windows virtual key in the LOW byte
!     and the modifiers in the HIGH byte (1 = shift, 2 = ctrl, 4 = alt):
!     CtrlS = 0253h -> 'S' (53h) with ctrl.  The DLL's CBK: flags use a
!     different bit order (ctrl = 1, shift = 2), so AddClarionKey and
!     TakeAlertKey swap the two low bits.  Verified against
!     clarion12\libsrc\win\KEYCODES.CLW.
!=====================================================================
  MEMBER

  INCLUDE('EQUATES.CLW'),ONCE
  INCLUDE('COMMANDBAR.INC'),ONCE

  MAP
!   NOTE: the MODULE() label below must NOT be 'COMMANDBAR.DLL'. Clarion
!   strips the extension and compares the result with the name of the
!   module being compiled - CommandBar.clw - decides these prototypes are
!   defined HERE, and every one of them fails with
!   "Missing procedure definition: CB_CREATE((LONG,...)".  The label is
!   only a grouping name; binding is by NAME() plus commandbar.lib.
    MODULE('ClaCommandBarDLL')
CB_Initialize        PROCEDURE(),SIGNED,PROC,PASCAL,NAME('CB_Initialize')
CB_Shutdown          PROCEDURE(),PASCAL,NAME('CB_Shutdown')
CB_Create            PROCEDURE(UNSIGNED hwndParent, ULONG style),LONG,PASCAL,NAME('CB_Create')
CB_Destroy           PROCEDURE(LONG cb),PASCAL,NAME('CB_Destroy')
CB_Layout            PROCEDURE(LONG cb),PASCAL,NAME('CB_Layout')
CB_GetClientRect     PROCEDURE(LONG cb, *SIGNED x, *SIGNED y, *SIGNED w, *SIGNED h),PASCAL,RAW,NAME('CB_GetClientRect')
CB_Redraw            PROCEDURE(LONG cb),PASCAL,NAME('CB_Redraw')
CB_SetTheme          PROCEDURE(LONG cb, SIGNED theme),PASCAL,NAME('CB_SetTheme')
CB_GetTheme          PROCEDURE(LONG cb),SIGNED,PASCAL,NAME('CB_GetTheme')
CB_SetColor          PROCEDURE(LONG cb, SIGNED slot, LONG color),PASCAL,NAME('CB_SetColor')
CB_GetColor          PROCEDURE(LONG cb, SIGNED slot),LONG,PASCAL,NAME('CB_GetColor')
CB_SetFont           PROCEDURE(LONG cb, SIGNED slot, *CSTRING face, SIGNED sizePt, SIGNED bold, SIGNED italic),PASCAL,RAW,NAME('CB_SetFont')
CB_SetMetric         PROCEDURE(LONG cb, SIGNED metric, SIGNED value),PASCAL,NAME('CB_SetMetric')
CB_GetMetric         PROCEDURE(LONG cb, SIGNED metric),SIGNED,PASCAL,NAME('CB_GetMetric')
CB_SetAccent         PROCEDURE(LONG cb, LONG accent),PASCAL,NAME('CB_SetAccent')
CB_AddImage          PROCEDURE(LONG cb, *CSTRING fileName),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddImage')
CB_AddImageStrip     PROCEDURE(LONG cb, *CSTRING fileName, SIGNED cx, *SIGNED count),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddImageStrip')
CB_AddImageHandle    PROCEDURE(LONG cb, UNSIGNED handle, SIGNED isIcon),SIGNED,PROC,PASCAL,NAME('CB_AddImageHandle')
CB_GetImageCount     PROCEDURE(LONG cb),SIGNED,PASCAL,NAME('CB_GetImageCount')
CB_AddBar            PROCEDURE(LONG cb, *CSTRING title, SIGNED dock, ULONG barStyle),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddBar')
CB_CreateMenu        PROCEDURE(LONG cb),SIGNED,PROC,PASCAL,NAME('CB_CreateMenu')
CB_DestroyContainer  PROCEDURE(LONG cb, SIGNED container),PASCAL,NAME('CB_DestroyContainer')
CB_ClearContainer    PROCEDURE(LONG cb, SIGNED container),PASCAL,NAME('CB_ClearContainer')
CB_SetBarDock        PROCEDURE(LONG cb, SIGNED bar, SIGNED dock, SIGNED row, SIGNED offset),PASCAL,NAME('CB_SetBarDock')
CB_GetBarDock        PROCEDURE(LONG cb, SIGNED bar),SIGNED,PASCAL,NAME('CB_GetBarDock')
CB_SetBarVisible     PROCEDURE(LONG cb, SIGNED bar, SIGNED visible),PASCAL,NAME('CB_SetBarVisible')
CB_GetBarVisible     PROCEDURE(LONG cb, SIGNED bar),SIGNED,PASCAL,NAME('CB_GetBarVisible')
CB_FloatBar          PROCEDURE(LONG cb, SIGNED bar, SIGNED x, SIGNED y),PASCAL,NAME('CB_FloatBar')
CB_SetBarRect        PROCEDURE(LONG cb, SIGNED bar, SIGNED x, SIGNED y, SIGNED w, SIGNED h),PASCAL,NAME('CB_SetBarRect')
CB_AddRibbonTab      PROCEDURE(LONG cb, SIGNED bar, *CSTRING text),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddRibbonTab')
CB_AddRibbonGroup    PROCEDURE(LONG cb, SIGNED tab, *CSTRING text),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddRibbonGroup')
CB_SetActiveTab      PROCEDURE(LONG cb, SIGNED bar, SIGNED tab),PASCAL,NAME('CB_SetActiveTab')
CB_GetActiveTab      PROCEDURE(LONG cb, SIGNED bar),SIGNED,PASCAL,NAME('CB_GetActiveTab')
CB_GetTabCount       PROCEDURE(LONG cb, SIGNED bar),SIGNED,PASCAL,NAME('CB_GetTabCount')
CB_GetTabAt          PROCEDURE(LONG cb, SIGNED bar, SIGNED index),SIGNED,PASCAL,NAME('CB_GetTabAt')
CB_TrackMenu         PROCEDURE(LONG cb, SIGNED menu, SIGNED x, SIGNED y),LONG,PROC,PASCAL,NAME('CB_TrackMenu')
CB_AddItem           PROCEDURE(LONG cb, SIGNED container, SIGNED itemType, LONG cmdId, *CSTRING text, SIGNED image),SIGNED,PROC,PASCAL,RAW,NAME('CB_AddItem')
CB_InsertItem        PROCEDURE(LONG cb, SIGNED container, SIGNED beforeItem, SIGNED itemType, LONG cmdId, *CSTRING text, SIGNED image),SIGNED,PROC,PASCAL,RAW,NAME('CB_InsertItem')
CB_RemoveItem        PROCEDURE(LONG cb, SIGNED item),PASCAL,NAME('CB_RemoveItem')
CB_SetItemText       PROCEDURE(LONG cb, SIGNED item, *CSTRING text),PASCAL,RAW,NAME('CB_SetItemText')
CB_GetItemText       PROCEDURE(LONG cb, SIGNED item, *CSTRING buf, SIGNED bufLen),SIGNED,PROC,PASCAL,RAW,NAME('CB_GetItemText')
CB_SetItemImage      PROCEDURE(LONG cb, SIGNED item, SIGNED image),PASCAL,NAME('CB_SetItemImage')
CB_SetItemStyle      PROCEDURE(LONG cb, SIGNED item, ULONG styleFlags),PASCAL,NAME('CB_SetItemStyle')
CB_GetItemStyle      PROCEDURE(LONG cb, SIGNED item),ULONG,PASCAL,NAME('CB_GetItemStyle')
CB_SetItemEnabled    PROCEDURE(LONG cb, SIGNED item, SIGNED enabled),PASCAL,NAME('CB_SetItemEnabled')
CB_GetItemEnabled    PROCEDURE(LONG cb, SIGNED item),SIGNED,PASCAL,NAME('CB_GetItemEnabled')
CB_SetItemChecked    PROCEDURE(LONG cb, SIGNED item, SIGNED checked),PASCAL,NAME('CB_SetItemChecked')
CB_GetItemChecked    PROCEDURE(LONG cb, SIGNED item),SIGNED,PASCAL,NAME('CB_GetItemChecked')
CB_SetItemVisible    PROCEDURE(LONG cb, SIGNED item, SIGNED visible),PASCAL,NAME('CB_SetItemVisible')
CB_GetItemVisible    PROCEDURE(LONG cb, SIGNED item),SIGNED,PASCAL,NAME('CB_GetItemVisible')
CB_SetItemTooltip    PROCEDURE(LONG cb, SIGNED item, *CSTRING text),PASCAL,RAW,NAME('CB_SetItemTooltip')
CB_SetItemShortcut   PROCEDURE(LONG cb, SIGNED item, *CSTRING text),PASCAL,RAW,NAME('CB_SetItemShortcut')
CB_SetItemWidth      PROCEDURE(LONG cb, SIGNED item, SIGNED px),PASCAL,NAME('CB_SetItemWidth')
CB_SetItemMenu       PROCEDURE(LONG cb, SIGNED item, SIGNED menu),PASCAL,NAME('CB_SetItemMenu')
CB_GetItemMenu       PROCEDURE(LONG cb, SIGNED item),SIGNED,PASCAL,NAME('CB_GetItemMenu')
CB_SetItemCmd        PROCEDURE(LONG cb, SIGNED item, LONG cmdId),PASCAL,NAME('CB_SetItemCmd')
CB_GetItemCmd        PROCEDURE(LONG cb, SIGNED item),LONG,PASCAL,NAME('CB_GetItemCmd')
CB_FindItem          PROCEDURE(LONG cb, LONG cmdId),SIGNED,PASCAL,NAME('CB_FindItem')
CB_EnableCmd         PROCEDURE(LONG cb, LONG cmdId, SIGNED enabled),PASCAL,NAME('CB_EnableCmd')
CB_CheckCmd          PROCEDURE(LONG cb, LONG cmdId, SIGNED checked),PASCAL,NAME('CB_CheckCmd')
CB_GetItemCount      PROCEDURE(LONG cb, SIGNED container),SIGNED,PASCAL,NAME('CB_GetItemCount')
CB_GetItemAt         PROCEDURE(LONG cb, SIGNED container, SIGNED index),SIGNED,PASCAL,NAME('CB_GetItemAt')
CB_SetItemValue      PROCEDURE(LONG cb, SIGNED item, *CSTRING text),PASCAL,RAW,NAME('CB_SetItemValue')
CB_GetItemValue      PROCEDURE(LONG cb, SIGNED item, *CSTRING buf, SIGNED bufLen),SIGNED,PROC,PASCAL,RAW,NAME('CB_GetItemValue')
CB_AddComboItem      PROCEDURE(LONG cb, SIGNED item, *CSTRING text),PASCAL,RAW,NAME('CB_AddComboItem')
CB_ClearComboItems   PROCEDURE(LONG cb, SIGNED item),PASCAL,NAME('CB_ClearComboItems')
CB_SetComboSel       PROCEDURE(LONG cb, SIGNED item, SIGNED index),PASCAL,NAME('CB_SetComboSel')
CB_GetComboSel       PROCEDURE(LONG cb, SIGNED item),SIGNED,PASCAL,NAME('CB_GetComboSel')
CB_SetItemColor      PROCEDURE(LONG cb, SIGNED item, LONG color),PASCAL,NAME('CB_SetItemColor')
CB_GetItemColor      PROCEDURE(LONG cb, SIGNED item),LONG,PASCAL,NAME('CB_GetItemColor')
CB_AddAccelerator    PROCEDURE(LONG cb, LONG cmdId, SIGNED key, SIGNED mods),PASCAL,NAME('CB_AddAccelerator')
CB_ClearAccelerators PROCEDURE(LONG cb),PASCAL,NAME('CB_ClearAccelerators')
CB_TranslateKey      PROCEDURE(LONG cb, SIGNED key, SIGNED mods),SIGNED,PROC,PASCAL,NAME('CB_TranslateKey')
CB_PollEvent         PROCEDURE(LONG cb, *SIGNED item, *LONG cmdId, *SIGNED evType, *LONG param),SIGNED,PROC,PASCAL,RAW,NAME('CB_PollEvent')
CB_GetCursorPos      PROCEDURE(*SIGNED x, *SIGNED y),PASCAL,RAW,NAME('CB_GetCursorPos')
CB_SetHostMenuVisible PROCEDURE(LONG cb, SIGNED visible),PASCAL,NAME('CB_SetHostMenuVisible')
CB_SetReserveSpace   PROCEDURE(LONG cb, SIGNED mode),PASCAL,NAME('CB_SetReserveSpace')
CB_SetHostReserveBottom PROCEDURE(LONG cb, SIGNED px),PASCAL,NAME('CB_SetHostReserveBottom')
CB_GetHostReserveBottom PROCEDURE(LONG cb),SIGNED,PASCAL,NAME('CB_GetHostReserveBottom')
CB_GetReserveSpace   PROCEDURE(LONG cb),SIGNED,PASCAL,NAME('CB_GetReserveSpace')
CB_GetHostMenuVisible PROCEDURE(LONG cb),SIGNED,PASCAL,NAME('CB_GetHostMenuVisible')
    END
  END

CBInitDone   BYTE(0)                        ! CB_Initialize() is once per process

!---------------------------------------------------------------------
! lifetime
!---------------------------------------------------------------------
CommandBarClass.Construct PROCEDURE()
  CODE
  SELF.CB            = 0
  SELF.Win          &= NULL
  SELF.Style         = CBS:Tooltips + CBS:Chevron + CBS:MenuIcons
  SELF.Theme         = CBT:SteelBlue
  SELF.Initialized   = 0
  SELF.TimerInterval = 10
  SELF.MenuBar       = 0
  SELF.MirrorBase    = 1000000
  SELF.MirrorBar     = 0
  SELF.MirrorHide    = 0
  SELF.MirrorMenus  &= NEW(CBMirrorQueue)
  SELF.LastItem      = 0
  SELF.LastCmd       = 0
  SELF.LastEvent     = 0
  SELF.LastParam     = 0

CommandBarClass.Destruct PROCEDURE()
  CODE
  IF SELF.CB
    CB_Destroy(SELF.CB)
    SELF.CB = 0
  END

CommandBarClass.Init PROCEDURE(WINDOW W, ULONG style=19)
  CODE
  IF SELF.CB THEN SELF.Kill().
  SELF.Win  &= W
  SELF.Style = style
  IF ~CBInitDone
    CB_Initialize()
    CBInitDone = 1
  END
  SELF.CB = CB_Create(W{PROP:Handle}, style)
  IF ~SELF.CB THEN RETURN 0.
  IF SELF.TimerInterval > 0 AND W{PROP:Timer} = 0
    W{PROP:Timer} = SELF.TimerInterval          ! the event pump needs a timer
  END
  SELF.Theme       = CB_GetTheme(SELF.CB)
  SELF.Initialized = 1
  RETURN 1

CommandBarClass.Kill PROCEDURE()
  CODE
  IF SELF.CB
    CB_Destroy(SELF.CB)
    SELF.CB = 0
  END
  SELF.MenuBar     = 0
  SELF.Initialized = 0

CommandBarClass.Layout PROCEDURE()
  CODE
  IF SELF.CB THEN CB_Layout(SELF.CB).

CommandBarClass.Redraw PROCEDURE()
  CODE
  IF SELF.CB THEN CB_Redraw(SELF.CB).

!---------------------------------------------------------------------
! the client area the bars left behind (pixels, window client coords)
!---------------------------------------------------------------------
CommandBarClass.ClientX PROCEDURE()
x SIGNED
y SIGNED
w SIGNED
h SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  x = 0; y = 0; w = 0; h = 0
  CB_GetClientRect(SELF.CB, x, y, w, h)
  RETURN x

CommandBarClass.ClientY PROCEDURE()
x SIGNED
y SIGNED
w SIGNED
h SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  x = 0; y = 0; w = 0; h = 0
  CB_GetClientRect(SELF.CB, x, y, w, h)
  RETURN y

CommandBarClass.ClientWidth PROCEDURE()
x SIGNED
y SIGNED
w SIGNED
h SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  x = 0; y = 0; w = 0; h = 0
  CB_GetClientRect(SELF.CB, x, y, w, h)
  RETURN w

CommandBarClass.ClientHeight PROCEDURE()
x SIGNED
y SIGNED
w SIGNED
h SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  x = 0; y = 0; w = 0; h = 0
  CB_GetClientRect(SELF.CB, x, y, w, h)
  RETURN h

CommandBarClass.FitControl PROCEDURE(SIGNED feq, SIGNED marginX=0, SIGNED marginY=0)
x   SIGNED
y   SIGNED
w   SIGNED
h   SIGNED
sav BYTE
  CODE
  IF ~SELF.CB OR ~feq OR SELF.Win &= NULL THEN RETURN.
  x = 0; y = 0; w = 0; h = 0
  CB_GetClientRect(SELF.CB, x, y, w, h)
  w -= 2 * marginX
  h -= 2 * marginY
  IF w < 1 THEN w = 1.
  IF h < 1 THEN h = 1.
  SETTARGET(SELF.Win)
  sav = 0{PROP:Pixels}
  0{PROP:Pixels} = TRUE
  feq{PROP:XPos}   = x + marginX
  feq{PROP:YPos}   = y + marginY
  feq{PROP:Width}  = w
  feq{PROP:Height} = h
  0{PROP:Pixels} = sav
  SETTARGET()

!---------------------------------------------------------------------
! theming
!---------------------------------------------------------------------
CommandBarClass.SetTheme PROCEDURE(SIGNED theme)
  CODE
  IF ~SELF.CB THEN RETURN.
  CB_SetTheme(SELF.CB, theme)
  SELF.Theme = theme

CommandBarClass.SetAccent PROCEDURE(LONG color)
  CODE
  IF SELF.CB THEN CB_SetAccent(SELF.CB, color).

CommandBarClass.SetColor PROCEDURE(SIGNED slot, LONG color)
  CODE
  IF SELF.CB THEN CB_SetColor(SELF.CB, slot, color).

CommandBarClass.GetColor PROCEDURE(SIGNED slot)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetColor(SELF.CB, slot)

CommandBarClass.SetFont PROCEDURE(SIGNED slot, STRING face, SHORT sizePt, BYTE bold=0, BYTE italic=0)
cFace CSTRING(64)
  CODE
  IF ~SELF.CB THEN RETURN.
  cFace = CLIP(LEFT(face))
  CB_SetFont(SELF.CB, slot, cFace, sizePt, bold, italic)

CommandBarClass.SetMetric PROCEDURE(SIGNED metric, SIGNED value)
  CODE
  IF SELF.CB THEN CB_SetMetric(SELF.CB, metric, value).

CommandBarClass.GetMetric PROCEDURE(SIGNED metric)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetMetric(SELF.CB, metric)

!---------------------------------------------------------------------
! images
!---------------------------------------------------------------------
CommandBarClass.AddImage PROCEDURE(STRING fileName)
cName CSTRING(261)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cName = CLIP(LEFT(fileName))
  RETURN CB_AddImage(SELF.CB, cName)

CommandBarClass.AddImageStrip PROCEDURE(STRING fileName, SIGNED cx, *SIGNED count)
cName CSTRING(261)
  CODE
  count = 0
  IF ~SELF.CB THEN RETURN 0.
  cName = CLIP(LEFT(fileName))
  RETURN CB_AddImageStrip(SELF.CB, cName, cx, count)

CommandBarClass.AddImageHandle PROCEDURE(LONG handle, BYTE isIcon=1)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_AddImageHandle(SELF.CB, handle, isIcon)

CommandBarClass.ImageCount PROCEDURE()
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetImageCount(SELF.CB)

!---------------------------------------------------------------------
! containers
!---------------------------------------------------------------------
CommandBarClass.AddBar PROCEDURE(STRING title, SIGNED dock=0, ULONG barStyle=6)
cTitle CSTRING(128)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cTitle = CLIP(LEFT(title))
  RETURN CB_AddBar(SELF.CB, cTitle, dock, barStyle)

CommandBarClass.AddMenuBar PROCEDURE(<STRING title>)
cTitle CSTRING(128)
id     SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  IF OMITTED(2) OR ~title
    cTitle = 'Menu'
  ELSE
    cTitle = CLIP(LEFT(title))
  END
!  A menu bar is a bar like any other, minus the gripper: it sits on
!  dock row 0 so it always ends up above the toolbars.
  id = CB_AddBar(SELF.CB, cTitle, CBD:Top, CBBS:MenuBar)
  IF id
    CB_SetBarDock(SELF.CB, id, CBD:Top, 0, 0)
    SELF.MenuBar = id
  END
  RETURN id

CommandBarClass.CreateMenu PROCEDURE()
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_CreateMenu(SELF.CB)

CommandBarClass.ClearContainer PROCEDURE(SIGNED container)
  CODE
  IF SELF.CB THEN CB_ClearContainer(SELF.CB, container).

CommandBarClass.DestroyContainer PROCEDURE(SIGNED container)
  CODE
  IF SELF.CB THEN CB_DestroyContainer(SELF.CB, container).

CommandBarClass.SetBarDock PROCEDURE(SIGNED bar, SIGNED dock, SIGNED row=0, SIGNED offset=0)
  CODE
  IF SELF.CB THEN CB_SetBarDock(SELF.CB, bar, dock, row, offset).

CommandBarClass.BarDock PROCEDURE(SIGNED bar)
  CODE
  IF ~SELF.CB THEN RETURN -1.
  RETURN CB_GetBarDock(SELF.CB, bar)

CommandBarClass.SetBarVisible PROCEDURE(SIGNED bar, BYTE visible=1)
  CODE
  IF SELF.CB THEN CB_SetBarVisible(SELF.CB, bar, visible).

CommandBarClass.BarVisible PROCEDURE(SIGNED bar)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetBarVisible(SELF.CB, bar)

CommandBarClass.FloatBar PROCEDURE(SIGNED bar, SIGNED x, SIGNED y)
  CODE
  IF SELF.CB THEN CB_FloatBar(SELF.CB, bar, x, y).

CommandBarClass.TrackMenu PROCEDURE(SIGNED menu, SIGNED x, SIGNED y)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_TrackMenu(SELF.CB, menu, x, y)

CommandBarClass.PopupMenu PROCEDURE(SIGNED menu)
x SIGNED
y SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  x = 0
  y = 0
  CB_GetCursorPos(x, y)                       ! screen coords, no Win32 import
  RETURN CB_TrackMenu(SELF.CB, menu, x, y)

!---------------------------------------------------------------------
! items
!---------------------------------------------------------------------
CommandBarClass.AddItem PROCEDURE(SIGNED container, SIGNED itemType, LONG cmd, STRING text, SIGNED image=0)
cText CSTRING(256)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cText = CLIP(LEFT(text))
  RETURN CB_AddItem(SELF.CB, container, itemType, cmd, cText, image)

CommandBarClass.InsertItem PROCEDURE(SIGNED container, SIGNED beforeItem, SIGNED itemType, LONG cmd, STRING text, SIGNED image=0)
cText CSTRING(256)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cText = CLIP(LEFT(text))
  RETURN CB_InsertItem(SELF.CB, container, beforeItem, itemType, cmd, cText, image)

CommandBarClass.AddButton PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0)
  CODE
  RETURN SELF.AddItem(container, CBI:Button, cmd, text, image)

CommandBarClass.AddToggle PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0)
  CODE
  RETURN SELF.AddItem(container, CBI:Toggle, cmd, text, image)

CommandBarClass.AddDropButton PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED menu, SIGNED image=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:DropDown, cmd, text, image)
  IF id AND menu THEN SELF.SetItemMenu(id, menu).
  RETURN id

CommandBarClass.AddSplitButton PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED menu, SIGNED image=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Split, cmd, text, image)
  IF id AND menu THEN SELF.SetItemMenu(id, menu).
  RETURN id

CommandBarClass.AddSeparator PROCEDURE(SIGNED container)
  CODE
  RETURN SELF.AddItem(container, CBI:Separator, 0, '', 0)

CommandBarClass.AddLabel PROCEDURE(SIGNED container, STRING text)
  CODE
  RETURN SELF.AddItem(container, CBI:Label, 0, text, 0)

CommandBarClass.AddEdit PROCEDURE(SIGNED container, LONG cmd, STRING value, SIGNED widthPx=110)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Edit, cmd, '', 0)
  IF id
    IF widthPx > 0 THEN SELF.SetItemWidth(id, widthPx).
    SELF.SetItemValue(id, value)
  END
  RETURN id

CommandBarClass.AddCombo PROCEDURE(SIGNED container, LONG cmd, STRING pipeList, SIGNED widthPx=130)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Combo, cmd, '', 0)
  IF id
    IF widthPx > 0 THEN SELF.SetItemWidth(id, widthPx).
    IF pipeList
      SELF.SetComboList(id, pipeList)
      SELF.SetComboSel(id, 0)
    END
  END
  RETURN id

CommandBarClass.AddCheckBox PROCEDURE(SIGNED container, LONG cmd, STRING text, BYTE checked=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:CheckBox, cmd, text, 0)
  IF id AND checked THEN SELF.SetItemChecked(id, 1).
  RETURN id

CommandBarClass.AddColorButton PROCEDURE(SIGNED container, LONG cmd, STRING text, LONG color, SIGNED image=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Color, cmd, text, image)
  IF id THEN SELF.SetItemColor(id, color).
  RETURN id

CommandBarClass.AddMenuTitle PROCEDURE(SIGNED bar, STRING text, SIGNED menu)
id SIGNED
  CODE
  id = SELF.AddItem(bar, CBI:Menu, 0, text, 0)
  IF id AND menu THEN SELF.SetItemMenu(id, menu).
  RETURN id

CommandBarClass.AddSubMenu PROCEDURE(SIGNED container, STRING text, SIGNED menu, SIGNED image=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Button, 0, text, image)
  IF id AND menu THEN SELF.SetItemMenu(id, menu).
  RETURN id

CommandBarClass.AddSpace PROCEDURE(SIGNED container, SIGNED widthPx=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Space, 0, '', 0)
  IF id AND widthPx > 0 THEN SELF.SetItemWidth(id, widthPx).
  RETURN id

CommandBarClass.RemoveItem PROCEDURE(SIGNED item)
  CODE
  IF SELF.CB THEN CB_RemoveItem(SELF.CB, item).


!---------------------------------------------------------------------
! ribbons, and bars you place yourself
!---------------------------------------------------------------------
CommandBarClass.SetBarRect PROCEDURE(SIGNED bar, SIGNED x, SIGNED y, SIGNED width, SIGNED height)
  CODE
  IF SELF.CB THEN CB_SetBarRect(SELF.CB, bar, x, y, width, height).

!  Reads the control's rectangle in PIXELS - the same save / set / restore
!  of 0{PROP:Pixels} that FitControl does, because PROP:XPos and friends
!  are dialog units until the window is switched over.
CommandBarClass.PlaceOnControl PROCEDURE(SIGNED bar, SIGNED feq, BYTE hideControl=1)
x   SIGNED
y   SIGNED
w   SIGNED
h   SIGNED
sav BYTE
  CODE
  IF ~SELF.CB OR ~feq OR SELF.Win &= NULL THEN RETURN.
  SETTARGET(SELF.Win)
  sav = 0{PROP:Pixels}
  0{PROP:Pixels} = TRUE
  x = feq{PROP:XPos}
  y = feq{PROP:YPos}
  w = feq{PROP:Width}
  h = feq{PROP:Height}
  0{PROP:Pixels} = sav
  !  Hidden, not destroyed: it still reports its position, so the ABC
  !  resizer goes on moving it and the bar follows on every Reposition.
  IF hideControl THEN feq{PROP:Hide} = 1.
  SETTARGET()
  IF w < 1 THEN w = 1.
  IF h < 1 THEN h = 1.
  CB_SetBarRect(SELF.CB, bar, x, y, w, h)

CommandBarClass.AddRibbon PROCEDURE(STRING title, SIGNED dock=0)
  CODE
  RETURN SELF.AddBar(title, dock, CBBS:Ribbon)

CommandBarClass.AddRibbonTab PROCEDURE(SIGNED bar, STRING text)
cText CSTRING(128)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cText = CLIP(LEFT(text))
  RETURN CB_AddRibbonTab(SELF.CB, bar, cText)

CommandBarClass.AddRibbonGroup PROCEDURE(SIGNED tab, STRING text)
cText CSTRING(128)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  cText = CLIP(LEFT(text))
  RETURN CB_AddRibbonGroup(SELF.CB, tab, cText)

CommandBarClass.SetActiveTab PROCEDURE(SIGNED bar, SIGNED tab)
  CODE
  IF SELF.CB THEN CB_SetActiveTab(SELF.CB, bar, tab).

CommandBarClass.ActiveTab PROCEDURE(SIGNED bar)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetActiveTab(SELF.CB, bar)

CommandBarClass.TabCount PROCEDURE(SIGNED bar)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetTabCount(SELF.CB, bar)

CommandBarClass.TabAt PROCEDURE(SIGNED bar, SIGNED index)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetTabAt(SELF.CB, bar, index)

CommandBarClass.AddLargeButton PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0)
id SIGNED
  CODE
  id = SELF.AddItem(container, CBI:Button, cmd, text, image)
  IF id THEN SELF.SetItemStyle(id, CBIS:TextBelow).
  RETURN id

!---------------------------------------------------------------------
! item properties
!---------------------------------------------------------------------
CommandBarClass.SetItemText PROCEDURE(SIGNED item, STRING text)
cText CSTRING(256)
  CODE
  IF ~SELF.CB THEN RETURN.
  cText = CLIP(LEFT(text))
  CB_SetItemText(SELF.CB, item, cText)

CommandBarClass.ItemText PROCEDURE(SIGNED item)
buf CSTRING(257)
  CODE
  IF ~SELF.CB THEN RETURN ''.
  buf = ''
  CB_GetItemText(SELF.CB, item, buf, SIZE(buf))
  RETURN buf

CommandBarClass.SetItemImage PROCEDURE(SIGNED item, SIGNED image)
  CODE
  IF SELF.CB THEN CB_SetItemImage(SELF.CB, item, image).

CommandBarClass.SetItemStyle PROCEDURE(SIGNED item, ULONG styleFlags)
  CODE
  IF SELF.CB THEN CB_SetItemStyle(SELF.CB, item, styleFlags).

CommandBarClass.ItemStyle PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemStyle(SELF.CB, item)

CommandBarClass.SetItemEnabled PROCEDURE(SIGNED item, BYTE enabled=1)
  CODE
  IF SELF.CB THEN CB_SetItemEnabled(SELF.CB, item, enabled).

CommandBarClass.ItemEnabled PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemEnabled(SELF.CB, item)

CommandBarClass.SetItemChecked PROCEDURE(SIGNED item, BYTE checked=1)
  CODE
  IF SELF.CB THEN CB_SetItemChecked(SELF.CB, item, checked).

CommandBarClass.ItemChecked PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemChecked(SELF.CB, item)

CommandBarClass.SetItemVisible PROCEDURE(SIGNED item, BYTE visible=1)
  CODE
  IF SELF.CB THEN CB_SetItemVisible(SELF.CB, item, visible).

CommandBarClass.ItemVisible PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemVisible(SELF.CB, item)

CommandBarClass.SetItemTooltip PROCEDURE(SIGNED item, STRING text)
cText CSTRING(256)
  CODE
  IF ~SELF.CB THEN RETURN.
  cText = CLIP(LEFT(text))
  CB_SetItemTooltip(SELF.CB, item, cText)

CommandBarClass.SetItemShortcut PROCEDURE(SIGNED item, STRING text)
cText CSTRING(64)
  CODE
  IF ~SELF.CB THEN RETURN.
  cText = CLIP(LEFT(text))
  CB_SetItemShortcut(SELF.CB, item, cText)

CommandBarClass.SetItemWidth PROCEDURE(SIGNED item, SIGNED px)
  CODE
  IF SELF.CB THEN CB_SetItemWidth(SELF.CB, item, px).

CommandBarClass.SetItemMenu PROCEDURE(SIGNED item, SIGNED menu)
  CODE
  IF SELF.CB THEN CB_SetItemMenu(SELF.CB, item, menu).

CommandBarClass.ItemMenu PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemMenu(SELF.CB, item)

CommandBarClass.SetItemCmd PROCEDURE(SIGNED item, LONG cmd)
  CODE
  IF SELF.CB THEN CB_SetItemCmd(SELF.CB, item, cmd).

CommandBarClass.ItemCmd PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemCmd(SELF.CB, item)

CommandBarClass.FindItem PROCEDURE(LONG cmd)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_FindItem(SELF.CB, cmd)

CommandBarClass.EnableCmd PROCEDURE(LONG cmd, BYTE enabled=1)
  CODE
  IF SELF.CB THEN CB_EnableCmd(SELF.CB, cmd, enabled).

CommandBarClass.CheckCmd PROCEDURE(LONG cmd, BYTE checked=1)
  CODE
  IF SELF.CB THEN CB_CheckCmd(SELF.CB, cmd, checked).

CommandBarClass.ItemCount PROCEDURE(SIGNED container)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemCount(SELF.CB, container)

CommandBarClass.ItemAt PROCEDURE(SIGNED container, SIGNED index)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemAt(SELF.CB, container, index)

!---------------------------------------------------------------------
! edit / combo / colour values
!---------------------------------------------------------------------
CommandBarClass.SetItemValue PROCEDURE(SIGNED item, STRING text)
cText CSTRING(1024)
  CODE
  IF ~SELF.CB THEN RETURN.
  cText = CLIP(LEFT(text))
  CB_SetItemValue(SELF.CB, item, cText)

CommandBarClass.ItemValue PROCEDURE(SIGNED item)
buf CSTRING(1025)
  CODE
  IF ~SELF.CB THEN RETURN ''.
  buf = ''
  CB_GetItemValue(SELF.CB, item, buf, SIZE(buf))
  RETURN buf

CommandBarClass.AddComboItem PROCEDURE(SIGNED item, STRING text)
cText CSTRING(256)
  CODE
  IF ~SELF.CB THEN RETURN.
  cText = CLIP(LEFT(text))
  CB_AddComboItem(SELF.CB, item, cText)

CommandBarClass.SetComboList PROCEDURE(SIGNED item, STRING pipeList)
i   SIGNED
one STRING(256)
  CODE
  IF ~SELF.CB THEN RETURN.
  CB_ClearComboItems(SELF.CB, item)
  LOOP i = 1 TO 512
    one = SELF.PipeItem(pipeList, i)
    IF one = '' AND i > 1 THEN BREAK.
    IF one = '' AND i = 1 THEN BREAK.
    SELF.AddComboItem(item, one)
  END

CommandBarClass.ClearComboItems PROCEDURE(SIGNED item)
  CODE
  IF SELF.CB THEN CB_ClearComboItems(SELF.CB, item).

CommandBarClass.SetComboSel PROCEDURE(SIGNED item, SIGNED index)
  CODE
  IF SELF.CB THEN CB_SetComboSel(SELF.CB, item, index).

CommandBarClass.ComboSel PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN -1.
  RETURN CB_GetComboSel(SELF.CB, item)

CommandBarClass.SetItemColor PROCEDURE(SIGNED item, LONG color)
  CODE
  IF SELF.CB THEN CB_SetItemColor(SELF.CB, item, color).

CommandBarClass.ItemColor PROCEDURE(SIGNED item)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetItemColor(SELF.CB, item)

!---------------------------------------------------------------------
! keyboard
!---------------------------------------------------------------------
CommandBarClass.AddAccelerator PROCEDURE(LONG cmd, SIGNED vkey, SIGNED mods=0)
  CODE
  IF SELF.CB THEN CB_AddAccelerator(SELF.CB, cmd, vkey, mods).

!  A Clarion KEYCODE carries the virtual key in the low byte and the
!  modifiers in the high byte, but with shift and ctrl the other way
!  round from the CBK: flags - hence the swap.
CommandBarClass.AddClarionKey PROCEDURE(LONG cmd, LONG clarionKeyCode)
vk   SIGNED
cm   SIGNED
mods SIGNED
  CODE
  IF ~SELF.CB THEN RETURN.
  vk   = BAND(clarionKeyCode, 0FFh)
  cm   = BAND(BSHIFT(clarionKeyCode, -8), 0FFh)
  mods = 0
  IF BAND(cm, 1) THEN mods += CBK:Shift.
  IF BAND(cm, 2) THEN mods += CBK:Ctrl.
  IF BAND(cm, 4) THEN mods += CBK:Alt.
  CB_AddAccelerator(SELF.CB, cmd, vk, mods)

CommandBarClass.ClearAccelerators PROCEDURE()
  CODE
  IF SELF.CB THEN CB_ClearAccelerators(SELF.CB).

CommandBarClass.TakeKey PROCEDURE(SIGNED vkey, SIGNED mods=0)
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_TranslateKey(SELF.CB, vkey, mods)

CommandBarClass.TakeAlertKey PROCEDURE(LONG clarionKeyCode)
vk   SIGNED
cm   SIGNED
mods SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  vk   = BAND(clarionKeyCode, 0FFh)
  cm   = BAND(BSHIFT(clarionKeyCode, -8), 0FFh)
  mods = 0
  IF BAND(cm, 1) THEN mods += CBK:Shift.
  IF BAND(cm, 2) THEN mods += CBK:Ctrl.
  IF BAND(cm, 4) THEN mods += CBK:Alt.
  RETURN CB_TranslateKey(SELF.CB, vk, mods)


!---------------------------------------------------------------------
! mirroring the window's own MENUBAR
!
! Measured on Clarion 12 (see INSTALL.md "Verified runtime notes"):
!   * 0{PROP:MenuBar} is the MENUBAR's field equate.
!   * A MENU answers PROP:Child,n with its immediate children IN
!     DECLARATION ORDER - submenus and separators included.  The
!     MENUBAR itself does NOT, so the top-level menus are found by
!     scanning for CREATE:menu controls whose PROP:Parent is the
!     menubar; Clarion numbers those sequentially, so ascending feq is
!     declaration order.
!   * A menu ITEM with empty PROP:Text is a SEPARATOR.
!   * Menu controls live OUTSIDE FIRSTFIELD()..LASTFIELD(), in a
!     synthetic range that starts at the menubar's own equate.
!---------------------------------------------------------------------
CommandBarClass.MirrorMenu PROCEDURE(SIGNED bar, BYTE hideOriginal=1)
mb SIGNED
  CODE
  IF ~SELF.CB OR SELF.Win &= NULL THEN RETURN 0.
  SETTARGET(SELF.Win)
  mb = 0{PROP:MenuBar}
  IF ~mb THEN mb = SELF.FindMenuBar().      ! ask, then look
  SETTARGET()
  RETURN SELF.MirrorMenuFrom(bar, mb, hideOriginal)

!  Menu controls do NOT live in one predictable place.  Measured on
!  Clarion 12: a MENUBAR or MENU carrying a USE gets an ordinary LOW
!  field equate (an AppGen frame answers PROP:MenuBar = 1), while one
!  without a USE gets a SYNTHETIC equate at 8000h and up - and a window
!  is free to mix the two, which is exactly what a hand-written
!  MENUBAR full of USE-less MENUs does.
!
!  So both ranges are scanned and filtered on PROP:Parent.  Scanning a
!  window around the menubar's own equate - which is what this used to
!  do - finds nothing at all when the menubar and its menus land in
!  different ranges, and the symptom is a mirrored bar with no titles
!  on it.
CommandBarClass.MirrorMenuFrom PROCEDURE(SIGNED bar, SIGNED menuBarFeq, BYTE hideOriginal=1)
MenuQ QUEUE,PRE(MQ)
Feq     SIGNED
Order   LONG
      END
pass SIGNED
lo   SIGNED
hi   SIGNED
stp  SIGNED
feq  SIGNED
top  SIGNED
sub  SIGNED
i    SIGNED
n    SIGNED
  CODE
  IF ~SELF.CB OR SELF.Win &= NULL OR ~menuBarFeq THEN RETURN 0.
  SELF.MirrorBar  = bar
  SELF.MirrorHide = hideOriginal
  SETTARGET(SELF.Win)
  FREE(MenuQ)

  !  ---- find every top-level MENU ----
  !
  !  Menu controls land in FOUR different equate ranges, and which ones
  !  depends on the kind of window and on whether the control carries a
  !  USE.  Measured on Clarion 12:
  !
  !                        carries a USE      no USE
  !    WINDOW              1, 2, 3 ...        32768, 32769 ... (upward)
  !    APPLICATION frame   -1, -2, -3 ...     32767, 32766 ... (downward)
  !
  !  and an APPLICATION frame answers LASTFIELD() = 0, so the ordinary
  !  field range is not even walkable there.  Scanning only the WINDOW
  !  ranges is why mirroring an MDI frame produced an empty bar.
  LOOP pass = 1 TO 4
    CASE pass
    OF 1                                     ! APPLICATION, named
      lo = -1
      hi = -2048
      stp = -1
    OF 2                                     ! WINDOW, named
      lo = 1
      hi = LASTFIELD()
      stp = 1
    OF 3                                     ! APPLICATION, unnamed
      lo = 32767
      hi = 32767 - 1023
      stp = -1
    ELSE                                     ! WINDOW, unnamed
      lo = 32768
      hi = 32768 + 1023
      stp = 1
    END
    IF stp > 0 AND hi < lo THEN CYCLE.
    LOOP feq = lo TO hi BY stp
      IF feq = menuBarFeq THEN CYCLE.
      IF feq{PROP:Type} <> CREATE:menu THEN CYCLE.
      IF feq{PROP:Parent} <> menuBarFeq THEN CYCLE.
      CLEAR(MenuQ)
      MQ:Feq   = feq
      MQ:Order = SELF.MinFeqIn(feq)
      ADD(MenuQ)
    END
  END

  !  ---- put them back in DECLARATION order ----
  !  The equates alone cannot do it: named and unnamed controls are two
  !  independent sequences, and on a frame they run backwards.  The
  !  lowest RANK anywhere in a menu's subtree does work, because the
  !  items inside the first menu are always declared before the items
  !  inside the second whichever range they land in.
  SORT(MenuQ, MQ:Order)

  n = 0
  LOOP i = 1 TO RECORDS(MenuQ)
    GET(MenuQ, i)
    feq = MQ:Feq
    sub = SELF.CreateMenu()
    top = SELF.AddMenuTitle(bar, feq{PROP:Text}, sub)
    IF ~top THEN CYCLE.
    IF feq{PROP:Disable} THEN SELF.SetItemEnabled(top, 0).
    SELF.MirrorInto(sub, feq)
    n += 1
  END
  FREE(MenuQ)

  !  A MENUBAR ignores PROP:Hide, so the DLL detaches the real Win32
  !  menu instead.  The ITEMs behind it stay valid, which is what makes
  !  POST(EVENT:Accepted) on a mirrored row still work.
  IF hideOriginal AND n THEN CB_SetHostMenuVisible(SELF.CB, 0).
  SETTARGET()
  RETURN n

!  Where a field equate sits in declaration order, as a small positive
!  number.  Each of the four ranges counts from its own end.
CommandBarClass.MenuRank PROCEDURE(SIGNED feq)
  CODE
  IF feq < 0 THEN RETURN -feq.                 ! APPLICATION, named
  IF feq >= 32768 THEN RETURN feq - 32767.     ! WINDOW, unnamed
  IF feq > 16384 THEN RETURN 32768 - feq.      ! APPLICATION, unnamed
  RETURN feq                                   ! WINDOW, named

!  Lowest field equate in a menu's whole subtree, its own included.
!  Assumes SETTARGET(SELF.Win) is active.
CommandBarClass.MinFeqIn PROCEDURE(SIGNED menuFeq)
n     SIGNED
child SIGNED
best  LONG
sub   LONG
  CODE
  !  Rank, not raw equate - and only NAMED controls count.  Every menu
  !  worth mirroring holds items with a USE (an ITEM without one cannot
  !  have ACCEPTED code anyway), so the first named item inside a menu
  !  is a reliable stand-in for where that menu was declared.  Unnamed
  !  controls are numbered in their own sequence and would interleave.
  best = 0
  IF ABS(menuFeq) <= 16384 THEN best = SELF.MenuRank(menuFeq).
  LOOP n = 1 TO 512
    child = menuFeq{PROP:Child, n}
    IF ~child THEN BREAK.
    IF ABS(child) <= 16384
      sub = SELF.MenuRank(child)
      IF sub > 0 AND (best = 0 OR sub < best) THEN best = sub.
    END
    IF child{PROP:Type} = CREATE:menu
      sub = SELF.MinFeqIn(child)
      IF sub > 0 AND (best = 0 OR sub < best) THEN best = sub.
    END
  END
  IF ~best THEN best = 1000000 + SELF.MenuRank(menuFeq).
  RETURN best

!  The MENUBAR control itself, wherever it landed.
CommandBarClass.FindMenuBar PROCEDURE()
pass SIGNED
lo   SIGNED
hi   SIGNED
stp  SIGNED
feq  SIGNED
res  SIGNED
  CODE
  IF SELF.Win &= NULL THEN RETURN 0.
  res = 0
  SETTARGET(SELF.Win)
  LOOP pass = 1 TO 4
    CASE pass
    OF 1
      lo = -1
      hi = -2048
      stp = -1
    OF 2
      lo = 1
      hi = LASTFIELD()
      stp = 1
    OF 3
      lo = 32767
      hi = 32767 - 1023
      stp = -1
    ELSE
      lo = 32768
      hi = 32768 + 1023
      stp = 1
    END
    IF stp > 0 AND hi < lo THEN CYCLE.
    LOOP feq = lo TO hi BY stp
      IF feq{PROP:Type} = CREATE:menubar
        res = feq
        BREAK
      END
    END
    IF res THEN BREAK.
  END
  SETTARGET()
  RETURN res

!  Why is my mirrored bar empty?  This says so in words.
CommandBarClass.MenuReport PROCEDURE()
mb    SIGNED
found SIGNED
pass  SIGNED
lo    SIGNED
hi    SIGNED
stp   SIGNED
feq   SIGNED
res   STRING(2000)
  CODE
  IF SELF.Win &= NULL THEN RETURN 'No window - call Init first.'.
  SETTARGET(SELF.Win)
  mb = 0{PROP:MenuBar}
  res = 'PROP:MenuBar = ' & mb
  IF ~mb
    mb = SELF.FindMenuBar()
    SETTARGET(SELF.Win)
    res = CLIP(res) & ' (0), scanned and found ' & mb
  END
  IF ~mb
    SETTARGET()
    RETURN CLIP(res) & '<13,10>This window has no MENUBAR.  If the menu ' &      |
           'belongs to the application FRAME, mirror it on the FRAME, ' &        |
           'not here.'
  END
  res = CLIP(res) & '  (' & CHOOSE(mb < 0, 'APPLICATION frame', 'WINDOW') &      |
        ')<13,10>FIRSTFIELD=' & FIRSTFIELD() & ' LASTFIELD=' & LASTFIELD()
  found = 0
  LOOP pass = 1 TO 4
    CASE pass
    OF 1
      lo = -1
      hi = -2048
      stp = -1
    OF 2
      lo = 1
      hi = LASTFIELD()
      stp = 1
    OF 3
      lo = 32767
      hi = 32767 - 1023
      stp = -1
    ELSE
      lo = 32768
      hi = 32768 + 1023
      stp = 1
    END
    IF stp > 0 AND hi < lo THEN CYCLE.
    LOOP feq = lo TO hi BY stp
      IF feq = mb THEN CYCLE.
      IF feq{PROP:Type} <> CREATE:menu THEN CYCLE.
      IF feq{PROP:Parent} <> mb THEN CYCLE.
      found += 1
      res = CLIP(res) & '<13,10>  menu feq=' & feq & '  order=' &                |
            SELF.MinFeqIn(feq) & '  [' & CLIP(feq{PROP:Text}) & ']'
    END
  END
  SETTARGET()
  IF ~found
    RETURN CLIP(res) & '<13,10>No MENU has PROP:Parent = ' & mb & '.'
  END
  RETURN CLIP(res) & '<13,10>' & found & ' top-level menus.'

!  One level of a mirrored menu.  Assumes SETTARGET(SELF.Win) is active.
!  PROP:Child hands back a MENU's immediate children in DECLARATION
!  order - submenus and separators included - on a WINDOW and on an
!  APPLICATION frame alike, so only the TOP level needs sorting.
CommandBarClass.MirrorInto PROCEDURE(SIGNED container, SIGNED menuFeq)
n     SIGNED
child SIGNED
ty    SIGNED
sub   SIGNED
row   SIGNED
txt   STRING(256)
ky    LONG
  CODE
  LOOP n = 1 TO 512
    child = menuFeq{PROP:Child, n}
    IF ~child THEN BREAK.
    ty  = child{PROP:Type}
    txt = child{PROP:Text}

    IF ty = CREATE:menu
      sub = SELF.CreateMenu()
      row = SELF.AddSubMenu(container, txt, sub)
      IF row AND child{PROP:Disable} THEN SELF.SetItemEnabled(row, 0).
      SELF.MirrorInto(sub, child)
      CYCLE
    END

    IF ty <> CREATE:item THEN CYCLE.

    IF ~txt                                        ! an ITEM with no text
      SELF.AddSeparator(container)                 ! is a SEPARATOR
      CYCLE
    END

    row = SELF.AddButton(container, SELF.MirrorBase + child, txt)
    IF ~row THEN CYCLE.
    ky = child{PROP:Key}
    IF ky THEN SELF.SetItemShortcut(row, SELF.KeyText(ky)).
    IF child{PROP:Disable} THEN SELF.SetItemEnabled(row, 0).
    IF child{PROP:Checked} THEN SELF.SetItemChecked(row, 1).
  END

!  A Clarion KEYCODE as text.  The virtual key is the low byte and the
!  modifiers are the high byte (1 = shift, 2 = ctrl, 4 = alt).
CommandBarClass.KeyText PROCEDURE(LONG clarionKeyCode)
vk   SIGNED
cm   SIGNED
res  STRING(40)
nm   STRING(12)
  CODE
  IF ~clarionKeyCode THEN RETURN ''.
  vk = BAND(clarionKeyCode, 0FFh)
  cm = BAND(BSHIFT(clarionKeyCode, -8), 0FFh)
  nm = ''
  CASE vk
  OF 8   ; nm = 'Backspace'
  OF 9   ; nm = 'Tab'
  OF 13  ; nm = 'Enter'
  OF 27  ; nm = 'Esc'
  OF 32  ; nm = 'Space'
  OF 33  ; nm = 'PgUp'
  OF 34  ; nm = 'PgDn'
  OF 35  ; nm = 'End'
  OF 36  ; nm = 'Home'
  OF 37  ; nm = 'Left'
  OF 38  ; nm = 'Up'
  OF 39  ; nm = 'Right'
  OF 40  ; nm = 'Down'
  OF 45  ; nm = 'Ins'
  OF 46  ; nm = 'Del'
  ELSE
    IF vk >= 112 AND vk <= 123
      nm = 'F' & (vk - 111)
    ELSIF (vk >= 48 AND vk <= 57) OR (vk >= 65 AND vk <= 90)
      nm = CHR(vk)
    END
  END
  IF ~nm THEN RETURN ''.
  res = ''
  IF BAND(cm, 2) THEN res = 'Ctrl+'.
  IF BAND(cm, 4) THEN res = CLIP(res) & 'Alt+'.
  IF BAND(cm, 1) THEN res = CLIP(res) & 'Shift+'.
  RETURN CLIP(res) & CLIP(nm)

CommandBarClass.ReserveSpace PROCEDURE(SIGNED mode)
  CODE
  IF SELF.CB THEN CB_SetReserveSpace(SELF.CB, mode).

CommandBarClass.ReserveMode PROCEDURE()
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetReserveSpace(SELF.CB)

CommandBarClass.SetHostMenu PROCEDURE(BYTE visible)
  CODE
  IF SELF.CB THEN CB_SetHostMenuVisible(SELF.CB, visible).

CommandBarClass.HostMenuVisible PROCEDURE()
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetHostMenuVisible(SELF.CB)


!  Re-read the MENUBAR onto the same bar.  In an MDI application the
!  frame's menu is not fixed: opening a child window MERGES that child's
!  own MENUBAR into it, and closing the child takes it away again.  A
!  mirror taken once at Init would go stale the first time a browse
!  opened, so call this whenever the menu may have changed.
CommandBarClass.RefreshMirror PROCEDURE()
i SIGNED
  CODE
  IF ~SELF.CB OR ~SELF.MirrorBar THEN RETURN 0.

  !  throw away the popups the last mirror made, then the titles
  IF ~SELF.MirrorMenus &= NULL
    LOOP i = 1 TO RECORDS(SELF.MirrorMenus)
      GET(SELF.MirrorMenus, i)
      SELF.DestroyContainer(SELF.MirrorMenus.Container)
    END
    FREE(SELF.MirrorMenus)
  END
  SELF.ClearContainer(SELF.MirrorBar)

  !  The host menu is already off the frame if it was ever taken off,
  !  and the DLL keeps re-asserting that, so do not ask again here.
  RETURN SELF.MirrorMenu(SELF.MirrorBar, 0)


CommandBarClass.HostReserveBottom PROCEDURE(SIGNED px)
  CODE
  IF SELF.CB THEN CB_SetHostReserveBottom(SELF.CB, px).


CommandBarClass.HostReserveHeight PROCEDURE()
  CODE
  IF ~SELF.CB THEN RETURN 0.
  RETURN CB_GetHostReserveBottom(SELF.CB)

!---------------------------------------------------------------------
! the event pump
!---------------------------------------------------------------------
CommandBarClass.TakeOne PROCEDURE()
itm   SIGNED
cmd   LONG
evt   SIGNED
prm   LONG
guard SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  guard = 0
  LOOP
    guard += 1
    IF guard > 2000 THEN RETURN 0.
    itm = 0
    cmd = 0
    evt = 0
    prm = 0
    IF ~CB_PollEvent(SELF.CB, itm, cmd, evt, prm) THEN RETURN 0.
    !  A row MirrorMenu built stands in for a real menu ITEM: hand the
    !  click straight to that ITEM and keep the event to ourselves, so
    !  the caller only ever sees its OWN commands.
    !  An APPLICATION frame hands out NEGATIVE equates for its menu
    !  controls, so a mirrored id sits just BELOW MirrorBase as often as
    !  above it - testing "cmd >= MirrorBase" silently dropped every
    !  mirrored click on a frame.  A window either side is what works.
    IF SELF.MirrorBase > 0 AND evt = CBE:Command AND                            |
       cmd > SELF.MirrorBase - 65536 AND cmd < SELF.MirrorBase + 65536
      POST(EVENT:Accepted, cmd - SELF.MirrorBase)
      CYCLE
    END
    SELF.LastItem  = itm
    SELF.LastCmd   = cmd
    SELF.LastEvent = evt
    SELF.LastParam = prm
    RETURN 1
  END

CommandBarClass.TakeEvent PROCEDURE()
handled BYTE
guard   SIGNED
  CODE
  IF ~SELF.CB THEN RETURN 0.
  handled = 0
  guard   = 0
  LOOP
    guard += 1
    IF guard > 2000 THEN BREAK.                ! a queue that never drains
    IF ~SELF.TakeOne() THEN BREAK.
    handled = 1
    CASE SELF.LastEvent
    OF CBE:Command
      SELF.TakeCommand(SELF.LastCmd, SELF.LastItem)
    OF CBE:Toggled
      SELF.TakeToggled(SELF.LastCmd, SELF.LastItem, SELF.LastParam)
    OF CBE:DropDown
      SELF.TakeDropDown(SELF.LastCmd, SELF.LastItem, SELF.LastParam)
    OF CBE:TextChanged
      SELF.TakeTextChanged(SELF.LastCmd, SELF.LastItem)
    OF CBE:SelChanged
      SELF.TakeSelChanged(SELF.LastCmd, SELF.LastItem, SELF.LastParam)
    OF CBE:ColorChanged
      SELF.TakeColorChanged(SELF.LastCmd, SELF.LastItem, SELF.LastParam)
    OF CBE:Layout
      SELF.TakeLayoutChanged()
    OF CBE:RightClick
      SELF.TakeRightClick(SELF.LastCmd, SELF.LastItem)
    END
  END
  RETURN handled

!  All eight are deliberately empty: override the ones you care about,
!  or ignore them entirely and read SELF.LastCmd after TakeEvent().
CommandBarClass.TakeCommand PROCEDURE(LONG cmd, SIGNED item)
  CODE

CommandBarClass.TakeToggled PROCEDURE(LONG cmd, SIGNED item, BYTE checked)
  CODE

CommandBarClass.TakeDropDown PROCEDURE(LONG cmd, SIGNED item, SIGNED menu)
  CODE

CommandBarClass.TakeTextChanged PROCEDURE(LONG cmd, SIGNED item)
  CODE

CommandBarClass.TakeSelChanged PROCEDURE(LONG cmd, SIGNED item, SIGNED index)
  CODE

CommandBarClass.TakeColorChanged PROCEDURE(LONG cmd, SIGNED item, LONG color)
  CODE

CommandBarClass.TakeLayoutChanged PROCEDURE()
  CODE

CommandBarClass.TakeRightClick PROCEDURE(LONG cmd, SIGNED item)
  CODE

!---------------------------------------------------------------------
! internals
!---------------------------------------------------------------------
!  Item n of a pipe-delimited list, '' when there is no such item.
CommandBarClass.PipeItem PROCEDURE(STRING list, SIGNED ordinal)
p     SIGNED
start SIGNED
n     SIGNED
len   SIGNED
  CODE
  IF ordinal < 1 THEN RETURN ''.
  len   = LEN(CLIP(list))
  IF ~len THEN RETURN ''.
  start = 1
  n     = 1
  LOOP p = 1 TO len
    IF list[p] = '|'
      IF n = ordinal
        IF p - 1 < start THEN RETURN ''.
        RETURN list[start : p - 1]
      END
      n += 1
      start = p + 1
    END
  END
  IF n = ordinal AND start <= len
    RETURN list[start : len]
  END
  RETURN ''
