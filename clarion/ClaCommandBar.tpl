#TEMPLATE(ClaCommandBar,'Direct2D Command Bars and Ribbons [v1.1 2026-08-19 14:20]'),FAMILY('ABC')
#!=============================================================================
#!  ClaCommandBar  -  Codejock-style command bars, ribbons and menus for
#!  Clarion 12 (32-bit).
#!
#!  COMMANDBAR.DLL draws everything; CommandBarClass (CommandBar.inc / .clw)
#!  is the Clarion wrapper.  FOUR templates ship here:
#!
#!    CommandBarGlobal      (APPLICATION) - add ONCE per application.  Places
#!                          the class (ABC category 'COMMANDBAR'), which is
#!                          what generates the _CommandBarLinkMode_ /
#!                          _CommandBarDllMode_ project defines, and adds
#!                          commandbar.lib to the project.  In a single-EXE
#!                          app the others are self-sufficient without it; in
#!                          a multi-DLL suite it must be on EVERY app.
#!
#!    CommandBarOnWindow    (EXTENSION, PROCEDURE) - bars, ribbons and menus
#!                          on any procedure that has a window.
#!
#!    CommandBarControl     (CONTROL, MULTI) - the same thing, but DROPPED on
#!                          the window from the control palette.  It places a
#!                          REGION, and a bar can be told to land exactly on
#!                          that region instead of docking to a window edge -
#!                          which is what you want on a browse or a form.
#!
#!    CommandBarFrame       (EXTENSION, PROCEDURE) - for an application FRAME.
#!                          Everything the others do, plus it can MIRROR the
#!                          frame's own MENUBAR onto a command bar and
#!                          optionally take the original menu off the frame.
#!                          Mirrored rows POST EVENT:Accepted to the original
#!                          ITEMs, so existing menu embed code keeps working.
#!
#!  REQUIRED FILES - copy to a folder on the Clarion redirection path (the app
#!  folder, or clarion12\accessory\libsrc\win), all ANSI with CRLF:
#!      CommandBar.inc   CommandBar.clw
#!  and put commandbar.dll beside the EXE, commandbar.lib where the linker
#!  finds it.  See INSTALL.md.
#!
#!  VERSION 1.1  -  2026-08-19 14:20
#!
#!  VERSION STAMP - THE CONVENTION.  Every edit bumps the version and the
#!  timestamp, because the #1 support symptom in the sister ClaPropGrid
#!  project was the IDE serving a STALE PARSED COPY out of the registry.  If
#!  the stamp in the prompt sheet is not the one below, close the IDE, re-run
#!  ClarionCL -tr, reopen.  The string is a literal in FIVE places - this
#!  comment, the #TEMPLATE line, and a #DISPLAY on each of the four templates.
#!=============================================================================
#!#############################################################################
#!  APPLICATION EXTENSION - CommandBarGlobal
#!#############################################################################
#EXTENSION(CommandBarGlobal,'ClaCommandBar - global settings (add once per application)'),APPLICATION
#SHEET
  #TAB('&General')
    #BOXED('ClaCommandBar')
      #DISPLAY('Version 1.1 - updated 2026-08-19 14:20')
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
#EXTENSION(CommandBarOnWindow,'ClaCommandBar - command bars and menus on this window'),PROCEDURE,HLP('~ClaCommandBar.htm')
#SHEET
  #TAB('&General')
    #DISPLAY('Version 1.1 - updated 2026-08-19 14:20')
    #DISPLAY('')
    #INSERT(%CBGeneralPrompts)
  #ENDTAB
  #TAB('&Appearance')
    #INSERT(%CBAppearancePrompts)
  #ENDTAB
  #TAB('&Colours')
    #INSERT(%CBColourPrompts)
  #ENDTAB
  #TAB('I&mages')
    #INSERT(%CBImagePrompts)
  #ENDTAB
  #TAB('&Bars')
    #INSERT(%CBBarPrompts)
  #ENDTAB
  #TAB('&Ribbon')
    #INSERT(%CBRibbonPrompts)
  #ENDTAB
  #TAB('Men&us')
    #INSERT(%CBMenuPrompts)
  #ENDTAB
  #TAB('&Items')
    #INSERT(%CBItemPrompts)
  #ENDTAB
  #TAB('&Keys')
    #INSERT(%CBAccelPrompts)
  #ENDTAB
#ENDSHEET
#ATSTART
  #INSERT(%CBDeclarations)
#ENDAT
#!
#AT(%AfterGlobalIncludes),WHERE(%CBDisable=0)
INCLUDE('CommandBar.inc'),ONCE
#ENDAT
#AT(%CustomGlobalDeclarations),WHERE(%CBDisable=0)
  #PROJECT('commandbar.lib')
#ENDAT
#AT(%DataSection),WHERE(%CBDisable=0)
#INSERT(%CBEmitData)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8590),WHERE(%CBDisable=0 AND ITEMS(%CBAccelList)),DESCRIPTION('ClaCommandBar: alert the shortcut keys')
  #INSERT(%CBEmitAlerts)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8600),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: build the command bars')
  #EMBED(%CBBeforeInit,'ClaCommandBar: before the bars are created'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuild)
    #EMBED(%CBAfterBuild,'ClaCommandBar: after the bars are built (add your own items here)'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuildEnd)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'TakeWindowEvent','(),BYTE'),PRIORITY(2000),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: event pump, keys and resize')
  #INSERT(%CBEmitTakeEvent)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Kill','(),BYTE'),PRIORITY(7500),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: destroy the command bars')
%CBObject.Kill()
#ENDAT
#AT(%ProcedureRoutines),WHERE(%CBDisable=0)
#INSERT(%CBEmitFitRoutine)
!---------------------------------------------------------------------------
CBPump:%ActiveTemplateInstance ROUTINE
!  Drain the queue.  Everything the bars raise arrives here.
!
!  The per-command CASE is written out here rather than in a shared
!  #GROUP for two reasons: a #GROUP has to be BALANCED, so its #IF and
!  #FOR cannot span two groups, and an #EMBED inside a #GROUP does not
!  register as an embed point.
  LOOP WHILE %CBObject.TakeOne()
    CASE %CBObject.LastEvent
    OF CBE:Command
#IF(ITEMS(%CBCmds))
      CASE %CBObject.LastCmd
  #FOR(%CBCmds)
      OF %CBCmds
    #SET(%CBActCmd,%CBCmds)
    #!  The FIRST item carrying this command id decides the action.
    #FOR(%CBItemList),WHERE(%CBItemCmd = %CBActCmd)
      #CASE(%CBItemAction)
      #OF('Call a procedure')
        #IF(%CBItemProc)
        %CBItemProc(%CBItemProcParms)
        #ENDIF
      #OF('Do a routine')
        #IF(%CBItemRoutine)
        DO %CBItemRoutine
        #ENDIF
      #OF('Emulate a control')
        #IF(%CBItemControl)
        POST(EVENT:Accepted,%CBItemControl)
        #ENDIF
      #OF('Post an event')
        #IF(%CBItemEvent)
          #IF(%CBItemEventCtl)
        POST(%CBItemEvent,%CBItemEventCtl)
          #ELSE
        POST(%CBItemEvent)
          #ENDIF
        #ENDIF
      #OF('Close the window')
        POST(EVENT:CloseWindow)
      #ENDCASE
      #BREAK
    #ENDFOR
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
    OF CBE:TabChanged
      #EMBED(%CBOnTabChanged,'ClaCommandBar: a ribbon tab was switched (LastParam = the tab)'),%ActiveTemplateInstance
    OF CBE:DropDown
      #EMBED(%CBOnDropDown,'ClaCommandBar: a menu is about to open'),%ActiveTemplateInstance
    OF CBE:Layout
      DO CBFit:%ActiveTemplateInstance
    END
  END
#ENDAT
#!#############################################################################
#!  CONTROL TEMPLATE - CommandBarControl
#!#############################################################################
#!  Dropped from the control palette onto a window, a browse or a form.  It
#!  places a REGION; tick "Land on the region" on a bar and that bar fills the
#!  region exactly instead of docking to a window edge, so it takes nothing
#!  off the client area and the ABC resizer keeps moving it.
#!#############################################################################
#CONTROL(CommandBarControl,'Command bar / ribbon on this window'),WINDOW,MULTI,DESCRIPTION('[CommandBar] ' & %CBObject),HLP('~ClaCommandBar.htm')
  CONTROLS
    REGION,AT(,,240,28),USE(?CommandBarRegion)
  END
#SHEET
  #TAB('&General')
    #DISPLAY('Version 1.1 - updated 2026-08-19 14:20')
    #DISPLAY('')
    #BOXED('The region this control dropped')
      #DISPLAY('Position the REGION where you want the bar, then tick')
      #DISPLAY('"Land on the region" on that bar in the Bars list.  Bars')
      #DISPLAY('without it dock to the window edges as usual.')
      #DISPLAY('')
      #DISPLAY('The region is hidden at run time; it goes on reporting its')
      #DISPLAY('position, so the resizer keeps the bar with it.')
    #ENDBOXED
    #INSERT(%CBGeneralPrompts)
  #ENDTAB
  #TAB('&Appearance')
    #INSERT(%CBAppearancePrompts)
  #ENDTAB
  #TAB('&Colours')
    #INSERT(%CBColourPrompts)
  #ENDTAB
  #TAB('I&mages')
    #INSERT(%CBImagePrompts)
  #ENDTAB
  #TAB('&Bars')
    #INSERT(%CBBarPrompts)
  #ENDTAB
  #TAB('&Ribbon')
    #INSERT(%CBRibbonPrompts)
  #ENDTAB
  #TAB('Men&us')
    #INSERT(%CBMenuPrompts)
  #ENDTAB
  #TAB('&Items')
    #INSERT(%CBItemPrompts)
  #ENDTAB
  #TAB('&Keys')
    #INSERT(%CBAccelPrompts)
  #ENDTAB
#ENDSHEET
#ATSTART
  #INSERT(%CBDeclarations)
  #!  the REGION this instance dropped - the same trick ClaPropGrid uses
  #FOR(%Control),WHERE(%ControlInstance = %ActiveTemplateInstance)
    #SET(%CBRegion,%Control)
  #ENDFOR
#ENDAT
#!
#AT(%AfterGlobalIncludes),WHERE(%CBDisable=0)
INCLUDE('CommandBar.inc'),ONCE
#ENDAT
#AT(%CustomGlobalDeclarations),WHERE(%CBDisable=0)
  #PROJECT('commandbar.lib')
#ENDAT
#AT(%DataSection),WHERE(%CBDisable=0)
#INSERT(%CBEmitData)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8590),WHERE(%CBDisable=0 AND ITEMS(%CBAccelList)),DESCRIPTION('ClaCommandBar: alert the shortcut keys')
  #INSERT(%CBEmitAlerts)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8600),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: build the command bars')
  #EMBED(%CBCBeforeInit,'ClaCommandBar: before the bars are created'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuild)
    #EMBED(%CBCAfterBuild,'ClaCommandBar: after the bars are built (add your own items here)'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuildEnd)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'TakeWindowEvent','(),BYTE'),PRIORITY(2000),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: event pump, keys and resize')
  #INSERT(%CBEmitTakeEvent)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Kill','(),BYTE'),PRIORITY(7500),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: destroy the command bars')
%CBObject.Kill()
#ENDAT
#AT(%ProcedureRoutines),WHERE(%CBDisable=0)
#INSERT(%CBEmitFitRoutine)
!---------------------------------------------------------------------------
CBPump:%ActiveTemplateInstance ROUTINE
!  Drain the queue.  Everything the bars raise arrives here.
!
!  The per-command CASE is written out here rather than in a shared
!  #GROUP for two reasons: a #GROUP has to be BALANCED, so its #IF and
!  #FOR cannot span two groups, and an #EMBED inside a #GROUP does not
!  register as an embed point.
  LOOP WHILE %CBObject.TakeOne()
    CASE %CBObject.LastEvent
    OF CBE:Command
#IF(ITEMS(%CBCmds))
      CASE %CBObject.LastCmd
  #FOR(%CBCmds)
      OF %CBCmds
    #SET(%CBActCmd,%CBCmds)
    #!  The FIRST item carrying this command id decides the action.
    #FOR(%CBItemList),WHERE(%CBItemCmd = %CBActCmd)
      #CASE(%CBItemAction)
      #OF('Call a procedure')
        #IF(%CBItemProc)
        %CBItemProc(%CBItemProcParms)
        #ENDIF
      #OF('Do a routine')
        #IF(%CBItemRoutine)
        DO %CBItemRoutine
        #ENDIF
      #OF('Emulate a control')
        #IF(%CBItemControl)
        POST(EVENT:Accepted,%CBItemControl)
        #ENDIF
      #OF('Post an event')
        #IF(%CBItemEvent)
          #IF(%CBItemEventCtl)
        POST(%CBItemEvent,%CBItemEventCtl)
          #ELSE
        POST(%CBItemEvent)
          #ENDIF
        #ENDIF
      #OF('Close the window')
        POST(EVENT:CloseWindow)
      #ENDCASE
      #BREAK
    #ENDFOR
        #EMBED(%CBCOnCommand,'ClaCommandBar: this command was chosen'),%ActiveTemplateInstance,%CBCmds
  #ENDFOR
      END
#ELSE
      !  No command ids are defined on the Items tab yet.
#ENDIF
    OF CBE:Toggled
      #EMBED(%CBCOnToggled,'ClaCommandBar: a toggle or check box flipped (LastParam = 1 when on)'),%ActiveTemplateInstance
    OF CBE:TextChanged
      #EMBED(%CBCOnTextChanged,'ClaCommandBar: an edit box was committed'),%ActiveTemplateInstance
    OF CBE:SelChanged
      #EMBED(%CBCOnSelChanged,'ClaCommandBar: a combo selection changed (LastParam = the 0-based index)'),%ActiveTemplateInstance
    OF CBE:ColorChanged
      #EMBED(%CBCOnColorChanged,'ClaCommandBar: a colour button changed (LastParam = the colour)'),%ActiveTemplateInstance
    OF CBE:RightClick
      #EMBED(%CBCOnRightClick,'ClaCommandBar: an item was right-clicked'),%ActiveTemplateInstance
    OF CBE:TabChanged
      #EMBED(%CBCOnTabChanged,'ClaCommandBar: a ribbon tab was switched (LastParam = the tab)'),%ActiveTemplateInstance
    OF CBE:DropDown
      #EMBED(%CBCOnDropDown,'ClaCommandBar: a menu is about to open'),%ActiveTemplateInstance
    OF CBE:Layout
      DO CBFit:%ActiveTemplateInstance
    END
  END
#ENDAT
#!#############################################################################
#!  PROCEDURE EXTENSION - CommandBarFrame
#!#############################################################################
#!  For an application FRAME.  Everything CommandBarOnWindow does, plus the
#!  Menu tab: mirror the frame's own MENUBAR onto a command bar, and
#!  optionally take the original menu off the frame altogether.
#!#############################################################################
#EXTENSION(CommandBarFrame,'ClaCommandBar - command bars on an application FRAME (can replace the menu)'),PROCEDURE,HLP('~ClaCommandBar.htm')
#SHEET
  #TAB('&General')
    #DISPLAY('Version 1.1 - updated 2026-08-19 14:20')
    #DISPLAY('')
    #INSERT(%CBGeneralPrompts)
  #ENDTAB
  #TAB('&Menu and toolbar')
    #BOXED('The frame''s own MENUBAR')
      #DISPLAY('Mirroring reads the MENUBAR on this frame at run time and')
      #DISPLAY('rebuilds it as a command bar - same order, same nesting, the')
      #DISPLAY('separators, the KEY() attributes as a shortcut column, and')
      #DISPLAY('disabled items still disabled.')
      #DISPLAY('')
      #DISPLAY('Choosing a mirrored row POSTs EVENT:Accepted to the ORIGINAL')
      #DISPLAY('menu ITEM, so every embed you already wrote goes on running.')
      #DISPLAY('You do not have to re-declare a single menu item here.')
      #PROMPT('&Menu:',DROP('Leave the menu alone|Mirror it onto a command bar|Mirror it and take the original menu off the frame')),%CBFMenuMode,DEFAULT('Mirror it and take the original menu off the frame')
      #ENABLE(%CBFMenuMode <> 'Leave the menu alone')
        #PROMPT('&Caption for the mirrored bar:',@s32),%CBFMenuBar,DEFAULT('Menu')
        #PROMPT('&Dock it on:',DROP('Top|Bottom|Left|Right')),%CBFMenuDock,DEFAULT('Top')
        #PROMPT('&Row (0 is nearest the edge):',SPIN(@n2,0,20,1)),%CBFMenuRow,DEFAULT(0)
        #PROMPT('Drag &gripper',CHECK),%CBFMenuGripper,DEFAULT(0),AT(10)
        #PROMPT('User may &float it',CHECK),%CBFMenuFloat,DEFAULT(0),AT(10)
        #DISPLAY('   A gripper is the ribbed handle at the near end.  With it')
        #DISPLAY('   the menu can be dragged to another edge, and - if you')
        #DISPLAY('   allow floating - torn off into a little window.')
        #DISPLAY('')
        #DISPLAY('A Clarion MENUBAR ignores PROP:Hide - it is a real Win32')
        #DISPLAY('menu on the frame - so "take the original menu off" detaches')
        #DISPLAY('it with SetMenu().  The ITEMs stay alive either way, which')
        #DISPLAY('is what makes the mirrored rows still work.  It is put back')
        #DISPLAY('when the window closes.')
      #ENDENABLE
    #ENDBOXED
    #BOXED('The frame''s own TOOLBAR')
      #DISPLAY('The same trick, on the row of buttons.  Every BUTTON, CHECK,')
      #DISPLAY('ENTRY, COMBO and PROMPT in the TOOLBAR becomes a bar item')
      #DISPLAY('carrying its ICON, its TIP and its disabled state, and')
      #DISPLAY('choosing one POSTs EVENT:Accepted to the ORIGINAL control.')
      #DISPLAY('')
      #DISPLAY('Worth more than it looks on an MDI frame: with the toolbar')
      #DISPLAY('mirrored and the real one hidden, Clarion''s toolbar merging')
      #DISPLAY('- which hides it, rebuilds it and swaps it every time a child')
      #DISPLAY('window opens - has nothing on screen left to disturb.')
      #PROMPT('&Toolbar:',DROP('Leave the toolbar alone|Mirror it onto a command bar|Mirror it and hide the original')),%CBFToolMode,DEFAULT('Leave the toolbar alone')
      #ENABLE(%CBFToolMode <> 'Leave the toolbar alone')
        #PROMPT('&Name for the mirrored bar:',@s32),%CBFToolBar,DEFAULT('Tools')
        #PROMPT('Dock it &on:',DROP('Top|Bottom|Left|Right')),%CBFToolDock,DEFAULT('Top')
        #PROMPT('R&ow (0 is nearest the edge):',SPIN(@n2,0,20,1)),%CBFToolRow,DEFAULT(1)
        #PROMPT('Drag g&ripper',CHECK),%CBFToolGripper,DEFAULT(1),AT(10)
        #PROMPT('User may fl&oat it',CHECK),%CBFToolFloat,DEFAULT(1),AT(10)
      #ENDENABLE
    #ENDBOXED
  #ENDTAB
  #TAB('&Appearance')
    #INSERT(%CBAppearancePrompts)
  #ENDTAB
  #TAB('&Colours')
    #INSERT(%CBColourPrompts)
  #ENDTAB
  #TAB('I&mages')
    #INSERT(%CBImagePrompts)
  #ENDTAB
  #TAB('&Bars')
    #INSERT(%CBBarPrompts)
  #ENDTAB
  #TAB('&Ribbon')
    #INSERT(%CBRibbonPrompts)
  #ENDTAB
  #TAB('Men&us')
    #INSERT(%CBMenuPrompts)
  #ENDTAB
  #TAB('&Items')
    #INSERT(%CBItemPrompts)
  #ENDTAB
  #TAB('&Keys')
    #INSERT(%CBAccelPrompts)
  #ENDTAB
#ENDSHEET
#ATSTART
  #INSERT(%CBDeclarations)
#ENDAT
#!
#AT(%AfterGlobalIncludes),WHERE(%CBDisable=0)
INCLUDE('CommandBar.inc'),ONCE
#ENDAT
#AT(%CustomGlobalDeclarations),WHERE(%CBDisable=0)
  #PROJECT('commandbar.lib')
#ENDAT
#AT(%DataSection),WHERE(%CBDisable=0)
#INSERT(%CBEmitData)
  #IF(%CBFMenuMode <> 'Leave the menu alone')
CBMirrorBar:%ActiveTemplateInstance SIGNED                       ! the mirrored menu bar
  #ENDIF
  #IF(%CBFToolMode <> 'Leave the toolbar alone')
CBToolBar:%ActiveTemplateInstance SIGNED                         ! the mirrored TOOLBAR bar
  #ENDIF
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8590),WHERE(%CBDisable=0 AND ITEMS(%CBAccelList)),DESCRIPTION('ClaCommandBar: alert the shortcut keys')
  #INSERT(%CBEmitAlerts)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Init','(),BYTE'),PRIORITY(8600),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: build the command bars and mirror the menu')
  #EMBED(%CBFBeforeInit,'ClaCommandBar: before the bars are created'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuild)
  #IF(%CBFMenuMode <> 'Leave the menu alone')
    !  Rebuild this frame's own MENUBAR as a command bar.  Mirrored rows
    !  POST EVENT:Accepted to the original ITEMs, so the menu embeds run.
    CBMirrorBar:%ActiveTemplateInstance = %CBObject.AddBar('%CBFMenuBar',%(%CBDockEquate(%CBFMenuDock)),%(%CBMirrorBarStyle()))
    %CBObject.SetBarDock(CBMirrorBar:%ActiveTemplateInstance,%(%CBDockEquate(%CBFMenuDock)),%CBFMenuRow,0)
    #IF(%CBFMenuMode = 'Mirror it and take the original menu off the frame')
    %CBObject.MirrorMenu(CBMirrorBar:%ActiveTemplateInstance,1)
    #ELSE
    %CBObject.MirrorMenu(CBMirrorBar:%ActiveTemplateInstance,0)
    #ENDIF
  #ENDIF
  #IF(%CBFToolMode <> 'Leave the toolbar alone')
    !  And the same for the frame's TOOLBAR: every control in it becomes a
    !  bar item that POSTs EVENT:Accepted to the original, so the toolbar
    !  embeds run untouched.  Hiding the real one leaves Clarion's MDI
    !  toolbar merging nothing on screen to disturb.
    CBToolBar:%ActiveTemplateInstance = %CBObject.AddBar('%CBFToolBar',%(%CBDockEquate(%CBFToolDock)),%(%CBToolBarStyle()))
    %CBObject.SetBarDock(CBToolBar:%ActiveTemplateInstance,%(%CBDockEquate(%CBFToolDock)),%CBFToolRow,0)
    #IF(%CBFToolMode = 'Mirror it and hide the original')
    %CBObject.MirrorToolbar(CBToolBar:%ActiveTemplateInstance,1)
    #ELSE
    %CBObject.MirrorToolbar(CBToolBar:%ActiveTemplateInstance,0)
    #ENDIF
  #ENDIF
    #EMBED(%CBFAfterBuild,'ClaCommandBar: after the bars are built (add your own items here)'),%ActiveTemplateInstance
  #INSERT(%CBEmitBuildEnd)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'TakeWindowEvent','(),BYTE'),PRIORITY(2000),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: event pump, keys and resize')
  #INSERT(%CBEmitTakeEvent)
#ENDAT
#AT(%WindowManagerMethodCodeSection,'Kill','(),BYTE'),PRIORITY(7500),WHERE(%CBDisable=0),DESCRIPTION('ClaCommandBar: destroy the command bars')
%CBObject.Kill()
#ENDAT
#AT(%ProcedureRoutines),WHERE(%CBDisable=0)
#INSERT(%CBEmitFitRoutine)
!---------------------------------------------------------------------------
CBPump:%ActiveTemplateInstance ROUTINE
!  Drain the queue.  Everything the bars raise arrives here.
!
!  The per-command CASE is written out here rather than in a shared
!  #GROUP for two reasons: a #GROUP has to be BALANCED, so its #IF and
!  #FOR cannot span two groups, and an #EMBED inside a #GROUP does not
!  register as an embed point.
  LOOP WHILE %CBObject.TakeOne()
    CASE %CBObject.LastEvent
    OF CBE:Command
#IF(ITEMS(%CBCmds))
      CASE %CBObject.LastCmd
  #FOR(%CBCmds)
      OF %CBCmds
    #SET(%CBActCmd,%CBCmds)
    #!  The FIRST item carrying this command id decides the action.
    #FOR(%CBItemList),WHERE(%CBItemCmd = %CBActCmd)
      #CASE(%CBItemAction)
      #OF('Call a procedure')
        #IF(%CBItemProc)
        %CBItemProc(%CBItemProcParms)
        #ENDIF
      #OF('Do a routine')
        #IF(%CBItemRoutine)
        DO %CBItemRoutine
        #ENDIF
      #OF('Emulate a control')
        #IF(%CBItemControl)
        POST(EVENT:Accepted,%CBItemControl)
        #ENDIF
      #OF('Post an event')
        #IF(%CBItemEvent)
          #IF(%CBItemEventCtl)
        POST(%CBItemEvent,%CBItemEventCtl)
          #ELSE
        POST(%CBItemEvent)
          #ENDIF
        #ENDIF
      #OF('Close the window')
        POST(EVENT:CloseWindow)
      #ENDCASE
      #BREAK
    #ENDFOR
        #EMBED(%CBFOnCommand,'ClaCommandBar: this command was chosen'),%ActiveTemplateInstance,%CBCmds
  #ENDFOR
      END
#ELSE
      !  No command ids are defined on the Items tab yet.
#ENDIF
    OF CBE:Toggled
      #EMBED(%CBFOnToggled,'ClaCommandBar: a toggle or check box flipped (LastParam = 1 when on)'),%ActiveTemplateInstance
    OF CBE:TextChanged
      #EMBED(%CBFOnTextChanged,'ClaCommandBar: an edit box was committed'),%ActiveTemplateInstance
    OF CBE:SelChanged
      #EMBED(%CBFOnSelChanged,'ClaCommandBar: a combo selection changed (LastParam = the 0-based index)'),%ActiveTemplateInstance
    OF CBE:ColorChanged
      #EMBED(%CBFOnColorChanged,'ClaCommandBar: a colour button changed (LastParam = the colour)'),%ActiveTemplateInstance
    OF CBE:RightClick
      #EMBED(%CBFOnRightClick,'ClaCommandBar: an item was right-clicked'),%ActiveTemplateInstance
    OF CBE:TabChanged
      #EMBED(%CBFOnTabChanged,'ClaCommandBar: a ribbon tab was switched (LastParam = the tab)'),%ActiveTemplateInstance
    OF CBE:DropDown
      #EMBED(%CBFOnDropDown,'ClaCommandBar: a menu is about to open'),%ActiveTemplateInstance
    OF CBE:Layout
      DO CBFit:%ActiveTemplateInstance
    END
  END
#ENDAT
#!=============================================================================
#!  SHARED GROUPS
#!
#!  These are inlined here rather than kept in a .tpw on purpose.  A template
#!  #INCLUDE is resolved against the CURRENT DIRECTORY, not against the folder
#!  the .tpl sits in, so a two-file chain can only be registered one of two
#!  broken ways -
#!
#!    ClarionCL -tr C:\full\path\ClaCommandBar.tpl   -> cannot find the .tpw
#!    cd C:\full\path & ClarionCL -tr ClaCommandBar.tpl
#!        -> registers, but the registry keeps the RELATIVE path and every app
#!           in the IDE then fails to open with
#!           "Could not open include file ClaCommandBar.tpl"
#!
#!  One self-contained file has neither problem.  A #GROUP has no end marker,
#!  which is why they all sit at the very END, after every #AT block.
#!
#!  THE ONE RULE THAT MATTERS BELOW (measured in the sister ClaPropGrid
#!  project): inside a #FOR over a MULTI list, a %() group call with ONE
#!  argument works, and anything else - two arguments, or none - expands to
#!  NOTHING and can silently kill the whole enclosing #AT with no error.  So
#!  every group called from inside a #FOR takes exactly one argument, and
#!  everything else is computed with #SET into a symbol declared in #ATSTART.
#!
#!  And numeric prompts arrive as STRINGS: #IF(%Sym) is TRUE when the value is
#!  '0'.  Every numeric test here is an explicit comparison.
#!=============================================================================
#!
#!-----------------------------------------------------------------------------
#! %CBGeneralPrompts - object, class, timer, and the client control.
#!-----------------------------------------------------------------------------
#GROUP(%CBGeneralPrompts)
  #BOXED('Object')
    #PROMPT('&Disable the command bars here',CHECK),%CBDisable,DEFAULT(0),AT(10)
    #PROMPT('&Object name:',@s64),%CBObject,REQ,DEFAULT('CommandBar')
    #PROMPT('&Class name:',@s64),%CBClass,REQ,DEFAULT('CommandBarClass')
  #ENDBOXED
  #BOXED('Event pump')
    #PROMPT('&Timer interval, hundredths of a second (0 = leave the window alone):',SPIN(@n4,0,1000,5)),%CBTimer,DEFAULT(10)
    #DISPLAY('The bars deliver their events through a queue that is drained on')
    #DISPLAY('EVENT:Timer, so the window needs a timer.  The class only sets')
    #DISPLAY('one if the window has none of its own.')
  #ENDBOXED
  #BOXED('The client area under the bars')
    #PROMPT('&Control to fill the space the bars leave:',CONTROL),%CBFitControl
    #PROMPT('Margin &X:',SPIN(@n3,0,100,1)),%CBFitMarginX,DEFAULT(0)
    #PROMPT('Margin &Y:',SPIN(@n3,0,100,1)),%CBFitMarginY,DEFAULT(0)
    #DISPLAY('Optional.  Point this at the LIST, SHEET or REGION that should')
    #DISPLAY('sit under the bars and it is repositioned on every resize, and')
    #DISPLAY('whenever a bar is shown, hidden, docked or floated.')
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBAppearancePrompts
#!-----------------------------------------------------------------------------
#GROUP(%CBAppearancePrompts)
  #BOXED('Theme')
    #PROMPT('&Theme:',DROP('Steel Blue|Office 2003|Office 2007|Office 2010|Office 2013|Office 2016|VS 2012 Light|VS 2012 Dark|Windows 11 Light|Windows 11 Dark|Slate Dark')),%CBTheme,DEFAULT('Steel Blue')
    #DISPLAY('A theme sets all 37 colours at once.  The Colours tab then')
    #DISPLAY('overrides individual ones on top of it.')
    #PROMPT('Re-colour the whole theme around one &accent colour',CHECK),%CBUseAccent,DEFAULT(0),AT(10)
    #ENABLE(%CBUseAccent)
      #PROMPT('A&ccent:',COLOR),%CBAccent,DEFAULT(002B69A8H)
      #DISPLAY('Every hover, pressed, checked and highlight shade is derived')
      #DISPLAY('from the accent, so one colour re-skins the whole set and the')
      #DISPLAY('light/dark character of the theme is kept.')
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
    #PROMPT('&Large icon size (ribbon buttons, and bars marked Large icons):',SPIN(@n3,16,128,4)),%CBLargeIcon,DEFAULT(32)
    #DISPLAY('Match the small icon size to the artwork you actually have.')
    #DISPLAY('Clarion ships 32x32 icons; squeezing those into a 16px slot')
    #DISPLAY('greys out every one-pixel stroke however good the filter is.')
    #PROMPT('Item &height (0 = work it out from the font):',SPIN(@n3,0,200,1)),%CBItemHeight,DEFAULT(0)
    #PROMPT('Button corner &radius (-1 = whatever the theme chose):',SPIN(@n-2,-1,16,1)),%CBCorner,DEFAULT(-1)
    #PROMPT('Minimum popup &menu width:',SPIN(@n4,60,600,10)),%CBMenuWidth,DEFAULT(130)
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBColourPrompts - individual slot overrides, applied ON TOP of the theme.
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
#! %CBImagePrompts
#!-----------------------------------------------------------------------------
#GROUP(%CBImagePrompts)
  #BOXED('Image files')
    #DISPLAY('Any format WIC reads: .ICO, .PNG, .GIF, .JPG, .BMP - alpha')
    #DISPLAY('honoured.  The name is resolved at RUN time, relative to the')
    #DISPLAY('working folder, so ship the images with the EXE.')
    #DISPLAY('')
    #DISPLAY('Give each one a short name - the Items list refers to it.  Use')
    #DISPLAY('letters, digits and underscores: the name becomes part of a')
    #DISPLAY('generated Clarion label.')
    #BUTTON('&Images...'),MULTI(%CBImageList,%CBImageName & '  =  ' & %CBImageFile),INLINE
      #PROMPT('&Name:',@s32),%CBImageName,REQ
      #PROMPT('&File:',@s255),%CBImageFile,REQ
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBBarPrompts
#!-----------------------------------------------------------------------------
#GROUP(%CBBarPrompts)
  #BOXED('Bars')
    #DISPLAY('One entry per bar.  Bars on the same side stack by Row; two bars')
    #DISPLAY('sharing a Row sit side by side in Offset order.')
    #DISPLAY('')
    #DISPLAY('A menu bar is just a bar - tick "Is the menu bar" and put Menu')
    #DISPLAY('title items on it.  A RIBBON is a bar too: tick "Is a ribbon"')
    #DISPLAY('and fill it in from the Ribbon tab instead of the Items tab.')
    #DISPLAY('')
    #DISPLAY('In a hurry?  Pick one of the classics and press the button -')
    #DISPLAY('it fills in the bar, the items, the icons, the tooltips and')
    #DISPLAY('the shortcuts that go with them:')
    #DISPLAY('')
    #DISPLAY('   Standard             New / Open / Save / Print / Cut / Copy /')
    #DISPLAY('                        Paste / Undo / Redo / search box / Find,')
    #DISPLAY('                        and Help at the far end')
    #DISPLAY('   Formatting           font and size combos, B / I / U, text')
    #DISPLAY('                        colour, left / centre / right, lists')
    #DISPLAY('   Browse and records   VCR keys, Insert / Change / Delete, a')
    #DISPLAY('                        locator box, Sort, Mark, Refresh, Print')
    #DISPLAY('   Navigation           Back / Forward / Stop / Refresh / Home,')
    #DISPLAY('                        a stretching address box, Go, Search')
    #DISPLAY('   Print and export     Print / Preview, an Export drop button')
    #DISPLAY('                        with PDF, Excel, CSV, HTML, XML, text')
    #DISPLAY('')
    #DISPLAY('Each press starts a NEW bar, on the first free row of the top')
    #DISPLAY('edge.  They are ordinary entries afterwards: rename them, drop')
    #DISPLAY('the ones you do not want, give the rest their embed code.')
    #PROMPT('&Toolbar:',DROP('Standard|Formatting|Browse and records|Navigation|Print and export')),%CBBarPreset,DEFAULT('Standard')
    #BUTTON('Add this &toolbar'),WHENACCEPTED(%CBAddPresetBar()),AT(,,140)
    #ENDBUTTON
    #BUTTON('&Bars...'),MULTI(%CBBarList,%CBBarName & '  (' & %CBBarDock & ', row ' & %CBBarRow & ')'),INLINE
      #PROMPT('&Name (the Items list refers to this):',@s32),%CBBarName,REQ
      #PROMPT('&Caption (shown when floating):',@s64),%CBBarTitle
      #PROMPT('&Dock:',DROP('Top|Bottom|Left|Right|Floating')),%CBBarDock,DEFAULT('Top')
      #PROMPT('&Row (0 is nearest the edge):',SPIN(@n2,0,20,1)),%CBBarRow,DEFAULT(0)
      #PROMPT('&Offset within the row:',SPIN(@n2,0,20,1)),%CBBarOffset,DEFAULT(0)
      #PROMPT('&Land on the region this control dropped',CHECK),%CBBarOnRegion,DEFAULT(0),AT(10)
      #DISPLAY('   (control template only - the extensions ignore it)')
      #PROMPT('Is the &menu bar',CHECK),%CBBarMenuBar,DEFAULT(0),AT(10)
      #PROMPT('Is a &ribbon',CHECK),%CBBarRibbon,DEFAULT(0),AT(10)
      #ENABLE(%CBBarRibbon)
        #PROMPT('Starts &collapsed to its tabs',CHECK),%CBBarRibbonMin,DEFAULT(0),AT(20)
        #DISPLAY('   Double-clicking a tab collapses the ribbon and opens it')
        #DISPLAY('   again; clicking a tab while collapsed opens it too.')
      #ENDENABLE
      #PROMPT('Drag &gripper',CHECK),%CBBarGripper,DEFAULT(1),AT(10)
      #PROMPT('User may &float it',CHECK),%CBBarFloatable,DEFAULT(1),AT(10)
      #PROMPT('&Large icons',CHECK),%CBBarLargeIcons,DEFAULT(0),AT(10)
      #PROMPT('No &border line',CHECK),%CBBarNoBorder,DEFAULT(0),AT(10)
      #PROMPT('&Start hidden',CHECK),%CBBarHidden,DEFAULT(0),AT(10)
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBRibbonPrompts - tabs and groups.  Items go in a GROUP, from the Items tab.
#!-----------------------------------------------------------------------------
#GROUP(%CBRibbonPrompts)
  #BOXED('Ribbon tabs')
    #DISPLAY('A ribbon bar holds TABS, a tab holds GROUPS, and a group holds')
    #DISPLAY('ordinary items - so a ribbon group is filled from the Items tab')
    #DISPLAY('exactly like a toolbar, by naming the group in "Put it in".')
    #DISPLAY('')
    #DISPLAY('Mark an item "Image above the text" and it becomes the big')
    #DISPLAY('button; everything beside it stacks three-deep in small rows.')
    #DISPLAY('')
    #DISPLAY('This builds the lot in one go, laid out like the ribbon in the')
    #DISPLAY('CommandBarShowcase example:')
    #DISPLAY('')
    #DISPLAY('   Home     Clipboard   [Paste]  Cut / Copy / Format')
    #DISPLAY('            Font        font and size combos, B, I, colour')
    #DISPLAY('            Editing     [Find]   Replace / Go To')
    #DISPLAY('   Insert   Pages       [Cover] [Blank]')
    #DISPLAY('            Illustr.    [Picture] [Chart]')
    #DISPLAY('            Links       Hyperlink / Bookmark / Reference')
    #DISPLAY('   View     Show        Ruler / Gridlines / Navigation pane')
    #DISPLAY('            Zoom        [Zoom]  Zoom in / Zoom out')
    #BUTTON('Add a standard &ribbon'),WHENACCEPTED(%CBAddPresetRibbon()),AT(,,140)
    #ENDBUTTON
    #BUTTON('Ribbon &tabs...'),MULTI(%CBTabList,%CBTabBar & ' : ' & %CBTabName & '  "' & %CBTabText & '"'),INLINE
      #PROMPT('On this &bar (a bar marked "Is a ribbon"):',@s32),%CBTabBar,REQ
      #PROMPT('&Name (the Groups list refers to this):',@s32),%CBTabName,REQ
      #PROMPT('&Caption:',@s64),%CBTabText,REQ
    #ENDBUTTON
  #ENDBOXED
  #BOXED('Ribbon groups')
    #BUTTON('Ribbon &groups...'),MULTI(%CBGroupList,%CBGroupTab & ' : ' & %CBGroupName & '  "' & %CBGroupText & '"'),INLINE
      #PROMPT('In this &tab:',@s32),%CBGroupTab,REQ
      #PROMPT('&Name (the Items list refers to this):',@s32),%CBGroupName,REQ
      #PROMPT('&Caption under the group:',@s64),%CBGroupText,REQ
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBMenuPrompts
#!-----------------------------------------------------------------------------
#GROUP(%CBMenuPrompts)
  #BOXED('Popup menus')
    #DISPLAY('A popup menu is a container with no bar of its own.  Give it a')
    #DISPLAY('name here, put items in it on the Items tab, and hang it off a')
    #DISPLAY('Menu title / Drop button / Split button - or leave it loose and')
    #DISPLAY('call CB.PopupMenu(CBMnu:<instance>:<name>) for a context menu.')
    #DISPLAY('')
    #DISPLAY('Pick a preset and press the button for a menu already filled')
    #DISPLAY('in, with icons and shortcut text:')
    #DISPLAY('')
    #DISPLAY('   File         New / Open / Save / Save As / Print / Exit')
    #DISPLAY('   Edit         Undo / Redo / Cut / Copy / Paste / Select all')
    #DISPLAY('   Browse row   Insert / Change / Delete / Refresh / Print list')
    #DISPLAY('   View         Toolbar and Status bar ticks, Zoom, Refresh')
    #DISPLAY('   Help         Contents / Search / About')
    #PROMPT('&Preset:',DROP('File|Edit|Browse row|View|Help')),%CBMenuPreset,DEFAULT('File')
    #BUTTON('Add this &preset menu'),WHENACCEPTED(%CBAddPresetMenu()),AT(,,140)
    #ENDBUTTON
    #BUTTON('&Menus...'),MULTI(%CBMenuList,%CBMenuName),INLINE
      #PROMPT('&Name:',@s32),%CBMenuName,REQ
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBItemPrompts - including what the item DOES.
#!-----------------------------------------------------------------------------
#GROUP(%CBItemPrompts)
  #BOXED('Items')
    #DISPLAY('Items appear in the order listed, inside the bar, menu or ribbon')
    #DISPLAY('group named in "Put it in".  Give a command id to anything you')
    #DISPLAY('want to act on - each distinct id gets its own embed point, and')
    #DISPLAY('the Action below runs just before that embed.')
    #BUTTON('&Items...'),MULTI(%CBItemList,%CBItemContainer & ' : ' & %CBItemType & '  ' & %CBItemText & CHOOSE(%CBItemCmd = 0,'',' [' & %CBItemCmd & ']')),INLINE
      #PROMPT('&Put it in (a bar, menu or ribbon group name):',@s32),%CBItemContainer,REQ
      #PROMPT('&Type:',DROP('Button|Toggle button|Drop button|Split button|Separator|Label|Edit box|Combo box|Check box|Colour button|Menu title|Flexible space')),%CBItemType,DEFAULT('Button')
      #PROMPT('Te&xt (& marks the accelerator letter):',@s64),%CBItemText
      #PROMPT('&Command id (0 = it raises nothing):',SPIN(@n_9,0,999999999,1)),%CBItemCmd,DEFAULT(0)
      #PROMPT('&Image (a name from the Images tab):',@s32),%CBItemImage
      #PROMPT('T&ooltip:',@s128),%CBItemTooltip
      #BOXED('What it does')
        #PROMPT('&Action:',DROP('Embed code only|Call a procedure|Do a routine|Emulate a control|Post an event|Close the window')),%CBItemAction,DEFAULT('Embed code only')
        #ENABLE(%CBItemAction = 'Call a procedure')
          #PROMPT('&Procedure:',PROCEDURE),%CBItemProc
          #PROMPT('P&arameters:',@s128),%CBItemProcParms
        #ENDENABLE
        #ENABLE(%CBItemAction = 'Do a routine')
          #PROMPT('&Routine name:',@s64),%CBItemRoutine
          #DISPLAY('The ROUTINE has to exist in this procedure.')
        #ENDENABLE
        #ENABLE(%CBItemAction = 'Emulate a control')
          #PROMPT('&Control:',CONTROL),%CBItemControl
          #DISPLAY('POSTs EVENT:Accepted to that control, so its own embed')
          #DISPLAY('code runs.  This is how you point a bar button at a BUTTON')
          #DISPLAY('or a menu ITEM that is already on the window.')
        #ENDENABLE
        #ENABLE(%CBItemAction = 'Post an event')
          #PROMPT('&Event:',@s48),%CBItemEvent,DEFAULT('EVENT:Accepted')
          #PROMPT('To this &control (blank = the window):',CONTROL),%CBItemEventCtl
        #ENDENABLE
        #DISPLAY('Whatever the action, the embed point for this command still')
        #DISPLAY('runs straight after it.')
      #ENDBOXED
      #ENABLE(%CBItemType='Drop button' OR %CBItemType='Split button' OR %CBItemType='Menu title' OR %CBItemType='Button')
        #PROMPT('Opens this &menu (a name from the Menus tab):',@s32),%CBItemMenu
      #ENDENABLE
      #ENABLE(%CBItemType='Combo box')
        #PROMPT('Choices, pipe se&parated (Red|Green|Blue):',@s255),%CBItemCombo
        #PROMPT('Starting c&hoice (0 = the first one):',SPIN(@n3,0,255,1)),%CBItemComboSel,DEFAULT(0)
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
        #PROMPT('Image a&bove the text (the big ribbon button)',CHECK),%CBItemTextBelow,DEFAULT(0),AT(10)
        #PROMPT('Ico&n only, never the text',CHECK),%CBItemIconOnly,DEFAULT(0),AT(10)
        #PROMPT('Te&xt only, never the image',CHECK),%CBItemTextOnly,DEFAULT(0),AT(10)
        #PROMPT('S&tretch to eat the leftover width',CHECK),%CBItemStretch,DEFAULT(0),AT(10)
      #ENDBOXED
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBAccelPrompts
#!-----------------------------------------------------------------------------
#GROUP(%CBAccelPrompts)
  #BOXED('Keyboard shortcuts')
    #DISPLAY('Each entry makes a key fire a command id.  The window is given an')
    #DISPLAY('ALRT() for the key automatically, and EVENT:AlertKey is forwarded')
    #DISPLAY('to the bars.')
    #DISPLAY('')
    #DISPLAY('Use Clarion key equates - CtrlS, F5Key, ShiftF3, AltX.')
    #BUTTON('&Shortcuts...'),MULTI(%CBAccelList,%CBAccelKey & '  ->  ' & %CBAccelCmd),INLINE
      #PROMPT('&Key (a Clarion equate, e.g. CtrlS):',@s32),%CBAccelKey,REQ
      #PROMPT('&Command id:',SPIN(@n_9,0,999999999,1)),%CBAccelCmd,REQ
    #ENDBUTTON
  #ENDBOXED
#!
#!-----------------------------------------------------------------------------
#! %CBDeclarations - the #ATSTART body, shared by all three window templates.
#!
#! %CBCmds is the de-duplicated list of command ids and it drives the
#! per-command CASE and its embed points.  The rest are scratch symbols: every
#! value a generated line needs is computed into one of them with #SET.
#!-----------------------------------------------------------------------------
#GROUP(%CBDeclarations)
  #DECLARE(%CBn)
  #DECLARE(%CBOwnerVar)
  #DECLARE(%CBMenuVar)
  #DECLARE(%CBImageVar)
  #DECLARE(%CBFlags)
  #DECLARE(%CBBarFlags)
  #DECLARE(%CBFound)
  #DECLARE(%CBRegion)
  #DECLARE(%CBActCmd)
  #DECLARE(%CBComboSel)
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
  #SET(%CBRegion,'')
  #IF(%CBClass = '')
    #SET(%CBClass,'CommandBarClass')
  #ENDIF
  #IF(%CBObject = '')
    #SET(%CBObject,'CommandBar')
  #ENDIF
#!
#!-----------------------------------------------------------------------------
#! %CBEmitData - the object and one variable per image, bar, tab, group, menu
#! and item.  Everything is keyed on %ActiveTemplateInstance, so several of
#! these on one window cannot collide.
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitData)
%CBObject            %CBClass                                    ! ClaCommandBar object
CBRelay:%ActiveTemplateInstance EQUATE(EVENT:User + 400 + %ActiveTemplateInstance) ! private "the window settled" event
  #FOR(%CBImageList),WHERE(%CBImageName)
CBImg:%ActiveTemplateInstance:%CBImageName SIGNED                ! image '%CBImageFile'
  #ENDFOR
  #FOR(%CBBarList),WHERE(%CBBarName)
CBBar:%ActiveTemplateInstance:%CBBarName SIGNED                  ! bar '%CBBarName'
  #ENDFOR
  #FOR(%CBTabList),WHERE(%CBTabName)
CBTab:%ActiveTemplateInstance:%CBTabName SIGNED                  ! ribbon tab '%CBTabText'
  #ENDFOR
  #FOR(%CBGroupList),WHERE(%CBGroupName)
CBGrp:%ActiveTemplateInstance:%CBGroupName SIGNED                ! ribbon group '%CBGroupText'
  #ENDFOR
  #FOR(%CBMenuList),WHERE(%CBMenuName)
CBMnu:%ActiveTemplateInstance:%CBMenuName SIGNED                 ! menu '%CBMenuName'
  #ENDFOR
  #FOR(%CBItemList)
    #SET(%CBn,INSTANCE(%CBItemList))
CBItm:%ActiveTemplateInstance:%CBn SIGNED                        ! %CBItemType in '%CBItemContainer'
  #ENDFOR
#!
#!-----------------------------------------------------------------------------
#! %CBEmitAlerts - the window needs an ALRT for every shortcut key, or
#! EVENT:AlertKey never fires and the accelerators are dead.
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitAlerts)
  #FOR(%CBAccelList),WHERE(%CBAccelKey)
  %Window{PROP:Alrt,255} = %CBAccelKey
  #ENDFOR
#!
#!-----------------------------------------------------------------------------
#! %CBEmitBuild - theme, images, containers and items.
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitBuild)
  %CBObject.TimerInterval = %CBTimer
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
    CBImg:%ActiveTemplateInstance:%CBImageName = %CBObject.AddImage('%CBImageFile')
  #ENDFOR
  #!---- menus first: an item can only be hung on a menu that exists ----
  #FOR(%CBMenuList),WHERE(%CBMenuName)
    CBMnu:%ActiveTemplateInstance:%CBMenuName = %CBObject.CreateMenu()
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
    #IF(%CBBarRibbon)
      #SET(%CBBarFlags,%CBBarFlags + 64)
    #ENDIF
    CBBar:%ActiveTemplateInstance:%CBBarName = %CBObject.AddBar('%CBBarTitle',%(%CBDockEquate(%CBBarDock)),%CBBarFlags)
    #IF(%CBBarOnRegion AND %CBRegion)
    !  land on the REGION this control dropped, instead of docking
    %CBObject.PlaceOnControl(CBBar:%ActiveTemplateInstance:%CBBarName,%CBRegion)
    #ELSE
    %CBObject.SetBarDock(CBBar:%ActiveTemplateInstance:%CBBarName,%(%CBDockEquate(%CBBarDock)),%CBBarRow,%CBBarOffset)
    #ENDIF
    #IF(%CBBarHidden)
    %CBObject.SetBarVisible(CBBar:%ActiveTemplateInstance:%CBBarName,0)
    #ENDIF
    #IF(%CBBarRibbon AND %CBBarRibbonMin)
    %CBObject.MinimizeRibbon(CBBar:%ActiveTemplateInstance:%CBBarName,1)
    #ENDIF
  #ENDFOR
  #!---- ribbon tabs ----
  #FOR(%CBTabList),WHERE(%CBTabName)
    #SET(%CBOwnerVar,'')
    #FOR(%CBBarList),WHERE(UPPER(%CBBarName) = UPPER(%CBTabBar))
      #SET(%CBOwnerVar,'CBBar:' & %ActiveTemplateInstance & ':' & %CBBarName)
      #BREAK
    #ENDFOR
    #IF(%CBOwnerVar = '')
      #ERROR('ClaCommandBar: ribbon tab "' & %CBTabName & '" is on bar "' & %CBTabBar & '", but there is no bar with that name.')
    #ELSE
    CBTab:%ActiveTemplateInstance:%CBTabName = %CBObject.AddRibbonTab(%CBOwnerVar,'%CBTabText')
    #ENDIF
  #ENDFOR
  #!---- ribbon groups ----
  #FOR(%CBGroupList),WHERE(%CBGroupName)
    #SET(%CBOwnerVar,'')
    #FOR(%CBTabList),WHERE(UPPER(%CBTabName) = UPPER(%CBGroupTab))
      #SET(%CBOwnerVar,'CBTab:' & %ActiveTemplateInstance & ':' & %CBTabName)
      #BREAK
    #ENDFOR
    #IF(%CBOwnerVar = '')
      #ERROR('ClaCommandBar: ribbon group "' & %CBGroupName & '" is in tab "' & %CBGroupTab & '", but there is no tab with that name.')
    #ELSE
    CBGrp:%ActiveTemplateInstance:%CBGroupName = %CBObject.AddRibbonGroup(%CBOwnerVar,'%CBGroupText')
    #ENDIF
  #ENDFOR
  #!---- items ----
  #FOR(%CBItemList)
    #SET(%CBn,INSTANCE(%CBItemList))
    #!  Resolve the container name against the bars, then the ribbon groups,
    #!  then the menus.  Nested #FORs rather than a #GROUP call: a group call
    #!  with more than one argument expands to nothing in here.
    #SET(%CBOwnerVar,'')
    #FOR(%CBBarList),WHERE(UPPER(%CBBarName) = UPPER(%CBItemContainer))
      #SET(%CBOwnerVar,'CBBar:' & %ActiveTemplateInstance & ':' & %CBBarName)
      #BREAK
    #ENDFOR
    #IF(%CBOwnerVar = '')
      #FOR(%CBGroupList),WHERE(UPPER(%CBGroupName) = UPPER(%CBItemContainer))
        #SET(%CBOwnerVar,'CBGrp:' & %ActiveTemplateInstance & ':' & %CBGroupName)
        #BREAK
      #ENDFOR
    #ENDIF
    #IF(%CBOwnerVar = '')
      #FOR(%CBMenuList),WHERE(UPPER(%CBMenuName) = UPPER(%CBItemContainer))
        #SET(%CBOwnerVar,'CBMnu:' & %ActiveTemplateInstance & ':' & %CBMenuName)
        #BREAK
      #ENDFOR
    #ENDIF
    #IF(%CBOwnerVar = '')
      #ERROR('ClaCommandBar: item ' & %CBn & ' says "Put it in: ' & %CBItemContainer & '" but there is no bar, ribbon group or menu with that name.')
    #ELSE
      #!  the popup this item opens, if any
      #SET(%CBMenuVar,'0')
      #IF(%CBItemMenu)
        #SET(%CBFound,0)
        #FOR(%CBMenuList),WHERE(UPPER(%CBMenuName) = UPPER(%CBItemMenu))
          #SET(%CBMenuVar,'CBMnu:' & %ActiveTemplateInstance & ':' & %CBMenuName)
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
          #SET(%CBImageVar,'CBImg:' & %ActiveTemplateInstance & ':' & %CBImageName)
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
      #!  SetItemStyle REPLACES the style word - so the flag the DLL sets
      #!  for those types itself has to be put back here, or ticking
      #!  nothing would leave a toggle that never stays down.
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
        #SET(%CBComboSel,%CBItemComboSel)
        #IF(%CBComboSel = '')
          #SET(%CBComboSel,0)
        #ENDIF
    %CBObject.SetComboSel(CBItm:%ActiveTemplateInstance:%CBn,%CBComboSel)
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
#!
#!-----------------------------------------------------------------------------
#! %CBEmitBuildEnd - closes the IF opened by %CBEmitBuild.
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitBuildEnd)
    %CBObject.Layout()
    DO CBFit:%ActiveTemplateInstance
  END
#!
#!-----------------------------------------------------------------------------
#! %CBEmitTakeEvent - a self-contained CASE EVENT() at PRIORITY(2000), ABOVE
#! the framework's own LOOP/CASE scaffolding (registered at 2500 in
#! ABWINDOW.TPW).  Using 2500 interleaves and produces a duplicate CASE
#! EVENT().  EVENT:Sized POSTs a private event rather than re-fitting on the
#! spot, because at the top of TakeWindowEvent the ABC resizer has not moved
#! the window's own controls yet.
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitTakeEvent)
  CASE EVENT()
  OF EVENT:Timer
    DO CBPump:%ActiveTemplateInstance
  OF EVENT:Sized
    POST(CBRelay:%ActiveTemplateInstance)
  OF CBRelay:%ActiveTemplateInstance
    %CBObject.Layout()
    DO CBFit:%ActiveTemplateInstance
#IF(ITEMS(%CBAccelList))
  OF EVENT:AlertKey
    IF %CBObject.TakeAlertKey(KEYCODE())
      DO CBPump:%ActiveTemplateInstance
      RETURN Level:Notify
    END
#ENDIF
  END
#!
#!-----------------------------------------------------------------------------
#! %CBEmitFitRoutine
#!-----------------------------------------------------------------------------
#GROUP(%CBEmitFitRoutine)
!---------------------------------------------------------------------------
CBFit:%ActiveTemplateInstance ROUTINE
!  Put the client control back in whatever space the bars left.
#IF(%CBFitControl)
  %CBObject.FitControl(%CBFitControl,%CBFitMarginX,%CBFitMarginY)
  DISPLAY
#ELSE
  !  No client control chosen on the General tab - nothing to re-fit.
  !  CB.ClientX/Y/Width/Height still report the space the bars left.
#ENDIF
#!
#!-----------------------------------------------------------------------------
#! %CBThemeEquate(theme text)
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
#! %CBDockEquate(dock text) - called from inside #FOR: ONE argument.
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
#! %CBTypeEquate(item type text) - called from inside #FOR: ONE argument.
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
#! %CBSlotEquate(slot text) - called from inside #FOR: ONE argument.
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
#! %CBMirrorBarStyle - the mirrored menu bar's CBBS: bits.  It is a menu bar
#! first; the gripper and floating are the frame template's own options.
#!-----------------------------------------------------------------------------
#GROUP(%CBToolBarStyle),AUTO
  #DECLARE(%CBTBits)
  #SET(%CBTBits,'0')
  #IF(%CBFToolGripper)
    #SET(%CBTBits,%CBTBits & ' + CBBS:Gripper')
  #ENDIF
  #IF(%CBFToolFloat)
    #SET(%CBTBits,%CBTBits & ' + CBBS:Floatable')
  #ENDIF
  #RETURN(%CBTBits)
#!
#GROUP(%CBMirrorBarStyle),AUTO
  #DECLARE(%CBMBits)
  #SET(%CBMBits,'CBBS:MenuBar')
  #IF(%CBFMenuGripper)
    #SET(%CBMBits,%CBMBits & ' + CBBS:Gripper')
  #ENDIF
  #IF(%CBFMenuFloat)
    #SET(%CBMBits,%CBMBits & ' + CBBS:Floatable')
  #ENDIF
  #RETURN(%CBMBits)
#!
#!-----------------------------------------------------------------------------
#! %CBStyleNumber - the CBS: manager style bits as a plain number.
#!-----------------------------------------------------------------------------
#GROUP(%CBStyleNumber)
  #RETURN(%CBStyleTooltips + 2 * %CBStyleChevron + 4 * %CBStyleFlat + 8 * %CBStyleHotText + 16 * %CBStyleMenuIcons)
#!=============================================================================
#! PRESETS - the "add a standard ..." buttons on the Bars, Ribbon and Menus
#! tabs.  Every one of these runs at DESIGN time and fills the very same
#! prompt lists you would otherwise type in by hand, so anything a preset
#! makes can be renamed, reordered or deleted afterwards like any other
#! entry.  Nothing here is special-cased in the generator.
#!
#! Icons are named by plain file name - NEW.ICO.  Add the icons to the
#! application's project and Clarion links them into the EXE, where the DLL
#! finds them by resource name (NEW.ICO -> NEW_ICO); a copy sitting next to
#! the EXE, or in an images folder beside it, is found too.
#!=============================================================================
#!
#!-----------------------------------------------------------------------------
#! %CBNextCmd - the next free command id, so a preset never collides with
#! items already on the window.  Presets start at 1000.
#!-----------------------------------------------------------------------------
#!-----------------------------------------------------------------------------
#! %CBFreeRow - the first row on this edge that no bar is using yet, so a
#! preset never lands on top of a menu bar already sitting on row 0.
#!-----------------------------------------------------------------------------
#GROUP(%CBFreeRow,%pDock),AUTO
  #DECLARE(%CBRowMax)
  #DECLARE(%CBRowAny)
  #SET(%CBRowMax,0)
  #SET(%CBRowAny,0)
  #FOR(%CBBarList),WHERE(%CBBarDock = %pDock)
    #SET(%CBRowAny,1)
    #IF(%CBBarRow > %CBRowMax)
      #SET(%CBRowMax,%CBBarRow)
    #ENDIF
  #ENDFOR
  #!  the frame template's mirrored menu bar is a bar too, and it is not in
  #!  the list - it is built from the Menu tab
  #IF(%CBFMenuMode <> 'Leave the menu alone' AND %CBFMenuDock = %pDock)
    #SET(%CBRowAny,1)
    #IF(%CBFMenuRow > %CBRowMax)
      #SET(%CBRowMax,%CBFMenuRow)
    #ENDIF
  #ENDIF
  #IF(%CBRowAny = 0)
    #RETURN(0)
  #ENDIF
  #RETURN(%CBRowMax + 1)
#!
#GROUP(%CBNextCmd),AUTO
  #DECLARE(%CBHighCmd)
  #SET(%CBHighCmd,999)
  #FOR(%CBItemList)
    #IF(%CBItemCmd > %CBHighCmd)
      #SET(%CBHighCmd,%CBItemCmd)
    #ENDIF
  #ENDFOR
  #RETURN(%CBHighCmd + 1)
#!
#!-----------------------------------------------------------------------------
#! %CBNameTaken / %CBFreeName - names become variables in the generated
#! source, so they have to be unique.  Pressing a preset button twice gives
#! Standard2, Ribbon2, FileMenu2 and so on rather than a duplicate.
#!-----------------------------------------------------------------------------
#GROUP(%CBNameTaken,%pName),AUTO
  #DECLARE(%CBTaken)
  #SET(%CBTaken,0)
  #FOR(%CBBarList)
    #IF(UPPER(%CBBarName) = UPPER(%pName))
      #SET(%CBTaken,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #FOR(%CBMenuList)
    #IF(UPPER(%CBMenuName) = UPPER(%pName))
      #SET(%CBTaken,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #FOR(%CBTabList)
    #IF(UPPER(%CBTabName) = UPPER(%pName))
      #SET(%CBTaken,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #FOR(%CBGroupList)
    #IF(UPPER(%CBGroupName) = UPPER(%pName))
      #SET(%CBTaken,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #RETURN(%CBTaken)
#!
#!  A group hands its answer back as TEXT, and '0' is not false to #IF or
#!  to #LOOP,WHILE - every string but the empty one is true.  Written as
#!  #LOOP,WHILE(%CBNameTaken(...)) this span for ever and hung AppGen, so
#!  the test is spelled out, and the count is capped as well: a template
#!  bug should never be able to lock the IDE up again.
#GROUP(%CBFreeName,%pBase),AUTO
  #DECLARE(%CBTryName)
  #DECLARE(%CBTryNum)
  #SET(%CBTryName,%pBase)
  #SET(%CBTryNum,1)
  #LOOP,WHILE(%CBNameTaken(%CBTryName) = 1)
    #SET(%CBTryNum,%CBTryNum + 1)
    #IF(%CBTryNum > 99)
      #BREAK
    #ENDIF
    #SET(%CBTryName,%pBase & %CBTryNum)
  #ENDLOOP
  #RETURN(%CBTryName)
#!
#!-----------------------------------------------------------------------------
#! %CBUseImage - name an icon on the Images tab, once.  Called again with the
#! same name it does nothing, so two presets can share NEW.ICO.
#!-----------------------------------------------------------------------------
#GROUP(%CBUseImage,%pName,%pFile),AUTO
  #DECLARE(%CBHaveImg)
  #SET(%CBHaveImg,0)
  #FOR(%CBImageList)
    #IF(UPPER(%CBImageName) = UPPER(%pName))
      #SET(%CBHaveImg,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #IF(%CBHaveImg = 0)
    #ADD(%CBImageList,ITEMS(%CBImageList)+1)
    #SET(%CBImageName,%pName)
    #SET(%CBImageFile,%pFile)
  #ENDIF
#!
#GROUP(%CBStdIcons),AUTO
  #INSERT(%CBUseImage,'New','NEW.ICO')
  #INSERT(%CBUseImage,'Open','OPEN.ICO')
  #INSERT(%CBUseImage,'Save','SAVE.ICO')
  #INSERT(%CBUseImage,'Print','PRINT.ICO')
  #INSERT(%CBUseImage,'Cut','CUT.ICO')
  #INSERT(%CBUseImage,'Copy','COPY.ICO')
  #INSERT(%CBUseImage,'Paste','PASTE.ICO')
  #INSERT(%CBUseImage,'Undo','UNDO.ICO')
  #INSERT(%CBUseImage,'Redo','REDO.ICO')
  #INSERT(%CBUseImage,'Find','FIND.ICO')
  #INSERT(%CBUseImage,'Insert','INSERT.ICO')
  #INSERT(%CBUseImage,'Change','EDIT.ICO')
  #INSERT(%CBUseImage,'Delete','DELETE.ICO')
  #INSERT(%CBUseImage,'Refresh','REFRESH.ICO')
  #INSERT(%CBUseImage,'Zoom','ZOOM.ICO')
  #INSERT(%CBUseImage,'Help','HELP.ICO')
  #INSERT(%CBUseImage,'About','ABOUT.ICO')
  #INSERT(%CBUseImage,'Exit','EXIT.ICO')
#!
#!-----------------------------------------------------------------------------
#! %CBPutItem - one item, with the fields a preset cares about.  Anything not
#! set stays empty, which reads as "off" everywhere in the generator, so the
#! caller can follow a #INSERT with a #SET to turn one flag on.
#!-----------------------------------------------------------------------------
#GROUP(%CBPutItem,%pIn,%pType,%pText,%pCmd,%pImage,%pTip),AUTO
  #ADD(%CBItemList,ITEMS(%CBItemList)+1)
  #SET(%CBItemContainer,%pIn)
  #SET(%CBItemType,%pType)
  #SET(%CBItemText,%pText)
  #SET(%CBItemCmd,%pCmd)
  #SET(%CBItemImage,%pImage)
  #SET(%CBItemTooltip,%pTip)
  #SET(%CBItemAction,'Embed code only')
  #SET(%CBItemWidth,0)
#!
#GROUP(%CBPutSep,%pIn),AUTO
  #INSERT(%CBPutItem,%pIn,'Separator','',0,'','')
#!
#GROUP(%CBPutRow,%pIn,%pText,%pCmd,%pImage,%pKey),AUTO
  #INSERT(%CBPutItem,%pIn,'Button',%pText,%pCmd,%pImage,'')
  #SET(%CBItemShortcut,%pKey)
#!
#GROUP(%CBPutBig,%pIn,%pText,%pCmd,%pImage,%pTip),AUTO
  #INSERT(%CBPutItem,%pIn,'Button',%pText,%pCmd,%pImage,%pTip)
  #SET(%CBItemTextBelow,1)
#!
#GROUP(%CBPutMenu,%pName),AUTO
  #ADD(%CBMenuList,ITEMS(%CBMenuList)+1)
  #SET(%CBMenuName,%pName)
#!
#GROUP(%CBPutTab,%pBar,%pName,%pText),AUTO
  #ADD(%CBTabList,ITEMS(%CBTabList)+1)
  #SET(%CBTabBar,%pBar)
  #SET(%CBTabName,%pName)
  #SET(%CBTabText,%pText)
#!
#GROUP(%CBPutGroup,%pTab,%pName,%pText),AUTO
  #ADD(%CBGroupList,ITEMS(%CBGroupList)+1)
  #SET(%CBGroupTab,%pTab)
  #SET(%CBGroupName,%pName)
  #SET(%CBGroupText,%pText)
#!
#GROUP(%CBPutAccel,%pKey,%pCmd),AUTO
  #DECLARE(%CBHaveKey)
  #SET(%CBHaveKey,0)
  #FOR(%CBAccelList)
    #IF(UPPER(%CBAccelKey) = UPPER(%pKey))
      #SET(%CBHaveKey,1)
      #BREAK
    #ENDIF
  #ENDFOR
  #IF(%CBHaveKey = 0)
    #ADD(%CBAccelList,ITEMS(%CBAccelList)+1)
    #SET(%CBAccelKey,%pKey)
    #SET(%CBAccelCmd,%pCmd)
  #ENDIF
#!
#GROUP(%CBPutDrop,%pIn,%pText,%pCmd,%pImage,%pMenu,%pTip),AUTO
  #INSERT(%CBPutItem,%pIn,'Drop button',%pText,%pCmd,%pImage,%pTip)
  #SET(%CBItemMenu,%pMenu)
#!
#!-----------------------------------------------------------------------------
#! 1.  A toolbar, from one of five classics.  Each one starts a NEW bar and
#!     fills it, so pressing the button twice gives you two bars rather than
#!     one long one.
#!-----------------------------------------------------------------------------
#GROUP(%CBAddPresetBar),AUTO
  #DECLARE(%CBpBar)
  #DECLARE(%CBpCmd)
  #DECLARE(%CBpMnu)
  #INSERT(%CBStdIcons)
  #SET(%CBpCmd,%CBNextCmd())
  #CASE(%CBBarPreset)
  #!===========================================================================
  #OF('Standard')
  #!  The row nearly every application has had since 1997.
    #SET(%CBpBar,%CBFreeName('Standard'))
    #INSERT(%CBPutBar,%CBpBar,'Standard')
    #INSERT(%CBPutItem,%CBpBar,'Button','&New',%CBpCmd,'New','New (Ctrl+N)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Open',%CBpCmd + 1,'Open','Open (Ctrl+O)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Save',%CBpCmd + 2,'Save','Save (Ctrl+S)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Print',%CBpCmd + 3,'Print','Print (Ctrl+P)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','Cu&t',%CBpCmd + 4,'Cut','Cut (Ctrl+X)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Copy',%CBpCmd + 5,'Copy','Copy (Ctrl+C)')
    #INSERT(%CBPutItem,%CBpBar,'Button','P&aste',%CBpCmd + 6,'Paste','Paste (Ctrl+V)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Undo',%CBpCmd + 7,'Undo','Undo (Ctrl+Z)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Redo',%CBpCmd + 8,'Redo','Redo (Ctrl+Y)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Edit box','',%CBpCmd + 9,'','What to look for')
    #SET(%CBItemWidth,160)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Find',%CBpCmd + 10,'Find','Find (Ctrl+F)')
    #INSERT(%CBPutItem,%CBpBar,'Flexible space','',0,'','')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Help',%CBpCmd + 11,'Help','Help (F1)')
    #INSERT(%CBPutAccel,'CtrlN',%CBpCmd)
    #INSERT(%CBPutAccel,'CtrlO',%CBpCmd + 1)
    #INSERT(%CBPutAccel,'CtrlS',%CBpCmd + 2)
    #INSERT(%CBPutAccel,'CtrlP',%CBpCmd + 3)
    #INSERT(%CBPutAccel,'CtrlF',%CBpCmd + 10)
    #INSERT(%CBPutAccel,'F1Key',%CBpCmd + 11)
  #!===========================================================================
  #OF('Formatting')
  #!  The other half of the Office pair - and the one that shows what the
  #!  bars can hold besides buttons.
    #INSERT(%CBUseImage,'Bold','BOLD.ICO')
    #INSERT(%CBUseImage,'Italic','ITALIC.ICO')
    #INSERT(%CBUseImage,'Underline','UNDRLINE.ICO')
    #INSERT(%CBUseImage,'AlignLeft','JUSTLFT.ICO')
    #INSERT(%CBUseImage,'AlignCentre','JUSTCTR.ICO')
    #INSERT(%CBUseImage,'AlignRight','JUSTRT.ICO')
    #INSERT(%CBUseImage,'Bullets','BULLETS.ICO')
    #INSERT(%CBUseImage,'Numbering','SBULLETS.ICO')
    #SET(%CBpBar,%CBFreeName('Formatting'))
    #INSERT(%CBPutBar,%CBpBar,'Formatting')
    #INSERT(%CBPutCombo,%CBpBar,%CBpCmd,'Segoe UI|Tahoma|Arial|Consolas|Times New Roman',140,0)
    #INSERT(%CBPutCombo,%CBpBar,%CBpCmd + 1,'8|9|10|11|12|14|18|24',55,1)
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 2,'Bold','Bold (Ctrl+B)')
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 3,'Italic','Italic (Ctrl+I)')
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 4,'Underline','Underline (Ctrl+U)')
    #INSERT(%CBPutColour,%CBpBar,%CBpCmd + 5,00800000H)
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 6,'AlignLeft','Align left')
    #SET(%CBItemChecked,1)
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 7,'AlignCentre','Centre')
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','',%CBpCmd + 8,'AlignRight','Align right')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 9,'Bullets','Bulleted list')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 10,'Numbering','Numbered list')
    #INSERT(%CBPutAccel,'CtrlB',%CBpCmd + 2)
    #INSERT(%CBPutAccel,'CtrlI',%CBpCmd + 3)
    #INSERT(%CBPutAccel,'CtrlU',%CBpCmd + 4)
  #!===========================================================================
  #OF('Browse and records')
  #!  The Clarion one: VCR keys, the three record buttons, and the things
  #!  that go with a browse.
    #INSERT(%CBUseImage,'First','VCRFIRST.ICO')
    #INSERT(%CBUseImage,'Prior','VCRPRIOR.ICO')
    #INSERT(%CBUseImage,'Next','VCRNEXT.ICO')
    #INSERT(%CBUseImage,'Last','VCRLAST.ICO')
    #INSERT(%CBUseImage,'Mark','MARK.ICO')
    #INSERT(%CBUseImage,'Sort','WASort.ICO')
    #SET(%CBpBar,%CBFreeName('Browse'))
    #INSERT(%CBPutBar,%CBpBar,'Browse')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd,'First','First record (Ctrl+Home)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 1,'Prior','Previous page (PgUp)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 2,'Next','Next page (PgDn)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 3,'Last','Last record (Ctrl+End)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Insert',%CBpCmd + 4,'Insert','Add a record (Ins)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Change',%CBpCmd + 5,'Change','Edit the record (Enter)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Delete',%CBpCmd + 6,'Delete','Delete the record (Del)')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Edit box','',%CBpCmd + 7,'','Type to locate')
    #SET(%CBItemWidth,150)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Locate',%CBpCmd + 8,'Find','Locate (Ctrl+F)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Sort',%CBpCmd + 9,'Sort','Change the order')
    #INSERT(%CBPutItem,%CBpBar,'Toggle button','&Mark',%CBpCmd + 10,'Mark','Tag this row')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Refresh',%CBpCmd + 11,'Refresh','Read the data again (F5)')
    #INSERT(%CBPutItem,%CBpBar,'Flexible space','',0,'','')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Print',%CBpCmd + 12,'Print','Print the list')
    #INSERT(%CBPutAccel,'CtrlF',%CBpCmd + 8)
    #INSERT(%CBPutAccel,'F5Key',%CBpCmd + 11)
  #!===========================================================================
  #OF('Navigation')
  #!  Browser-shaped: the address box stretches to eat whatever width is
  #!  left, which is what "Stretch" on an item is for.
    #INSERT(%CBUseImage,'Back','Prevtag.ico')
    #INSERT(%CBUseImage,'Forward','Nexttag.ico')
    #INSERT(%CBUseImage,'Stop','CANCEL.ICO')
    #INSERT(%CBUseImage,'Home','b_Root.ICO')
    #INSERT(%CBUseImage,'Search','Search.ico')
    #SET(%CBpBar,%CBFreeName('Navigation'))
    #INSERT(%CBPutBar,%CBpBar,'Navigation')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Back',%CBpCmd,'Back','Back (Alt+Left)')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Forward',%CBpCmd + 1,'Forward','Forward (Alt+Right)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 2,'Stop','Stop (Esc)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 3,'Refresh','Refresh (F5)')
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 4,'Home','Home')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Label','Address:',0,'','')
    #INSERT(%CBPutItem,%CBpBar,'Edit box','',%CBpCmd + 5,'','Where to go')
    #SET(%CBItemStretch,1)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Go',%CBpCmd + 6,'','Go there')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','',%CBpCmd + 7,'Search','Search')
    #INSERT(%CBPutAccel,'F5Key',%CBpCmd + 3)
  #!===========================================================================
  #ELSE
  #!  Print and export, with the export formats hanging off a DROP BUTTON -
  #!  which is how a bar button carries a menu.
    #INSERT(%CBUseImage,'Preview','ZOOM.ICO')
    #INSERT(%CBUseImage,'PDF','EXP_PDF.ico')
    #INSERT(%CBUseImage,'Excel','EXP_XLS.ico')
    #INSERT(%CBUseImage,'CSV','EXP_CSV.ico')
    #INSERT(%CBUseImage,'HTML','EXP_HTM.ico')
    #INSERT(%CBUseImage,'XML','EXP_XML.ico')
    #INSERT(%CBUseImage,'Text','EXP_TXT.ico')
    #SET(%CBpBar,%CBFreeName('Report'))
    #INSERT(%CBPutBar,%CBpBar,'Report')
    #SET(%CBpMnu,%CBFreeName('ExportMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&PDF...',%CBpCmd + 3,'PDF','')
    #INSERT(%CBPutRow,%CBpMnu,'&Excel...',%CBpCmd + 4,'Excel','')
    #INSERT(%CBPutRow,%CBpMnu,'&CSV...',%CBpCmd + 5,'CSV','')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&HTML...',%CBpCmd + 6,'HTML','')
    #INSERT(%CBPutRow,%CBpMnu,'&XML...',%CBpCmd + 7,'XML','')
    #INSERT(%CBPutRow,%CBpMnu,'Plain &text...',%CBpCmd + 8,'Text','')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Print',%CBpCmd,'Print','Print (Ctrl+P)')
    #INSERT(%CBPutItem,%CBpBar,'Button','Pre&view',%CBpCmd + 1,'Preview','Print preview')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutDrop,%CBpBar,'&Export',%CBpCmd + 2,'Save',%CBpMnu,'Export the data')
    #INSERT(%CBPutSep,%CBpBar)
    #INSERT(%CBPutItem,%CBpBar,'Button','&Refresh',%CBpCmd + 9,'Refresh','Read the data again (F5)')
    #INSERT(%CBPutItem,%CBpBar,'Flexible space','',0,'','')
    #INSERT(%CBPutItem,%CBpBar,'Button','&Close',%CBpCmd + 10,'Exit','Close this window')
    #INSERT(%CBPutAccel,'CtrlP',%CBpCmd)
    #INSERT(%CBPutAccel,'F5Key',%CBpCmd + 9)
  #ENDCASE
#!
#!  One docked bar, on the first row of the top edge nobody is using.
#GROUP(%CBPutBar,%pName,%pTitle),AUTO
  #ADD(%CBBarList,ITEMS(%CBBarList)+1)
  #SET(%CBBarName,%pName)
  #SET(%CBBarTitle,%pTitle)
  #SET(%CBBarDock,'Top')
  #SET(%CBBarRow,%CBFreeRow('Top'))
  #SET(%CBBarOffset,0)
  #SET(%CBBarGripper,1)
  #SET(%CBBarFloatable,1)
#!
#GROUP(%CBPutCombo,%pIn,%pCmd,%pChoices,%pWidth,%pSel),AUTO
  #INSERT(%CBPutItem,%pIn,'Combo box','',%pCmd,'','')
  #SET(%CBItemCombo,%pChoices)
  #SET(%CBItemWidth,%pWidth)
  #SET(%CBItemComboSel,%pSel)
#!
#GROUP(%CBPutToggle,%pIn,%pText,%pCmd),AUTO
  #INSERT(%CBPutItem,%pIn,'Toggle button',%pText,%pCmd,'','')
#!
#GROUP(%CBPutCheck,%pIn,%pText,%pCmd,%pOn),AUTO
  #INSERT(%CBPutItem,%pIn,'Check box',%pText,%pCmd,'','')
  #SET(%CBItemChecked,%pOn)
#!
#GROUP(%CBPutColour,%pIn,%pCmd,%pColour),AUTO
  #INSERT(%CBPutItem,%pIn,'Colour button','',%pCmd,'','Text colour')
  #SET(%CBItemColor,%pColour)
#!
#!-----------------------------------------------------------------------------
#! 2.  A standard ribbon.  Laid out like the one in CommandBarShowcase,
#!     because that is the shape people recognise: a big button on the left
#!     of a group with small ones stacked beside it, and real controls -
#!     the font and size combos, B and I toggles, a colour button - in
#!     among them.  Every group gets its caption underneath.
#!
#!       Home     Clipboard  [Paste]  Cut / Copy / Format
#!                Font       font combo, size combo, B, I, colour
#!                Editing    [Find]   Replace / Go To
#!       Insert   Pages      [Cover] [Blank]
#!                Illustr.   [Picture] [Chart]
#!                Links      Hyperlink / Bookmark / Reference
#!       View     Show       Ruler / Gridlines / Navigation pane
#!                Zoom       [Zoom]  Zoom in / Zoom out
#!-----------------------------------------------------------------------------
#GROUP(%CBAddPresetRibbon),AUTO
  #DECLARE(%CBpBar)
  #DECLARE(%CBpCmd)
  #DECLARE(%CBpTab)
  #DECLARE(%CBpGrp)
  #INSERT(%CBStdIcons)
  #INSERT(%CBUseImage,'Picture','OPEN.ICO')
  #INSERT(%CBUseImage,'Chart','PRINT.ICO')
  #SET(%CBpBar,%CBFreeName('Ribbon'))
  #SET(%CBpCmd,%CBNextCmd())
  #!  a ribbon wants room for the big buttons
  #IF(%CBIconSize = 16)
    #SET(%CBIconSize,20)
  #ENDIF
  #IF(%CBLargeIcon < 32)
    #SET(%CBLargeIcon,32)
  #ENDIF
  #ADD(%CBBarList,ITEMS(%CBBarList)+1)
  #SET(%CBBarName,%CBpBar)
  #SET(%CBBarTitle,'Ribbon')
  #SET(%CBBarDock,'Top')
  #SET(%CBBarRow,%CBFreeRow('Top'))
  #SET(%CBBarOffset,0)
  #SET(%CBBarRibbon,1)
  #SET(%CBBarGripper,0)
  #SET(%CBBarFloatable,0)
  #!---- Home ----
  #SET(%CBpTab,%CBFreeName('Home'))
  #INSERT(%CBPutTab,%CBpBar,%CBpTab,'&Home')
  #SET(%CBpGrp,%CBFreeName('Clipboard'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Clipboard')
  #INSERT(%CBPutBig,%CBpGrp,'Paste',%CBpCmd,'Paste','Paste (Ctrl+V)')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Cut',%CBpCmd + 1,'Cut','Cut (Ctrl+X)')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Copy',%CBpCmd + 2,'Copy','Copy (Ctrl+C)')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Format',%CBpCmd + 3,'','Copy formatting')
  #SET(%CBpGrp,%CBFreeName('Font'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Font')
  #INSERT(%CBPutCombo,%CBpGrp,%CBpCmd + 4,'Segoe UI|Tahoma|Consolas|Times New Roman',130,0)
  #INSERT(%CBPutCombo,%CBpGrp,%CBpCmd + 5,'8|9|10|12|14|18',50,1)
  #INSERT(%CBPutToggle,%CBpGrp,'B',%CBpCmd + 6)
  #INSERT(%CBPutToggle,%CBpGrp,'I',%CBpCmd + 7)
  #INSERT(%CBPutColour,%CBpGrp,%CBpCmd + 8,00800000H)
  #SET(%CBpGrp,%CBFreeName('Editing'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Editing')
  #INSERT(%CBPutBig,%CBpGrp,'Find',%CBpCmd + 9,'Find','Find (Ctrl+F)')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Replace',%CBpCmd + 10,'','Replace')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Go To',%CBpCmd + 11,'','Go to')
  #!---- Insert ----
  #SET(%CBpTab,%CBFreeName('Insert'))
  #INSERT(%CBPutTab,%CBpBar,%CBpTab,'&Insert')
  #SET(%CBpGrp,%CBFreeName('Pages'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Pages')
  #INSERT(%CBPutBig,%CBpGrp,'Cover',%CBpCmd + 12,'New','A cover page')
  #INSERT(%CBPutBig,%CBpGrp,'Blank',%CBpCmd + 13,'New','A blank page')
  #SET(%CBpGrp,%CBFreeName('Illustrations'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Illustrations')
  #INSERT(%CBPutBig,%CBpGrp,'Picture',%CBpCmd + 14,'Picture','Insert a picture')
  #INSERT(%CBPutBig,%CBpGrp,'Chart',%CBpCmd + 15,'Chart','Insert a chart')
  #SET(%CBpGrp,%CBFreeName('Links'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Links')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Hyperlink',%CBpCmd + 16,'','Insert a link')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Bookmark',%CBpCmd + 17,'','Insert a bookmark')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Reference',%CBpCmd + 18,'','Cross-reference')
  #!---- View ----
  #SET(%CBpTab,%CBFreeName('View'))
  #INSERT(%CBPutTab,%CBpBar,%CBpTab,'&View')
  #SET(%CBpGrp,%CBFreeName('Show'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Show')
  #INSERT(%CBPutCheck,%CBpGrp,'Ruler',%CBpCmd + 19,1)
  #INSERT(%CBPutCheck,%CBpGrp,'Gridlines',%CBpCmd + 20,0)
  #INSERT(%CBPutCheck,%CBpGrp,'Navigation pane',%CBpCmd + 21,0)
  #SET(%CBpGrp,%CBFreeName('Zoom'))
  #INSERT(%CBPutGroup,%CBpTab,%CBpGrp,'Zoom')
  #INSERT(%CBPutBig,%CBpGrp,'Zoom',%CBpCmd + 22,'Zoom','Zoom')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Zoom in',%CBpCmd + 23,'','Zoom in')
  #INSERT(%CBPutItem,%CBpGrp,'Button','Zoom out',%CBpCmd + 24,'','Zoom out')
#!
#!-----------------------------------------------------------------------------
#! 3.  A popup menu, from one of five presets.
#!-----------------------------------------------------------------------------
#GROUP(%CBAddPresetMenu),AUTO
  #DECLARE(%CBpMnu)
  #DECLARE(%CBpCmd)
  #INSERT(%CBStdIcons)
  #SET(%CBpCmd,%CBNextCmd())
  #CASE(%CBMenuPreset)
  #OF('File')
    #SET(%CBpMnu,%CBFreeName('FileMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&New',%CBpCmd,'New','Ctrl+N')
    #INSERT(%CBPutRow,%CBpMnu,'&Open...',%CBpCmd + 1,'Open','Ctrl+O')
    #INSERT(%CBPutRow,%CBpMnu,'&Save',%CBpCmd + 2,'Save','Ctrl+S')
    #INSERT(%CBPutRow,%CBpMnu,'Save &As...',%CBpCmd + 3,'','')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Print...',%CBpCmd + 4,'Print','Ctrl+P')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'E&xit',%CBpCmd + 5,'Exit','')
  #OF('Edit')
    #SET(%CBpMnu,%CBFreeName('EditMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Undo',%CBpCmd,'Undo','Ctrl+Z')
    #INSERT(%CBPutRow,%CBpMnu,'&Redo',%CBpCmd + 1,'Redo','Ctrl+Y')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'Cu&t',%CBpCmd + 2,'Cut','Ctrl+X')
    #INSERT(%CBPutRow,%CBpMnu,'&Copy',%CBpCmd + 3,'Copy','Ctrl+C')
    #INSERT(%CBPutRow,%CBpMnu,'&Paste',%CBpCmd + 4,'Paste','Ctrl+V')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'Select &all',%CBpCmd + 5,'','Ctrl+A')
  #OF('Browse row')
    #SET(%CBpMnu,%CBFreeName('RowMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Insert',%CBpCmd,'Insert','Ins')
    #INSERT(%CBPutRow,%CBpMnu,'&Change',%CBpCmd + 1,'Change','Enter')
    #SET(%CBItemDefault,1)
    #INSERT(%CBPutRow,%CBpMnu,'&Delete',%CBpCmd + 2,'Delete','Del')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Refresh',%CBpCmd + 3,'Refresh','F5')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Print list...',%CBpCmd + 4,'Print','')
  #OF('View')
    #SET(%CBpMnu,%CBFreeName('ViewMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutItem,%CBpMnu,'Button','&Toolbar',%CBpCmd,'','')
    #SET(%CBItemAutoCheck,1)
    #SET(%CBItemChecked,1)
    #INSERT(%CBPutItem,%CBpMnu,'Button','&Status bar',%CBpCmd + 1,'','')
    #SET(%CBItemAutoCheck,1)
    #SET(%CBItemChecked,1)
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'Zoom &in',%CBpCmd + 2,'Zoom','Ctrl++')
    #INSERT(%CBPutRow,%CBpMnu,'Zoom &out',%CBpCmd + 3,'','Ctrl+-')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Refresh',%CBpCmd + 4,'Refresh','F5')
  #ELSE
    #SET(%CBpMnu,%CBFreeName('HelpMenu'))
    #INSERT(%CBPutMenu,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&Contents',%CBpCmd,'Help','F1')
    #INSERT(%CBPutRow,%CBpMnu,'&Search for help on...',%CBpCmd + 1,'Find','')
    #INSERT(%CBPutSep,%CBpMnu)
    #INSERT(%CBPutRow,%CBpMnu,'&About...',%CBpCmd + 2,'About','')
  #ENDCASE
#!
#!=============================================================================
#! End of ClaCommandBar.tpl
#!=============================================================================
