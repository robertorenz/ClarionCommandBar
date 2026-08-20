/*=====================================================================
  commandbar.cpp  -  ClaCommandBar public API, manager lifetime and the
                     layout arithmetic.

  Drawing, window procedures and menu tracking live in cb_render.cpp.
  =====================================================================*/
#include "cb_internal.h"
#include <stdarg.h>
#include <stdio.h>

/*=====================================================================
  DllMain
  =====================================================================*/
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_inst = hInst;
        DisableThreadLibraryCalls(hInst);
    }
    return TRUE;
}

/* The manager is hung off the host window (and off each host child we
   hook) with this property, so any window procedure can find it. */
static const wchar_t* CBPROP = L"ClaCommandBar.Mgr";

/*  Diagnostic trace, off unless CB_HOSTLOG=1 is in the environment.
    Defined with the host-space reservation further down; declared here
    because the parent subclass wants it too. */
static void CBHostLog(const char* fmt, ...);

/*=====================================================================
  Lookups and the event queue
  =====================================================================*/
CBItem* CBFindItem(CBManager* m, int id)
{
    if (!m || id <= 0) return NULL;
    std::map<int, CBItem*>::iterator i = m->items.find(id);
    return (i == m->items.end()) ? NULL : i->second;
}

CBContainer* CBFindContainer(CBManager* m, int id)
{
    if (!m || id <= 0) return NULL;
    std::map<int, CBContainer*>::iterator i = m->containers.find(id);
    return (i == m->containers.end()) ? NULL : i->second;
}

void CBQueue(CBManager* m, int item, long cmd, int type, long param)
{
    if (!m || m->destroying) return;

    if (m->suppressContainer)
    {
        CBItem* it = CBFindItem(m, item);
        if (it && it->container == m->suppressContainer) return;
    }

    /* Building a bar re-lays out after every single AddItem, so a host
       that just wants "the client area moved" would otherwise get a
       burst of identical events.  One pending is enough. */
    if (type == CBE_LAYOUT && !m->events.empty() &&
        m->events.back().type == CBE_LAYOUT)
        return;

    CBEvent e;
    e.item  = item;
    e.cmd   = cmd;
    e.type  = type;
    e.param = param;

    /* A poll queue that nobody drains must not grow without bound. */
    if (m->events.size() >= 512) m->events.pop_front();
    m->events.push_back(e);

    if (m->proc) m->proc(m->procUser, item, cmd, type, param);
}

/*=====================================================================
  Metrics - stored as design pixels at 96 DPI, returned scaled.
  =====================================================================*/
int CBMetric(CBManager* m, int metric)
{
    if (!m || metric < 1 || metric > CBM_ROWGAP) return 0;
    int v = m->metric[metric];
    if (!v) return 0;
    int s = (int)(v * m->dpiScale + 0.5f);
    return s < 1 ? 1 : s;
}

/* The uniform height of a normal (not text-below) item on this bar. */
static int BarItemHeight(CBManager* m, CBContainer* c)
{
    int fixed = CBMetric(m, CBM_ITEMHEIGHT);
    if (fixed > 0) return fixed;
    int icon = (c->style & CBBS_LARGEICONS) ? CBMetric(m, CBM_LARGEICON)
                                            : CBMetric(m, CBM_ICONSIZE);
    int fh = CBFontHeight(m, CBF_ITEM);
    int h  = (fh > icon ? fh : icon) + 2 * CBMetric(m, CBM_PADY);
    int min = (int)(22 * m->dpiScale);
    return h < min ? min : h;
}

/*=====================================================================
  Measuring one item
  =====================================================================*/
void CBMeasureItem(CBManager* m, CBContainer* c, CBItem* it, int* pw, int* ph)
{
    const int padX = CBMetric(m, CBM_PADX);
    const int padY = CBMetric(m, CBM_PADY);
    const int icon = (c->style & CBBS_LARGEICONS) ? CBMetric(m, CBM_LARGEICON)
                                                  : CBMetric(m, CBM_ICONSIZE);
    const int baseH = BarItemHeight(m, c);
    const int arrowW = (int)(13 * m->dpiScale);

    int w = 0, h = baseH;

    bool wantIcon = it->image > 0 && !(it->style & CBIS_TEXTONLY);
    bool wantText = !it->text.empty() && !(it->style & CBIS_ICONONLY);

    float tw = 0, th = 0;
    if (wantText) CBMeasure(m, CBF_ITEM, false, it->text, &tw, &th);

    /* DirectWrite reports a FRACTIONAL width; the draw rect is whole
       pixels.  Truncating here leaves the layout a fraction short of
       what it needs and the ellipsis trimmer eats the last character -
       which is why this rounds UP and then adds a pixel of slack. */
    int textW = wantText ? (int)(tw + 0.5f) + 2 : 0;

    switch (it->type)
    {
    case CBI_SEPARATOR:
        w = CBMetric(m, CBM_SEPWIDTH);
        break;

    case CBI_SPACE:
        w = it->width > 0 ? (int)(it->width * m->dpiScale) : 0;
        break;

    case CBI_LABEL:
        w = it->width > 0 ? (int)(it->width * m->dpiScale) : textW + padX;
        break;

    case CBI_EDIT:
        w = it->width > 0 ? (int)(it->width * m->dpiScale) : (int)(110 * m->dpiScale);
        break;

    case CBI_COMBO:
        w = it->width > 0 ? (int)(it->width * m->dpiScale) : (int)(130 * m->dpiScale);
        break;

    case CBI_CHECKBOX:
        w = 2 * padX + (int)(13 * m->dpiScale);
        if (wantText) w += (int)(5 * m->dpiScale) + textW;
        break;

    default:
        if (it->style & CBIS_TEXTBELOW)
        {
            int iw = wantIcon ? icon : 0;
            w = (iw > textW ? iw : textW) + 2 * padX;
            h = 2 * padY + (wantIcon ? icon + 2 : 0)
                         + (wantText ? (int)(th + 0.5f) : 0);
            if (h < baseH) h = baseH;
        }
        else
        {
            w = 2 * padX;
            if (wantIcon) w += icon;
            if (it->type == CBI_COLOR && !wantIcon) w += icon;
            if (wantText)
            {
                if (wantIcon || it->type == CBI_COLOR) w += (int)(5 * m->dpiScale);
                w += textW;
            }
            /* A menu title wants room to breathe on both sides - a menu
               bar reading "File Edit View" packed tight looks wrong. */
            if (it->type == CBI_MENU) w += 2 * padX;
        }
        if (it->type == CBI_DROPDOWN)                       w += arrowW - padX / 2;
        if (it->type == CBI_SPLIT || it->type == CBI_COLOR) w += (int)(16 * m->dpiScale);
        break;
    }

    int minW = (int)(8 * m->dpiScale);
    if (w < minW && it->type != CBI_SPACE) w = minW;

    if (pw) *pw = w;
    if (ph) *ph = h;
}

/*=====================================================================
  Laying out one bar

  availW / availH are what the dock gave us.  On exit measW / measH
  hold the size the bar actually wants and every visible item carries
  its rect.
  =====================================================================*/
static void LayoutBarVertical(CBManager* m, CBContainer* c, int availH)
{
    const int padX = CBMetric(m, CBM_BARPADX);
    const int padY = CBMetric(m, CBM_BARPADY);
    const int gap  = CBMetric(m, CBM_GAP);
    const int grip = (c->style & CBBS_GRIPPER) ? (int)(8 * m->dpiScale) : 0;

    int maxW = 0;
    std::vector<int> hs;
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        /* hs is indexed by position, so it must stay in step with
           c->items even for an id that no longer resolves. */
        if (!it) { hs.push_back(0); continue; }
        it->overflow = false;
        SetRectEmpty(&it->rc);
        SetRectEmpty(&it->arrow);
        if (!it->visible) { hs.push_back(0); continue; }
        int w = 0, h = 0;
        CBMeasureItem(m, c, it, &w, &h);
        if (it->type == CBI_SEPARATOR) h = CBMetric(m, CBM_SEPWIDTH);
        hs.push_back(h);
        if (w > maxW) maxW = w;
    }

    int barW = maxW + 2 * padX;
    int y = padY + grip;
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it || !it->visible) continue;
        int h = hs[i];
        it->rc.left   = padX;
        it->rc.right  = barW - padX;
        it->rc.top    = y;
        it->rc.bottom = y + h;
        it->row       = 0;
        if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
        {
            it->arrow = it->rc;
            it->arrow.left = it->rc.right - (int)(16 * m->dpiScale);
        }
        y += h + gap;
    }

    c->laid     = c->items;
    c->measW    = barW;
    c->measH    = y - gap + padY;
    c->rowCount = 1;
    c->hasChevron = false;
    if (c->measH < availH) c->measH = c->measH;   /* vertical bars never stretch */
}

void CBLayoutBar(CBManager* m, CBContainer* c, int availW, int availH)
{
    if (c->style & CBBS_RIBBON)
    {
        CBLayoutRibbon(m, c, availW);
        return;
    }
    if (c->dock == CBD_LEFT || c->dock == CBD_RIGHT)
    {
        LayoutBarVertical(m, c, availH);
        return;
    }
    c->laid = c->items;

    const int padX   = CBMetric(m, CBM_BARPADX);
    const int padY   = CBMetric(m, CBM_BARPADY);
    const int gap    = CBMetric(m, CBM_GAP);
    const int rowGap = CBMetric(m, CBM_ROWGAP);
    const int grip   = (c->style & CBBS_GRIPPER) ? (int)(8 * m->dpiScale) : 0;
    const int capH   = (c->dock == CBD_FLOAT) ? (int)(17 * m->dpiScale) : 0;
    const int chevW  = (int)(14 * m->dpiScale);

    const int x0 = padX + grip;

    /* ---- measure ---- */
    std::vector<int> ws, hs;
    ws.resize(c->items.size(), 0);
    hs.resize(c->items.size(), 0);
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it) continue;
        it->overflow = false;
        SetRectEmpty(&it->rc);
        SetRectEmpty(&it->arrow);
        if (!it->visible) continue;
        CBMeasureItem(m, c, it, &ws[i], &hs[i]);
    }

    /* ---- flow into rows ---- */
    const bool chevronMode = (m->style & CBS_CHEVRON) != 0;
    std::vector<std::vector<size_t> > rows;
    std::vector<size_t> right;
    rows.push_back(std::vector<size_t>());

    int limit = availW - padX;
    bool overflowed = false;

    /* Two attempts: the first without room for a chevron, and if
       anything spilled, a second with the chevron's width reserved. */
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        rows.clear();
        rows.push_back(std::vector<size_t>());
        right.clear();
        overflowed = false;
        limit = availW - padX - (attempt ? chevW : 0);

        int x = x0;
        for (size_t i = 0; i < c->items.size(); ++i)
        {
            CBItem* it = CBFindItem(m, c->items[i]);
            if (!it || !it->visible) continue;
            it->overflow = false;

            if (it->style & CBIS_RIGHTALIGN) { right.push_back(i); continue; }

            int w = ws[i];
            bool fits = (x + w <= limit) || rows.back().empty();

            if (!fits)
            {
                if (chevronMode)
                {
                    it->overflow = true;
                    overflowed = true;
                    continue;
                }
                rows.push_back(std::vector<size_t>());
                x = x0;
            }
            rows.back().push_back(i);
            x += w + gap;

            if (it->style & CBIS_WRAP)
            {
                rows.push_back(std::vector<size_t>());
                x = x0;
            }
        }
        if (!(chevronMode && overflowed)) break;
        if (attempt == 1) break;
    }

    c->hasChevron = chevronMode && overflowed;

    /* ---- assign rectangles ---- */
    int y = padY + capH;
    int maxRight = 0;

    for (size_t r = 0; r < rows.size(); ++r)
    {
        if (rows[r].empty() && rows.size() > 1) continue;

        int rowH = 0;
        int used = 0;
        std::vector<size_t> flex;
        for (size_t k = 0; k < rows[r].size(); ++k)
        {
            size_t i = rows[r][k];
            CBItem* it = CBFindItem(m, c->items[i]);
            if (hs[i] > rowH) rowH = hs[i];
            used += ws[i] + gap;
            if ((it->type == CBI_SPACE && it->width <= 0) ||
                (it->style & CBIS_STRETCH))
                flex.push_back(i);
        }
        if (rowH == 0) rowH = BarItemHeight(m, c);

        /* Right-aligned items ride on the last row. */
        int rightW = 0;
        if (r + 1 == rows.size())
        {
            for (size_t k = 0; k < right.size(); ++k)
            {
                rightW += ws[right[k]] + gap;
                if (hs[right[k]] > rowH) rowH = hs[right[k]];
            }
        }

        /* Flexible items eat whatever is left over. */
        if (!flex.empty())
        {
            int leftover = limit - x0 - (used - gap) - rightW;
            if (leftover > 0)
            {
                int each = leftover / (int)flex.size();
                for (size_t k = 0; k < flex.size(); ++k) ws[flex[k]] += each;
            }
        }

        int x = x0;
        for (size_t k = 0; k < rows[r].size(); ++k)
        {
            size_t i = rows[r][k];
            CBItem* it = CBFindItem(m, c->items[i]);
            it->row = (int)r;
            it->rc.left   = x;
            it->rc.right  = x + ws[i];
            if (it->style & CBIS_TEXTBELOW)
            {
                it->rc.top    = y;
                it->rc.bottom = y + rowH;
            }
            else
            {
                int h = hs[i] > rowH ? rowH : hs[i];
                it->rc.top    = y + (rowH - h) / 2;
                it->rc.bottom = it->rc.top + h;
            }
            if (it->type == CBI_SEPARATOR)
            {
                it->rc.top    = y + 2;
                it->rc.bottom = y + rowH - 2;
            }
            if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
            {
                it->arrow = it->rc;
                it->arrow.left = it->rc.right - (int)(16 * m->dpiScale);
            }
            x += ws[i] + gap;
            if (x > maxRight) maxRight = x;
        }

        if (r + 1 == rows.size() && !right.empty())
        {
            int rx = availW - padX - (c->hasChevron ? chevW : 0);
            for (size_t k = right.size(); k-- > 0; )
            {
                size_t i = right[k];
                CBItem* it = CBFindItem(m, c->items[i]);
                it->row = (int)r;
                it->rc.right  = rx;
                it->rc.left   = rx - ws[i];
                int h = hs[i] > rowH ? rowH : hs[i];
                it->rc.top    = y + (rowH - h) / 2;
                it->rc.bottom = it->rc.top + h;
                if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
                {
                    it->arrow = it->rc;
                    it->arrow.left = it->rc.right - (int)(16 * m->dpiScale);
                }
                rx -= ws[i] + gap;
            }
        }

        y += rowH + rowGap;
    }

    c->rowCount = (int)rows.size();
    c->measH    = y - rowGap + padY;
    c->measW    = (maxRight + padX > availW) ? availW : maxRight + padX;
    if (c->measW < (int)(40 * m->dpiScale)) c->measW = (int)(40 * m->dpiScale);

    if (c->hasChevron)
    {
        c->chevronRc.right  = availW - padX / 2;
        c->chevronRc.left   = c->chevronRc.right - chevW;
        c->chevronRc.top    = padY + capH;
        c->chevronRc.bottom = c->measH - padY;
        if (!c->chevronMenu)
        {
            c->chevronMenu = CB_CreateMenu(m);
            CBContainer* cm = CBFindContainer(m, c->chevronMenu);
            if (cm) cm->ownerItem = 0;
        }
    }
    else
        SetRectEmpty(&c->chevronRc);
}

/*=====================================================================
  Laying out a popup menu
  =====================================================================*/
void CBLayoutMenu(CBManager* m, CBContainer* c)
{
    const int icon    = CBMetric(m, CBM_ICONSIZE);
    const int border  = 1;
    const int padTop  = (int)(3 * m->dpiScale);
    const int gutter  = (m->style & CBS_MENUICONS) ? icon + (int)(8 * m->dpiScale)
                                                   : (int)(6 * m->dpiScale);
    const int fh      = CBFontHeight(m, CBF_MENU);

    int rowH = fh + (int)(8 * m->dpiScale);
    if (rowH < icon + (int)(6 * m->dpiScale)) rowH = icon + (int)(6 * m->dpiScale);
    const int sepH = (int)(7 * m->dpiScale);

    float maxText = 0, maxShort = 0;
    bool  anySub  = false;
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it || !it->visible) continue;
        float w = 0;
        /* The bold twin of a DEFAULT row is wider than the normal one -
           measure what will actually be drawn, or it gets trimmed. */
        CBMeasure(m, CBF_MENU, (it->style & CBIS_DEFAULT) != 0, it->text, &w, NULL);
        if (w > maxText) maxText = w;
        if (!it->shortcut.empty())
        {
            CBMeasure(m, CBF_MENU, false, it->shortcut, &w, NULL);
            if (w > maxShort) maxShort = w;
        }
        if (it->menu) anySub = true;
    }

    c->gutterW   = gutter;
    c->shortcutW = maxShort > 0 ? (int)(maxShort + 0.5f) + 2 : 0;

    int w = gutter + (int)(6 * m->dpiScale) + (int)(maxText + 0.5f) + 2
          + (c->shortcutW > 0 ? (int)(24 * m->dpiScale) + c->shortcutW : 0)
          + (anySub ? (int)(16 * m->dpiScale) : (int)(10 * m->dpiScale))
          + 2 * border;
    int minW = CBMetric(m, CBM_MENUWIDTH);
    if (w < minW) w = minW;

    int y = padTop;
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it) continue;
        SetRectEmpty(&it->rc);
        SetRectEmpty(&it->arrow);
        if (!it->visible) continue;
        int h = (it->type == CBI_SEPARATOR) ? sepH : rowH;
        it->rc.left   = border;
        it->rc.right  = w - border;
        it->rc.top    = y;
        it->rc.bottom = y + h;
        y += h;
    }

    c->measW = w;
    c->measH = y + padTop;
}

/*=====================================================================
  Hit testing a bar
  =====================================================================*/
int CBItemHitTest(CBManager* m, CBContainer* c, POINT pt, int* zone)
{
    if (zone) *zone = CBHIT_NONE;
    if (!c) return 0;

    if (c->hasChevron && PtInRect(&c->chevronRc, pt))
    {
        if (zone) *zone = CBHIT_CHEVRON;
        return -1;
    }
    for (size_t i = 0; i < c->laid.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->laid[i]);
        if (!it || !it->visible || it->overflow) continue;
        if (it->type == CBI_SEPARATOR || it->type == CBI_SPACE) continue;
        if (!PtInRect(&it->rc, pt)) continue;

        int z = CBHIT_ITEM;
        if ((it->type == CBI_SPLIT || it->type == CBI_COLOR) &&
            !IsRectEmpty(&it->arrow) && pt.x >= it->arrow.left)
            z = CBHIT_ARROW;
        if (zone) *zone = z;
        return it->id;
    }
    return 0;
}

/*=====================================================================
  Bar windows and the dock layout
  =====================================================================*/
void CBEnsureBarWindow(CBManager* m, CBContainer* c)
{
    if (c->kind != CBK_BAR || c->hwnd) return;

    DWORD style   = WS_CLIPSIBLINGS;
    DWORD exStyle = 0;
    if (c->dock == CBD_FLOAT)
    {
        style   |= WS_POPUP;
        exStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    }
    else style |= WS_CHILD;

    c->hwnd = CreateWindowExW(exStyle, CBWC_BAR, L"", style,
                              0, 0, 10, 10,
                              (c->dock == CBD_FLOAT) ? m->parent : m->parent,
                              NULL, g_inst, NULL);
    if (!c->hwnd) return;
    SetWindowLongPtrW(c->hwnd, GWLP_USERDATA, (LONG_PTR)c);
}

/* Bars on one side, grouped and ordered the way the caller docked them. */
struct DockRowInfo { int row; std::vector<CBContainer*> bars; };

static void CollectSide(CBManager* m, int dock, std::vector<DockRowInfo>* out)
{
    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || !c->visible || c->dock != dock) continue;

        size_t k = 0;
        for (; k < out->size(); ++k) if ((*out)[k].row == c->dockRow) break;
        if (k == out->size())
        {
            DockRowInfo r;
            r.row = c->dockRow;
            out->push_back(r);
        }
        (*out)[k].bars.push_back(c);
    }
    /* rows ascending, then bars within a row by their offset */
    for (size_t a = 0; a + 1 < out->size(); ++a)
        for (size_t b = a + 1; b < out->size(); ++b)
            if ((*out)[b].row < (*out)[a].row)
            {
                DockRowInfo t = (*out)[a]; (*out)[a] = (*out)[b]; (*out)[b] = t;
            }
    for (size_t r = 0; r < out->size(); ++r)
    {
        std::vector<CBContainer*>& v = (*out)[r].bars;
        for (size_t a = 0; a + 1 < v.size(); ++a)
            for (size_t b = a + 1; b < v.size(); ++b)
                if (v[b]->dockOffset < v[a]->dockOffset)
                { CBContainer* t = v[a]; v[a] = v[b]; v[b] = t; }
    }

    /*  A RIBBON owns its row.

        Bars sharing a row are handed out left to right, each taking the
        width it asked for - and a ribbon asks for ALL of it, because a
        ribbon is a full-width band with tabs across it.  Put one on the
        same row as a menu bar and whichever came first swallowed the row:
        with the ribbon first the menu was squeezed to a single pixel and
        simply vanished.

        Rather than let that happen, a ribbon sharing a row is moved to a
        row of its own directly below, keeping the thin bars where they
        were.  Anyone who really did mean row 0 for both gets the only
        arrangement that can show both. */
    for (size_t r = 0; r < out->size(); ++r)
    {
        std::vector<CBContainer*>& v = (*out)[r].bars;
        if (v.size() < 2) continue;

        std::vector<CBContainer*> ribbons;
        for (size_t k = v.size(); k-- > 0; )
            if (v[k]->style & CBBS_RIBBON)
            {
                ribbons.insert(ribbons.begin(), v[k]);
                v.erase(v.begin() + k);
            }
        if (ribbons.empty()) continue;

        if (v.empty())                       /* nothing but ribbons here */
        {
            v.push_back(ribbons[0]);
            ribbons.erase(ribbons.begin());
            if (ribbons.empty()) continue;
        }

        const int rowNo = (*out)[r].row;
        for (size_t q = 0; q < ribbons.size(); ++q)
        {
            DockRowInfo nr;
            nr.row = rowNo;
            nr.bars.push_back(ribbons[q]);
            out->insert(out->begin() + (r + 1 + q), nr);
        }
        r += ribbons.size();
    }
}

/*  Is this bar already exactly where the layout wants it?  Tells a layout
    that changed something from one that just re-ran, and hands back the
    rect being vacated so the host can be told to erase it. */
static bool CBBarWouldMove(CBManager* m, HWND h, int x, int y, int w, int t,
                           RECT* was)
{
    RECT r;
    if (!GetWindowRect(h, &r)) return true;
    POINT tl;
    tl.x = r.left;
    tl.y = r.top;
    ScreenToClient(m->parent, &tl);

    if (was)
    {
        was->left   = tl.x;
        was->top    = tl.y;
        was->right  = tl.x + (r.right - r.left);
        was->bottom = tl.y + (r.bottom - r.top);
    }
    return tl.x != x || tl.y != y ||
           (r.right - r.left) != w || (r.bottom - r.top) != t;
}

/*  Remember where a bar used to be.  Moving a bar leaves its old pixels
    on the host - the host has no reason to repaint a strip it never drew
    in - and a bar that lands in a new dock row next to the old one then
    reads as a DUPLICATE of itself sitting alongside. */
static void CBAddVacated(RECT* dirty, bool* any, const RECT* was)
{
    if (!*any) { *dirty = *was; *any = true; return; }
    if (was->left   < dirty->left)   dirty->left   = was->left;
    if (was->top    < dirty->top)    dirty->top    = was->top;
    if (was->right  > dirty->right)  dirty->right  = was->right;
    if (was->bottom > dirty->bottom) dirty->bottom = was->bottom;
}

void CBRelayout(CBManager* m)
{
    if (!m || m->inLayout || m->destroying || !IsWindow(m->parent)) return;
    m->inLayout = true;

    RECT pc;
    GetClientRect(m->parent, &pc);

    /*  The host keeps a strip along the bottom for its status bar and
        paints it itself - there is no window there to find.  Bars must
        stop above it, or a left, right or bottom bar covers it. */
    const int hostB = (m->userResB >= 0) ? m->userResB : m->hostResB;

    int left = 0, top = 0, right = pc.right;
    int bottom = pc.bottom - hostB;
    if (bottom <= top) bottom = pc.bottom;

    bool moved = false;         /* did any bar window really move or resize? */
    RECT vacated;               /* where bars used to be, to be erased     */
    bool anyVacated = false;
    SetRectEmpty(&vacated);

    HDWP dwp = BeginDeferWindowPos(8);

    for (int pass = 0; pass < 4; ++pass)
    {
        int dock = (pass == 0) ? CBD_TOP : (pass == 1) ? CBD_BOTTOM
                 : (pass == 2) ? CBD_LEFT : CBD_RIGHT;
        bool horizontal = (dock == CBD_TOP || dock == CBD_BOTTOM);

        std::vector<DockRowInfo> rows;
        CollectSide(m, dock, &rows);

        for (size_t r = 0; r < rows.size(); ++r)
        {
            std::vector<CBContainer*>& bars = rows[r].bars;
            if (bars.empty()) continue;

            int availW = horizontal ? (right - left) : 0;
            int availH = horizontal ? 0 : (bottom - top);
            if (horizontal && availW < 1) availW = 1;
            if (!horizontal && availH < 1) availH = 1;

            /* pass 1: what does each bar want? */
            int stripSize = 0;
            for (size_t b = 0; b < bars.size(); ++b)
            {
                CBEnsureBarWindow(m, bars[b]);
                CBLayoutBar(m, bars[b], horizontal ? availW : 0,
                                        horizontal ? 0 : availH);
                int want = horizontal ? bars[b]->measH : bars[b]->measW;
                if (want > stripSize) stripSize = want;
            }
            if (stripSize < 1) continue;

            /* pass 2: hand out the strip and re-flow at the real width */
            if (horizontal)
            {
                int x = left;
                for (size_t b = 0; b < bars.size(); ++b)
                {
                    bool last = (b + 1 == bars.size());
                    int w = last ? (right - x) : bars[b]->measW;
                    if (w < 1) w = 1;
                    if (x + w > right) w = right - x;
                    if (w < 1) w = 1;

                    CBLayoutBar(m, bars[b], w, stripSize);
                    int y = (dock == CBD_TOP) ? top : (bottom - stripSize);
                    /* HWND_TOP, not SWP_NOZORDER: a Clarion window puts a
                       full-size 'ClaChildClient' over its whole client
                       area, and our bars are its SIBLINGS.  Left where
                       they were created they sit underneath it and never
                       show.  Re-asserted on every layout because the host
                       may reshuffle z-order on a resize. */
                    RECT was;
                    if (CBBarWouldMove(m, bars[b]->hwnd, x, y, w, stripSize, &was))
                    {
                        moved = true;
                        CBAddVacated(&vacated, &anyVacated, &was);
                    }
                    dwp = DeferWindowPos(dwp, bars[b]->hwnd, HWND_TOP, x, y, w,
                                         stripSize,
                                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    x += w;
                }
                if (dock == CBD_TOP) top += stripSize;
                else                 bottom -= stripSize;
            }
            else
            {
                int y = top;
                for (size_t b = 0; b < bars.size(); ++b)
                {
                    bool last = (b + 1 == bars.size());
                    int h = last ? (bottom - y) : bars[b]->measH;
                    if (h < 1) h = 1;
                    if (y + h > bottom) h = bottom - y;
                    if (h < 1) h = 1;

                    CBLayoutBar(m, bars[b], stripSize, h);
                    int x = (dock == CBD_LEFT) ? left : (right - stripSize);
                    RECT was;
                    if (CBBarWouldMove(m, bars[b]->hwnd, x, y, stripSize, h, &was))
                    {
                        moved = true;
                        CBAddVacated(&vacated, &anyVacated, &was);
                    }
                    dwp = DeferWindowPos(dwp, bars[b]->hwnd, HWND_TOP, x, y,
                                         stripSize, h,
                                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    y += h;
                }
                if (dock == CBD_LEFT) left += stripSize;
                else                  right -= stripSize;
            }
        }
    }
    if (dwp) EndDeferWindowPos(dwp);

    /* floating bars sit outside the dock arithmetic */
    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || c->dock != CBD_FLOAT) continue;
        if (!c->visible)
        {
            if (c->hwnd) ShowWindow(c->hwnd, SW_HIDE);
            continue;
        }
        CBEnsureBarWindow(m, c);
        CBLayoutBar(m, c, 10000, 0);
        int w = c->measW, h = c->measH;
        RECT wasF;
        if (CBBarWouldMove(m, c->hwnd, c->floatX, c->floatY, w, h, &wasF))
        {
            moved = true;
            CBAddVacated(&vacated, &anyVacated, &wasF);
        }
        SetWindowPos(c->hwnd, HWND_TOP, c->floatX, c->floatY, w, h,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    /* Bars the caller placed itself.  They are deliberately OUTSIDE the
       dock arithmetic above, so they take nothing off the client rect -
       that is what lets the control template drop a bar exactly onto a
       REGION the developer positioned on the window. */
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || c->dock != CBD_FIXED) continue;
        if (!c->visible)
        {
            if (c->hwnd) ShowWindow(c->hwnd, SW_HIDE);
            continue;
        }
        CBEnsureBarWindow(m, c);
        int fw = c->fixedRc.right - c->fixedRc.left;
        int fh = c->fixedRc.bottom - c->fixedRc.top;
        if (fw < 1) fw = 1;
        if (fh < 1) fh = 1;
        CBLayoutBar(m, c, fw, fh);
        if (c->measH > fh) fh = c->measH;      /* never clip a ribbon */
        RECT wasX;
        if (CBBarWouldMove(m, c->hwnd, c->fixedRc.left, c->fixedRc.top, fw, fh, &wasX))
        {
            moved = true;
            CBAddVacated(&vacated, &anyVacated, &wasX);
        }
        SetWindowPos(c->hwnd, HWND_TOP, c->fixedRc.left, c->fixedRc.top,
                     fw, fh, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    /* hidden docked bars */
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || c->dock == CBD_FLOAT ||
            c->dock == CBD_FIXED) continue;
        if (!c->visible && c->hwnd) ShowWindow(c->hwnd, SW_HIDE);
    }

    RECT nc;
    nc.left = left; nc.top = top; nc.right = right; nc.bottom = bottom;
    bool changed = !EqualRect(&nc, &m->clientRc);
    m->clientRc = nc;
    m->insL = nc.left;
    m->insT = nc.top;
    m->insR = pc.right - nc.right;
    /*  Only what the BARS took, not the host's own strip - the transform
        that moves the host's children adds that back on its own. */
    m->insB = (pc.bottom - hostB) - nc.bottom;
    if (m->insB < 0) m->insB = 0;

    /* The host lays its own children out against the FULL client area
       and never reads clientRc, so on a frame they have to be pushed
       out of the strips the bars just took. */
    CBReserveFromHost(m);

    /*  Repaint the bars only when this layout actually did something.
        A layout runs several times over an MDI child opening or closing -
        the host shuffles its own toolbar and client around and each nudge
        brings us back through here - and repainting every bar each time
        is a visible flash with nothing behind it.  Moving a window
        repaints it by itself, so the only case left for an explicit
        invalidate is a bar that stayed put while the layout around it
        changed. */
    if (changed || moved)
        for (i = m->containers.begin(); i != m->containers.end(); ++i)
            if (i->second->kind == CBK_BAR && i->second->hwnd)
                InvalidateRect(i->second->hwnd, NULL, FALSE);

    /*  Erase whatever the bars uncovered.  WS_CLIPCHILDREN is on the host,
        so this repaints only the parts no bar is sitting on - the strip a
        bar just left, and nothing underneath the bars themselves. */
    if (anyVacated)
    {
        InvalidateRect(m->parent, &vacated, TRUE);

        /*  The host's own children are SIBLINGS of the bars, so the
            host repainting itself does not touch them - whatever a bar
            drew over one of them stays there.  Repaint the ones the
            bars just uncovered. */
        for (size_t hk = 0; hk < m->hostKids.size(); ++hk)
        {
            HWND kh = m->hostKids[hk].hwnd;
            if (!IsWindow(kh) || !IsWindowVisible(kh)) continue;
            RECT kr, hit;
            GetWindowRect(kh, &kr);
            POINT tl;
            tl.x = kr.left;
            tl.y = kr.top;
            ScreenToClient(m->parent, &tl);
            RECT kc;
            kc.left   = tl.x;
            kc.top    = tl.y;
            kc.right  = tl.x + (kr.right - kr.left);
            kc.bottom = tl.y + (kr.bottom - kr.top);
            if (IntersectRect(&hit, &kc, &vacated))
                InvalidateRect(kh, NULL, TRUE);
        }
        UpdateWindow(m->parent);
    }

    m->inLayout = false;
    if (changed) CBQueue(m, 0, 0, CBE_LAYOUT, 0);
}

/*=====================================================================
  Parent subclass - keeps the bars right even when the host forgets to
  call CB_Layout after a resize.
  =====================================================================*/
/* Clarion puts its menu back.  CB_SetHostMenuVisible(0) detaches it and
   really does take effect - GetMenu() answers NULL straight afterwards -
   but the runtime re-attaches it while the window finishes opening, and
   the menu bar reappears above our mirrored one.  So the detach has to
   be RE-ASSERTED: any time the frame is sized, activated or repainted
   and the menu is back while we are meant to be holding it, take it off
   again.  Cheap - GetMenu() is a lookup, and this only fires while a
   mirrored bar is actually replacing the menu. */
static void CBEnforceHostMenu(CBManager* m, HWND hwnd)
{
    if (!m->hostMenu || m->destroying) return;
    if (!GetMenu(hwnd)) return;
    CBHostLog("MENUOFF  Clarion put its menu back - taking it off again\r\n");
    SetMenu(hwnd, NULL);
    DrawMenuBar(hwnd);
    CBRelayout(m);
}

static LRESULT CALLBACK CBParentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CBManager* m = (CBManager*)GetPropW(hwnd, CBPROP);
    if (!m || !m->oldParentProc) return DefWindowProcW(hwnd, msg, wp, lp);
    WNDPROC old = m->oldParentProc;

    switch (msg)
    {
    case WM_NCPAINT:
    case WM_NCACTIVATE:
    case WM_ACTIVATE:
    case WM_SETFOCUS:
    {
        LRESULT r = CallWindowProcW(old, hwnd, msg, wp, lp);
        CBEnforceHostMenu(m, hwnd);
        return r;
    }
    case WM_SIZE:
    {
        LRESULT r = CallWindowProcW(old, hwnd, msg, wp, lp);
        CBEnforceHostMenu(m, hwnd);
        CBRelayout(m);
        return r;
    }
    case WM_DPICHANGED:
    {
        LRESULT r = CallWindowProcW(old, hwnd, msg, wp, lp);
        m->dpiScale = (float)LOWORD(wp) / 96.0f;
        CBReleaseFonts(m);
        CBRelayout(m);
        return r;
    }
    case WM_PARENTNOTIFY:
    {
        /*  The host just created or destroyed a child of its own.  On an
            MDI frame that is exactly what a merge does: opening a child
            window makes Clarion build a NEW ClaToolBar for the merged
            toolbar and hide the old one - and the new one starts at the
            top of the client area, which is underneath the bars.  Left
            alone it stays there and reads as a toolbar that vanished
            the moment a procedure opened.

            Posted rather than handled here: the host is still in the
            middle of building the window. */
        LRESULT r = CallWindowProcW(old, hwnd, msg, wp, lp);
        const UINT what = LOWORD(wp);
        if (what == WM_CREATE || what == WM_DESTROY)
            PostMessageW(hwnd, CBMSG_HOSTKIDS, 0, 0);
        return r;
    }
    case CBMSG_HOSTKIDS:
        CBReserveFromHost(m);
        return 0;

    case CBMSG_RELAYOUT:
        CBRelayout(m);
        return 0;

    case CBMSG_APPLYDRAG:
        CBApplyDrag(m);
        return 0;

    case WM_NCDESTROY:
    {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)old);
        RemovePropW(hwnd, CBPROP);
        m->oldParentProc = NULL;
        return CallWindowProcW(old, hwnd, msg, wp, lp);
    }
    }
    return CallWindowProcW(old, hwnd, msg, wp, lp);
}

/*=====================================================================
  Lifetime
  =====================================================================*/
static bool  g_comInit = false;

int CBAPI CB_Initialize(void)
{
    if (InterlockedIncrement(&g_initCount) > 1) return 1;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    g_comInit = SUCCEEDED(hr);          /* RPC_E_CHANGED_MODE: already up */

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2d)) || !g_d2d)
    {
        InterlockedDecrement(&g_initCount);
        return 0;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   (IUnknown**)&g_dw)) || !g_dw)
    {
        g_d2d->Release();
        g_d2d = NULL;
        InterlockedDecrement(&g_initCount);
        return 0;
    }
    /* WIC is only needed for images - a missing one is not fatal. */
    CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(&g_wic));

    if (!g_inst) g_inst = GetModuleHandleW(NULL);
    CBRegisterClasses();
    return 1;
}

void CBAPI CB_Shutdown(void)
{
    if (InterlockedDecrement(&g_initCount) > 0) return;
    if (g_initCount < 0) { g_initCount = 0; return; }

    if (g_wic) { g_wic->Release(); g_wic = NULL; }
    if (g_dw)  { g_dw->Release();  g_dw  = NULL; }
    if (g_d2d) { g_d2d->Release(); g_d2d = NULL; }
    if (g_comInit) { CoUninitialize(); g_comInit = false; }
}

static float DpiScaleFor(HWND h)
{
    typedef UINT (WINAPI *PFNGDFW)(HWND);
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    if (u32)
    {
        PFNGDFW f = (PFNGDFW)GetProcAddress(u32, "GetDpiForWindow");
        if (f)
        {
            UINT d = f(h);
            if (d >= 72) return (float)d / 96.0f;
        }
    }
    HDC dc = GetDC(h);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(h, dc);
    if (dpi < 72) dpi = 96;
    return (float)dpi / 96.0f;
}

HCB CBAPI CB_Create(HWND hwndParent, unsigned long style)
{
    if (!g_d2d && !CB_Initialize()) return NULL;
    if (!IsWindow(hwndParent)) return NULL;

    CBManager* m = new CBManager();
    m->parent   = hwndParent;
    m->style    = style;
    m->dpiScale = DpiScaleFor(hwndParent);

    m->metric[CBM_ITEMHEIGHT]  = 0;
    m->metric[CBM_ICONSIZE]    = 16;
    m->metric[CBM_LARGEICON]   = 32;
    m->metric[CBM_PADX]        = 6;
    m->metric[CBM_PADY]        = 3;
    m->metric[CBM_GAP]         = 2;
    m->metric[CBM_BARPADX]     = 4;
    m->metric[CBM_BARPADY]     = 2;
    m->metric[CBM_SEPWIDTH]    = 7;
    m->metric[CBM_CORNER]      = 3;
    m->metric[CBM_MENUWIDTH]   = 130;
    m->metric[CBM_GUTTERWIDTH] = 0;
    m->metric[CBM_ROWGAP]      = 2;

    for (int i = 1; i <= 4; ++i)
    {
        m->font[i].face   = L"Segoe UI";
        m->font[i].sizePt = 9;
    }
    m->font[CBF_CAPTION].sizePt = 8;
    m->font[CBF_CAPTION].bold   = 1;

    CBApplyTheme(m, CBT_STEELBLUE);

    m->images.push_back(CBImage());     /* index 0 is never used */

    /* Without WS_CLIPCHILDREN the host erases its background straight
       over our child windows and the bars flicker or vanish. */
    LONG_PTR ps = GetWindowLongPtrW(hwndParent, GWL_STYLE);
    if (!(ps & WS_CLIPCHILDREN))
        SetWindowLongPtrW(hwndParent, GWL_STYLE, ps | WS_CLIPCHILDREN);

    SetPropW(hwndParent, CBPROP, (HANDLE)m);
    m->oldParentProc = (WNDPROC)SetWindowLongPtrW(hwndParent, GWLP_WNDPROC,
                                                  (LONG_PTR)CBParentProc);

    GetClientRect(hwndParent, &m->clientRc);
    return (HCB)m;
}

void CBAPI CB_Destroy(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    m->destroying = true;

    CBEndEdit(m, false);
    CBCloseMenus(m);

    CBReleaseHostChildren(m);

    if (m->hostMenu && IsWindow(m->parent))
    {
        SetMenu(m->parent, m->hostMenu);    /* leave the host as we found it */
        m->hostMenu = NULL;
    }
    if (m->oldParentProc && IsWindow(m->parent))
        SetWindowLongPtrW(m->parent, GWLP_WNDPROC, (LONG_PTR)m->oldParentProc);
    if (IsWindow(m->parent)) RemovePropW(m->parent, CBPROP);
    m->oldParentProc = NULL;

    if (m->tipRt)  { m->tipRt->Release(); m->tipRt = NULL; }
    if (m->tipWnd) { DestroyWindow(m->tipWnd); m->tipWnd = NULL; }
    if (m->editFont) { DeleteObject(m->editFont); m->editFont = NULL; }

    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        CBDiscardRT(c);
        if (c->hwnd) DestroyWindow(c->hwnd);
        delete c;
    }
    m->containers.clear();

    std::map<int, CBItem*>::iterator j;
    for (j = m->items.begin(); j != m->items.end(); ++j) delete j->second;
    m->items.clear();

    CBReleaseFonts(m);
    delete m;
}

void CBAPI CB_Layout(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (m) CBRelayout(m);
}

void CBAPI CB_GetClientRect(HCB cb, int* x, int* y, int* w, int* h)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    if (x) *x = m->clientRc.left;
    if (y) *y = m->clientRc.top;
    if (w) *w = m->clientRc.right - m->clientRc.left;
    if (h) *h = m->clientRc.bottom - m->clientRc.top;
}

void CBAPI CB_Redraw(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
        if (i->second->kind == CBK_BAR && i->second->hwnd)
            InvalidateRect(i->second->hwnd, NULL, FALSE);
}

/*=====================================================================
  Theming
  =====================================================================*/
void CBAPI CB_SetTheme(HCB cb, int theme)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    CBApplyTheme(m, theme);
    CBRelayout(m);
    CB_Redraw(cb);
}

int CBAPI CB_GetTheme(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    return m ? m->theme : 0;
}

void CBAPI CB_SetColor(HCB cb, int slot, COLORREF color)
{
    CBManager* m = (CBManager*)cb;
    if (!m || slot < 1 || slot > CBC_COUNT) return;
    m->col[slot] = color;
    CB_Redraw(cb);
}

COLORREF CBAPI CB_GetColor(HCB cb, int slot)
{
    CBManager* m = (CBManager*)cb;
    if (!m || slot < 1 || slot > CBC_COUNT) return 0;
    return m->col[slot];
}

void CBAPI CB_SetFont(HCB cb, int slot, const char* face, int sizePt,
                      int bold, int italic)
{
    CBManager* m = (CBManager*)cb;
    if (!m || slot < 1 || slot > 4) return;
    std::wstring f = CBToWide(face);
    if (!f.empty()) m->font[slot].face = f;
    if (sizePt > 0) m->font[slot].sizePt = sizePt;
    m->font[slot].bold   = bold   ? 1 : 0;
    m->font[slot].italic = italic ? 1 : 0;

    if (m->font[slot].fmt)     { m->font[slot].fmt->Release();     m->font[slot].fmt = NULL; }
    if (m->font[slot].fmtBold) { m->font[slot].fmtBold->Release(); m->font[slot].fmtBold = NULL; }
    if (slot == CBF_ITEM && m->editFont)
    {
        DeleteObject(m->editFont);
        m->editFont = NULL;
    }
    CBRelayout(m);
    CB_Redraw(cb);
}

void CBAPI CB_SetMetric(HCB cb, int metric, int value)
{
    CBManager* m = (CBManager*)cb;
    if (!m || metric < 1 || metric > CBM_ROWGAP) return;
    m->metric[metric] = value;
    CBRelayout(m);
    CB_Redraw(cb);
}

int CBAPI CB_GetMetric(HCB cb, int metric)
{
    CBManager* m = (CBManager*)cb;
    if (!m || metric < 1 || metric > CBM_ROWGAP) return 0;
    return m->metric[metric];
}

void CBAPI CB_SetAccent(HCB cb, COLORREF accent)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    CBApplyAccent(m, accent);
    CB_Redraw(cb);
}

/*=====================================================================
  Images
  =====================================================================*/
int CBAPI CB_AddImage(HCB cb, const char* fileName)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !fileName) return 0;
    CBImage img;
    if (!CBDecodeFile(CBToWide(fileName).c_str(), &img)) return 0;
    m->images.push_back(img);
    return (int)m->images.size() - 1;
}

int CBAPI CB_AddImageStrip(HCB cb, const char* fileName, int cx, int* count)
{
    CBManager* m = (CBManager*)cb;
    if (count) *count = 0;
    if (!m || !fileName) return 0;

    std::vector<CBImage> parts;
    if (!CBDecodeStrip(CBToWide(fileName).c_str(), cx, &parts) || parts.empty())
        return 0;

    int first = (int)m->images.size();
    for (size_t i = 0; i < parts.size(); ++i) m->images.push_back(parts[i]);
    if (count) *count = (int)parts.size();
    return first;
}

int CBAPI CB_AddImageHandle(HCB cb, HANDLE hIconOrBitmap, int isIcon)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !hIconOrBitmap) return 0;
    CBImage img;
    if (!CBDecodeHandle(hIconOrBitmap, isIcon != 0, &img)) return 0;
    m->images.push_back(img);
    return (int)m->images.size() - 1;
}

int CBAPI CB_GetImageCount(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    return m ? (int)m->images.size() - 1 : 0;
}

/*=====================================================================
  Containers
  =====================================================================*/
int CBAPI CB_AddBar(HCB cb, const char* title, int dock, unsigned long barStyle)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;

    CBContainer* c = new CBContainer();
    c->id      = m->nextContainer++;
    c->kind    = CBK_BAR;
    c->mgr     = m;
    c->title   = CBToWide(title);
    c->dock    = (dock < CBD_TOP || dock > CBD_FLOAT) ? CBD_TOP : dock;
    c->style   = barStyle;
    c->visible = true;
    m->containers[c->id] = c;

    CBEnsureBarWindow(m, c);
    CBRelayout(m);
    return c->id;
}

int CBAPI CB_CreateMenu(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;
    CBContainer* c = new CBContainer();
    c->id     = m->nextContainer++;
    c->kind   = CBK_MENU;
    c->mgr    = m;
    m->containers[c->id] = c;
    return c->id;
}

void CBAPI CB_ClearContainer(HCB cb, int container)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, container);
    if (!c) return;
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (it) { m->items.erase(it->id); delete it; }
    }
    c->items.clear();
    c->selIndex = -1;
    if (c->kind != CBK_MENU) CBRelayout(m);
}

void CBAPI CB_DestroyContainer(HCB cb, int container)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, container);
    if (!c) return;

    CB_ClearContainer(cb, container);
    CBDiscardRT(c);
    if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = NULL; }
    bool wasBar = (c->kind != CBK_MENU);
    m->containers.erase(container);
    delete c;
    if (wasBar) CBRelayout(m);
}

void CBAPI CB_SetBarDock(HCB cb, int bar, int dock, int row, int offset)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, bar);
    if (!c || c->kind != CBK_BAR) return;
    if (dock < CBD_TOP || dock > CBD_FLOAT) dock = CBD_TOP;

    bool styleChange = ((c->dock == CBD_FLOAT) != (dock == CBD_FLOAT));
    c->dock       = dock;
    c->dockRow    = row;
    c->dockOffset = offset;

    /* A child window cannot become a popup: rebuild it. */
    if (styleChange && c->hwnd)
    {
        CBDiscardRT(c);
        DestroyWindow(c->hwnd);
        c->hwnd = NULL;
    }
    CBEnsureBarWindow(m, c);
    CBRelayout(m);
}

int CBAPI CB_GetBarDock(HCB cb, int bar)
{
    CBContainer* c = CBFindContainer((CBManager*)cb, bar);
    return (c && c->kind == CBK_BAR) ? c->dock : -1;
}

void CBAPI CB_SetBarVisible(HCB cb, int bar, int visible)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, bar);
    if (!c || c->kind != CBK_BAR) return;
    c->visible = (visible != 0);
    if (!c->visible && c->hwnd) ShowWindow(c->hwnd, SW_HIDE);
    CBRelayout(m);
}

int CBAPI CB_GetBarVisible(HCB cb, int bar)
{
    CBContainer* c = CBFindContainer((CBManager*)cb, bar);
    return (c && c->kind == CBK_BAR && c->visible) ? 1 : 0;
}

void CBAPI CB_FloatBar(HCB cb, int bar, int x, int y)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, bar);
    if (!c || c->kind != CBK_BAR) return;
    c->floatX = x;
    c->floatY = y;
    CB_SetBarDock(cb, bar, CBD_FLOAT, c->dockRow, c->dockOffset);
}

long CBAPI CB_TrackMenu(HCB cb, int menu, int x, int y)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, menu);
    if (!c || c->kind != CBK_MENU) return 0;
    return CBTrackPopup(m, menu, x, y, 0, 0, NULL);
}

/*=====================================================================
  Items
  =====================================================================*/
static int AddItemImpl(CBManager* m, int container, int beforeItem, int type,
                       long cmdId, const char* text, int image)
{
    CBContainer* c = CBFindContainer(m, container);
    if (!c) return 0;
    if (type < CBI_BUTTON || type > CBI_SPACE) type = CBI_BUTTON;

    CBItem* it = new CBItem();
    it->id        = m->nextItem++;
    it->container = container;
    it->type      = type;
    it->cmd       = cmdId;
    it->image     = image;
    it->text      = CBStripAmp(CBToWide(text), &it->underline);
    if (type == CBI_TOGGLE || type == CBI_CHECKBOX) it->style |= CBIS_AUTOCHECK;

    m->items[it->id] = it;

    if (beforeItem > 0)
    {
        for (size_t i = 0; i < c->items.size(); ++i)
            if (c->items[i] == beforeItem)
            {
                c->items.insert(c->items.begin() + i, it->id);
                if (c->kind != CBK_MENU) CBRelayout(m);
                return it->id;
            }
    }
    c->items.push_back(it->id);
    if (c->kind != CBK_MENU) CBRelayout(m);
    return it->id;
}

int CBAPI CB_AddItem(HCB cb, int container, int type, long cmdId,
                     const char* text, int image)
{
    return AddItemImpl((CBManager*)cb, container, 0, type, cmdId, text, image);
}

int CBAPI CB_InsertItem(HCB cb, int container, int beforeItem, int type,
                        long cmdId, const char* text, int image)
{
    return AddItemImpl((CBManager*)cb, container, beforeItem, type, cmdId,
                       text, image);
}

void CBAPI CB_RemoveItem(HCB cb, int item)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    CBContainer* c = CBFindContainer(m, it->container);
    if (c)
        for (size_t i = 0; i < c->items.size(); ++i)
            if (c->items[i] == item) { c->items.erase(c->items.begin() + i); break; }
    m->items.erase(item);
    bool wasBar = c && c->kind != CBK_MENU;
    delete it;
    if (wasBar) CBRelayout(m);
}

/* Every setter that can change an item's SIZE re-flows the bar it is
   on; the ones that only change its look just repaint. */
static void Touch(CBManager* m, CBItem* it, bool resize)
{
    CBContainer* c = CBFindContainer(m, it->container);
    if (!c) return;
    if (c->kind == CBK_MENU) return;
    if (resize) CBRelayout(m);
    else if (c->hwnd) InvalidateRect(c->hwnd, NULL, FALSE);
}

void CBAPI CB_SetItemText(HCB cb, int item, const char* text)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->text = CBStripAmp(CBToWide(text), &it->underline);
    Touch(m, it, true);
}

int CBAPI CB_GetItemText(HCB cb, int item, char* buf, int bufLen)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (!buf || bufLen < 1) return 0;
    buf[0] = 0;
    if (!it) return 0;
    std::string a = CBToAnsi(it->text);
    int n = (int)a.size();
    if (n > bufLen - 1) n = bufLen - 1;
    if (n > 0) memcpy(buf, a.c_str(), n);
    buf[n] = 0;
    return n;
}

void CBAPI CB_SetItemImage(HCB cb, int item, int image)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    bool resize = (it->image == 0) != (image == 0);
    it->image = image;
    Touch(m, it, resize);
}

void CBAPI CB_SetItemStyle(HCB cb, int item, unsigned long styleFlags)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->style = styleFlags;
    Touch(m, it, true);
}

unsigned long CBAPI CB_GetItemStyle(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return it ? it->style : 0;
}

void CBAPI CB_SetItemEnabled(HCB cb, int item, int enabled)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->enabled = (enabled != 0);
    Touch(m, it, false);
}

int CBAPI CB_GetItemEnabled(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return (it && it->enabled) ? 1 : 0;
}

void CBAPI CB_SetItemChecked(HCB cb, int item, int checked)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->checked = (checked != 0);
    Touch(m, it, false);
}

int CBAPI CB_GetItemChecked(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return (it && it->checked) ? 1 : 0;
}

void CBAPI CB_SetItemVisible(HCB cb, int item, int visible)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->visible = (visible != 0);
    Touch(m, it, true);
}

int CBAPI CB_GetItemVisible(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return (it && it->visible) ? 1 : 0;
}

void CBAPI CB_SetItemTooltip(HCB cb, int item, const char* text)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (it) it->tooltip = CBToWide(text);
}

void CBAPI CB_SetItemShortcut(HCB cb, int item, const char* text)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (it) it->shortcut = CBToWide(text);
}

void CBAPI CB_SetItemWidth(HCB cb, int item, int px)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->width = px;
    Touch(m, it, true);
}

void CBAPI CB_SetItemMenu(HCB cb, int item, int menu)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    CBContainer* mc = CBFindContainer(m, menu);
    it->menu = (menu > 0 && mc && mc->kind == CBK_MENU) ? menu : 0;
    Touch(m, it, true);
}

int CBAPI CB_GetItemMenu(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return it ? it->menu : 0;
}

void CBAPI CB_SetItemCmd(HCB cb, int item, long cmdId)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (it) it->cmd = cmdId;
}

long CBAPI CB_GetItemCmd(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return it ? it->cmd : 0;
}

int CBAPI CB_FindItem(HCB cb, long cmdId)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;
    std::map<int, CBItem*>::iterator i;
    for (i = m->items.begin(); i != m->items.end(); ++i)
        if (i->second->cmd == cmdId) return i->first;
    return 0;
}

void CBAPI CB_EnableCmd(HCB cb, long cmdId, int enabled)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    std::map<int, CBItem*>::iterator i;
    for (i = m->items.begin(); i != m->items.end(); ++i)
        if (i->second->cmd == cmdId) i->second->enabled = (enabled != 0);
    CB_Redraw(cb);
}

void CBAPI CB_CheckCmd(HCB cb, long cmdId, int checked)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    std::map<int, CBItem*>::iterator i;
    for (i = m->items.begin(); i != m->items.end(); ++i)
        if (i->second->cmd == cmdId) i->second->checked = (checked != 0);
    CB_Redraw(cb);
}

int CBAPI CB_GetItemCount(HCB cb, int container)
{
    CBContainer* c = CBFindContainer((CBManager*)cb, container);
    return c ? (int)c->items.size() : 0;
}

int CBAPI CB_GetItemAt(HCB cb, int container, int index)
{
    CBContainer* c = CBFindContainer((CBManager*)cb, container);
    if (!c || index < 0 || index >= (int)c->items.size()) return 0;
    return c->items[index];
}

/*=====================================================================
  Values
  =====================================================================*/
void CBAPI CB_SetItemValue(HCB cb, int item, const char* text)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->value = CBToWide(text);
    if (it->type == CBI_COMBO)
    {
        it->comboSel = -1;
        for (size_t i = 0; i < it->combo.size(); ++i)
            if (it->combo[i] == it->value) { it->comboSel = (int)i; break; }
    }
    Touch(m, it, false);
}

int CBAPI CB_GetItemValue(HCB cb, int item, char* buf, int bufLen)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (!buf || bufLen < 1) return 0;
    buf[0] = 0;
    if (!it) return 0;
    std::string a = CBToAnsi(it->value);
    int n = (int)a.size();
    if (n > bufLen - 1) n = bufLen - 1;
    if (n > 0) memcpy(buf, a.c_str(), n);
    buf[n] = 0;
    return n;
}

void CBAPI CB_AddComboItem(HCB cb, int item, const char* text)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (it) it->combo.push_back(CBToWide(text));
}

void CBAPI CB_ClearComboItems(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    if (!it) return;
    it->combo.clear();
    it->comboSel = -1;
}

void CBAPI CB_SetComboSel(HCB cb, int item, int index)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    if (index < 0 || index >= (int)it->combo.size())
    {
        it->comboSel = -1;
        it->value.clear();
    }
    else
    {
        it->comboSel = index;
        it->value    = it->combo[index];
    }
    Touch(m, it, false);
}

int CBAPI CB_GetComboSel(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return it ? it->comboSel : -1;
}

void CBAPI CB_SetItemColor(HCB cb, int item, COLORREF color)
{
    CBManager* m = (CBManager*)cb;
    CBItem* it = CBFindItem(m, item);
    if (!it) return;
    it->color = color;
    Touch(m, it, false);
}

COLORREF CBAPI CB_GetItemColor(HCB cb, int item)
{
    CBItem* it = CBFindItem((CBManager*)cb, item);
    return it ? it->color : 0;
}

/*=====================================================================
  Keyboard
  =====================================================================*/
void CBAPI CB_AddAccelerator(HCB cb, long cmdId, int key, int mods)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !key) return;
    CBAccel a;
    a.cmd  = cmdId;
    a.key  = key;
    a.mods = mods;
    m->accels.push_back(a);
}

void CBAPI CB_ClearAccelerators(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (m) m->accels.clear();
}

int CBAPI CB_TranslateKey(HCB cb, int key, int mods)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;

    for (size_t i = 0; i < m->accels.size(); ++i)
    {
        if (m->accels[i].key != key || m->accels[i].mods != mods) continue;

        long cmd = m->accels[i].cmd;
        int  itemId = CB_FindItem(cb, cmd);
        CBItem* it = CBFindItem(m, itemId);
        if (it && !it->enabled) return 1;      /* matched, but greyed out */

        if (it && (it->style & CBIS_AUTOCHECK))
        {
            it->checked = !it->checked;
            CBQueue(m, it->id, cmd, CBE_TOGGLED, it->checked ? 1 : 0);
            CB_Redraw(cb);
        }
        CBQueue(m, itemId, cmd, CBE_COMMAND, 0);
        return 1;
    }

    /* Alt+letter drops the matching menu title. */
    if (mods == CBK_ALT && key >= 'A' && key <= 'Z')
    {
        std::map<int, CBContainer*>::iterator ci;
        for (ci = m->containers.begin(); ci != m->containers.end(); ++ci)
        {
            CBContainer* c = ci->second;
            if (c->kind != CBK_BAR || !c->visible || !c->hwnd) continue;
            for (size_t k = 0; k < c->items.size(); ++k)
            {
                CBItem* it = CBFindItem(m, c->items[k]);
                if (!it || !it->visible || !it->enabled || !it->menu) continue;
                if (it->type != CBI_MENU && it->type != CBI_DROPDOWN) continue;
                if (it->underline < 0 || it->underline >= (int)it->text.size()) continue;
                if ((int)towupper(it->text[it->underline]) != key) continue;

                RECT ex = it->rc;
                POINT tl; tl.x = ex.left;  tl.y = ex.top;
                POINT br; br.x = ex.right; br.y = ex.bottom;
                ClientToScreen(c->hwnd, &tl);
                ClientToScreen(c->hwnd, &br);
                ex.left = tl.x; ex.top = tl.y; ex.right = br.x; ex.bottom = br.y;
                CBTrackPopup(m, it->menu, ex.left, ex.bottom, it->id, c->id, &ex);
                return 1;
            }
        }
    }
    return 0;
}

/*=====================================================================
  Events
  =====================================================================*/
int CBAPI CB_PollEvent(HCB cb, int* item, long* cmdId, int* evType, long* param)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;
    if (m->events.empty())
    {
        /*  The pump drains the queue every tick, so this runs once a
            tick: the moment to notice a toolbar the host rebuilt behind
            our back. */
        CBWatchHostKids(m);
        return 0;
    }
    CBEvent e = m->events.front();
    m->events.pop_front();
    if (item)   *item   = e.item;
    if (cmdId)  *cmdId  = e.cmd;
    if (evType) *evType = e.type;
    if (param)  *param  = e.param;
    return 1;
}

void CBAPI CB_SetCallback(HCB cb, CB_EVENTPROC proc, long userData)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    m->proc     = proc;
    m->procUser = userData;
}

/*=====================================================================
  Laying out a ribbon

  A ribbon bar holds TAB containers; a tab holds GROUP containers; a
  group holds ordinary items.  Only the ACTIVE tab is laid out - the
  others cost nothing until they are selected.

      +-------------------------------------------------+
      | Home | Insert |                                 |   tab strip
      +-------------------------------------------------+
      | [ Paste ] | [B][I][U] | [ Find ]                |   content
      | [ Cut   ] | [A][A][A] |                         |
      | Clipboard |   Font    | Editing                 |   captions
      +-------------------------------------------------+

  An item marked CBIS_TEXTBELOW is LARGE: icon over text, full content
  height.  Everything else is SMALL and stacks in up to three rows,
  filling a column before starting the next - which is how every ribbon
  lays a group out.
  =====================================================================*/
static int RibbonTabHeight(CBManager* m)
{
    return CBFontHeight(m, CBF_ITEM) + (int)(9 * m->dpiScale);
}
static int RibbonCaptionHeight(CBManager* m)
{
    return CBFontHeight(m, CBF_ITEM) + (int)(4 * m->dpiScale);
}
static int RibbonContentHeight(CBManager* m)
{
    int large = CBMetric(m, CBM_LARGEICON);
    return large + CBFontHeight(m, CBF_ITEM) + (int)(12 * m->dpiScale);
}

void CBLayoutRibbon(CBManager* m, CBContainer* c, int availW)
{
    const int padX    = CBMetric(m, CBM_BARPADX);
    const int padY    = CBMetric(m, CBM_BARPADY);
    const int gap     = CBMetric(m, CBM_GAP);
    const int tabH    = RibbonTabHeight(m);
    const int capH    = RibbonCaptionHeight(m);
    const int contH   = RibbonContentHeight(m);
    const int smallIc = CBMetric(m, CBM_ICONSIZE);
    const int largeIc = CBMetric(m, CBM_LARGEICON);
    const int itemPad = CBMetric(m, CBM_PADX);

    c->laid.clear();
    c->hasChevron = false;
    SetRectEmpty(&c->chevronRc);

    /* ---- the tab strip ---- */
    int x = padX + (int)(6 * m->dpiScale);
    for (size_t t = 0; t < c->items.size(); ++t)
    {
        CBContainer* tab = CBFindContainer(m, c->items[t]);
        if (!tab || tab->kind != CBK_TAB) continue;
        float tw = 0;
        CBMeasure(m, CBF_ITEM, false, tab->caption, &tw, NULL);
        int w = (int)(tw + 0.5f) + 2 + 4 * itemPad;
        tab->tabRc.left   = x;
        tab->tabRc.right  = x + w;
        tab->tabRc.top    = padY;
        tab->tabRc.bottom = padY + tabH;
        x += w;
        if (!c->activeTab) c->activeTab = tab->id;
    }

    /*  The little collapse button, at the far end of the tab strip - the
        one older ribbons put there.  Laid out before the early return so
        it is still there to click when the ribbon is collapsed. */
    {
        const int barW = (availW > 0) ? availW : (x + padX + (int)(40 * m->dpiScale));
        const int side = tabH - (int)(10 * m->dpiScale);
        const int sz   = (side < (int)(12 * m->dpiScale)) ? (int)(12 * m->dpiScale) : side;
        c->minRc.right  = barW - padX - (int)(4 * m->dpiScale);
        c->minRc.left   = c->minRc.right - sz;
        c->minRc.top    = padY + (tabH - sz) / 2;
        c->minRc.bottom = c->minRc.top + sz;
        if (c->minRc.left < x) SetRectEmpty(&c->minRc);   /* no room for it */
    }

    const int contentTop = padY + tabH;

    /*  Collapsed: the tab strip is the whole bar.  The groups keep their
        items but are left with empty rects, so nothing paints and nothing
        can be hit until it is opened again. */
    if (c->minimized)
    {
        CBContainer* act = CBFindContainer(m, c->activeTab);
        if (act && act->kind == CBK_TAB)
            for (size_t g = 0; g < act->items.size(); ++g)
            {
                CBContainer* grp = CBFindContainer(m, act->items[g]);
                if (!grp) continue;
                SetRectEmpty(&grp->groupRc);
                for (size_t k = 0; k < grp->items.size(); ++k)
                {
                    CBItem* it = CBFindItem(m, grp->items[k]);
                    if (it) { SetRectEmpty(&it->rc); SetRectEmpty(&it->arrow); }
                }
            }
        c->rowCount = 1;
        c->measH    = contentTop + padY;
        c->measW    = availW > 0 ? availW : x + padX;
        return;
    }

    /* ---- the active tab's groups ---- */
    CBContainer* active = CBFindContainer(m, c->activeTab);
    int gx = padX + (int)(4 * m->dpiScale);

    if (active && active->kind == CBK_TAB)
    {
        for (size_t g = 0; g < active->items.size(); ++g)
        {
            CBContainer* grp = CBFindContainer(m, active->items[g]);
            if (!grp || grp->kind != CBK_GROUP) continue;

            int colX     = gx + itemPad;
            int smallRow = contH / 3;
            int rowInCol = 0;
            int colW     = 0;
            int startX   = colX;

            for (size_t k = 0; k < grp->items.size(); ++k)
            {
                CBItem* it = CBFindItem(m, grp->items[k]);
                if (!it) continue;
                SetRectEmpty(&it->rc);
                SetRectEmpty(&it->arrow);
                it->overflow = false;
                if (!it->visible) continue;

                bool large = (it->style & CBIS_TEXTBELOW) != 0;
                float tw = 0;
                if (!it->text.empty() && !(it->style & CBIS_ICONONLY))
                    CBMeasure(m, CBF_ITEM, false, it->text, &tw, NULL);
                int textW = (int)(tw + 0.5f) + 2;

                if (large)
                {
                    if (rowInCol) { colX += colW + gap; rowInCol = 0; colW = 0; }
                    int w = (largeIc > textW ? largeIc : textW) + 2 * itemPad;
                    /* A split or colour button is drawn as command-half +
                       arrow-half, and the drawing code derives the content
                       rect from arrow.left.  Leave the arrow rect empty and
                       that content rect collapses to nothing, so the whole
                       button disappears - reserve the zone here. */
                    int arrowW = 0;
                    if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
                        arrowW = (int)(16 * m->dpiScale);
                    else if (it->type == CBI_DROPDOWN)
                        arrowW = (int)(13 * m->dpiScale);
                    w += arrowW;

                    it->rc.left   = colX;
                    it->rc.right  = colX + w;
                    it->rc.top    = contentTop + padY;
                    it->rc.bottom = contentTop + contH - padY;
                    if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
                    {
                        it->arrow = it->rc;
                        it->arrow.left = it->rc.right - arrowW;
                    }
                    colX += w + gap;
                    c->laid.push_back(it->id);
                    continue;
                }

                if (it->type == CBI_SEPARATOR)
                {
                    if (rowInCol) { colX += colW + gap; rowInCol = 0; colW = 0; }
                    it->rc.left   = colX;
                    it->rc.right  = colX + CBMetric(m, CBM_SEPWIDTH);
                    it->rc.top    = contentTop + padY;
                    it->rc.bottom = contentTop + contH - padY;
                    colX += (it->rc.right - it->rc.left) + gap;
                    c->laid.push_back(it->id);
                    continue;
                }

                int w = 2 * itemPad;
                bool wantIcon = it->image > 0 && !(it->style & CBIS_TEXTONLY);
                if (wantIcon) w += smallIc;
                if (textW > 2 && !(it->style & CBIS_ICONONLY))
                    w += (wantIcon ? (int)(4 * m->dpiScale) : 0) + textW;
                if (it->type == CBI_DROPDOWN) w += (int)(12 * m->dpiScale);
                if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
                    w += (int)(16 * m->dpiScale);
                if (it->type == CBI_EDIT || it->type == CBI_COMBO)
                    w = it->width > 0 ? (int)(it->width * m->dpiScale)
                                      : (int)(110 * m->dpiScale);

                if (rowInCol == 0) startX = colX;
                it->rc.left   = startX;
                it->rc.right  = startX + w;
                it->rc.top    = contentTop + padY + rowInCol * smallRow;
                it->rc.bottom = it->rc.top + smallRow - 1;
                if (w > colW) colW = w;
                if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
                {
                    it->arrow = it->rc;
                    it->arrow.left = it->rc.right - (int)(16 * m->dpiScale);
                }
                c->laid.push_back(it->id);

                if (++rowInCol >= 3) { colX = startX + colW + gap; rowInCol = 0; colW = 0; }
            }
            if (rowInCol) colX = startX + colW + gap;

            /* the group box has to be at least as wide as its caption */
            float cw = 0;
            CBMeasure(m, CBF_ITEM, false, grp->caption, &cw, NULL);
            int need = (int)(cw + 0.5f) + 2 + 2 * itemPad;
            int gw   = (colX - gx) + itemPad;
            if (gw < need) gw = need;

            grp->groupRc.left   = gx;
            grp->groupRc.right  = gx + gw;
            grp->groupRc.top    = contentTop;
            grp->groupRc.bottom = contentTop + contH + capH;
            gx += gw;
        }
    }

    c->rowCount = 1;
    c->measH    = contentTop + contH + capH + padY;
    c->measW    = availW > 0 ? availW : gx + padX;
}

void CBAPI CB_SetRibbonMinimized(HCB cb, int bar, int minimized)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR || !(b->style & CBBS_RIBBON)) return;
    const bool want = (minimized != 0);
    if (b->minimized == want) return;
    b->minimized = want;
    CBRelayout(m);
    if (b->hwnd) InvalidateRect(b->hwnd, NULL, FALSE);
    CBQueue(m, 0, b->id, CBE_LAYOUT, want ? 1 : 0);
}

int CBAPI CB_GetRibbonMinimized(HCB cb, int bar)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    return (b && b->kind == CBK_BAR && b->minimized) ? 1 : 0;
}

/* Is the point on the ribbon's collapse button? */
bool CBRibbonMinHit(CBContainer* c, POINT pt)
{
    return c && (c->style & CBBS_RIBBON) && !IsRectEmpty(&c->minRc) &&
           PtInRect(&c->minRc, pt);
}

/* The tab under a point, or 0. */
int CBTabHitTest(CBManager* m, CBContainer* c, POINT pt)
{
    if (!c || !(c->style & CBBS_RIBBON)) return 0;
    for (size_t t = 0; t < c->items.size(); ++t)
    {
        CBContainer* tab = CBFindContainer(m, c->items[t]);
        if (!tab || tab->kind != CBK_TAB) continue;
        if (PtInRect(&tab->tabRc, pt)) return tab->id;
    }
    return 0;
}

/*=====================================================================
  Ribbon API
  =====================================================================*/
int CBAPI CB_AddRibbonTab(HCB cb, int bar, const char* text)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR) return 0;

    CBContainer* t = new CBContainer();
    t->id      = m->nextContainer++;
    t->kind    = CBK_TAB;
    t->mgr     = m;
    t->owner   = bar;
    t->caption = CBStripAmp(CBToWide(text), NULL);
    m->containers[t->id] = t;

    b->items.push_back(t->id);
    if (!b->activeTab) b->activeTab = t->id;
    b->style |= CBBS_RIBBON;
    CBRelayout(m);
    return t->id;
}

int CBAPI CB_AddRibbonGroup(HCB cb, int tab, const char* text)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* t = CBFindContainer(m, tab);
    if (!t || t->kind != CBK_TAB) return 0;

    CBContainer* g = new CBContainer();
    g->id      = m->nextContainer++;
    g->kind    = CBK_GROUP;
    g->mgr     = m;
    g->owner   = tab;
    g->caption = CBStripAmp(CBToWide(text), NULL);
    m->containers[g->id] = g;

    t->items.push_back(g->id);
    CBRelayout(m);
    return g->id;
}

void CBAPI CB_SetActiveTab(HCB cb, int bar, int tab)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR) return;
    if (b->activeTab == tab) return;
    b->activeTab = tab;
    CBRelayout(m);
    if (b->hwnd) InvalidateRect(b->hwnd, NULL, FALSE);
    CBQueue(m, 0, 0, CBE_TABCHANGED, (long)tab);
}

int CBAPI CB_GetActiveTab(HCB cb, int bar)
{
    CBContainer* b = CBFindContainer((CBManager*)cb, bar);
    return (b && b->kind == CBK_BAR) ? b->activeTab : 0;
}

int CBAPI CB_GetTabCount(HCB cb, int bar)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR) return 0;
    int n = 0;
    for (size_t i = 0; i < b->items.size(); ++i)
    {
        CBContainer* t = CBFindContainer(m, b->items[i]);
        if (t && t->kind == CBK_TAB) ++n;
    }
    return n;
}

int CBAPI CB_GetTabAt(HCB cb, int bar, int index)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR || index < 0) return 0;
    int n = 0;
    for (size_t i = 0; i < b->items.size(); ++i)
    {
        CBContainer* t = CBFindContainer(m, b->items[i]);
        if (t && t->kind == CBK_TAB && n++ == index) return t->id;
    }
    return 0;
}

/* Make room at `row` on `dock` and put `bar` there. */
void CBInsertBarRow(CBManager* m, int bar, int dock, int row)
{
    CBContainer* b = CBFindContainer(m, bar);
    if (!b || b->kind != CBK_BAR) return;
    if (row < 0) row = 0;

    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || c == b) continue;
        if (c->dock != dock) continue;
        if (c->dockRow >= row) c->dockRow++;
    }
    CB_SetBarDock(m, bar, dock, row, 0);
}

void CBAPI CB_SetBarRect(HCB cb, int bar, int x, int y, int w, int h)
{
    CBManager* m = (CBManager*)cb;
    CBContainer* c = CBFindContainer(m, bar);
    if (!c || c->kind != CBK_BAR) return;
    c->fixedRc.left   = x;
    c->fixedRc.top    = y;
    c->fixedRc.right  = x + w;
    c->fixedRc.bottom = y + h;
    if (c->dock != CBD_FIXED)
    {
        bool wasPopup = (c->dock == CBD_FLOAT);
        c->dock = CBD_FIXED;
        if (wasPopup && c->hwnd)          /* a popup cannot become a child */
        {
            CBDiscardRT(c);
            DestroyWindow(c->hwnd);
            c->hwnd = NULL;
        }
    }
    CBEnsureBarWindow(m, c);
    CBRelayout(m);
}

/*=====================================================================
  Reserving space from the host

  CB_GetClientRect says what the bars left over, but the HOST does not
  read it.  A Clarion APPLICATION frame lays its own ClaToolBar and
  MDIClient out against the FULL client area, so a docked bar simply
  ends up drawn on top of them.  Measured on a frame 910x606 carrying a
  toolbar:

      ClaToolBar   at 0,0    910 x 61     <- a top BAND, fixed height
      MDIClient    at 0,61   910 x 522    <- FILLS, stopping 23px short
                                             of the bottom for the
                                             status bar

  Two things make this harder than moving the windows:

    * Clarion derives the MDI client's top from the toolbar's HEIGHT,
      not from where the toolbar actually is, so repositioning them
      afterwards just starts a fight it wins.
    * It skips laying out at all when the frame size has not changed,
      so there is no event to piggyback on either.

  So each host child is SUBCLASSED, its CANONICAL rect - the one the
  host asks for, against the full client area - is remembered, and
  WM_WINDOWPOSCHANGING is corrected in flight.  To apply a change in bar
  heights the canonical rect is simply replayed through the same hook,
  which makes the whole thing idempotent: transforming always starts
  from what the host wanted, never from what we last did.
  =====================================================================*/

static const wchar_t* CBKIDPROP = L"ClaCommandBar.Kid";

static void CBRectToClient(HWND parent, HWND child, RECT* out)
{
    RECT wr;
    GetWindowRect(child, &wr);
    POINT tl;
    tl.x = wr.left;
    tl.y = wr.top;
    POINT br;
    br.x = wr.right;
    br.y = wr.bottom;
    ScreenToClient(parent, &tl);
    ScreenToClient(parent, &br);
    out->left = tl.x;
    out->top = tl.y;
    out->right = br.x;
    out->bottom = br.y;
}

/*  Temporary diagnostic - set CB_HOSTLOG=1 to trace host-child layout. */
static void CBHostLog(const char* fmt, ...)
{
    static int on = -1;
    if (on < 0)
    {
        char v[8];
        on = (GetEnvironmentVariableA("CB_HOSTLOG", v, 8) > 0 && v[0] == '1') ? 1 : 0;
    }
    if (!on) return;

    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    if (n <= 0) return;

    char path[MAX_PATH];
    DWORD tn = GetTempPathA(MAX_PATH, path);
    if (!tn) return;
    strcat_s(path, MAX_PATH, "cbhost.log");
    HANDLE f = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    WriteFile(f, buf, (DWORD)n, &w, NULL);
    CloseHandle(f);
}

static const char* CBClsOf(HWND h)
{
    static char c[64];
    if (!GetClassNameA(h, c, 64)) c[0] = 0;
    return c;
}

static bool CBIsOurWindow(HWND h)
{
    wchar_t cls[64];
    if (!GetClassNameW(h, cls, 64)) return false;
    return wcsncmp(cls, L"ClaCommandBar.", 14) == 0;
}

static bool CBClassIs(HWND h, const wchar_t* name)
{
    wchar_t cls[64];
    if (!GetClassNameW(h, cls, 64)) return false;
    return _wcsicmp(cls, name) == 0;
}

struct CBSweep
{
    CBManager*         m;
    std::vector<HWND>* out;
};

static BOOL CALLBACK CBSweepProc(HWND h, LPARAM lp)
{
    CBSweep* s = (CBSweep*)lp;
    if (GetParent(h) != s->m->parent) return TRUE;   /* direct children only */
    if (CBIsOurWindow(h)) return TRUE;
    s->out->push_back(h);
    return TRUE;
}

static void CBHostChildren(CBManager* m, std::vector<HWND>* out)
{
    CBSweep s;
    s.m = m;
    s.out = out;
    EnumChildWindows(m->parent, CBSweepProc, (LPARAM)&s);
}

/* Auto mode: only a host that lays out its OWN children needs this. */
static bool CBHostOwnsLayout(CBManager* m)
{
    std::vector<HWND> kids;
    CBHostChildren(m, &kids);
    for (size_t i = 0; i < kids.size(); ++i)
        if (CBClassIs(kids[i], L"MDIClient") || CBClassIs(kids[i], L"ClaToolBar"))
            return true;
    return false;
}

static CBHostKid* CBFindKid(CBManager* m, HWND h)
{
    for (size_t i = 0; i < m->hostKids.size(); ++i)
        if (m->hostKids[i].hwnd == h) return &m->hostKids[i];
    return NULL;
}

/* Turn the rect the host asked for into the rect it should get. */
static void CBTransformHostRect(CBManager* m, const RECT* canon, RECT* out)
{
    RECT pc;
    GetClientRect(m->parent, &pc);
    *out = *canon;
    if (pc.right < 2 || pc.bottom < 2) return;

    const int cw = canon->right - canon->left;
    const int ch = canon->bottom - canon->top;
    if (cw < 1 || ch < 1) return;

    /* what the bars took off each side */
    const int dl = m->insL;
    const int dt = m->insT;
    const int dr = m->insR;
    const int db = m->insB;
    if (!dl && !dt && !dr && !db) return;

    /* room the host left past this child, kept intact - that is where a
       status bar under the MDI client lives */
    const int insetR = pc.right  - canon->right;
    const int insetB = pc.bottom - canon->bottom;

    /* A FILLER spans most of the host; anything smaller is a BAND with a
       fixed size that should only be pushed along. */
    const bool fillV = (ch * 10 > pc.bottom * 6);
    const bool fillH = (cw * 10 > pc.right  * 6);

    out->left   = canon->left + dl;
    out->top    = canon->top  + dt;
    out->right  = fillH ? (pc.right  - dr - insetR) : (out->left + cw);
    out->bottom = fillV ? (pc.bottom - db - insetB) : (out->top  + ch);
    if (out->right  <= out->left) out->right  = out->left + 1;
    if (out->bottom <= out->top)  out->bottom = out->top + 1;
}

/*  Push the host's own CANONICAL rect at the window again with the hook
    LIVE, so the correction runs on the way through and lands on top of
    whatever the host's own window procedure decides.  Handing it the
    already transformed rect with the hook muted does not work: Clarion's
    MDI client procedure re-imposes the frame's layout inside the very
    same SetWindowPos call, so the move is undone before it is ever
    visible - which is why the client kept sitting under the toolbar. */
static void CBReplayHostKid(CBManager* m, HWND h, const RECT* canon)
{
    (void)m;
    /*  SWP_NOCOPYBITS: moving a window normally copies its pixels to the
        new place, and a host child that had a bar sitting over part of
        it carries THE BAR'S pixels along - which is how dragging a bar
        straight from one side to the other left a copy of it printed on
        the toolbar.  Repaint instead of blitting. */
    SetWindowPos(h, NULL, canon->left, canon->top,
                 canon->right - canon->left,
                 canon->bottom - canon->top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

static LRESULT CALLBACK CBHostKidProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    CBManager* m   = (CBManager*)GetPropW(h, CBPROP);
    WNDPROC    old = (WNDPROC)GetPropW(h, CBKIDPROP);
    if (!old) return DefWindowProcW(h, msg, wp, lp);

    const bool live = m && !m->destroying && m->reserve != 0;

    if (msg == WM_WINDOWPOSCHANGING && live)
    {
        WINDOWPOS* p = (WINDOWPOS*)lp;

        /*  Let the host adjust the position FIRST - it is the one that
            knows where its toolbar and its client belong - and correct
            the answer afterwards.  The other way round loses every time
            the host has an opinion of its own. */
        LRESULT r = CallWindowProcW(old, h, msg, wp, lp);

        CBHostKid* k = CBFindKid(m, h);

        /*  Only a real move or size is a layout.  A show, a hide or a
            bare z-order change carries no geometry, and forcing
            coordinates onto one of those is what made the frame's
            toolbar vanish the moment an MDI child merged into it. */
        const bool moving = (p->flags & (SWP_NOMOVE | SWP_NOSIZE))
                                != (SWP_NOMOVE | SWP_NOSIZE);
        if (p->flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW))
            CBHostLog("%-8s %-12s flags=%08X\r\n",
                      (p->flags & SWP_HIDEWINDOW) ? "HIDE" : "SHOW",
                      CBClsOf(h), p->flags);
        if (k && moving)
        {
            RECT canon;
            canon.left   = (p->flags & SWP_NOMOVE) ? k->canon.left : p->x;
            canon.top    = (p->flags & SWP_NOMOVE) ? k->canon.top  : p->y;
            canon.right  = canon.left + ((p->flags & SWP_NOSIZE)
                              ? (k->canon.right - k->canon.left) : p->cx);
            canon.bottom = canon.top + ((p->flags & SWP_NOSIZE)
                              ? (k->canon.bottom - k->canon.top) : p->cy);

            /*  A collapsed rect is the host parking the window, not
                laying it out.  Remembering one as canonical would strand
                the window for good. */
            if (canon.right > canon.left && canon.bottom > canon.top)
            {
                k->canon = canon;

                RECT t;
                CBTransformHostRect(m, &canon, &t);
                CBHostLog("CHANGING %-12s flags=%08X canon=%d,%d,%d,%d -> %d,%d,%d,%d \r\n",
                          CBClsOf(h), p->flags,
                          canon.left, canon.top, canon.right, canon.bottom,
                          t.left, t.top, t.right, t.bottom);
                p->x  = t.left;
                p->y  = t.top;
                p->cx = t.right - t.left;
                p->cy = t.bottom - t.top;
                p->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
                p->flags |= SWP_NOCOPYBITS;   /* see CBReplayHostKid */
            }
        }
        return r;
    }

    if (msg == WM_NCDESTROY)
    {
        SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)old);
        RemovePropW(h, CBKIDPROP);
        RemovePropW(h, CBPROP);
    }
    return CallWindowProcW(old, h, msg, wp, lp);
}

/* Replay every hooked child's canonical rect so a change in the bars
   takes effect now.  The host does not re-lay out on its own unless the
   frame size changed, so waiting for it is not an option. */
static void CBApplyHostLayout(CBManager* m)
{
    for (size_t i = 0; i < m->hostKids.size(); ++i)
    {
        CBHostKid& k = m->hostKids[i];
        if (!IsWindow(k.hwnd)) continue;
        RECT t;
        CBTransformHostRect(m, &k.canon, &t);
        RECT cur;
        CBRectToClient(m->parent, k.hwnd, &cur);
        CBHostLog("APPLY    %-12s canon=%d,%d,%d,%d cur=%d,%d,%d,%d -> %d,%d,%d,%d%s\r\n",
                  CBClsOf(k.hwnd),
                  k.canon.left, k.canon.top, k.canon.right, k.canon.bottom,
                  cur.left, cur.top, cur.right, cur.bottom,
                  t.left, t.top, t.right, t.bottom,
                  EqualRect(&t, &cur) ? " (already)" : "");
        if (EqualRect(&t, &cur)) continue;
        RECT canon = k.canon;               /* the vector may move under us */
        CBReplayHostKid(m, k.hwnd, &canon);
    }
}

/*  How much of the bottom does the host keep for itself?

    There is nothing to enumerate - a Clarion frame paints its status bar
    rather than putting a window there - so it is read off the host's own
    intent instead: the MDI client is a FILLER, and wherever it stops
    short of the bottom is the strip the host has kept.  Measured on a
    frame 1374x776: the client's canonical rect ends at 753, so 23px
    belong to the status bar and no bar may use them.

    Only a filler counts.  A band like the toolbar stops short of the
    bottom by most of the window, which means nothing at all. */
static int CBMeasureHostReserveB(CBManager* m)
{
    RECT pc;
    GetClientRect(m->parent, &pc);
    if (pc.bottom < 8) return 0;

    int keep = 0;
    for (size_t i = 0; i < m->hostKids.size(); ++i)
    {
        const CBHostKid& k = m->hostKids[i];
        if (!IsWindow(k.hwnd) || !IsWindowVisible(k.hwnd)) continue;

        const int ch = k.canon.bottom - k.canon.top;
        if (ch * 10 <= pc.bottom * 6) continue;        /* a band, not a filler */

        int gap = pc.bottom - k.canon.bottom;
        if (gap < 0) gap = 0;
        /* A whole band's worth is somebody else's window, not a painted
           strip - leave that to the ordinary layout. */
        if (gap > pc.bottom / 4) continue;
        if (gap > keep) keep = gap;
    }
    return keep;
}

void CBReserveFromHost(CBManager* m)
{
    if (!m || m->destroying || !IsWindow(m->parent)) return;
    if (m->reserve == 0) return;
    if (m->reserve < 0 && !CBHostOwnsLayout(m)) return;

    std::vector<HWND> kids;
    CBHostChildren(m, &kids);

    for (size_t i = m->hostKids.size(); i-- > 0; )
        if (!IsWindow(m->hostKids[i].hwnd))
            m->hostKids.erase(m->hostKids.begin() + i);

    for (size_t k = 0; k < kids.size(); ++k)
    {
        HWND h = kids[k];
        if (CBFindKid(m, h)) continue;                 /* already hooked */

        CBHostKid nk;
        nk.hwnd = h;
        /* Nothing has been transformed yet, so where it sits right now
           IS what the host wanted. */
        CBRectToClient(m->parent, h, &nk.canon);
        nk.oldProc = (WNDPROC)SetWindowLongPtrW(h, GWLP_WNDPROC,
                                                (LONG_PTR)CBHostKidProc);
        SetPropW(h, CBKIDPROP, (HANDLE)nk.oldProc);
        SetPropW(h, CBPROP, (HANDLE)m);
        m->hostKids.push_back(nk);
        CBHostLog("HOOK     %-12s canon=%d,%d,%d,%d\r\n", CBClsOf(h),
                  nk.canon.left, nk.canon.top, nk.canon.right, nk.canon.bottom);
    }

    const int keep = CBMeasureHostReserveB(m);
    if (keep != m->hostResB)
    {
        m->hostResB = keep;
        /*  Lay out again knowing it - posted, because this runs from
            inside the layout that would have to be redone. */
        PostMessageW(m->parent, CBMSG_RELAYOUT, 0, 0);
    }

    CBApplyHostLayout(m);
}

/*  Has the host grown a child we have not hooked?  Clarion builds the
    merged toolbar without any layout happening afterwards, so waiting
    for CBRelayout to notice is waiting for something that may never
    come.  WM_PARENTNOTIFY covers it when the host sends one; this is
    the backstop for when it does not, and it is cheap enough to run on
    every poll - an MDI frame has about four direct children. */
void CBWatchHostKids(CBManager* m)
{
    if (!m || m->destroying || m->reserve == 0 || !IsWindow(m->parent)) return;
    if (m->hostKids.empty()) return;      /* nothing hooked yet - not our case */

    std::vector<HWND> kids;
    CBHostChildren(m, &kids);
    for (size_t i = 0; i < kids.size(); ++i)
    {
        if (CBFindKid(m, kids[i])) continue;
        CBReserveFromHost(m);             /* something new - hook and place it */
        return;
    }
}

void CBReleaseHostChildren(CBManager* m)
{
    for (size_t i = 0; i < m->hostKids.size(); ++i)
    {
        CBHostKid& k = m->hostKids[i];
        if (!IsWindow(k.hwnd)) continue;
        if (k.oldProc) SetWindowLongPtrW(k.hwnd, GWLP_WNDPROC, (LONG_PTR)k.oldProc);
        RemovePropW(k.hwnd, CBKIDPROP);
        RemovePropW(k.hwnd, CBPROP);
        /* put it back where the host wanted it */
        SetWindowPos(k.hwnd, NULL, k.canon.left, k.canon.top,
                     k.canon.right - k.canon.left,
                     k.canon.bottom - k.canon.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    m->hostKids.clear();
}

void CBAPI CB_SetReserveSpace(HCB cb, int mode)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    if (mode == 0) CBReleaseHostChildren(m);
    m->reserve = mode;
    CBRelayout(m);
}

int CBAPI CB_GetReserveSpace(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    return m ? m->reserve : 0;
}

/*=====================================================================
  Saving and restoring where the user put the bars

  A bar the user dragged somewhere is worth remembering, and every part
  of that lives here rather than in a dozen new getters: the manager
  writes a small text blob and reads it back, and the caller only has to
  find somewhere to keep a string.

      CBLAYOUT 1
      bar "Standard" dock=0 row=0 off=0 vis=1 fx=180 fy=240 min=0
      bar "Ribbon" dock=0 row=1 off=0 vis=1 fx=100 fy=100 min=1

  Bars are matched by TITLE, not by id.  An id is creation order, so
  inserting a bar would shift every id after it and silently hand the
  saved position of one bar to another; a title survives that.  A bar in
  the file that no longer exists is ignored, and a bar that exists with
  nothing saved for it keeps whatever the program gave it - so adding and
  removing bars between releases degrades quietly instead of throwing an
  old layout away.
  =====================================================================*/
static std::string CBQuoteTitle(const std::wstring& w)
{
    std::string s = CBToAnsi(w);
    std::string out;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char ch = s[i];
        if (ch == '"' || ch == '\\') out += '\\';
        if (ch == '\r' || ch == '\n') ch = ' ';
        out += ch;
    }
    return out;
}

int CBAPI CB_SaveLayout(HCB cb, char* buf, int cbBuf)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;

    std::string out = "CBLAYOUT 1|";
    char line[512];
    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR) continue;
        _snprintf_s(line, sizeof(line), _TRUNCATE,
                    "bar \"%s\" dock=%d row=%d off=%d vis=%d fx=%d fy=%d min=%d|",
                    CBQuoteTitle(c->title).c_str(), c->dock, c->dockRow,
                    c->dockOffset, c->visible ? 1 : 0, c->floatX, c->floatY,
                    c->minimized ? 1 : 0);
        out += line;
    }

    const int need = (int)out.size() + 1;
    if (buf && cbBuf > 0)
    {
        int n = (need <= cbBuf) ? need : cbBuf;
        memcpy(buf, out.c_str(), n - 1);
        buf[n - 1] = 0;
    }
    return need;
}

static bool CBReadKey(const char* line, const char* key, int* out)
{
    const char* p = strstr(line, key);
    if (!p) return false;
    *out = atoi(p + strlen(key));
    return true;
}

int CBAPI CB_LoadLayout(HCB cb, const char* text)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !text) return 0;
    if (strncmp(text, "CBLAYOUT", 8) != 0) return 0;

    int applied = 0;
    const char* p = text;
    while (*p)
    {
        /*  A pipe, not a newline: the whole point is that this goes in
            ONE INI entry, and an INI entry cannot hold a line break.
            Newlines are accepted as well, so a blob kept somewhere
            roomier still reads back. */
        size_t n = strcspn(p, "|\r\n");
        std::string line(p, n);
        p += n;
        while (*p == '|' || *p == '\r' || *p == '\n') ++p;

        if (line.compare(0, 5, "bar \"") != 0) continue;
        size_t a = 5, b = a;
        std::string title;
        while (b < line.size() && line[b] != '"')
        {
            if (line[b] == '\\' && b + 1 < line.size()) ++b;
            title += line[b++];
        }
        if (b >= line.size()) continue;

        /* the bar with this title */
        CBContainer* bar = NULL;
        std::wstring want = CBToWide(title.c_str());
        std::map<int, CBContainer*>::iterator i;
        for (i = m->containers.begin(); i != m->containers.end(); ++i)
            if (i->second->kind == CBK_BAR && i->second->title == want)
            { bar = i->second; break; }
        if (!bar) continue;                    /* gone since it was saved */

        const char* l = line.c_str();
        int v = 0;
        if (CBReadKey(l, "dock=", &v) && v >= CBD_TOP && v <= CBD_FIXED) bar->dock = v;
        if (CBReadKey(l, "row=",  &v) && v >= 0) bar->dockRow = v;
        if (CBReadKey(l, "off=",  &v) && v >= 0) bar->dockOffset = v;
        if (CBReadKey(l, "vis=",  &v)) bar->visible = (v != 0);
        if (CBReadKey(l, "fx=",   &v)) bar->floatX = v;
        if (CBReadKey(l, "fy=",   &v)) bar->floatY = v;
        if (CBReadKey(l, "min=",  &v) && (bar->style & CBBS_RIBBON)) bar->minimized = (v != 0);
        applied++;
    }

    CBRelayout(m);
    return applied;
}

void CBAPI CB_SetHostReserveBottom(HCB cb, int px)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return;
    m->userResB = (px < 0) ? -1 : px;
    CBRelayout(m);
}

int CBAPI CB_GetHostReserveBottom(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (!m) return 0;
    return (m->userResB >= 0) ? m->userResB : m->hostResB;
}

/*=====================================================================
  Odds and ends
  =====================================================================*/
void CBAPI CB_SetHostMenuVisible(HCB cb, int visible)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !IsWindow(m->parent)) return;

    if (visible)
    {
        if (!m->hostMenu) return;
        SetMenu(m->parent, m->hostMenu);
        m->hostMenu = NULL;
    }
    else
    {
        HMENU h = GetMenu(m->parent);
        if (!h) return;                 /* already off, or never had one */
        m->hostMenu = h;
        SetMenu(m->parent, NULL);
    }
    DrawMenuBar(m->parent);
    CBRelayout(m);                      /* the client area just changed  */
}

int CBAPI CB_GetHostMenuVisible(HCB cb)
{
    CBManager* m = (CBManager*)cb;
    if (!m || !IsWindow(m->parent)) return 0;
    return m->hostMenu ? 0 : (GetMenu(m->parent) ? 1 : 0);
}

void CBAPI CB_GetCursorPos(int* x, int* y)
{
    POINT p;
    p.x = 0;
    p.y = 0;
    GetCursorPos(&p);
    if (x) *x = p.x;
    if (y) *y = p.y;
}
