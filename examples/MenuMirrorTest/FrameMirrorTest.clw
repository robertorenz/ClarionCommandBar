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
  AppFrame{PROP:StatusText, 1} = 'MirrorMenu found ' & n & ' menus - report on the clipboard'

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
      AppFrame{PROP:StatusText, 1} = '?MNew fired from the mirrored bar'
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
