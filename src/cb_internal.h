/*=====================================================================
  cb_internal.h  -  ClaCommandBar internals, shared by commandbar.cpp
                    (API + layout) and cb_render.cpp (drawing + window
                    procedures + menu tracking).

  Nothing in here is exported.  The public contract is commandbar.h.

  Coordinate convention: every render target is created at 96 DPI, so
  one DIP is one pixel and all layout arithmetic below is in PIXELS.
  High-DPI is handled by scaling the METRICS and the FONT SIZES by
  Manager::dpiScale, never by scaling the render target.
  =====================================================================*/
#ifndef CB_INTERNAL_H
#define CB_INTERNAL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <deque>
#include <map>

#include "commandbar.h"

/*---------------------------------------------------------------------
  Window class names.  Registered once in CB_Initialize.
  ---------------------------------------------------------------------*/
#define CBWC_BAR   L"ClaCommandBar.Bar"
#define CBWC_MENU  L"ClaCommandBar.Menu"
#define CBWC_TIP   L"ClaCommandBar.Tip"
#define CBWC_HINT  L"ClaCommandBar.Hint"

/*---------------------------------------------------------------------
  Timers used on a bar window.
  ---------------------------------------------------------------------*/
/* Applying a drag DESTROYS and recreates the dragged bar's window when
   it changes between child and popup - which must not happen inside
   that window's own message handler.  The drop is posted to the parent
   and applied from there instead. */
#define CBMSG_APPLYDRAG (WM_APP + 0x5B)
/*  Posted to the host when it creates or destroys a child of its own, so
    the new window is picked up and pushed clear of the bars.  Posted,
    not sent: it arrives after the host has finished building it. */
#define CBMSG_HOSTKIDS  (WM_APP + 0x5C)
/*  Posted when the strip the host keeps for itself turns out to be a
    different size, so the bars can be laid out again knowing it. */
#define CBMSG_RELAYOUT  (WM_APP + 0x5D)

#define CBTIMER_TIPSHOW  1      /* hover dwell before a tooltip appears */
#define CBTIMER_TIPHIDE  2      /* auto-hide the tooltip again          */
#define CB_TIPDELAY      600
#define CB_TIPDURATION   8000

/*---------------------------------------------------------------------
  Hit-test zones inside one item.
  ---------------------------------------------------------------------*/
#define CBHIT_NONE    0
#define CBHIT_ITEM    1         /* the command half of the item         */
#define CBHIT_ARROW   2         /* the drop-arrow half of a SPLIT/COLOR */
#define CBHIT_CHEVRON 3         /* the overflow chevron                 */

/*---------------------------------------------------------------------
  What a container IS.  Bars and menus were the whole story until
  ribbons arrived; a ribbon needs two more, and they are containers
  rather than item types so that CB_AddItem fills a ribbon group with
  exactly the same call that fills a toolbar.
  ---------------------------------------------------------------------*/
#define CBK_BAR    0        /* a docked / floating strip               */
#define CBK_MENU   1        /* a popup menu                            */
#define CBK_TAB    2        /* one tab of a ribbon; holds groups       */
#define CBK_GROUP  3        /* one group of a ribbon tab; holds items  */

struct CBManager;

/*---------------------------------------------------------------------
  A registered font slot.  fmt is created lazily and released whenever
  the face / size / weight changes.
  ---------------------------------------------------------------------*/
struct CBFont
{
    std::wstring       face;
    int                sizePt;
    int                bold;
    int                italic;
    IDWriteTextFormat* fmt;      /* normal   */
    IDWriteTextFormat* fmtBold;  /* the same font forced bold           */

    CBFont() : sizePt(9), bold(0), italic(0), fmt(NULL), fmtBold(NULL) {}
};

/*---------------------------------------------------------------------
  A decoded image, kept as device-independent premultiplied BGRA so it
  can be re-uploaded to any render target (each window owns its own).
  ---------------------------------------------------------------------*/
struct CBImage
{
    UINT               w, h;
    std::vector<BYTE>  px;

    CBImage() : w(0), h(0) {}
};

/*---------------------------------------------------------------------
  One item.  Items live in the manager's map and are referenced by id
  from their container's list, so removing one never invalidates the
  ids of the others.
  ---------------------------------------------------------------------*/
struct CBItem
{
    int           id;
    int           container;
    int           type;          /* CBI_*                              */
    long          cmd;
    std::wstring  text;          /* display text, '&' already stripped */
    int           underline;     /* index of the accelerator char, -1  */
    std::wstring  tooltip;
    std::wstring  shortcut;
    std::wstring  value;         /* EDIT / COMBO current text          */
    int           image;
    unsigned long style;         /* CBIS_*                             */
    bool          enabled;
    bool          checked;
    bool          visible;
    int           menu;          /* container id of the popup, 0 none  */
    int           width;         /* fixed px, 0 = automatic            */
    COLORREF      color;         /* CBI_COLOR swatch                   */
    std::vector<std::wstring> combo;
    int           comboSel;

    /*  A slider, a spin box and a progress bar are the same three
        numbers wearing different clothes. */
    int           vlo, vhi, vval;

    /*  A gallery's cells, and how they are arranged. */
    struct Cell { int image; std::wstring text; };
    std::vector<Cell> cells;
    int           gCols, gCellW, gCellH, gSel, gHot;

    /* ---- filled in by the layout pass ---- */
    RECT          rc;            /* whole item, container client coords */
    RECT          arrow;         /* the drop-arrow zone (SPLIT/COLOR)   */
    int           row;
    bool          overflow;      /* pushed into the chevron menu        */

    CBItem()
        : id(0), container(0), type(CBI_BUTTON), cmd(0), underline(-1),
          image(0), style(0), enabled(true), checked(false), visible(true),
          menu(0), width(0), color(RGB(0, 0, 0)), comboSel(-1),
          vlo(0), vhi(100), vval(0),
          gCols(4), gCellW(56), gCellH(48), gSel(-1), gHot(-1),
          row(0), overflow(false)
    {
        SetRectEmpty(&rc);
        SetRectEmpty(&arrow);
    }
};

/*---------------------------------------------------------------------
  A container: a BAR (docked or floating strip) or a MENU (popup).
  One struct serves both so CB_AddItem can take either id.
  ---------------------------------------------------------------------*/
struct CBContainer
{
    int               id;
    int               kind;      /* CBK_*                              */
    CBManager*        mgr;
    std::vector<int>  items;     /* items, or - for a ribbon bar/tab -
                                    the ids of its tabs / groups       */
    int               owner;     /* the bar a tab belongs to, or the
                                    tab a group belongs to             */

    /* ---- bar ---- */
    std::wstring      title;
    int               dock;        /* CBD_*                            */
    int               dockRow;     /* which strip on that side          */
    int               dockOffset;  /* order within the strip            */
    unsigned long     style;       /* CBBS_*                           */
    bool              visible;
    int               floatX, floatY;
    RECT              fixedRc;    /* CBD_FIXED placement, client px    */

    /* ---- the window, and its device-dependent resources ---- */
    HWND                      hwnd;
    ID2D1HwndRenderTarget*    rt;
    ID2D1SolidColorBrush*     brush;  /* one reusable brush per target  */
    std::vector<ID2D1Bitmap*> bmp;    /* parallel to mgr->images        */
    int                       bmpSize;/* the px size bmp[] was built for */

    /* ---- layout results ---- */
    /* The item ids actually laid out on this bar.  For a plain bar that
       is just `items`; for a RIBBON it is the active tab's group items,
       so hit-testing and painting can walk one list either way. */
    std::vector<int>  laid;
    int   measW, measH;    /* natural size in px                        */
    int   rowCount;
    RECT  chevronRc;
    bool  hasChevron;
    int   chevronMenu;     /* container id, rebuilt on every overflow   */

    /* ---- interaction state ---- */
    int   hotItem;
    int   hotZone;
    int   pressItem;
    int   pressZone;
    int   openItem;        /* item whose popup is on screen             */
    int   tipItem;         /* item the tooltip is showing for           */

    /* ---- menu-only ---- */
    int   ownerItem;       /* the item this popup hangs off             */

    /* ---- ribbon ---- */
    int   activeTab;       /* on a ribbon bar: the tab on show          */
    int   hotTab;          /* tab strip hot-tracking                    */
    bool  minimized;       /* ribbon collapsed to its tab strip          */
    RECT  minRc;           /* the little collapse button, right of the tabs */
    bool  hotMin;          /* the pointer is on it                        */
    RECT  tabRc;           /* on a tab: its rect in the strip           */
    RECT  groupRc;         /* on a group: its whole box                 */
    std::wstring caption;  /* tab / group caption                       */
    int   selIndex;        /* keyboard selection, -1 none               */
    int   gutterW;
    int   shortcutW;

    CBContainer()
        : id(0), kind(CBK_BAR), mgr(NULL), owner(0), dock(CBD_TOP), dockRow(0),
          dockOffset(0), style(0), visible(true), floatX(100), floatY(100),
          hwnd(NULL), rt(NULL), brush(NULL), bmpSize(0),
          measW(0), measH(0), rowCount(1),
          hasChevron(false), chevronMenu(0), hotItem(0), hotZone(CBHIT_NONE),
          pressItem(0), pressZone(CBHIT_NONE), openItem(0), tipItem(0),
          ownerItem(0), selIndex(-1), gutterW(0), shortcutW(0),
          activeTab(0), hotTab(0), minimized(false), hotMin(false)
    {
        SetRectEmpty(&chevronRc);
        SetRectEmpty(&tabRc);
        SetRectEmpty(&minRc);
        SetRectEmpty(&groupRc);
        SetRectEmpty(&fixedRc);
    }
};

/*---------------------------------------------------------------------
  A queued user action.
  ---------------------------------------------------------------------*/
/*---------------------------------------------------------------------
  One of the HOST's own child windows, subclassed so its layout can be
  corrected in flight.  `canon` is the rect the host last asked for,
  measured against the FULL client area - transforming always starts
  from that, never from where the window currently is, which is what
  keeps the whole thing idempotent.
  ---------------------------------------------------------------------*/
struct CBHostKid
{
    HWND    hwnd;
    WNDPROC oldProc;
    RECT    canon;
};

struct CBEvent
{
    int  item;
    long cmd;
    int  type;
    long param;
};

struct CBAccel
{
    long cmd;
    int  key;
    int  mods;
};

/*---------------------------------------------------------------------
  A theme is stored as a SEED, not as 37 loose colours: the seed says
  what the theme IS (its surfaces, its text, its one accent) and
  CBBuildPalette derives every hover / pressed / checked / gutter shade
  from it.  That is what makes CB_SetAccent a one-liner - swap the
  accent in the seed, rebuild, and the whole palette stays coherent.
  ---------------------------------------------------------------------*/
struct CBSeed
{
    COLORREF barTop, barBot, barBorder;
    COLORREF text, textDis;
    COLORREF accent;
    COLORREF menuBack, menuBorder;
    COLORREF editBack;
    bool     dark;
    int      corner;      /* the theme's natural button radius, px     */
};

void CBGetSeed(int theme, CBSeed* out);
void CBBuildPalette(const CBSeed& s, COLORREF* col);

/*---------------------------------------------------------------------
  The manager.
  ---------------------------------------------------------------------*/
struct CBManager
{
    HWND           parent;
    WNDPROC        oldParentProc;   /* NULL if not subclassed           */
    unsigned long  style;           /* CBS_*                            */
    int            theme;
    COLORREF       col[CBC_COUNT + 1];
    CBFont         font[5];         /* 1..4, [0] unused                 */
    int            metric[CBM_ROWGAP + 1];
    float          dpiScale;

    std::vector<CBImage>        images;      /* [0] unused              */
    std::map<int, CBContainer*> containers;
    std::map<int, CBItem*>      items;
    int                         nextContainer;
    int                         nextItem;

    std::deque<CBEvent>         events;
    CB_EVENTPROC                proc;
    long                        procUser;

    std::vector<CBAccel>        accels;

    RECT           clientRc;        /* what CB_GetClientRect reports    */
    /*  What the docked bars take off each edge, as THICKNESSES.  Kept
        apart from clientRc on purpose: a thickness stays true when the
        host is resized, whereas clientRc's right/bottom are coordinates
        that go stale the instant the window changes size - and the host
        moves its own children long before it tells us to lay out. */
    int            insL, insT, insR, insB;
    /*  A strip along the bottom the HOST paints itself and no bar may
        use.  A Clarion APPLICATION frame draws its status bar there -
        there is no status window to find, the frame just stops its MDI
        client short of the bottom - so it is measured from how far short
        the client stops.  Without it a left, right or bottom bar runs
        straight over the status bar. */
    int            hostResB;
    int            userResB;        /* set by hand; <0 means "measure it" */
    bool           inLayout;
    bool           destroying;

    /* tooltip window, shared by every bar of this manager */
    HWND           tipWnd;
    ID2D1HwndRenderTarget* tipRt;
    std::wstring   tipText;

    CBSeed         seed;            /* what the current theme IS        */

    /* The host's own children.  They are SUBCLASSED, not repositioned:
       Clarion derives the MDI client's top from the toolbar's HEIGHT
       rather than its position, so moving them after the fact just
       starts a fight.  Correcting WM_WINDOWPOSCHANGING wins instead. */
    std::vector<CBHostKid> hostKids;
    int            reserve;         /* 1 on, 0 off, -1 auto             */

    /* the popup-menu chain currently on screen (container ids) */
    std::vector<int> menuChain;
    bool           menuCancelled;
    long           menuResult;
    int            menuResultItem;
    /* Set while a menu is tracking when the mouse crosses onto another
       top-level CBI_MENU item of the owning bar: the loop exits and the
       bar reopens on that item, which is what makes a menu bar feel
       like a menu bar. */
    int            menuSwitchItem;
    /* While a throwaway popup is up (the list behind a CBI_COMBO), the
       events its rows raise are the manager's own business, not the
       host's.  CBQueue drops anything belonging to this container. */
    int            suppressContainer;

    /* The host's own menu bar while it is detached (CB_SetHostMenuVisible).
       Kept, never destroyed - the field equates behind it stay live. */
    HMENU          hostMenu;

    /* Dragging a bar by its gripper (docked) or its caption (floating).
       Nothing moves until the drop: a translucent hint window shows
       where the bar would land, which is the only way to make docking
       to an edge readable. */
    int            dragBar;
    POINT          dragOff;      /* grab point inside the bar          */
    POINT          dragStart;    /* screen, for the movement threshold */
    bool           dragActive;   /* past the threshold                 */
    int            slideItem;    /* the slider the mouse is dragging   */
    HWND           dragHint;
    int            dragDock;     /* CBD_* the drop would apply         */
    int            dragRow;
    int            dragApply;    /* the bar the posted drop belongs to */
    /* How thick the bar would be docked on each side, measured once at
       the start of the drag.  A toolbar docked on TOP is wide and short;
       the same bar on LEFT is narrow and tall, so its CURRENT size is
       the wrong hint for a vertical target. */
    int            dragThick[6];

    /* the overlay EDIT control used while typing in a CBI_EDIT item.
       Painting our own caret and selection would mean reimplementing
       clipboard, IME and shift-selection; a real EDIT parked over the
       item for the duration gets all of that for free. */
    HWND           editWnd;
    WNDPROC        editOldProc;
    int            editItem;
    HFONT          editFont;

    CBManager()
        : parent(NULL), oldParentProc(NULL), style(0), theme(CBT_STEELBLUE),
          dpiScale(1.0f), nextContainer(1), nextItem(1), proc(NULL),
          procUser(0), inLayout(false), destroying(false), tipWnd(NULL),
          tipRt(NULL), menuCancelled(false), menuResult(0), menuResultItem(0),
          menuSwitchItem(0), suppressContainer(0), hostMenu(NULL),

          reserve(-1),
          dragBar(0), dragActive(false), slideItem(0), dragHint(NULL),
          dragDock(CBD_FLOAT), dragRow(0), dragApply(0),
          editWnd(NULL), editOldProc(NULL),
          editItem(0), editFont(NULL)
    {
        dragOff.x = dragOff.y = 0;
        dragStart.x = dragStart.y = 0;
        ZeroMemory(dragThick, sizeof(dragThick));
        SetRectEmpty(&clientRc);
        insL = insT = insR = insB = 0;
        hostResB = 0;
        userResB = -1;
        ZeroMemory(col, sizeof(col));
        ZeroMemory(metric, sizeof(metric));
    }
};

/*---------------------------------------------------------------------
  Process-wide factories, owned by CB_Initialize / CB_Shutdown.
  ---------------------------------------------------------------------*/
extern ID2D1Factory*      g_d2d;
extern IDWriteFactory*    g_dw;
extern IWICImagingFactory* g_wic;
extern HINSTANCE          g_inst;
extern LONG               g_initCount;

/*---------------------------------------------------------------------
  Small helpers (cb_render.cpp).
  ---------------------------------------------------------------------*/
std::wstring   CBToWide(const char* s);
std::string    CBToAnsi(const std::wstring& w);
/* Strips '&' for display and reports where the accelerator landed. */
std::wstring   CBStripAmp(const std::wstring& in, int* underline);

D2D1_COLOR_F   CBColor(COLORREF c);
D2D1_COLOR_F   CBColorA(COLORREF c, float alpha);
COLORREF       CBBlend(COLORREF a, COLORREF b, float t);
COLORREF       CBLighten(COLORREF c, float t);
COLORREF       CBDarken(COLORREF c, float t);
bool           CBIsDark(COLORREF c);
double         CBLuma(COLORREF c);

/* Theme + accent.  BuildPalette fills mgr->col from the seed of the
   theme the manager currently carries. */
void           CBApplyTheme(CBManager* m, int theme);
void           CBApplyAccent(CBManager* m, COLORREF accent);

/* Fonts.  bold=true asks for the bold twin of the slot. */
IDWriteTextFormat* CBGetFormat(CBManager* m, int slot, bool bold);
void           CBReleaseFonts(CBManager* m);
/* Measure a single line with one of the slots. */
void           CBMeasure(CBManager* m, int slot, bool bold,
                         const std::wstring& text, float* w, float* h);
int            CBFontHeight(CBManager* m, int slot);

/* Images */
bool           CBDecodeFile(const wchar_t* file, CBImage* out);
bool           CBDecodeStrip(const wchar_t* file, int cx,
                             std::vector<CBImage>* out);
bool           CBDecodeHandle(HANDLE h, bool isIcon, CBImage* out);
/* size is the px box the image will be drawn in: the bitmap is
   pre-scaled to it with WIC so D2D never has to resample. */
ID2D1Bitmap*   CBGetBitmap(CBContainer* c, int image, int size);
void           CBDiscardBitmaps(CBContainer* c);

/* Metrics, already scaled for DPI. */
int            CBMetric(CBManager* m, int metric);

/* Layout (commandbar.cpp) */
void           CBMeasureItem(CBManager* m, CBContainer* c, CBItem* it,
                             int* w, int* h);
void           CBLayoutBar(CBManager* m, CBContainer* c, int availW,
                           int availH);
void           CBLayoutMenu(CBManager* m, CBContainer* c);
void           CBLayoutRibbon(CBManager* m, CBContainer* c, int availW);
void           CBPaintRibbon(CBManager* m, CBContainer* c);
/* Hit test a ribbon's tab strip: the tab container id, or 0. */
int            CBTabHitTest(CBManager* m, CBContainer* c, POINT pt);
bool           CBRibbonMinHit(CBContainer* c, POINT pt);
void           CBRelayout(CBManager* m);
void           CBQueue(CBManager* m, int item, long cmd, int type, long param);
CBItem*        CBFindItem(CBManager* m, int id);
CBContainer*   CBFindContainer(CBManager* m, int id);
void           CBEnsureBarWindow(CBManager* m, CBContainer* c);
int            CBItemHitTest(CBManager* m, CBContainer* c, POINT pt,
                             int* zone);

/* Rendering + window procedures (cb_render.cpp) */
void           CBRegisterClasses(void);
LRESULT CALLBACK CBBarProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CBMenuProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CBTipProc(HWND, UINT, WPARAM, LPARAM);
void           CBPaintBar(CBManager* m, CBContainer* c);
void           CBPaintMenu(CBManager* m, CBContainer* c);
bool           CBEnsureRT(CBContainer* c);
void           CBDiscardRT(CBContainer* c);

/* Popup menus */
long           CBTrackPopup(CBManager* m, int menu, int x, int y,
                            int ownerItem, int ownerBar, const RECT* exclude);
void           CBCloseMenus(CBManager* m);

/* Tooltip */
void           CBShowTip(CBManager* m, CBContainer* c, CBItem* it);
void           CBHideTip(CBManager* m);

/* Dragging a bar to a new dock, or off into a floating frame */
bool           CBGripperRect(CBManager* m, CBContainer* c, RECT* out);
void           CBBeginDrag(CBManager* m, CBContainer* c, POINT screenPt);
void           CBUpdateDrag(CBManager* m, POINT screenPt);
void           CBEndDrag(CBManager* m, bool apply);
void           CBApplyDrag(CBManager* m);
/* Hook the HOST's own child windows so they lay out beside the bars
   instead of underneath them, and unhook them again. */
void           CBReserveFromHost(CBManager* m);
/*  Cheap check for a host child we have not seen yet - a merged toolbar
    the host built without telling us.  Enumerates a handful of windows;
    only does real work when something new turns up. */
void           CBWatchHostKids(CBManager* m);
void           CBReleaseHostChildren(CBManager* m);

/* Dock `bar` at `row`, pushing every other bar on that side down. */
void           CBInsertBarRow(CBManager* m, int bar, int dock, int row);

/* Overlay edit control */
void           CBBeginEdit(CBManager* m, CBContainer* c, CBItem* it);
void           CBEndEdit(CBManager* m, bool commit);

#endif /* CB_INTERNAL_H */
