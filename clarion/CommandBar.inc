!ABCIncludeFile(COMMANDBAR)
!=====================================================================
! CommandBar.inc - Clarion wrapper class for COMMANDBAR.DLL
!                  (ClaCommandBar: Direct2D command bars)
!
! Multi-DLL:  where this class's CODE lives is decided by the two
!             project defines _CommandBarLinkMode_ / _CommandBarDllMode_,
!             which the ClaCommandBar templates generate through the ABC
!             class category 'COMMANDBAR'.  With neither define present
!             (a hand-coded project) LINK behaves as 1 and DLL as 0,
!             which is exactly the single-EXE behaviour.  Never EQUATE
!             them here - they must arrive as project defines.
!
! The model:  one CommandBarClass object per WINDOW.  It owns
!             CONTAINERS (a BAR docked to an edge, or a popup MENU) and
!             ITEMS inside them.  Every item carries a COMMAND ID of
!             your choosing; several items may share one (the toolbar
!             Save and the menu Save normally do), and every event
!             reports both the item and the command.
!=====================================================================
  OMIT('_EndOfInclude_',_CommandBarPresent_)
_CommandBarPresent_  EQUATE(1)

!---- link/dll mode defaults ----------------------------------------
! Template-generated apps define _CommandBarModesSet_ plus the two mode
! pragmas through the ABC 'COMMANDBAR' class category.  A hand-coded
! project that defines nothing gets safe single-EXE defaults here: the
! class code LINKs in and nothing is imported from a DLL.  With NO
! values at all the DLL() attribute is undefined and every virtual call
! dies with an access violation - so never remove this block.
  OMIT('_EndOfModes_',_CommandBarModesSet_)
_CommandBarLinkMode_ EQUATE(1)
_CommandBarDllMode_  EQUATE(0)
  _EndOfModes_

!---- item types (match CBI_* in commandbar.h) -----------------------
CBI:Button      EQUATE(1)   ! plain push button
CBI:Toggle      EQUATE(2)   ! stays down until clicked again
CBI:DropDown    EQUATE(3)   ! the whole button drops its menu
CBI:Split       EQUATE(4)   ! left = command, right arrow = menu
CBI:Separator   EQUATE(5)
CBI:Label       EQUATE(6)   ! static text
CBI:Edit        EQUATE(7)   ! in-bar type-in field
CBI:Combo       EQUATE(8)   ! in-bar drop list
CBI:CheckBox    EQUATE(9)
CBI:Color       EQUATE(10)  ! swatch button + colour picker
CBI:Menu        EQUATE(11)  ! top-level title on a menu bar
CBI:Space       EQUATE(12)  ! flexible gap - pushes what follows right

!---- item style flags -----------------------------------------------
CBIS:TextOnly   EQUATE(0001h)
CBIS:IconOnly   EQUATE(0002h)
CBIS:TextBelow  EQUATE(0004h)   ! large button: image over text
CBIS:BeginGroup EQUATE(0008h)
CBIS:Wrap       EQUATE(0010h)   ! start a new row after this item
CBIS:RightAlign EQUATE(0020h)   ! push this item to the far end
CBIS:Default    EQUATE(0040h)   ! menu default item - drawn bold
CBIS:Radio      EQUATE(0080h)   ! menu check mark is a radio dot
CBIS:AutoCheck  EQUATE(0100h)   ! toggle / checkbox flips itself
CBIS:Stretch    EQUATE(0200h)   ! edit / combo eats the leftover width

!---- dock sides -----------------------------------------------------
CBD:Top         EQUATE(0)
CBD:Bottom      EQUATE(1)
CBD:Left        EQUATE(2)
CBD:Right       EQUATE(3)
CBD:Float       EQUATE(4)
CBD:Fixed       EQUATE(5)   ! you place it: SetBarRect / PlaceOnControl

!---- bar style flags ------------------------------------------------
CBBS:MenuBar    EQUATE(0001h)
CBBS:Gripper    EQUATE(0002h)
CBBS:Floatable  EQUATE(0004h)
CBBS:Locked     EQUATE(0008h)
CBBS:NoBorder   EQUATE(0010h)
CBBS:LargeIcons EQUATE(0020h)
CBBS:Ribbon     EQUATE(0040h)   ! a ribbon: tabs of groups of items

!---- manager style flags (Init) -------------------------------------
CBS:Tooltips    EQUATE(0001h)
CBS:Chevron     EQUATE(0002h)   ! overflow chevron when too narrow
CBS:FlatLook    EQUATE(0004h)
CBS:HotText     EQUATE(0008h)   ! item text recolours on hover
CBS:MenuIcons   EQUATE(0010h)   ! icon gutter down the popup menus

!---- built-in themes ------------------------------------------------
CBT:SteelBlue   EQUATE(1)       ! the default
CBT:Office2003  EQUATE(2)
CBT:Office2007  EQUATE(3)
CBT:Office2010  EQUATE(4)
CBT:Office2013  EQUATE(5)
CBT:Office2016  EQUATE(6)
CBT:VS2012Light EQUATE(7)
CBT:VS2012Dark  EQUATE(8)
CBT:Win11Light  EQUATE(9)
CBT:Win11Dark   EQUATE(10)
CBT:SlateDark   EQUATE(11)
CBT:Custom      EQUATE(99)      ! keep the colours, change them by hand

!---- colour slots ---------------------------------------------------
! Clarion (and Win32 COLORREF) store colours as 0BBGGRRh, so the hex
! digits read blue-green-red.  Use the template's colour picker, or
! Clarion's COLOR: equates, and you never have to think about it.
CBC:BarBack        EQUATE(1)
CBC:BarBack2       EQUATE(2)    ! bottom of the bar gradient
CBC:BarBorder      EQUATE(3)
CBC:ItemText       EQUATE(4)
CBC:ItemTextHot    EQUATE(5)
CBC:ItemTextDis    EQUATE(6)
CBC:HotBack        EQUATE(7)
CBC:HotBorder      EQUATE(8)
CBC:PressBack      EQUATE(9)
CBC:PressBorder    EQUATE(10)
CBC:CheckBack      EQUATE(11)
CBC:CheckBorder    EQUATE(12)
CBC:Separator      EQUATE(13)
CBC:Gripper        EQUATE(14)
CBC:MenuBack       EQUATE(15)
CBC:MenuBorder     EQUATE(16)
CBC:MenuGutter     EQUATE(17)
CBC:MenuHot        EQUATE(18)
CBC:MenuHotBorder  EQUATE(19)
CBC:MenuText       EQUATE(20)
CBC:MenuTextHot    EQUATE(21)
CBC:MenuTextDis    EQUATE(22)
CBC:MenuShortcut   EQUATE(23)
CBC:MenuSep        EQUATE(24)
CBC:MenuCheck      EQUATE(25)
CBC:CaptionBack    EQUATE(26)
CBC:CaptionText    EQUATE(27)
CBC:FloatBorder    EQUATE(28)
CBC:Chevron        EQUATE(29)
CBC:EditBack       EQUATE(30)
CBC:EditBorder     EQUATE(31)
CBC:EditText       EQUATE(32)
CBC:EditSel        EQUATE(33)
CBC:TipBack        EQUATE(34)
CBC:TipBorder      EQUATE(35)
CBC:TipText        EQUATE(36)
CBC:Accent         EQUATE(37)

!---- font slots -----------------------------------------------------
CBF:Item        EQUATE(1)
CBF:Menu        EQUATE(2)
CBF:Caption     EQUATE(3)
CBF:Tooltip     EQUATE(4)

!---- metrics (design pixels at 96 DPI - the DLL scales them) --------
CBM:ItemHeight  EQUATE(1)   ! 0 = work it out from the font
CBM:IconSize    EQUATE(2)   ! default 16
CBM:LargeIcon   EQUATE(3)   ! default 32
CBM:PadX        EQUATE(4)
CBM:PadY        EQUATE(5)
CBM:Gap         EQUATE(6)
CBM:BarPadX     EQUATE(7)
CBM:BarPadY     EQUATE(8)
CBM:SepWidth    EQUATE(9)
CBM:Corner      EQUATE(10)  ! button corner radius, 0 = square
CBM:MenuWidth   EQUATE(11)
CBM:GutterWidth EQUATE(12)
CBM:RowGap      EQUATE(13)

!---- event types ----------------------------------------------------
CBE:Command      EQUATE(1)
CBE:Toggled      EQUATE(2)
CBE:DropDown     EQUATE(3)
CBE:TextChanged  EQUATE(4)
CBE:SelChanged   EQUATE(5)
CBE:ColorChanged EQUATE(6)
CBE:Layout       EQUATE(7)
CBE:RightClick   EQUATE(8)
CBE:TabChanged   EQUATE(9)   ! ribbon tab switched (LastParam = tab id)

!---- accelerator modifier flags -------------------------------------
! NOTE these are NOT in the same bit order as a Clarion KEYCODE (which
! is shift=1, ctrl=2, alt=4).  AddClarionKey / TakeAlertKey do the swap
! for you - see CommandBar.clw.
CBK:Ctrl        EQUATE(0001h)
CBK:Shift       EQUATE(0002h)
CBK:Alt         EQUATE(0004h)

!---- the containers one MirrorMenu built -----------------------------
! Kept so RefreshMirror can throw the whole mirrored menu away and read
! the MENUBAR again - which is what an MDI app needs, because opening a
! child window MERGES that child's menu into the frame's.
CBMirrorQueue QUEUE,TYPE
Container       SIGNED
              END

CommandBarClass CLASS,TYPE,MODULE('CommandBar.clw'),LINK('CommandBar.clw',_CommandBarLinkMode_),DLL(_CommandBarDllMode_)
CB              LONG                        ! HCB handle (0 = dead)
Win             &WINDOW                     ! owning Clarion window
Style           ULONG                       ! CBS: flags passed to Create
Theme           SIGNED                      ! CBT: currently loaded
Initialized     BYTE
TimerInterval   SIGNED                      ! forced on the window (0 = leave alone)
MenuBar         SIGNED                      ! bar id made by AddMenuBar, 0 = none
!  MirrorMenu gives every mirrored row a command id of MirrorBase + the
!  original ITEM's field equate, and TakeOne turns those back into a
!  POST(EVENT:Accepted) on that ITEM and swallows them.  That is what
!  makes mirroring transparent: all your existing menu embed code runs
!  untouched.  Set it to 0 to see the raw command ids instead.
MirrorBase      LONG
!  The bar the last MirrorMenu built into, and the popup containers it
!  made - RefreshMirror needs both.
MirrorBar       SIGNED
!  The TOOLBAR the last MirrorToolbar read, so it can be put back.
MirrorTBFeq     SIGNED
MirrorHide      BYTE
MirrorMenus     &CBMirrorQueue,PRIVATE
!  The event most recently pulled off the queue.  Read these from your
!  TakeCommand / TakeToggled override, or straight after TakeEvent().
LastItem        SIGNED
LastCmd         LONG
LastEvent       SIGNED
LastParam       LONG
Construct       PROCEDURE()
Destruct        PROCEDURE()
!-- lifetime --
Init            PROCEDURE(WINDOW W, ULONG style=19),BYTE,PROC,VIRTUAL  ! 19 = Tooltips+Chevron+MenuIcons
Kill            PROCEDURE(),VIRTUAL
Layout          PROCEDURE()                 ! re-run the dock layout (EVENT:Sized)
Redraw          PROCEDURE()
!-- the client area the bars left behind, in PIXELS --
ClientX         PROCEDURE(),SIGNED
ClientY         PROCEDURE(),SIGNED
ClientWidth     PROCEDURE(),SIGNED
ClientHeight    PROCEDURE(),SIGNED
!  Park a Clarion control in whatever the bars did not take.  Call it
!  from EVENT:Sized (or from TakeLayoutChanged) and a LIST, SHEET or
!  REGION always fills the space under the bars.
FitControl      PROCEDURE(SIGNED feq, SIGNED marginX=0, SIGNED marginY=0),VIRTUAL
!-- theming --
SetTheme        PROCEDURE(SIGNED theme)
SetAccent       PROCEDURE(LONG color)       ! recolour the whole palette
SetColor        PROCEDURE(SIGNED slot, LONG color)
GetColor        PROCEDURE(SIGNED slot),LONG
SetFont         PROCEDURE(SIGNED slot, STRING face, SHORT sizePt, BYTE bold=0, BYTE italic=0)
SetMetric       PROCEDURE(SIGNED metric, SIGNED value)
GetMetric       PROCEDURE(SIGNED metric),SIGNED
!-- images.  Every image index is >= 1; 0 means "no image". --
AddImage        PROCEDURE(STRING fileName),SIGNED,PROC
AddImageStrip   PROCEDURE(STRING fileName, SIGNED cx, *SIGNED count),SIGNED,PROC
AddImageHandle  PROCEDURE(LONG handle, BYTE isIcon=1),SIGNED,PROC
ImageCount      PROCEDURE(),SIGNED
!-- containers --
AddBar          PROCEDURE(STRING title, SIGNED dock=0, ULONG barStyle=6),SIGNED,PROC  ! 6 = Gripper+Floatable
AddMenuBar      PROCEDURE(<STRING title>),SIGNED,PROC
CreateMenu      PROCEDURE(),SIGNED,PROC
ClearContainer  PROCEDURE(SIGNED container)
DestroyContainer PROCEDURE(SIGNED container)
SetBarDock      PROCEDURE(SIGNED bar, SIGNED dock, SIGNED row=0, SIGNED offset=0)
BarDock         PROCEDURE(SIGNED bar),SIGNED
SetBarVisible   PROCEDURE(SIGNED bar, BYTE visible=1)
BarVisible      PROCEDURE(SIGNED bar),BYTE
FloatBar        PROCEDURE(SIGNED bar, SIGNED x, SIGNED y)
!  Place a bar yourself, in window PIXELS.  It then sits outside the
!  dock arithmetic and takes nothing off the client area.
SetBarRect      PROCEDURE(SIGNED bar, SIGNED x, SIGNED y, SIGNED width, SIGNED height)
!  The same thing, but reading the rectangle off one of the window's own
!  controls - drop a REGION where you want the bar and point this at it.
!  The region is hidden by default; it goes on reporting its position, so
!  the ABC resizer keeps moving the bar for you.
PlaceOnControl  PROCEDURE(SIGNED bar, SIGNED feq, BYTE hideControl=1),VIRTUAL
!-- ribbons.  A ribbon bar holds TABS, a tab holds GROUPS, and a group
!-- holds ordinary items - AddButton and friends fill a group exactly as
!-- they fill a toolbar.
AddRibbon       PROCEDURE(STRING title, SIGNED dock=0),SIGNED,PROC
AddRibbonTab    PROCEDURE(SIGNED bar, STRING text),SIGNED,PROC
AddRibbonGroup  PROCEDURE(SIGNED tab, STRING text),SIGNED,PROC
SetActiveTab    PROCEDURE(SIGNED bar, SIGNED tab)
ActiveTab       PROCEDURE(SIGNED bar),SIGNED
TabCount        PROCEDURE(SIGNED bar),SIGNED
TabAt           PROCEDURE(SIGNED bar, SIGNED index),SIGNED   ! index is 0-based
!  Both block until the user picks or cancels, and return the command
!  id chosen (0 = cancelled).  The pick ALSO arrives through TakeEvent.
TrackMenu       PROCEDURE(SIGNED menu, SIGNED x, SIGNED y),LONG,PROC
PopupMenu       PROCEDURE(SIGNED menu),LONG,PROC   ! at the mouse pointer
!-- items --
AddItem         PROCEDURE(SIGNED container, SIGNED itemType, LONG cmd, STRING text, SIGNED image=0),SIGNED,PROC
InsertItem      PROCEDURE(SIGNED container, SIGNED beforeItem, SIGNED itemType, LONG cmd, STRING text, SIGNED image=0),SIGNED,PROC
!  The shorthands.  Every one is AddItem with the type filled in.
AddButton       PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0),SIGNED,PROC
AddToggle       PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0),SIGNED,PROC
AddDropButton   PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED menu, SIGNED image=0),SIGNED,PROC
AddSplitButton  PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED menu, SIGNED image=0),SIGNED,PROC
AddSeparator    PROCEDURE(SIGNED container),SIGNED,PROC
AddLabel        PROCEDURE(SIGNED container, STRING text),SIGNED,PROC
AddEdit         PROCEDURE(SIGNED container, LONG cmd, STRING value, SIGNED widthPx=110),SIGNED,PROC
AddCombo        PROCEDURE(SIGNED container, LONG cmd, STRING pipeList, SIGNED widthPx=130),SIGNED,PROC
AddCheckBox     PROCEDURE(SIGNED container, LONG cmd, STRING text, BYTE checked=0),SIGNED,PROC
AddColorButton  PROCEDURE(SIGNED container, LONG cmd, STRING text, LONG color, SIGNED image=0),SIGNED,PROC
AddMenuTitle    PROCEDURE(SIGNED bar, STRING text, SIGNED menu),SIGNED,PROC
!-- Mirror the window's own MENUBAR onto a command bar: same order, same
!-- nesting, same separators, shortcuts and disabled state.  Choosing a
!-- mirrored row POSTs EVENT:Accepted to the original ITEM, so nothing
!-- you already wrote has to change.  Returns the number of top-level
!-- menus mirrored.  Call it AFTER the window is open.
MirrorMenu      PROCEDURE(SIGNED bar, BYTE hideOriginal=1),SIGNED,PROC,VIRTUAL
!-- The same, but pointed at a MENUBAR you name yourself.  Use it when
!-- 0{PROP:MenuBar} comes back 0 - an MDI child asked about the FRAME's
!-- menu, say - or when a window carries more than one.
MirrorMenuFrom  PROCEDURE(SIGNED bar, SIGNED menuBarFeq, BYTE hideOriginal=1),SIGNED,PROC,VIRTUAL
!-- The window's MENUBAR control, found by scanning rather than by
!-- asking.  0 if the window has none.
FindMenuBar     PROCEDURE(),SIGNED
!-- What MirrorMenu can see, as readable text: the menubar equate and
!-- every top-level menu it found.  Paste it into a MESSAGE() when a
!-- mirrored bar comes out empty and it will tell you why.
MenuReport      PROCEDURE(),STRING
!-- Mirror the window's own TOOLBAR onto a command bar, the way MirrorMenu
!-- does the MENUBAR.  Every BUTTON, CHECK, ENTRY, COMBO and PROMPT in it
!-- becomes a bar item carrying its ICON, its TIP and its disabled state,
!-- and choosing one POSTs EVENT:Accepted to the ORIGINAL control - so the
!-- embed code you already wrote goes on running.
!--
!-- On an APPLICATION frame this is worth more than it looks: with the
!-- toolbar mirrored you can hide the real one, and then Clarion's MDI
!-- toolbar merging - which hides it, rebuilds it and swaps it on every
!-- child window - has nothing on screen left to disturb.
!-- Returns how many controls it found.
MirrorToolbar   PROCEDURE(SIGNED bar, BYTE hideOriginal=1),SIGNED,PROC,VIRTUAL
MirrorToolbarFrom PROCEDURE(SIGNED bar, SIGNED toolbarFeq, BYTE hideOriginal=1),SIGNED,PROC,VIRTUAL
!-- The field equate of the window's TOOLBAR, or 0.
FindToolbar     PROCEDURE(),SIGNED,VIRTUAL
!-- The first control of this CREATE: type anywhere in the window, or 0.
FindControlOfType PROCEDURE(SIGNED wantType),SIGNED,VIRTUAL
!-- Show or hide the real TOOLBAR and everything in it.
ShowHostToolbar PROCEDURE(SIGNED toolbarFeq, BYTE visible),PROC,VIRTUAL
!-- Throw the mirrored menu away and read the MENUBAR again, onto the
!-- same bar.  Call it when the frame's menu has CHANGED - which in an
!-- MDI app means whenever a child window opens or closes, because
!-- Clarion MERGES the child's own MENUBAR into the frame's.  (Setting
!-- PROP:NoMerge on the child stops the merge instead, if you would
!-- rather the frame's menu never changed.)  Returns the new count.
RefreshMirror   PROCEDURE(),SIGNED,PROC,VIRTUAL
!-- 'Ctrl+N' for a Clarion KEYCODE - what MirrorMenu puts in the shortcut
!-- column, and useful on its own.
KeyText         PROCEDURE(LONG clarionKeyCode),STRING
!-- Take the window's own MENUBAR off the frame, or put it back.  A
!-- Clarion MENUBAR ignores PROP:Hide - it is a real Win32 menu - so
!-- this is what MirrorMenu uses to REPLACE the menu rather than sit
!-- under it.  The ITEMs stay alive either way.
SetHostMenu     PROCEDURE(BYTE visible)
!-- Push the HOST's own child windows out of the docked strips.  A
!-- Clarion APPLICATION frame lays its ClaToolBar and MDIClient out
!-- against the FULL client area, so without this a docked bar is drawn
!-- ON TOP of the frame's toolbar.  mode: 1 = always, 0 = never,
!-- -1 = AUTO (the default) which turns it on for a frame and leaves a
!-- plain window to FitControl.
ReserveSpace    PROCEDURE(SIGNED mode)
ReserveMode     PROCEDURE(),SIGNED
!-- A strip along the bottom the HOST paints itself, that no bar may use.
!-- On an APPLICATION frame it is measured automatically - the frame stops
!-- its MDI client short of the bottom and that gap IS the status bar - so
!-- nothing needs setting.  A plain WINDOW with a STATUS bar has no such
!-- child to measure, so give it the height in PIXELS and a bottom, left
!-- or right bar will stop above the status bar instead of covering it.
!-- Pass -1 to go back to measuring.
HostReserveBottom PROCEDURE(SIGNED px),PROC,VIRTUAL
HostReserveHeight PROCEDURE(),SIGNED,VIRTUAL
!-- Collapse a RIBBON bar to its tab strip, or open it again.  The user can
!-- do it too: double-clicking a tab collapses and re-opens, and clicking a
!-- tab on a collapsed ribbon opens it.
MinimizeRibbon  PROCEDURE(SIGNED bar, BYTE minimized),PROC,VIRTUAL
RibbonMinimized PROCEDURE(SIGNED bar),SIGNED,VIRTUAL
HostMenuVisible PROCEDURE(),BYTE
AddSubMenu      PROCEDURE(SIGNED container, STRING text, SIGNED menu, SIGNED image=0),SIGNED,PROC
AddSpace        PROCEDURE(SIGNED container, SIGNED widthPx=0),SIGNED,PROC
!  A large ribbon button: image over text, full group height.  Same as
!  AddButton plus CBIS:TextBelow, which is what makes it the big one.
AddLargeButton  PROCEDURE(SIGNED container, LONG cmd, STRING text, SIGNED image=0),SIGNED,PROC
RemoveItem      PROCEDURE(SIGNED item)
!-- item properties --
SetItemText     PROCEDURE(SIGNED item, STRING text)
ItemText        PROCEDURE(SIGNED item),STRING
SetItemImage    PROCEDURE(SIGNED item, SIGNED image)
SetItemStyle    PROCEDURE(SIGNED item, ULONG styleFlags)
ItemStyle       PROCEDURE(SIGNED item),ULONG
SetItemEnabled  PROCEDURE(SIGNED item, BYTE enabled=1)
ItemEnabled     PROCEDURE(SIGNED item),BYTE
SetItemChecked  PROCEDURE(SIGNED item, BYTE checked=1)
ItemChecked     PROCEDURE(SIGNED item),BYTE
SetItemVisible  PROCEDURE(SIGNED item, BYTE visible=1)
ItemVisible     PROCEDURE(SIGNED item),BYTE
SetItemTooltip  PROCEDURE(SIGNED item, STRING text)
SetItemShortcut PROCEDURE(SIGNED item, STRING text)   ! the 'Ctrl+S' column
SetItemWidth    PROCEDURE(SIGNED item, SIGNED px)
SetItemMenu     PROCEDURE(SIGNED item, SIGNED menu)
ItemMenu        PROCEDURE(SIGNED item),SIGNED
SetItemCmd      PROCEDURE(SIGNED item, LONG cmd)
ItemCmd         PROCEDURE(SIGNED item),LONG
!  Reach every item carrying one command id at once - the way to grey
!  out "Save" on the toolbar AND in the File menu in a single call.
FindItem        PROCEDURE(LONG cmd),SIGNED
EnableCmd       PROCEDURE(LONG cmd, BYTE enabled=1)
CheckCmd        PROCEDURE(LONG cmd, BYTE checked=1)
ItemCount       PROCEDURE(SIGNED container),SIGNED
ItemAt          PROCEDURE(SIGNED container, SIGNED index),SIGNED   ! index is 0-based
!-- edit / combo / colour values --
SetItemValue    PROCEDURE(SIGNED item, STRING text)
ItemValue       PROCEDURE(SIGNED item),STRING
AddComboItem    PROCEDURE(SIGNED item, STRING text)
SetComboList    PROCEDURE(SIGNED item, STRING pipeList)  ! 'Red|Green|Blue'
ClearComboItems PROCEDURE(SIGNED item)
SetComboSel     PROCEDURE(SIGNED item, SIGNED index)     ! 0-based, -1 = none
ComboSel        PROCEDURE(SIGNED item),SIGNED
SetItemColor    PROCEDURE(SIGNED item, LONG color)
ItemColor       PROCEDURE(SIGNED item),LONG
!-- keyboard --
AddAccelerator  PROCEDURE(LONG cmd, SIGNED vkey, SIGNED mods=0)
!  The Clarion-native way: AddClarionKey(CMD:Save, CtrlS).  Clarion
!  keycodes carry the virtual key in the low byte and shift/ctrl/alt in
!  the high byte, so the mapping is exact.
AddClarionKey   PROCEDURE(LONG cmd, LONG clarionKeyCode)
ClearAccelerators PROCEDURE()
TakeKey         PROCEDURE(SIGNED vkey, SIGNED mods=0),BYTE,PROC
!  Call from EVENT:AlertKey:  IF CB.TakeAlertKey(KEYCODE()) THEN CYCLE.
TakeAlertKey    PROCEDURE(LONG clarionKeyCode),BYTE,PROC
!-- event pump.  Two ways to use it, pick one:
!--   1. hand-coded: LOOP WHILE CB.TakeOne() ... read SELF.Last*
!--   2. derived:    CB.TakeEvent() and override the Take... methods
!-- TakeEvent is TakeOne in a loop with a CASE on the event type, so
!-- never mix the two on the same object - the first one to run drains
!-- the queue and the other sees nothing.
TakeOne         PROCEDURE(),BYTE,PROC    ! one event into Last*, 0 = queue empty
TakeEvent       PROCEDURE(),BYTE,PROC,VIRTUAL
TakeCommand     PROCEDURE(LONG cmd, SIGNED item),VIRTUAL
TakeToggled     PROCEDURE(LONG cmd, SIGNED item, BYTE checked),VIRTUAL
TakeDropDown    PROCEDURE(LONG cmd, SIGNED item, SIGNED menu),VIRTUAL
TakeTextChanged PROCEDURE(LONG cmd, SIGNED item),VIRTUAL
TakeSelChanged  PROCEDURE(LONG cmd, SIGNED item, SIGNED index),VIRTUAL
TakeColorChanged PROCEDURE(LONG cmd, SIGNED item, LONG color),VIRTUAL
TakeLayoutChanged PROCEDURE(),VIRTUAL
TakeRightClick  PROCEDURE(LONG cmd, SIGNED item),VIRTUAL
!-- a pipe-delimited list helper, public because callers building
!-- combo lists and menus keep needing it.  Item n, '' past the end.
PipeItem        PROCEDURE(STRING list, SIGNED ordinal),STRING
!-- recursive worker behind MirrorMenu
MirrorInto      PROCEDURE(SIGNED container, SIGNED menuFeq),PRIVATE
!-- Lowest field equate anywhere in a menu's subtree.  Top-level menus
!-- are sorted on it - see MirrorMenuFrom for why that is the only
!-- reliable way to recover declaration order.
MinFeqIn        PROCEDURE(SIGNED menuFeq),LONG,PRIVATE
!-- A field equate turned into its position in declaration order.  A
!-- WINDOW numbers menu controls upward from 1 and an APPLICATION frame
!-- numbers them DOWNWARD from -1, so the raw equate cannot be compared
!-- across the two - see MirrorMenuFrom.
MenuRank        PROCEDURE(SIGNED feq),LONG,PRIVATE
              END

  _EndOfInclude_
