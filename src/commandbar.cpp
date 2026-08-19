/*=====================================================================
  commandbar.cpp  -  ClaCommandBar public API, manager lifetime and the
                     layout arithmetic.

  Drawing, window procedures and menu tracking live in cb_render.cpp.
  =====================================================================*/
#include "cb_internal.h"

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
}

void CBRelayout(CBManager* m)
{
    if (!m || m->inLayout || m->destroying || !IsWindow(m->parent)) return;
    m->inLayout = true;

    RECT pc;
    GetClientRect(m->parent, &pc);
    int left = 0, top = 0, right = pc.right, bottom = pc.bottom;

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

    for (i = m->containers.begin(); i != m->containers.end(); ++i)
        if (i->second->kind == CBK_BAR && i->second->hwnd)
            InvalidateRect(i->second->hwnd, NULL, FALSE);

    m->inLayout = false;
    if (changed) CBQueue(m, 0, 0, CBE_LAYOUT, 0);
}

/*=====================================================================
  Parent subclass - keeps the bars right even when the host forgets to
  call CB_Layout after a resize.
  =====================================================================*/
static const wchar_t* CBPROP = L"ClaCommandBar.Mgr";

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
    if (!m || m->events.empty()) return 0;
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

    const int contentTop = padY + tabH;

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
