!  The case that reported an empty bar: an MDI APPLICATION FRAME.
!  A frame numbers its menu controls DOWNWARD from -1 (named) and
!  DOWNWARD from 32767 (unnamed), and answers LASTFIELD() = 0 - nothing
!  like the WINDOW the earlier tests used.
  PROGRAM

  PRAGMA('link(commandbar.lib)')

  INCLUDE('EQUATES.CLW'),ONCE
  INCLUDE('KEYCODES.CLW'),ONCE
  INCLUDE('CommandBar.inc'),ONCE

  MAP
ChildWin PROCEDURE()
  END

CB       CommandBarClass
bar      SIGNED
n        SIGNED
rep      STRING(2000)

AppFrame APPLICATION('Frame mirror test - an MDI APPLICATION'),AT(,,520,300), |
           SYSTEM,MAX,RESIZE,FONT('Segoe UI',9),TIMER(10),STATUS(-1)
       MENUBAR,USE(?MenuBar)
         MENU('&File'),USE(?FileMenu)
           ITEM('&New'),USE(?MNew),KEY(CtrlN)
           ITEM('&Open...'),USE(?MOpen),KEY(CtrlO)
           MENU('Open &Recent')
             ITEM('ledger.tps'),USE(?MR1)
             ITEM('customers.tps'),USE(?MR2)
           END
           ITEM,SEPARATOR
           ITEM('E&xit'),USE(?MExit)
         END
         MENU('&Edit'),USE(?EditMenu)
           ITEM('Cu&t'),USE(?MCut),KEY(CtrlX)
           ITEM('&Copy'),USE(?MCopy),KEY(CtrlC)
           ITEM('&Paste'),USE(?MPaste),DISABLE
         END
         MENU('&Trees')
           ITEM('&Show'),USE(?MTrees)
         END
         MENU('&Window'),USE(?WindowMenu)
           ITEM('&Cascade'),USE(?MCascade)
         END
         MENU('&Help'),USE(?HelpMenu)
           ITEM('&About'),USE(?MAbout)
         END
       END
       TOOLBAR,USE(?Toolbar)
         BUTTON('VUTools'),USE(?B1),AT(4,2,50,14)
         BUTTON('VuShowcase'),USE(?B2),AT(58,2,60,14)
         BUTTON('Stimulsoft'),USE(?B3),AT(4,18,60,14)
       END
     END

  CODE
  OPEN(AppFrame)
  IF ~CB.Init(AppFrame, CBS:Tooltips + CBS:MenuIcons)
    MESSAGE('commandbar.dll did not start.')
    CLOSE(AppFrame)
    RETURN
  END
  CB.SetTheme(CBT:SteelBlue)

  bar = CB.AddBar('Menu', CBD:Top, CBBS:MenuBar)
  n   = CB.MirrorMenu(bar, 1)

  rep = 'MirrorMenu returned ' & n & ' top-level menus.<13,10><13,10>' &        |
        CLIP(CB.MenuReport())
  SETCLIPBOARD(CLIP(rep))
  AppFrame{PROP:StatusText, 1} = 'menus=' & n &                                  |
      '  reserveMode=' & CB.ReserveMode() &                                      |
      '  clientY=' & CB.ClientY() & '  clientH=' & CB.ClientHeight()

  ACCEPT
    CASE EVENT()
    OF EVENT:Timer
      LOOP WHILE CB.TakeOne()
      END
      CYCLE
    OF EVENT:Sized
      CB.Layout()
    END
    CASE ACCEPTED()
    OF ?MNew
      AppFrame{PROP:StatusText, 1} = '?MNew fired - opening an MDI child'
      START(ChildWin)
    OF ?MR1
      AppFrame{PROP:StatusText, 1} = '?MR1 fired - from the SUBMENU'
    OF ?MTrees
      AppFrame{PROP:StatusText, 1} = '?MTrees fired - a MENU with no USE'
    OF ?MAbout
      AppFrame{PROP:StatusText, 1} = '?MAbout fired'
    OF ?MExit
      POST(EVENT:CloseWindow)
    END
  END
  CB.Kill()
  CLOSE(AppFrame)

!  An MDI child carrying its own MENUBAR and TOOLBAR, so Clarion MERGES
!  both into the frame while it is open.  That merge is what made the
!  frame's toolbar vanish.
ChildWin PROCEDURE()

ChildQ QUEUE,PRE(CQ)
Name     STRING(30)
       END
i      SIGNED

Window WINDOW('Browse the Enrollment File'),AT(,,300,180),MDI,SYSTEM,GRAY,RESIZE, |
         FONT('Segoe UI',9)
       MENUBAR
         MENU('&Child')
           ITEM('Child &action'),USE(?CAction)
         END
       END
       TOOLBAR
         BUTTON('Child tool'),USE(?CTool),AT(4,2,60,14)
       END
       LIST,AT(4,4,292,150),USE(?CList),FROM(ChildQ),HVSCROLL, |
         FORMAT('280L(2)|M~Name~@s30@')
       BUTTON('&Close'),AT(240,160,50,14),USE(?CClose),STD(STD:Close)
     END

  CODE
  OPEN(Window)
  LOOP i = 1 TO 8
    CLEAR(ChildQ)
    CQ:Name = 'Enrollment row ' & i
    ADD(ChildQ)
  END
  DISPLAY
  ACCEPT
  END
  CLOSE(Window)
