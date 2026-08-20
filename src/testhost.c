/*=====================================================================
  testhost.c  -  a plain Win32 host that exercises COMMANDBAR.DLL
                 without involving Clarion.

  Build with src\build.bat, run bin\testhost.exe.  Everything the
  engine can draw is on screen: a menu bar with submenus, a toolbar
  with every item type, a second toolbar row, a left-docked bar, and a
  View menu that cycles all eleven themes so you can see each one.
  =====================================================================*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "commandbar.h"

static HCB  g_cb   = NULL;
static int  g_startTheme = 0;      /* testhost.exe 7  starts on theme 7 */
static char g_last[256] = "Ready.";
static int  g_barMain = 0, g_barMenu = 0, g_barSide = 0, g_barFmt = 0;
static int  g_barRibbon = 0;
static int g_itemZoom, g_itemSpin, g_itemProg;
static int  g_itemBold = 0, g_itemFont = 0, g_itemFind = 0, g_itemColor = 0;

/* ---- command ids ---- */
#define CMD_NEW      101
#define CMD_OPEN     102
#define CMD_SAVE     103
#define CMD_EXIT     104
#define CMD_CUT      110
#define CMD_COPY     111
#define CMD_PASTE    112
#define CMD_BOLD     120
#define CMD_ITALIC   121
#define CMD_UNDER    122
#define CMD_COLOR    130
#define CMD_FONT     131
#define CMD_FIND     132
#define CMD_WRAP     133
#define CMD_ABOUT    140
#define CMD_SIDE     141
#define CMD_FLOAT    142
#define CMD_THEME    200          /* +0..+10 for the eleven themes    */

static const char* kThemeName[] = {
    "Steel Blue", "Office 2003", "Office 2007", "Office 2010",
    "Office 2013", "Office 2016", "VS 2012 Light", "VS 2012 Dark",
    "Windows 11 Light", "Windows 11 Dark", "Slate Dark"
};

/* Stock icons, scaled down by the engine - enough to prove the image
   path works without shipping art with the test. */
static int StockImage(HCB cb, LPCTSTR id)
{
    HICON h = LoadIcon(NULL, id);
    return h ? CB_AddImageHandle(cb, (HANDLE)h, 1) : 0;
}

static void SetTheme(int index);

static void BuildBars(HWND hwnd)
{
    int imgInfo, imgWarn, imgErr, imgQ, imgApp;
    int mFile, mEdit, mView, mHelp, mRecent, mTheme, mOpen, mFind;
    int it, i;

    g_cb = CB_Create(hwnd, CBS_TOOLTIPS | CBS_CHEVRON | CBS_HOTTEXT |
                           CBS_MENUICONS);
    if (!g_cb) return;

    CB_SetTheme(g_cb, CBT_STEELBLUE);

    imgInfo = StockImage(g_cb, IDI_INFORMATION);
    imgWarn = StockImage(g_cb, IDI_WARNING);
    imgErr  = StockImage(g_cb, IDI_ERROR);
    imgQ    = StockImage(g_cb, IDI_QUESTION);
    imgApp  = StockImage(g_cb, IDI_APPLICATION);

    /* ---------------- menu bar ---------------- */
    g_barMenu = CB_AddBar(g_cb, "Menu", CBD_TOP, CBBS_MENUBAR);
    CB_SetBarDock(g_cb, g_barMenu, CBD_TOP, 0, 0);

    mFile   = CB_CreateMenu(g_cb);
    mRecent = CB_CreateMenu(g_cb);
    mEdit   = CB_CreateMenu(g_cb);
    mView   = CB_CreateMenu(g_cb);
    mTheme  = CB_CreateMenu(g_cb);
    mHelp   = CB_CreateMenu(g_cb);

    it = CB_AddItem(g_cb, g_barMenu, CBI_MENU, 0, "&File", 0);
    CB_SetItemMenu(g_cb, it, mFile);
    it = CB_AddItem(g_cb, g_barMenu, CBI_MENU, 0, "&Edit", 0);
    CB_SetItemMenu(g_cb, it, mEdit);
    it = CB_AddItem(g_cb, g_barMenu, CBI_MENU, 0, "&View", 0);
    CB_SetItemMenu(g_cb, it, mView);
    it = CB_AddItem(g_cb, g_barMenu, CBI_MENU, 0, "&Help", 0);
    CB_SetItemMenu(g_cb, it, mHelp);

    it = CB_AddItem(g_cb, mFile, CBI_BUTTON, CMD_NEW, "&New", imgInfo);
    CB_SetItemShortcut(g_cb, it, "Ctrl+N");
    CB_SetItemStyle(g_cb, it, CBIS_DEFAULT);
    it = CB_AddItem(g_cb, mFile, CBI_BUTTON, CMD_OPEN, "&Open...", imgApp);
    CB_SetItemShortcut(g_cb, it, "Ctrl+O");
    it = CB_AddItem(g_cb, mFile, CBI_BUTTON, 0, "Open &Recent", 0);
    CB_SetItemMenu(g_cb, it, mRecent);
    it = CB_AddItem(g_cb, mFile, CBI_BUTTON, CMD_SAVE, "&Save", imgWarn);
    CB_SetItemShortcut(g_cb, it, "Ctrl+S");
    CB_AddItem(g_cb, mFile, CBI_SEPARATOR, 0, "", 0);
    it = CB_AddItem(g_cb, mFile, CBI_BUTTON, CMD_EXIT, "E&xit", 0);
    CB_SetItemShortcut(g_cb, it, "Alt+F4");

    CB_AddItem(g_cb, mRecent, CBI_BUTTON, 301, "C:\\work\\ledger.tps", 0);
    CB_AddItem(g_cb, mRecent, CBI_BUTTON, 302, "C:\\work\\customers.tps", 0);
    CB_AddItem(g_cb, mRecent, CBI_BUTTON, 303, "C:\\work\\invoices.tps", 0);

    it = CB_AddItem(g_cb, mEdit, CBI_BUTTON, CMD_CUT, "Cu&t", 0);
    CB_SetItemShortcut(g_cb, it, "Ctrl+X");
    it = CB_AddItem(g_cb, mEdit, CBI_BUTTON, CMD_COPY, "&Copy", 0);
    CB_SetItemShortcut(g_cb, it, "Ctrl+C");
    it = CB_AddItem(g_cb, mEdit, CBI_BUTTON, CMD_PASTE, "&Paste", 0);
    CB_SetItemShortcut(g_cb, it, "Ctrl+V");
    CB_SetItemEnabled(g_cb, it, 0);
    CB_AddItem(g_cb, mEdit, CBI_SEPARATOR, 0, "", 0);
    mFind = CB_CreateMenu(g_cb);
    it = CB_AddItem(g_cb, mEdit, CBI_BUTTON, 0, "F&ind", 0);
    CB_SetItemMenu(g_cb, it, mFind);
    CB_AddItem(g_cb, mFind, CBI_BUTTON, 310, "Find &Next", 0);
    CB_AddItem(g_cb, mFind, CBI_BUTTON, 311, "Find &Previous", 0);
    CB_AddItem(g_cb, mFind, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, mFind, CBI_BUTTON, 312, "&Replace...", 0);

    it = CB_AddItem(g_cb, mView, CBI_BUTTON, 0, "&Theme", 0);
    CB_SetItemMenu(g_cb, it, mTheme);
    for (i = 0; i < 11; ++i)
    {
        it = CB_AddItem(g_cb, mTheme, CBI_BUTTON, CMD_THEME + i, kThemeName[i], 0);
        CB_SetItemStyle(g_cb, it, CBIS_RADIO);
        if (i == 0) CB_SetItemChecked(g_cb, it, 1);
    }
    CB_AddItem(g_cb, mView, CBI_SEPARATOR, 0, "", 0);
    it = CB_AddItem(g_cb, mView, CBI_BUTTON, CMD_SIDE, "&Side bar", 0);
    CB_SetItemStyle(g_cb, it, CBIS_AUTOCHECK);
    CB_SetItemChecked(g_cb, it, 1);
    it = CB_AddItem(g_cb, mView, CBI_BUTTON, CMD_WRAP, "&Word wrap", 0);
    CB_SetItemStyle(g_cb, it, CBIS_AUTOCHECK);
    CB_AddItem(g_cb, mView, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, mView, CBI_BUTTON, CMD_FLOAT, "&Float the format bar", 0);

    CB_AddItem(g_cb, mHelp, CBI_BUTTON, CMD_ABOUT, "&About ClaCommandBar", imgQ);

    /* ---------------- main toolbar ---------------- */
    g_barMain = CB_AddBar(g_cb, "Standard", CBD_TOP, CBBS_GRIPPER | CBBS_FLOATABLE);
    CB_SetBarDock(g_cb, g_barMain, CBD_TOP, 1, 0);

    it = CB_AddItem(g_cb, g_barMain, CBI_BUTTON, CMD_NEW, "New", imgInfo);
    CB_SetItemTooltip(g_cb, it, "New document (Ctrl+N)");

    mOpen = CB_CreateMenu(g_cb);
    CB_AddItem(g_cb, mOpen, CBI_BUTTON, 301, "ledger.tps", 0);
    CB_AddItem(g_cb, mOpen, CBI_BUTTON, 302, "customers.tps", 0);
    CB_AddItem(g_cb, mOpen, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, mOpen, CBI_BUTTON, CMD_OPEN, "&Browse...", 0);
    it = CB_AddItem(g_cb, g_barMain, CBI_SPLIT, CMD_OPEN, "Open", imgApp);
    CB_SetItemMenu(g_cb, it, mOpen);
    CB_SetItemTooltip(g_cb, it, "Open - the arrow lists recent files");

    it = CB_AddItem(g_cb, g_barMain, CBI_BUTTON, CMD_SAVE, "Save", imgWarn);
    CB_SetItemTooltip(g_cb, it, "Save (Ctrl+S)");

    CB_AddItem(g_cb, g_barMain, CBI_SEPARATOR, 0, "", 0);

    it = CB_AddItem(g_cb, g_barMain, CBI_TOGGLE, CMD_BOLD, "B", 0);
    CB_SetItemTooltip(g_cb, it, "Bold");
    g_itemBold = it;
    it = CB_AddItem(g_cb, g_barMain, CBI_TOGGLE, CMD_ITALIC, "I", 0);
    CB_SetItemTooltip(g_cb, it, "Italic");
    it = CB_AddItem(g_cb, g_barMain, CBI_TOGGLE, CMD_UNDER, "U", 0);
    CB_SetItemTooltip(g_cb, it, "Underline");

    CB_AddItem(g_cb, g_barMain, CBI_SEPARATOR, 0, "", 0);

    it = CB_AddItem(g_cb, g_barMain, CBI_COLOR, CMD_COLOR, "", imgErr);
    CB_SetItemColor(g_cb, it, RGB(192, 32, 32));
    CB_SetItemTooltip(g_cb, it, "Text colour - the arrow opens the picker");
    g_itemColor = it;

    it = CB_AddItem(g_cb, g_barMain, CBI_DROPDOWN, CMD_FONT, "Style", 0);
    {
        int mStyle = CB_CreateMenu(g_cb);
        CB_AddItem(g_cb, mStyle, CBI_BUTTON, 401, "Heading 1", 0);
        CB_AddItem(g_cb, mStyle, CBI_BUTTON, 402, "Heading 2", 0);
        CB_AddItem(g_cb, mStyle, CBI_BUTTON, 403, "Body text", 0);
        CB_SetItemMenu(g_cb, it, mStyle);
    }

    CB_AddItem(g_cb, g_barMain, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, g_barMain, CBI_LABEL, 0, "Find:", 0);
    it = CB_AddItem(g_cb, g_barMain, CBI_EDIT, CMD_FIND, "", 0);
    CB_SetItemValue(g_cb, it, "customer");
    CB_SetItemWidth(g_cb, it, 120);
    CB_SetItemTooltip(g_cb, it, "Type and press Enter");
    g_itemFind = it;

    it = CB_AddItem(g_cb, g_barMain, CBI_CHECKBOX, CMD_WRAP, "Wrap", 0);
    CB_SetItemTooltip(g_cb, it, "Word wrap");

    /* right-aligned, so it hugs the far end however wide the window is */
    it = CB_AddItem(g_cb, g_barMain, CBI_BUTTON, CMD_ABOUT, "About", imgQ);
    CB_SetItemStyle(g_cb, it, CBIS_RIGHTALIGN);

    /* ---------------- second row: the format bar ---------------- */
    g_barFmt = CB_AddBar(g_cb, "Format", CBD_TOP, CBBS_GRIPPER | CBBS_FLOATABLE);
    CB_SetBarDock(g_cb, g_barFmt, CBD_TOP, 2, 0);

    CB_AddItem(g_cb, g_barFmt, CBI_LABEL, 0, "Font:", 0);
    it = CB_AddItem(g_cb, g_barFmt, CBI_COMBO, CMD_FONT, "", 0);
    CB_AddComboItem(g_cb, it, "Segoe UI");
    CB_AddComboItem(g_cb, it, "Tahoma");
    CB_AddComboItem(g_cb, it, "Consolas");
    CB_AddComboItem(g_cb, it, "Times New Roman");
    CB_SetComboSel(g_cb, it, 0);
    CB_SetItemWidth(g_cb, it, 140);
    g_itemFont = it;

    it = CB_AddItem(g_cb, g_barFmt, CBI_COMBO, 0, "", 0);
    CB_AddComboItem(g_cb, it, "8");
    CB_AddComboItem(g_cb, it, "9");
    CB_AddComboItem(g_cb, it, "10");
    CB_AddComboItem(g_cb, it, "12");
    CB_AddComboItem(g_cb, it, "14");
    CB_SetComboSel(g_cb, it, 1);
    CB_SetItemWidth(g_cb, it, 60);

    CB_AddItem(g_cb, g_barFmt, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, g_barFmt, CBI_BUTTON, 501, "Left", 0);
    CB_AddItem(g_cb, g_barFmt, CBI_BUTTON, 502, "Centre", 0);
    CB_AddItem(g_cb, g_barFmt, CBI_BUTTON, 503, "Right", 0);

    /* ---- the items that carry a number ---- */
    CB_AddItem(g_cb, g_barFmt, CBI_SEPARATOR, 0, "", 0);
    CB_AddItem(g_cb, g_barFmt, CBI_LABEL, 0, "Zoom:", 0);
    g_itemZoom = CB_AddSlider(g_cb, g_barFmt, 601, 10, 400, 100, 130);
    g_itemSpin = CB_AddSpin(g_cb, g_barFmt, 602, 1, 99, 12, 60);
    CB_AddItem(g_cb, g_barFmt, CBI_SEPARATOR, 0, "", 0);
    g_itemProg = CB_AddProgress(g_cb, g_barFmt, 0, 100, 62, 130);
    CB_SetItemText(g_cb, g_itemProg, "62%");

    /* ---------------- a ribbon: tabs of groups of items ---------------- */
    /*  a gallery goes in a ribbon group, below */
    g_barRibbon = CB_AddBar(g_cb, "Ribbon", CBD_TOP, CBBS_RIBBON);
    CB_SetBarDock(g_cb, g_barRibbon, CBD_TOP, 3, 0);
    {
        int tHome, tInsert, tView, grp;

        tHome = CB_AddRibbonTab(g_cb, g_barRibbon, "&Home");

        grp = CB_AddRibbonGroup(g_cb, tHome, "Clipboard");

    {   /* a GALLERY: a grid of picture choices, which is what a ribbon
           has always been missing */
        int gal = CB_AddGallery(g_cb, grp, 610, 4, 58, 52);
        CB_AddGalleryCell(g_cb, gal, imgInfo, "New");
        CB_AddGalleryCell(g_cb, gal, imgWarn, "Open");
        CB_AddGalleryCell(g_cb, gal, imgErr,  "Save");
        CB_AddGalleryCell(g_cb, gal, imgQ,    "Print");
        CB_SetGallerySel(g_cb, gal, 2);
    }
        it = CB_AddItem(g_cb, grp, CBI_SPLIT, CMD_OPEN, "Paste", imgApp);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
        CB_SetItemMenu(g_cb, it, mOpen);
        CB_AddItem(g_cb, grp, CBI_BUTTON, CMD_CUT,   "Cut",   imgInfo);
        CB_AddItem(g_cb, grp, CBI_BUTTON, CMD_COPY,  "Copy",  imgInfo);
        CB_AddItem(g_cb, grp, CBI_BUTTON, CMD_PASTE, "Paste", imgInfo);

        grp = CB_AddRibbonGroup(g_cb, tHome, "Font");
        it = CB_AddItem(g_cb, grp, CBI_COMBO, CMD_FONT, "", 0);
        CB_AddComboItem(g_cb, it, "Segoe UI");
        CB_AddComboItem(g_cb, it, "Consolas");
        CB_SetComboSel(g_cb, it, 0);
        CB_SetItemWidth(g_cb, it, 120);
        CB_AddItem(g_cb, grp, CBI_TOGGLE, CMD_BOLD,   "B", 0);
        CB_AddItem(g_cb, grp, CBI_TOGGLE, CMD_ITALIC, "I", 0);
        it = CB_AddItem(g_cb, grp, CBI_COLOR, CMD_COLOR, "", 0);
        CB_SetItemColor(g_cb, it, RGB(32, 96, 176));

        grp = CB_AddRibbonGroup(g_cb, tHome, "Editing");
        it = CB_AddItem(g_cb, grp, CBI_BUTTON, CMD_FIND, "Find", imgQ);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
        CB_AddItem(g_cb, grp, CBI_BUTTON, 0, "Replace", 0);
        CB_AddItem(g_cb, grp, CBI_BUTTON, 0, "Select",  0);

        tInsert = CB_AddRibbonTab(g_cb, g_barRibbon, "&Insert");
        grp = CB_AddRibbonGroup(g_cb, tInsert, "Tables");
        it = CB_AddItem(g_cb, grp, CBI_DROPDOWN, 0, "Table", imgApp);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
        CB_SetItemMenu(g_cb, it, mOpen);
        grp = CB_AddRibbonGroup(g_cb, tInsert, "Illustrations");
        it = CB_AddItem(g_cb, grp, CBI_BUTTON, 0, "Picture", imgWarn);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
        it = CB_AddItem(g_cb, grp, CBI_BUTTON, 0, "Chart", imgErr);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);

        tView = CB_AddRibbonTab(g_cb, g_barRibbon, "&View");
        grp = CB_AddRibbonGroup(g_cb, tView, "Show");
        CB_AddItem(g_cb, grp, CBI_CHECKBOX, 0, "Ruler",     0);
        CB_AddItem(g_cb, grp, CBI_CHECKBOX, 0, "Gridlines", 0);
        CB_AddItem(g_cb, grp, CBI_CHECKBOX, 0, "Nav pane",  0);
        grp = CB_AddRibbonGroup(g_cb, tView, "Zoom");
        it = CB_AddItem(g_cb, grp, CBI_BUTTON, 0, "Zoom", imgInfo);
        CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
    }

    /* ---------------- left-docked bar, large icons ---------------- */
    g_barSide = CB_AddBar(g_cb, "Tools", CBD_LEFT,
                          CBBS_GRIPPER | CBBS_LARGEICONS);
    it = CB_AddItem(g_cb, g_barSide, CBI_BUTTON, 601, "Home", imgInfo);
    CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
    it = CB_AddItem(g_cb, g_barSide, CBI_BUTTON, 602, "Reports", imgApp);
    CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);
    it = CB_AddItem(g_cb, g_barSide, CBI_BUTTON, 603, "Setup", imgWarn);
    CB_SetItemStyle(g_cb, it, CBIS_TEXTBELOW);

    /* ---------------- accelerators ---------------- */
    CB_AddAccelerator(g_cb, CMD_NEW,  'N', CBK_CTRL);
    CB_AddAccelerator(g_cb, CMD_OPEN, 'O', CBK_CTRL);
    CB_AddAccelerator(g_cb, CMD_SAVE, 'S', CBK_CTRL);
    CB_AddAccelerator(g_cb, CMD_BOLD, 'B', CBK_CTRL);

    if (g_startTheme) SetTheme(g_startTheme);
    CB_Layout(g_cb);
}

static void SetTheme(int index)
{
    int mi, i, n;
    static const int kTheme[] = {
        CBT_STEELBLUE, CBT_OFFICE2003, CBT_OFFICE2007, CBT_OFFICE2010,
        CBT_OFFICE2013, CBT_OFFICE2016, CBT_VS2012LIGHT, CBT_VS2012DARK,
        CBT_WIN11LIGHT, CBT_WIN11DARK, CBT_SLATEDARK
    };
    if (index < 0 || index > 10) return;
    CB_SetTheme(g_cb, kTheme[index]);

    /* keep the radio marks honest */
    for (i = 0; i < 11; ++i)
    {
        mi = CB_FindItem(g_cb, CMD_THEME + i);
        if (mi) CB_SetItemChecked(g_cb, mi, i == index ? 1 : 0);
    }
    n = 0; (void)n;
    sprintf(g_last, "Theme: %s", kThemeName[index]);
}

static void PumpEvents(HWND hwnd)
{
    int item = 0, ev = 0;
    long cmd = 0, param = 0;
    char buf[128];

    while (CB_PollEvent(g_cb, &item, &cmd, &ev, &param))
    {
        switch (ev)
        {
        case CBE_COMMAND:
            if (cmd >= CMD_THEME && cmd <= CMD_THEME + 10)
            {
                SetTheme((int)(cmd - CMD_THEME));
                break;
            }
            if (cmd == CMD_EXIT) { PostMessage(hwnd, WM_CLOSE, 0, 0); break; }
            if (cmd == CMD_SIDE)
                CB_SetBarVisible(g_cb, g_barSide,
                                 !CB_GetBarVisible(g_cb, g_barSide));
            if (cmd == CMD_FLOAT) CB_FloatBar(g_cb, g_barFmt, 320, 320);
            sprintf(g_last, "COMMAND  cmd=%ld  item=%d", cmd, item);
            break;
        case CBE_TOGGLED:
            sprintf(g_last, "TOGGLED  cmd=%ld  now=%s", cmd, param ? "on" : "off");
            break;
        case CBE_TEXTCHANGED:
            CB_GetItemValue(g_cb, item, buf, sizeof(buf));
            sprintf(g_last, "TEXT     cmd=%ld  \"%s\"", cmd, buf);
            break;
        case CBE_SELCHANGED:
            CB_GetItemValue(g_cb, item, buf, sizeof(buf));
            sprintf(g_last, "SELECT   cmd=%ld  [%ld] \"%s\"", cmd, param, buf);
            break;
        case CBE_COLORCHANGED:
            sprintf(g_last, "COLOUR   cmd=%ld  %06lX", cmd, param & 0xFFFFFF);
            break;
        case CBE_RCLICK:
            sprintf(g_last, "RCLICK   cmd=%ld  item=%d", cmd, item);
            break;
        case CBE_LAYOUT:
            break;
        default:
            break;
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        BuildBars(hwnd);
        SetTimer(hwnd, 1, 40, NULL);
        return 0;

    case WM_TIMER:
        PumpEvents(hwnd);
        return 0;

    case WM_KEYDOWN:
    {
        int mods = 0;
        if (GetKeyState(VK_CONTROL) < 0) mods |= CBK_CTRL;
        if (GetKeyState(VK_SHIFT)   < 0) mods |= CBK_SHIFT;
        if (GetKeyState(VK_MENU)    < 0) mods |= CBK_ALT;
        if (CB_TranslateKey(g_cb, (int)wp, mods)) { PumpEvents(hwnd); return 0; }
        break;
    }
    case WM_SYSKEYDOWN:
    {
        if (CB_TranslateKey(g_cb, (int)wp, CBK_ALT)) { PumpEvents(hwnd); return 0; }
        break;
    }

    case WM_RBUTTONUP:
    {
        /* context menu, built on the spot */
        static int ctx = 0;
        POINT p;
        GetCursorPos(&p);
        if (!ctx)
        {
            ctx = CB_CreateMenu(g_cb);
            CB_AddItem(g_cb, ctx, CBI_BUTTON, CMD_CUT,   "Cu&t",   0);
            CB_AddItem(g_cb, ctx, CBI_BUTTON, CMD_COPY,  "&Copy",  0);
            CB_AddItem(g_cb, ctx, CBI_BUTTON, CMD_PASTE, "&Paste", 0);
            CB_AddItem(g_cb, ctx, CBI_SEPARATOR, 0, "", 0);
            CB_AddItem(g_cb, ctx, CBI_BUTTON, CMD_ABOUT, "&About", 0);
        }
        CB_TrackMenu(g_cb, ctx, p.x, p.y);
        PumpEvents(hwnd);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        int x = 0, y = 0, w = 0, h = 0;
        RECT r;
        HFONT f = CreateFont(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                             0, 0, CLEARTYPE_QUALITY, 0, TEXT("Segoe UI"));
        HFONT old = (HFONT)SelectObject(dc, f);

        CB_GetClientRect(g_cb, &x, &y, &w, &h);
        r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
        FillRect(dc, &r, (HBRUSH)(COLOR_WINDOW + 1));

        SetBkMode(dc, TRANSPARENT);
        r.left += 16; r.top += 16;
        DrawTextA(dc, g_last, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE);
        r.top += 26;
        DrawTextA(dc,
                  "The white area is what CB_GetClientRect() reports - the bars "
                  "took the rest.\r\n"
                  "Try: the menu bar (slide across it while open), the Open "
                  "split button,\r\n"
                  "the colour arrow, the Find box, both combos, right-click "
                  "here, Ctrl+B,\r\n"
                  "Alt+F, and View > Theme to see all eleven palettes.",
                  -1, &r, DT_LEFT | DT_TOP | DT_EXPANDTABS);

        SelectObject(dc, old);
        DeleteObject(f);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        CB_Destroy(g_cb);
        g_cb = NULL;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;
    (void)hPrev;

    if (cmd && *cmd) g_startTheme = atoi(cmd);

    if (!CB_Initialize())
    {
        MessageBoxA(NULL, "CB_Initialize failed - Direct2D is not available.",
                    "ClaCommandBar", MB_ICONERROR);
        return 1;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("ClaCommandBarTestHost");
    RegisterClass(&wc);

    hwnd = CreateWindow(TEXT("ClaCommandBarTestHost"),
                        TEXT("ClaCommandBar - test host"),
                        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                        980, 620, NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    CB_Shutdown();
    return 0;
}
