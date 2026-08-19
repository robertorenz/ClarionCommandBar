!  Reproduces the shape that produced an EMPTY mirrored bar:
!  the MENUBAR carries a USE (so it gets a LOW field equate) while its
!  MENUs do not (so they get SYNTHETIC equates at 8000h and up).  The
!  &Edit menu DOES carry a USE, so the window mixes both ranges too.
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

Window WINDOW('Mirror test - menubar WITH a use, menus WITHOUT'),AT(,,460,220), |
         GRAY,SYSTEM,MAX,RESIZE,FONT('Segoe UI',9),TIMER(10)
       MENUBAR,USE(?MenuBar)
         MENU('&File')
           ITEM('&New'),USE(?MFileNew),KEY(CtrlN)
           ITEM('&Open...'),USE(?MFileOpen),KEY(CtrlO)
           MENU('Open &Recent')
             ITEM('ledger.tps'),USE(?MR1)
             ITEM('customers.tps'),USE(?MR2)
           END
           ITEM,SEPARATOR
           ITEM('E&xit'),USE(?MFileExit)
         END
         MENU('&Edit'),USE(?EditMenu)
           ITEM('Cu&t'),USE(?MCut),KEY(CtrlX)
           ITEM('&Copy'),USE(?MCopy)
           ITEM('&Paste'),USE(?MPaste),DISABLE
         END
         MENU('&Help')
           ITEM('&About'),USE(?MAbout)
         END
       END
       TEXT,AT(6,40,448,174),USE(rep),READONLY,VSCROLL,FONT('Consolas',9)
     END

  CODE
  OPEN(Window)
  IF ~CB.Init(Window, CBS:Tooltips + CBS:MenuIcons)
    MESSAGE('commandbar.dll did not start.')
    CLOSE(Window)
    RETURN
  END
  CB.SetTheme(CBT:SteelBlue)

  bar = CB.AddBar('Menu', CBD:Top, CBBS:MenuBar)
  n   = CB.MirrorMenu(bar, 1)

  rep = 'MirrorMenu returned ' & n & ' top-level menus.' & '<13,10><13,10>' &    |
        CLIP(CB.MenuReport())
  SETCLIPBOARD(CLIP(rep))
  DISPLAY

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
    OF ?MFileNew
      rep = CLIP(rep) & '<13,10>?MFileNew fired from the mirrored bar.'
      DISPLAY
    OF ?MR1
      rep = CLIP(rep) & '<13,10>?MR1 fired - from the SUBMENU.'
      DISPLAY
    OF ?MAbout
      rep = CLIP(rep) & '<13,10>?MAbout fired - a menu with no USE on its MENU.'
      DISPLAY
    END
  END
  CB.Kill()
  CLOSE(Window)
