#TEMPLATE(ClaCommandBar,'Direct2D Command Bars [v1.0 2026-08-19 09:40]'),FAMILY('ABC')
#!=============================================================================
#!  ClaCommandBar  -  Codejock-style command bars for Clarion 12 (32-bit).
#!
#!  COMMANDBAR.DLL draws the bars, the menus and every popup; CommandBarClass
#!  (CommandBar.inc / .clw) is the Clarion wrapper.  Two templates ship here:
#!
#!    CommandBarGlobal      (APPLICATION) - add ONCE per application.  Places
#!                          the class (ABC category 'COMMANDBAR'), which is
#!                          what generates the _CommandBarLinkMode_ /
#!                          _CommandBarDllMode_ project defines, and adds
#!                          commandbar.lib to the project.  In a single-EXE
#!                          app the procedure extension is self-sufficient
#!                          without it (undefined LINK/DLL modes mean
#!                          "compile the class in"), but in a multi-DLL suite
#!                          it must be on EVERY app.
#!
#!    CommandBarOnWindow    (EXTENSION, PROCEDURE) - the one you actually use.
#!                          Define bars, popup menus and items at design time;
#!                          it generates the whole build, the event pump and
#!                          one embed point per command id.
#!
#!  REQUIRED FILES - copy to a folder on the Clarion redirection path (the app
#!  folder, or clarion12\accessory\libsrc\win), all ANSI:
#!      CommandBar.inc   CommandBar.clw
#!  and put commandbar.dll beside the EXE, commandbar.lib where the linker
#!  finds it.  See INSTALL.md.
#!
#!  VERSION 1.0  -  2026-08-19 09:40
#!
#!  VERSION STAMP - THE CONVENTION.  Every edit to this chain bumps the
#!  version and refreshes the timestamp, because the #1 support symptom in
#!  the sister ClaPropGrid project was the IDE serving a STALE PARSED COPY of
#!  the template out of the registry.  With the stamp on screen you can tell
#!  at a glance whether the prompts you are looking at came from the file you
#!  just edited: if the version in the prompt sheet is not the one below,
#!  close the IDE, re-run ClarionCL -tr, reopen.
#!
#!  The template language cannot single-source it - the #TEMPLATE description
#!  is read at REGISTRATION time, long before any #GROUP can run - so the
#!  string is a literal in FOUR places and they must be kept in step:
#!      1. this comment
#!      2. the #TEMPLATE(...) description line above
#!      3. #DISPLAY on CommandBarGlobal   -> General tab
#!      4. #DISPLAY on CommandBarOnWindow -> General tab
#!=============================================================================
#!#############################################################################
#!  APPLICATION EXTENSION - CommandBarGlobal
#!#############################################################################
#EXTENSION(CommandBarGlobal,'ClaCommandBar - global settings (add once per application)'),APPLICATION
#SHEET
  #TAB('&General')
    #BOXED('ClaCommandBar')
      #DISPLAY('Version 1.0 - updated 2026-08-19 09:40')
      #DISPLAY('')
      #DISPLAY('Add this extension ONCE, at the application level.  It places')
      #DISPLAY('CommandBarClass in the build (ABC class category COMMANDBAR)')
      #DISPLAY('and adds commandbar.lib to the project.  In a multi-DLL suite')
      #DISPLAY('EVERY application in the suite needs it, or that app quietly')
      #DISPLAY('compiles a private copy of the class.')
    #ENDBOXED
    #BOXED('Options')
      #PROMPT('&Disable this template',CHECK),%CBGDisable,DEFAULT(0),AT(10)
      #PROMPT('Add commandbar.&lib to the project',CHECK),%CBGProject,DEFAULT(1),AT(10)
      #PROMPT('Import &library name:',@s64),%CBGLibName,DEFAULT('commandbar.lib')
    #ENDBOXED
    #BOXED('CommandBarClass location')
      #INSERT(%AbcLibraryPrompts(ABC))
    #ENDBOXED
  #ENDTAB
#ENDSHEET
#!
#AT(%BeforeGenerateApplication),WHERE(%CBGDisable=0)
  #CALL(%AddCategory(ABC),'COMMANDBAR')
  #CALL(%SetCategoryLocationFromPrompts(ABC),'COMMANDBAR','CommandBar','')
  #PDEFINE('_CommandBarModesSet_',1)
#ENDAT
#!
#AT(%AfterGlobalIncludes),WHERE(%CBGDisable=0)
INCLUDE('CommandBar.inc'),ONCE
#ENDAT
#!
#AT(%CustomGlobalDeclarations),WHERE(%CBGDisable=0 AND %CBGProject=1 AND %CBGLibName)
  #PROJECT(%CBGLibName)
#ENDAT
#!#############################################################################
#!  PROCEDURE EXTENSION - CommandBarOnWindow
#!#############################################################################
#!  Bars, menus and items are three lists.  An item says which container it
#!  belongs to BY NAME, and a drop / split / menu-title item says which popup
#!  menu it opens BY NAME.  Names that resolve to nothing are reported with
#!  #ERROR at generate time rather than silently producing an empty bar.
#!#############################################################################
#EXTENSION(CommandBarOnWindow,'ClaCommandBar - command bars and menus on this window'),PROCEDURE,HLP('~ClaCommandBar.htm')
#SHEET
  #TAB('&General')
    #DISPLAY('Version 1.0 - updated 2026-08-19 09:40')
    #DISPLAY('')
    #BOXED('Object')
      #PROMPT('&Disable the command bars on this window',CHECK),%CBDisable,DEFAULT(0),AT(10)
      #PROMPT('&Object name:',@s64),%CBObject,REQ,DEFAULT('CommandBar')
      #PROMPT('&Class name:',@s64),%CBClass,REQ,DEFAULT('CommandBarClass')
    #ENDBOXED
    #BOXED('Event pump')
      #PROMPT('&Timer interval, hundredths of a second (0 = leave the window alone):',SPIN(@n4,0,1000,5)),%CBTimer,DEFAULT(10)
      #DISPLAY('The bars deliver their events through a queue that is drained')
      #DISPLAY('on EVENT:Timer, so the window needs a timer.  The class only')
      #DISPLAY('sets one if the window has none of its own.')
    #ENDBOXED
    #BOXED('The client area under the bars')
      #PROMPT('&Control to fill the space the bars leave:',CONTROL),%CBFitControl
      #PROMPT('Margin &X:',SPIN(@n3,0,100,1)),%CBFitMarginX,DEFAULT(0)
      #PROMPT('Margin &Y:',SPIN(@n3,0,100,1)),%CBFitMarginY,DEFAULT(0)
      #DISPLAY('Optional.  Point this at the LIST, SHEET or REGION that should')
      #DISPLAY('sit under the bars and it is repositioned on every resize and')
      #DISPLAY('whenever a bar is shown, hidden, docked or floated.')
    #ENDBOXED
  #ENDTAB
  #TAB('&Appearance')
    #INSERT(%CBAppearancePrompts)
  #ENDTAB
  #TAB('&Colours')
    #INSERT(%CBColourPrompts)
  #ENDTAB
  #TAB('&Images')
    #BOXED('Image files')
      #DISPLAY('Any format WIC reads: .ICO, .PNG, .GIF, .JPG, .BMP - alpha')
      #DISPLAY('honoured.  The file name is resolved at RUN time, relative to')
      #DISPLAY('the working folder, so ship the images with the EXE.')
      #DISPLAY('')
      #DISPLAY('Give each one a short name; the Items list refers to it.')
      #BUTTON('&Images...'),MULTI(%CBImageList,%CBImageName & '  =  ' & %CBImageFile),INLINE
        #PROMPT('&Name:',@s32),%CBImageName,REQ
        #PROMPT('&File:',@s255),%CBImageFile,REQ
      #ENDBUTTON
    #ENDBOXED
  #ENDTAB
  #TAB('&Bars')
    #BOXED('Bars')
      #DISPLAY('One entry per bar.  Bars on the same side stack by Row; two')
      #DISPLAY('bars sharing a Row sit side by side in Offset order.')
      #DISPLAY('A menu bar is just a bar - tick "Is the menu bar" and put')
      #DISPLAY('Menu title items on it.')
      #BUTTON('&Bars...'),MULTI(%CBBarList,%CBBarName & '  (' & %CBBarDock & ', row ' & %CBBarRow & ')'),INLINE
        #PROMPT('&Name (the Items list refers to this):',@s32),%CBBarName,REQ
        #PROMPT('&Caption (shown when floating):',@s64),%CBBarTitle
        #PROMPT('&Dock:',DROP('Top|Bottom|Left|Right|Floating')),%CBBarDock,DEFAULT('Top')
        #PROMPT('&Row (0 is nearest the edge):',SPIN(@n2,0,20,1)),%CBBarRow,DEFAULT(0)
        #PROMPT('&Offset within the row:',SPIN(@n2,0,20,1)),%CBBarOffset,DEFAULT(0)
        #PROMPT('Is the &menu bar',CHECK),%CBBarMenuBar,DEFAULT(0),AT(10)
        #PROMPT('Drag &gripper',CHECK),%CBBarGripper,DEFAULT(1),AT(10)
        #PROMPT('User may &float it',CHECK),%CBBarFloatable,DEFAULT(1),AT(10)
        #PROMPT('&Large icons',CHECK),%CBBarLargeIcons,DEFAULT(0),AT(10)
        #PROMPT('No &border line',CHECK),%CBBarNoBorder,DEFAULT(0),AT(10)
        #PROMPT('&Start hidden',CHECK),%CBBarHidden,DEFAULT(0),AT(10)
      #ENDBUTTON
    #ENDBOXED
  #ENDTAB
  #TAB('&Menus')
    #BOXED('Popup menus')
      #DISPLAY('A popup menu is a container with no bar of its own.  Give it a')
      #DISPLAY('name here, put items in it on the Items tab, and hang it off a')
      #DISPLAY('Menu title / Drop button / Split button - or leave it loose and')
      #DISPLAY('call CB.PopupMenu(CBMnu:<name>) yourself for a context menu.')
      #BUTTON('&Menus...'),MULTI(%CBMenuList,%CBMenuName),INLINE
        #PROMPT('&Name:',@s32),%CBMenuName,REQ
      #ENDBUTTON
    #ENDBOXED
  #ENDTAB
  #TAB('&Items')
    #BOXED('Items')
      #DISPLAY('Items appear in the order listed, inside the bar or menu named')
      #DISPLAY('in "Put it in".  Give a command id to anything you want to act')
      #DISPLAY('on - each distinct id gets its own embed point.')
      #BUTTON('&Items...'),MULTI(%CBItemList,%CBItemContainer & ' : ' & %CBItemType & '  ' & %CBItemText & CHOOSE(%CBItemCmd = 0,'',' [' & %CBItemCmd & ']')),INLINE
        #PROMPT('&Put it in (a bar or menu name):',@s32),%CBItemContainer,REQ
        #PROMPT('&Type:',DROP('Button|Toggle button|Drop button|Split button|Separator|Label|Edit box|Combo box|Check box|Colour button|Menu title|Flexible space')),%CBItemType,DEFAULT('Button')
        #PROMPT('Te&xt (& marks the accelerator letter):',@s64),%CBItemText
        #PROMPT('&Command id (0 = it raises nothing):',SPIN(@n_9,0,999999999,1)),%CBItemCmd,DEFAULT(0)
        #PROMPT('&Image (a name from the Images tab):',@s32),%CBItemImage
        #PROMPT('T&ooltip:',@s128),%CBItemTooltip
        #ENABLE(%CBItemType='Drop button' OR %CBItemType='Split button' OR %CBItemType='Menu title' OR %CBItemType='Button')
          #PROMPT('Opens this &menu (a name from the Menus tab):',@s32),%CBItemMenu
        #ENDENABLE
        #ENABLE(%CBItemType='Combo box')
          #PROMPT('Choices, pipe se&parated (Red|Green|Blue):',@s255),%CBItemCombo
        #ENDENABLE
        #ENABLE(%CBItemType='Edit box')
          #PROMPT('Starting &value:',@s128),%CBItemValue
        #ENDENABLE
        #ENABLE(%CBItemType='Colour button')
          #PROMPT('Starting co&lour:',COLOR),%CBItemColor,DEFAULT(00000080H)
        #ENDENABLE
        #ENABLE(%CBItemType='Edit box' OR %CBItemType='Combo box' OR %CBItemType='Label' OR %CBItemType='Flexible space')
          #PROMPT('&Width in pixels (0 = automatic):',SPIN(@n4,0,2000,10)),%CBItemWidth,DEFAULT(0)
        #ENDENABLE
        #BOXED('In a popup menu')
          #PROMPT('&Shortcut text shown on the right ("Ctrl+S"):',@s32),%CBItemShortcut
          #PROMPT('&Bold (the default item)',CHECK),%CBItemDefault,DEFAULT(0),AT(10)
          #PROMPT('Check mark is a &radio dot',CHECK),%CBItemRadio,DEFAULT(0),AT(10)
        #ENDBOXED
        #BOXED('State and layout')
          #PROMPT('Starts &checked / pressed',CHECK),%CBItemChecked,DEFAULT(0),AT(10)
          #PROMPT('Starts disa&bled',CHECK),%CBItemDisabled,DEFAULT(0),AT(10)
          #PROMPT('Starts h&idden',CHECK),%CBItemHidden,DEFAULT(0),AT(10)
          #PROMPT('Clicking it flips its own check mark (&auto-check)',CHECK),%CBItemAutoCheck,DEFAULT(0),AT(10)
          #DISPLAY('Auto-check is for MENU ROWS you want to behave like a')
          #DISPLAY('setting.  Toggle buttons and check boxes always do it.')
          #PROMPT('Push it to the &far end of the bar',CHECK),%CBItemRightAlign,DEFAULT(0),AT(10)
          #PROMPT('Start a &new row after it',CHECK),%CBItemWrap,DEFAULT(0),AT(10)
          #PROMPT('Image a&bove the text (large button)',CHECK),%CBItemTextBelow,DEFAULT(0),AT(10)
          #PROMPT('Ico&n only, never the text',CHECK),%CBItemIconOnly,DEFAULT(0),AT(10)
          #PROMPT('Te&xt only, never the image',CHECK),%CBItemTextOnly,DEFAULT(0),AT(10)
          #PROMPT('S&tretch to eat the leftover width',CHECK),%CBItemStretch,DEFAULT(0),AT(10)
        #ENDBOXED
      #ENDBUTTON
    #ENDBOXED
  #ENDTAB
  #TAB('&Keys')
    #BOXED('Keyboard shortcuts')
      #DISPLAY('Each entry makes a key fire a command id.  The window is given')
      #DISPLAY('an ALRT() for the key automatically, and EVENT:AlertKey is')
      #DISPLAY('forwarded to the bars.')
      #DISPLAY('')
      #DISPLAY('Use Clarion key equates - CtrlS, F5Key, ShiftF3, AltX.')
      #BUTTON('&Shortcuts...'),MULTI(%CBAccelList,%CBAccelKey & '  ->  ' & %CBAccelCmd),INLINE
        #PROMPT('&Key (a Clarion equate, e.g. CtrlS):',@s32),%CBAccelKey,REQ
        #PROMPT('&Command id:',SPIN(@n_9,0,999999999,1)),%CBAccelCmd,REQ
      #ENDBUTTON
    #ENDBOXED
  #ENDTAB
#ENDSHEET
#!-----------------------------------------------------------------------------
#! Parse-time state.
#!
#! %CBCmds is the de-duplicated list of command ids, and it is what drives the
#! per-command CASE and its embed points.  %CBn / %CBOwner / %CBFlags are
#! scratch: every value that a generated line needs is computed into one of
#! them with #SET, because a %() group call with anything other than exactly
#! ONE argument expands empty inside a #FOR over a MULTI list (see the note at
#! the top of ClaCommandBar.tpw).
#!-----------------------------------------------------------------------------
#ATSTART
  #DECLARE(%CBn)
  #DECLARE(%CBOwner)
  #DECLARE(%CBOwnerVar)
  #DECLARE(%CBMenuVar)
  #DECLARE(%CBImageVar)
  #DECLARE(%CBFlags)
  #DECLARE(%CBBarFlags)
  #DECLARE(%CBFound)
  #IF(VAREXISTS(%CBCmds) = 0)
    #DECLARE(%CBCmds),MULTI,UNIQUE
  #ENDIF
  #FREE(%CBCmds)
  #FOR(%CBItemList),WHERE(%CBItemCmd > 0)
    #ADD(%CBCmds,%CBItemCmd)
  #ENDFOR
  #FOR(%CBAccelList),WHERE(%CBAccelCmd > 0)
    #ADD(%CBCmds,%CBAccelCmd)
  #ENDFOR
  #IF(%CBClass = '')
    #SET(%CBClass,'CommandBarClass')
  #ENDIF
  #IF(%CBObject = '')
    #SET(%CBObject,'CommandBar')
  #ENDIF
#ENDAT
#!
#! The class TYPE must be visible in EVERY module, so the include goes to
#! %AfterGlobalIncludes (PROGRAM-module global scope), not
#! %CustomGlobalDeclarations.  ONCE de-dupes against the global extension.
#AT(%AfterGlobalIncludes),WHERE(%CBDisable=0)
INCLUDE('CommandBar.inc'),ONCE
#ENDAT
#!
#AT(%CustomGlobalDeclarations),WHERE(%CBDisable=0)
  #PROJECT('commandbar.lib')
#ENDAT
#!
#!-----------------------------------------------------------------------------
#! Data: the object, and one variable per image, bar, menu and item.
#! Everything is keyed on %ActiveTemplateInstance so two of these on one
#! window cannot collide.
#!-----------------------------------------------------------------------------
#AT(%DataSection),WHERE(%CBDisable=0)
%CBObject            %CBClass                                    ! ClaCommandBar object
CBRelayout:%CBObject EQUATE(EVENT:User + 400 + %ActiveTemplateInstance) ! private "the window settled" event
  #FOR(%CBImageList)
    #SET(%CBn,INSTANCE(%CBImageList))
CBImg:%CBImageName   SIGNED                                      ! image '%CBImageFile'
  #ENDFOR
  #FOR(%CBBarList)
CBBar:%CBBarName     SIGNED                                      ! bar '%CBBarName'
  #ENDFOR
  #FOR(%CBMenuList)
CBMnu:%CBMenuName    SIGNED                                      ! menu '%CBMenuName'
  #ENDFOR
  #FOR(%CBItemList)
    #SET(%CBn,INSTANCE(%CBItemList))
CBItm:%ActiveTemplateInstance:%CBn SIGNED                        ! %CBItemType in '%CBItemContainer'
  #ENDFOR
#ENDAT
#!
#!-----------------------------------------------------------------------------
#! The window needs an ALRT for every shortcut key, or EVENT:AlertKey never
#! fires and the accelerators are dead.
#!-----------------------------------------------------------------------------
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8590),WHERE(%CBDisable=0 AND ITEMS(%CBAccelList)),DESCRIPTION('ClaCommandBar: alert the shortcut keys')
  #FOR(%CBAccelList),WHERE(%CBAccelKey)
  %Window{PROP:Alrt,255} = %CBAccelKey
  #ENDFOR
#ENDAT
#!
#! PRIORITY(8600): ABC opens the window at 8000, restores its INI size at 8250
#! and runs the field templates at 8500, so at 8600 the window is at its final
#! size and PROP:Handle is live.
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8600),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: build the command bars')
  %CBObject.TimerInterval = %CBTimer
  #EMBED(%CBBeforeInit,'ClaCommandBar: before the bars are created'),%ActiveTemplateInstance
  IF %CBObject.Init(%Window, %(%CBStyleNumber()))                 ! CBS: style bits
    %CBObject.SetTheme(%(%CBThemeEquate(%CBTheme)))
  #IF(%CBUseAccent)
    %CBObject.SetAccent(%CBAccent)
  #ENDIF
  #FOR(%CBColourList),WHERE(%CBColourSlot)
    %CBObject.SetColor(%(%CBSlotEquate(%CBColourSlot)),%CBColourValue)
  #ENDFOR
  #IF(%CBFontItemFace)
    %CBObject.SetFont(CBF:Item,'%CBFontItemFace',%CBFontItemSize,%CBFontItemBold,0)
  #ENDIF
  #IF(%CBFontMenuFace)
    %CBObject.SetFont(CBF:Menu,'%CBFontMenuFace',%CBFontMenuSize,%CBFontMenuBold,0)
    %CBObject.SetFont(CBF:Tooltip,'%CBFontMenuFace',%CBFontMenuSize,0,0)
  #ENDIF
  #IF(%CBIconSize > 0)
    %CBObject.SetMetric(CBM:IconSize,%CBIconSize)
  #ENDIF
  #IF(%CBLargeIcon > 0)
    %CBObject.SetMetric(CBM:LargeIcon,%CBLargeIcon)
  #ENDIF
  #IF(%CBItemHeight > 0)
    %CBObject.SetMetric(CBM:ItemHeight,%CBItemHeight)
  #ENDIF
  #IF(%CBCorner >= 0)
    %CBObject.SetMetric(CBM:Corner,%CBCorner)
  #ENDIF
  #IF(%CBMenuWidth > 0)
    %CBObject.SetMetric(CBM:MenuWidth,%CBMenuWidth)
  #ENDIF
  #!---- images ----
  #FOR(%CBImageList),WHERE(%CBImageName AND %CBImageFile)
    CBImg:%CBImageName = %CBObject.AddImage('%CBImageFile')
  #ENDFOR
  #!---- menus first: an item can only be hung on a menu that exists ----
  #FOR(%CBMenuList),WHERE(%CBMenuName)
    CBMnu:%CBMenuName = %CBObject.CreateMenu()
  #ENDFOR
  #!---- bars ----
  #FOR(%CBBarList),WHERE(%CBBarName)
    #SET(%CBBarFlags,0)
    #IF(%CBBarMenuBar)
      #SET(%CBBarFlags,%CBBarFlags + 1)
    #ENDIF
    #IF(%CBBarGripper)
      #SET(%CBBarFlags,%CBBarFlags + 2)
    #ENDIF
    #IF(%CBBarFloatable)
      #SET(%CBBarFlags,%CBBarFlags + 4)
    #ENDIF
    #IF(%CBBarNoBorder)
      #SET(%CBBarFlags,%CBBarFlags + 16)
    #ENDIF
    #IF(%CBBarLargeIcons)
      #SET(%CBBarFlags,%CBBarFlags + 32)
    #ENDIF
    CBBar:%CBBarName = %CBObject.AddBar('%CBBarTitle',%(%CBDockEquate(%CBBarDock)),%CBBarFlags)
    %CBObject.SetBarDock(CBBar:%CBBarName,%(%CBDockEquate(%CBBarDock)),%CBBarRow,%CBBarOffset)
    #IF(%CBBarHidden)
    %CBObject.SetBarVisible(CBBar:%CBBarName,0)
    #ENDIF
  #ENDFOR
  #!---- items ----
  #FOR(%CBItemList)
    #SET(%CBn,INSTANCE(%CBItemList))
    #!  Resolve the container name against the bar list, then the menu list.
    #!  Two nested #FORs rather than a #GROUP call: a group call with more
    #!  than one argument expands to nothing in here (ClaCommandBar.tpw).
    #SET(%CBOwnerVar,'')
    #FOR(%CBBarList),WHERE(UPPER(%CBBarName) = UPPER(%CBItemContainer))
      #SET(%CBOwnerVar,'CBBar:' & %CBBarName)
      #BREAK
    #ENDFOR
    #IF(%CBOwnerVar = '')
      #FOR(%CBMenuList),WHERE(UPPER(%CBMenuName) = UPPER(%CBItemContainer))
        #SET(%CBOwnerVar,'CBMnu:' & %CBMenuName)
        #BREAK
      #ENDFOR
    #ENDIF
    #IF(%CBOwnerVar = '')
      #ERROR('ClaCommandBar: item ' & %CBn & ' says "Put it in: ' & %CBItemContainer & '" but there is no bar or menu with that name.')
    #ELSE
      #!  the popup this item opens, if any
      #SET(%CBMenuVar,'0')
      #IF(%CBItemMenu)
        #SET(%CBFound,0)
        #FOR(%CBMenuList),WHERE(UPPER(%CBMenuName) = UPPER(%CBItemMenu))
          #SET(%CBMenuVar,'CBMnu:' & %CBMenuName)
          #SET(%CBFound,1)
          #BREAK
        #ENDFOR
        #IF(%CBFound = 0)
          #ERROR('ClaCommandBar: item ' & %CBn & ' opens menu "' & %CBItemMenu & '" but there is no menu with that name.')
        #ENDIF
      #ENDIF
      #!  the image, if any
      #SET(%CBImageVar,'0')
      #IF(%CBItemImage)
        #SET(%CBFound,0)
        #FOR(%CBImageList),WHERE(UPPER(%CBImageName) = UPPER(%CBItemImage))
          #SET(%CBImageVar,'CBImg:' & %CBImageName)
          #SET(%CBFound,1)
          #BREAK
        #ENDFOR
        #IF(%CBFound = 0)
          #ERROR('ClaCommandBar: item ' & %CBn & ' uses image "' & %CBItemImage & '" but there is no image with that name.')
        #ENDIF
      #ENDIF
      #!  the CBIS: style bits
      #SET(%CBFlags,0)
      #IF(%CBItemTextOnly)
        #SET(%CBFlags,%CBFlags + 1)
      #ENDIF
      #IF(%CBItemIconOnly)
        #SET(%CBFlags,%CBFlags + 2)
      #ENDIF
      #IF(%CBItemTextBelow)
        #SET(%CBFlags,%CBFlags + 4)
      #ENDIF
      #IF(%CBItemWrap)
        #SET(%CBFlags,%CBFlags + 16)
      #ENDIF
      #IF(%CBItemRightAlign)
        #SET(%CBFlags,%CBFlags + 32)
      #ENDIF
      #IF(%CBItemDefault)
        #SET(%CBFlags,%CBFlags + 64)
      #ENDIF
      #IF(%CBItemRadio)
        #SET(%CBFlags,%CBFlags + 128)
      #ENDIF
      #!  A toggle or a check box is checkable by definition, and
      #!  SetItemStyle REPLACES the style word - so the flag the DLL
      #!  sets for those types itself has to be put back here, or
      #!  ticking nothing would leave a toggle that never stays down.
      #IF(%CBItemAutoCheck OR %CBItemType = 'Toggle button' OR %CBItemType = 'Check box')
        #SET(%CBFlags,%CBFlags + 256)
      #ENDIF
      #IF(%CBItemStretch)
        #SET(%CBFlags,%CBFlags + 512)
      #ENDIF
    CBItm:%ActiveTemplateInstance:%CBn = %CBObject.AddItem(%CBOwnerVar,%(%CBTypeEquate(%CBItemType)),%CBItemCmd,'%CBItemText',%CBImageVar)
      #IF(%CBFlags)
    %CBObject.SetItemStyle(CBItm:%ActiveTemplateInstance:%CBn,%CBFlags)
      #ENDIF
      #IF(%CBItemMenu)
    %CBObject.SetItemMenu(CBItm:%ActiveTemplateInstance:%CBn,%CBMenuVar)
      #ENDIF
      #IF(%CBItemTooltip)
    %CBObject.SetItemTooltip(CBItm:%ActiveTemplateInstance:%CBn,'%CBItemTooltip')
      #ENDIF
      #IF(%CBItemShortcut)
    %CBObject.SetItemShortcut(CBItm:%ActiveTemplateInstance:%CBn,'%CBItemShortcut')
      #ENDIF
      #IF(%CBItemWidth > 0)
    %CBObject.SetItemWidth(CBItm:%ActiveTemplateInstance:%CBn,%CBItemWidth)
      #ENDIF
      #IF(%CBItemCombo)
    %CBObject.SetComboList(CBItm:%ActiveTemplateInstance:%CBn,'%CBItemCombo')
    %CBObject.SetComboSel(CBItm:%ActiveTemplateInstance:%CBn,0)
      #ENDIF
      #IF(%CBItemValue)
    %CBObject.SetItemValue(CBItm:%ActiveTemplateInstance:%CBn,'%CBItemValue')
      #ENDIF
      #IF(%CBItemType = 'Colour button')
    %CBObject.SetItemColor(CBItm:%ActiveTemplateInstance:%CBn,%CBItemColor)
      #ENDIF
      #IF(%CBItemChecked)
    %CBObject.SetItemChecked(CBItm:%ActiveTemplateInstance:%CBn,1)
      #ENDIF
      #IF(%CBItemDisabled)
    %CBObject.SetItemEnabled(CBItm:%ActiveTemplateInstance:%CBn,0)
      #ENDIF
      #IF(%CBItemHidden)
    %CBObject.SetItemVisible(CBItm:%ActiveTemplateInstance:%CBn,0)
      #ENDIF
    #ENDIF
  #ENDFOR
  #!---- keyboard shortcuts ----
  #FOR(%CBAccelList),WHERE(%CBAccelKey AND %CBAccelCmd > 0)
    %CBObject.AddClarionKey(%CBAccelCmd,%CBAccelKey)
  #ENDFOR
    #EMBED(%CBAfterBuild,'ClaCommandBar: after the bars are built (add your own items here)'),%ActiveTemplateInstance
    %CBObject.Layout()
    DO CBFit:%CBObject
  END
#ENDAT
#!
#! Self-contained CASE EVENT() at PRIORITY(2000) - ABOVE the framework's own
#! LOOP/CASE scaffolding, which is registered at 2500 (ABWINDOW.TPW:563).
#! Using 2500 interleaves and produces a duplicate CASE EVENT().
#! EVENT:Sized POSTs a private event rather than re-fitting straight away,
#! because at the top of TakeWindowEvent the ABC resizer has not moved the
#! window's own controls yet.
#AT(%WindowManagerMethodCodeSection,'TakeWindowEvent','(),BYTE'),PRIORITY(2000),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: event pump, keys and resize')
  CASE EVENT()
  OF EVENT:Timer
    DO CBPump:%CBObject
  OF EVENT:Sized
    POST(CBRelayout:%CBObject)
  OF CBRelayout:%CBObject
    %CBObject.Layout()
    DO CBFit:%CBObject
#IF(ITEMS(%CBAccelList))
  OF EVENT:AlertKey
    IF %CBObject.TakeAlertKey(KEYCODE())
      DO CBPump:%CBObject
      RETURN Level:Notify
    END
#ENDIF
  END
#ENDAT
#!
#! PRIORITY(7500) is above ABC's "Kill already called" short-stop (5100), and
#! CommandBarClass.Kill is idempotent anyway.
#AT(%WindowManagerMethodCodeSection,'Kill','(),BYTE'),PRIORITY(7500),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: destroy the command bars')
%CBObject.Kill()
#ENDAT
#!
#!-----------------------------------------------------------------------------
#! The two ROUTINEs.  They live in the procedure, so ThisWindow's methods can
#! DO them - exactly like ABC's own "Do DefineListboxStyle" (ABWINDOW.TPW:438).
#!
#! CBPump drains the queue one event at a time and dispatches on the command
#! id, so each command gets its own embed point.
#!-----------------------------------------------------------------------------
#AT(%ProcedureRoutines),WHERE(%CBDisable=0)
!---------------------------------------------------------------------------
CBFit:%CBObject ROUTINE
!  Put the client control back in whatever space the bars left.
#IF(%CBFitControl)
  %CBObject.FitControl(%CBFitControl,%CBFitMarginX,%CBFitMarginY)
  DISPLAY
#ELSE
  !  No client control chosen on the General tab - nothing to re-fit.
  !  CB.ClientX/Y/Width/Height still report the space the bars left.
#ENDIF
  #EMBED(%CBOnFit,'ClaCommandBar: after the client control was re-fitted'),%ActiveTemplateInstance
!---------------------------------------------------------------------------
CBPump:%CBObject ROUTINE
!  Drain the queue.  Everything the bars raise arrives here.
  LOOP WHILE %CBObject.TakeOne()
    #EMBED(%CBAnyEvent,'ClaCommandBar: any event, before it is dispatched'),%ActiveTemplateInstance
    CASE %CBObject.LastEvent
    OF CBE:Command
    #IF(ITEMS(%CBCmds))
      CASE %CBObject.LastCmd
      #FOR(%CBCmds)
      OF %CBCmds
        #!  The description must be a literal - the template parser will
        #!  not take an expression here.  The command id is carried by
        #!  the %CBCmds instance qualifier, so AppGen still lists one
        #!  embed point per command and names it after the id.
        #EMBED(%CBOnCommand,'ClaCommandBar: this command was chosen'),%ActiveTemplateInstance,%CBCmds
      #ENDFOR
      END
    #ELSE
      !  No command ids are defined on the Items tab yet.
    #ENDIF
    OF CBE:Toggled
      #EMBED(%CBOnToggled,'ClaCommandBar: a toggle or check box flipped (LastParam = 1 when on)'),%ActiveTemplateInstance
    OF CBE:TextChanged
      #EMBED(%CBOnTextChanged,'ClaCommandBar: an edit box was committed'),%ActiveTemplateInstance
    OF CBE:SelChanged
      #EMBED(%CBOnSelChanged,'ClaCommandBar: a combo selection changed (LastParam = the 0-based index)'),%ActiveTemplateInstance
    OF CBE:ColorChanged
      #EMBED(%CBOnColorChanged,'ClaCommandBar: a colour button changed (LastParam = the colour)'),%ActiveTemplateInstance
    OF CBE:RightClick
      #EMBED(%CBOnRightClick,'ClaCommandBar: an item was right-clicked'),%ActiveTemplateInstance
    OF CBE:DropDown
      #EMBED(%CBOnDropDown,'ClaCommandBar: a menu is about to open'),%ActiveTemplateInstance
    OF CBE:Layout
      DO CBFit:%CBObject
    END
  END
#ENDAT
#!=============================================================================
#!  SHARED GROUPS
#!
#!  These lived in a separate ClaCommandBar.tpw at first.  They are inlined
#!  here on purpose: a template #INCLUDE is resolved against the CURRENT
#!  DIRECTORY, not against the folder the .tpl sits in, so a two-file chain
#!  can only be registered one of two broken ways -
#!
#!    ClarionCL -tr C:ull\path\ClaCommandBar.tpl   -> cannot find the .tpw
#!    cd C:ull\path & ClarionCL -tr ClaCommandBar.tpl
#!        -> registers, but the registry keeps the RELATIVE path and every
#!           app in the IDE then fails to open with
#!           "Could not open include file ClaCommandBar.tpl"
#!
#!  One self-contained file has neither problem: register it from anywhere,
#!  with a full path, and it works.  A #GROUP has no end marker, which is
#!  why they all sit at the very END of the file, after every #AT block.
#!=============================================================================
#!-----------------------------------------------------------------------------
#! %CBAppearancePrompts - theme, fonts and metrics.
#!-----------------------------------------------------------------------------
#GROUP(%CBAppearancePrompts)
  #BOXED('Theme')
    #PROMPT('&Theme:',DROP('Steel Blue|Office 2003|Office 2007|Office 2010|Office 2013|Office 2016|VS 2012 Light|VS 2012 Dark|Windows 11 Light|Windows 11 Dark|Slate Dark')),%CBTheme,DEFAULT('Steel Blue')
    #DISPLAY('A theme sets all 37 colours at once.  The Colours tab below')
    #DISPLAY('then overrides individual ones on top of it.')
    #PROMPT('Re-colour the whole theme around one &accent colour',CHECK),%CBUseAccent,DEFAULT(0),AT(10)
    #ENABLE(%CBUseAccent)
      #PROMPT('A&ccent:',COLOR),%CBAccent,DEFAULT(002B69A8H)
      #DISPLAY('Every hover, pressed, checked and highlight shade is derived')
      #DISPLAY('from the accent, so one colour re-skins the whole set and')
      #DISPLAY('the light/dark character of the theme is kept.')
    #ENDENABLE
  #ENDBOXED
  #BOXED('Look')
    #PROMPT('Show &tooltips',CHECK),%CBStyleTooltips,DEFAULT(1),AT(10)
    #PROMPT('O&verflow chevron when a bar is too narrow',CHECK),%CBStyleChevron,DEFAULT(1),AT(10)
    #PROMPT('&Icon gutter down the popup menus',CHECK),%CBStyleMenuIcons,DEFAULT(1),AT(10)
    #PROMPT('&Recolour item text on hover',CHECK),%CBStyleHotText,DEFAULT(0),AT(10)
    #PROMPT('&Flat look',CHECK),%CBStyleFlat,DEFAULT(0),AT(10)
  #ENDBOXED
  #BOXED('Bar item font')
    #PROMPT('Fa&ce:',@s32),%CBFontItemFace,DEFAULT('Segoe UI')
    #PROMPT('Si&ze (pt):',SPIN(@n3,6,48,1)),%CBFontItemSize,DEFAULT(9)
    #PROMPT('B&old',CHECK),%CBFontItemBold,DEFAULT(0),AT(10)
  #ENDBOXED
  #BOXED('Popup menu font')
    #PROMPT('Fac&e:',@s32),%CBFontMenuFace,DEFAULT('Segoe UI')
    #PROMPT('Size (&pt):',SPIN(@n3,6,48,1)),%CBFontMenuSize,DEFAULT(9)
    #PROMPT('Bol&d',CHECK),%CBFontMenuBold,DEFAULT(0),AT(10)
  #ENDBOXED
  #BOXED('Metrics (design pixels at 96 DPI - the DLL scales them)')
    #PROMPT('&Small icon size:',SPIN(@n3,8,64,2)),%CBIconSize,DEFAULT(16)
    #PROMPT('&Large icon size (bars marked Large icons):',SPIN(@n3,16,128,4)),%CBLargeIcon,DEFAULT(32)
    #DISPLAY('Match the small icon size to the artwork you actually have.')
    #DISPLAY('Clarion ships 32x32 icons; squeezing those into a 16px slot')
    #DISPLAY('greys out every one-pixel stroke however good the filter is.')
    #PROMPT('Item &height (0 = work it out from the font):',SPIN(@n3,0,200,1)),%CBItemHeight,DEFAULT(0)
    #PROMPT('Button corner &radius (0 = square):',SPIN(@n2,0,16,1)),%CBCorner,DEFAULT(-1)
    #DISPLAY('Corner radius -1 keeps whatever the theme chose.')
    #PROMPT('Minimum popup &menu width:',SPIN(@n4,60,600,10)),%CBMenuWidth,DEFAULT(130)
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBColourPrompts - individual slot overrides, applied ON TOP of the theme.
#! A list, rather than 37 prompts, so the sheet stays readable.
#!-----------------------------------------------------------------------------
#GROUP(%CBColourPrompts)
  #BOXED('Colour overrides')
    #DISPLAY('Leave this empty and the theme decides everything.  Each entry')
    #DISPLAY('replaces one slot after the theme has been loaded.')
    #DISPLAY('')
    #DISPLAY('Clarion and Win32 both store a colour as 0BBGGRRh, so the hex')
    #DISPLAY('digits read blue-green-red.  Use the picker and forget it.')
    #BUTTON('Colour &overrides...'),MULTI(%CBColourList,%CBColourSlot),INLINE
      #PROMPT('&Slot:',DROP('Bar background|Bar background (gradient bottom)|Bar border|Item text|Item text hot|Item text disabled|Hover fill|Hover border|Pressed fill|Pressed border|Checked fill|Checked border|Separator|Gripper|Menu background|Menu border|Menu gutter|Menu highlight|Menu highlight border|Menu text|Menu text highlighted|Menu text disabled|Menu shortcut|Menu separator|Menu check mark|Floating caption back|Floating caption text|Floating border|Chevron|Edit background|Edit border|Edit text|Edit selection|Tooltip background|Tooltip border|Tooltip text|Accent')),%CBColourSlot,REQ
      #PROMPT('&Colour:',COLOR),%CBColourValue
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBThemeEquate(theme text) - map the DROP choice to the CBT: equate.
#!-----------------------------------------------------------------------------
#GROUP(%CBThemeEquate,%pTheme)
  #CASE(%pTheme)
  #OF('Office 2003')
    #RETURN('CBT:Office2003')
  #OF('Office 2007')
    #RETURN('CBT:Office2007')
  #OF('Office 2010')
    #RETURN('CBT:Office2010')
  #OF('Office 2013')
    #RETURN('CBT:Office2013')
  #OF('Office 2016')
    #RETURN('CBT:Office2016')
  #OF('VS 2012 Light')
    #RETURN('CBT:VS2012Light')
  #OF('VS 2012 Dark')
    #RETURN('CBT:VS2012Dark')
  #OF('Windows 11 Light')
    #RETURN('CBT:Win11Light')
  #OF('Windows 11 Dark')
    #RETURN('CBT:Win11Dark')
  #OF('Slate Dark')
    #RETURN('CBT:SlateDark')
  #ELSE
    #RETURN('CBT:SteelBlue')
  #ENDCASE
#!
#!-----------------------------------------------------------------------------
#! %CBDockEquate(dock text) - called from inside #FOR(%CBBarList): ONE argument.
#!-----------------------------------------------------------------------------
#GROUP(%CBDockEquate,%pDock)
  #CASE(%pDock)
  #OF('Bottom')
    #RETURN('CBD:Bottom')
  #OF('Left')
    #RETURN('CBD:Left')
  #OF('Right')
    #RETURN('CBD:Right')
  #OF('Floating')
    #RETURN('CBD:Float')
  #ELSE
    #RETURN('CBD:Top')
  #ENDCASE
#!
#!-----------------------------------------------------------------------------
#! %CBTypeEquate(item type text) - called from inside #FOR(%CBItemList): ONE arg.
#!-----------------------------------------------------------------------------
#GROUP(%CBTypeEquate,%pType)
  #CASE(%pType)
  #OF('Toggle button')
    #RETURN('CBI:Toggle')
  #OF('Drop button')
    #RETURN('CBI:DropDown')
  #OF('Split button')
    #RETURN('CBI:Split')
  #OF('Separator')
    #RETURN('CBI:Separator')
  #OF('Label')
    #RETURN('CBI:Label')
  #OF('Edit box')
    #RETURN('CBI:Edit')
  #OF('Combo box')
    #RETURN('CBI:Combo')
  #OF('Check box')
    #RETURN('CBI:CheckBox')
  #OF('Colour button')
    #RETURN('CBI:Color')
  #OF('Menu title')
    #RETURN('CBI:Menu')
  #OF('Flexible space')
    #RETURN('CBI:Space')
  #ELSE
    #RETURN('CBI:Button')
  #ENDCASE
#!
#!-----------------------------------------------------------------------------
#! %CBSlotEquate(slot text) - called from inside #FOR(%CBColourList): ONE arg.
#!-----------------------------------------------------------------------------
#GROUP(%CBSlotEquate,%pSlot)
  #CASE(%pSlot)
  #OF('Bar background (gradient bottom)')
    #RETURN('CBC:BarBack2')
  #OF('Bar border')
    #RETURN('CBC:BarBorder')
  #OF('Item text')
    #RETURN('CBC:ItemText')
  #OF('Item text hot')
    #RETURN('CBC:ItemTextHot')
  #OF('Item text disabled')
    #RETURN('CBC:ItemTextDis')
  #OF('Hover fill')
    #RETURN('CBC:HotBack')
  #OF('Hover border')
    #RETURN('CBC:HotBorder')
  #OF('Pressed fill')
    #RETURN('CBC:PressBack')
  #OF('Pressed border')
    #RETURN('CBC:PressBorder')
  #OF('Checked fill')
    #RETURN('CBC:CheckBack')
  #OF('Checked border')
    #RETURN('CBC:CheckBorder')
  #OF('Separator')
    #RETURN('CBC:Separator')
  #OF('Gripper')
    #RETURN('CBC:Gripper')
  #OF('Menu background')
    #RETURN('CBC:MenuBack')
  #OF('Menu border')
    #RETURN('CBC:MenuBorder')
  #OF('Menu gutter')
    #RETURN('CBC:MenuGutter')
  #OF('Menu highlight')
    #RETURN('CBC:MenuHot')
  #OF('Menu highlight border')
    #RETURN('CBC:MenuHotBorder')
  #OF('Menu text')
    #RETURN('CBC:MenuText')
  #OF('Menu text highlighted')
    #RETURN('CBC:MenuTextHot')
  #OF('Menu text disabled')
    #RETURN('CBC:MenuTextDis')
  #OF('Menu shortcut')
    #RETURN('CBC:MenuShortcut')
  #OF('Menu separator')
    #RETURN('CBC:MenuSep')
  #OF('Menu check mark')
    #RETURN('CBC:MenuCheck')
  #OF('Floating caption back')
    #RETURN('CBC:CaptionBack')
  #OF('Floating caption text')
    #RETURN('CBC:CaptionText')
  #OF('Floating border')
    #RETURN('CBC:FloatBorder')
  #OF('Chevron')
    #RETURN('CBC:Chevron')
  #OF('Edit background')
    #RETURN('CBC:EditBack')
  #OF('Edit border')
    #RETURN('CBC:EditBorder')
  #OF('Edit text')
    #RETURN('CBC:EditText')
  #OF('Edit selection')
    #RETURN('CBC:EditSel')
  #OF('Tooltip background')
    #RETURN('CBC:TipBack')
  #OF('Tooltip border')
    #RETURN('CBC:TipBorder')
  #OF('Tooltip text')
    #RETURN('CBC:TipText')
  #OF('Accent')
    #RETURN('CBC:Accent')
  #ELSE
    #RETURN('CBC:BarBack')
  #ENDCASE
#!
#!-----------------------------------------------------------------------------
#! %CBStyleNumber - the CBS: manager style bits as a plain number.
#!-----------------------------------------------------------------------------
#GROUP(%CBStyleNumber)
  #RETURN(%CBStyleTooltips + 2 * %CBStyleChevron + 4 * %CBStyleFlat + 8 * %CBStyleHotText + 16 * %CBStyleMenuIcons)
#!
#!=============================================================================
#! End of ClaCommandBar.tpw
#!=============================================================================
