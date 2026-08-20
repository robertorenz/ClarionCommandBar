/*=====================================================================
  cb_render.cpp  -  ClaCommandBar rendering, theming, window procedures
                    and popup-menu tracking.

  Everything device-dependent lives here.  The public API and the
  layout arithmetic are in commandbar.cpp.
  =====================================================================*/
#include "cb_internal.h"
#include <math.h>

ID2D1Factory*       g_d2d  = NULL;
IDWriteFactory*     g_dw   = NULL;
IWICImagingFactory* g_wic  = NULL;
HINSTANCE           g_inst = NULL;
LONG                g_initCount = 0;

static bool g_classesDone = false;

/*=====================================================================
  1.  Strings
  =====================================================================*/
std::wstring CBToWide(const char* s)
{
    if (!s || !*s) return std::wstring();
    int n = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    if (n <= 1) return std::wstring();
    std::wstring w((size_t)(n - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s, -1, &w[0], n);
    return w;
}

std::string CBToAnsi(const std::wstring& w)
{
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    if (n <= 1) return std::string();
    std::string s((size_t)(n - 1), '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &s[0], n, NULL, NULL);
    return s;
}

/* '&File' -> 'File' with underline 0.  '&&' collapses to one '&'. */
std::wstring CBStripAmp(const std::wstring& in, int* underline)
{
    std::wstring out;
    out.reserve(in.size());
    if (underline) *underline = -1;
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == L'&')
        {
            if (i + 1 < in.size() && in[i + 1] == L'&') { out += L'&'; ++i; }
            else if (i + 1 < in.size())
            {
                if (underline && *underline < 0) *underline = (int)out.size();
            }
        }
        else out += in[i];
    }
    return out;
}

/*=====================================================================
  2.  Colour arithmetic
  =====================================================================*/
D2D1_COLOR_F CBColor(COLORREF c)
{
    return D2D1::ColorF(GetRValue(c) / 255.0f,
                        GetGValue(c) / 255.0f,
                        GetBValue(c) / 255.0f, 1.0f);
}

D2D1_COLOR_F CBColorA(COLORREF c, float alpha)
{
    return D2D1::ColorF(GetRValue(c) / 255.0f,
                        GetGValue(c) / 255.0f,
                        GetBValue(c) / 255.0f, alpha);
}

static int ClampB(double v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (int)(v + 0.5);
}

/* t = 0 gives a, t = 1 gives b. */
COLORREF CBBlend(COLORREF a, COLORREF b, float t)
{
    return RGB(ClampB(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t),
               ClampB(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t),
               ClampB(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t));
}

COLORREF CBLighten(COLORREF c, float t) { return CBBlend(c, RGB(255, 255, 255), t); }
COLORREF CBDarken (COLORREF c, float t) { return CBBlend(c, RGB(0, 0, 0), t); }

double CBLuma(COLORREF c)
{
    return 0.2126 * GetRValue(c) + 0.7152 * GetGValue(c) + 0.0722 * GetBValue(c);
}

bool CBIsDark(COLORREF c) { return CBLuma(c) < 128.0; }

/* Nudge fg away from bg until they are at least minDelta apart in
   luma.  Keeps hue, so an accent stays recognisably itself. */
static COLORREF CBContrast(COLORREF fg, COLORREF bg, double minDelta)
{
    double lb = CBLuma(bg);
    bool   goDark = (lb >= 128.0);
    for (int i = 0; i < 20; ++i)
    {
        if (fabs(CBLuma(fg) - lb) >= minDelta) break;
        fg = goDark ? CBDarken(fg, 0.12f) : CBLighten(fg, 0.12f);
    }
    return fg;
}

/*=====================================================================
  3.  Themes

  A theme is a SEED (see cb_internal.h).  Everything else - hover,
  pressed, checked, gutters, captions, tooltips - is derived, which is
  why CB_SetAccent can recolour the whole set coherently in one call.
  =====================================================================*/
void CBGetSeed(int theme, CBSeed* s)
{
    /* the default, and the fallback for an unknown id */
    s->barTop    = RGB(250, 251, 252);
    s->barBot    = RGB(238, 241, 245);
    s->barBorder = RGB(205, 212, 220);
    s->text      = RGB( 32,  38,  45);
    s->textDis   = RGB(150, 157, 165);
    s->accent    = RGB( 43, 105, 168);
    s->menuBack  = RGB(252, 253, 254);
    s->menuBorder= RGB(198, 206, 215);
    s->editBack  = RGB(255, 255, 255);
    s->dark      = false;
    s->corner    = 3;

    switch (theme)
    {
    case CBT_STEELBLUE:
        break;                                  /* the defaults above */

    case CBT_OFFICE2003:
        s->barTop    = RGB(221, 231, 245);
        s->barBot    = RGB(190, 208, 233);
        s->barBorder = RGB(148, 168, 200);
        s->text      = RGB( 20,  30,  50);
        s->textDis   = RGB(150, 155, 165);
        s->accent    = RGB(  0,  84, 166);
        s->menuBack  = RGB(250, 250, 250);
        s->menuBorder= RGB(140, 160, 190);
        s->corner    = 2;
        break;

    case CBT_OFFICE2007:
        s->barTop    = RGB(243, 247, 252);
        s->barBot    = RGB(213, 226, 242);
        s->barBorder = RGB(158, 178, 205);
        s->text      = RGB( 28,  38,  52);
        s->textDis   = RGB(155, 160, 168);
        s->accent    = RGB(224, 158,  58);      /* the classic amber  */
        s->menuBack  = RGB(250, 252, 254);
        s->menuBorder= RGB(150, 172, 200);
        s->corner    = 3;
        break;

    case CBT_OFFICE2010:
        s->barTop    = RGB(246, 246, 246);
        s->barBot    = RGB(229, 230, 232);
        s->barBorder = RGB(196, 197, 200);
        s->text      = RGB( 40,  40,  44);
        s->textDis   = RGB(160, 160, 164);
        s->accent    = RGB( 70, 130, 180);
        s->menuBack  = RGB(253, 253, 253);
        s->menuBorder= RGB(188, 189, 192);
        s->corner    = 2;
        break;

    case CBT_OFFICE2013:
        s->barTop    = RGB(255, 255, 255);
        s->barBot    = RGB(255, 255, 255);
        s->barBorder = RGB(212, 212, 212);
        s->text      = RGB( 50,  50,  50);
        s->textDis   = RGB(168, 168, 168);
        s->accent    = RGB( 43,  87, 154);
        s->menuBack  = RGB(255, 255, 255);
        s->menuBorder= RGB(199, 199, 199);
        s->corner    = 0;
        break;

    case CBT_OFFICE2016:
        s->barTop    = RGB(245, 246, 248);
        s->barBot    = RGB(237, 239, 243);
        s->barBorder = RGB(213, 216, 221);
        s->text      = RGB( 38,  42,  48);
        s->textDis   = RGB(160, 164, 170);
        s->accent    = RGB( 31,  90, 150);
        s->menuBack  = RGB(252, 252, 253);
        s->menuBorder= RGB(206, 210, 216);
        s->corner    = 2;
        break;

    case CBT_VS2012LIGHT:
        s->barTop    = RGB(245, 245, 245);
        s->barBot    = RGB(245, 245, 245);
        s->barBorder = RGB(204, 204, 204);
        s->text      = RGB( 30,  30,  30);
        s->textDis   = RGB(160, 160, 160);
        s->accent    = RGB(  0, 122, 204);
        s->menuBack  = RGB(246, 246, 246);
        s->menuBorder= RGB(204, 204, 204);
        s->corner    = 0;
        break;

    case CBT_VS2012DARK:
        s->barTop    = RGB( 45,  45,  48);
        s->barBot    = RGB( 45,  45,  48);
        s->barBorder = RGB( 63,  63,  70);
        s->text      = RGB(241, 241, 241);
        s->textDis   = RGB(109, 109, 109);
        s->accent    = RGB(  0, 122, 204);
        s->menuBack  = RGB( 27,  27,  28);
        s->menuBorder= RGB( 51,  51,  55);
        s->editBack  = RGB( 37,  37,  38);
        s->dark      = true;
        s->corner    = 0;
        break;

    case CBT_WIN11LIGHT:
        s->barTop    = RGB(249, 249, 249);
        s->barBot    = RGB(243, 243, 243);
        s->barBorder = RGB(229, 229, 229);
        s->text      = RGB( 26,  26,  26);
        s->textDis   = RGB(160, 160, 160);
        s->accent    = RGB(  0,  95, 184);
        s->menuBack  = RGB(249, 249, 249);
        s->menuBorder= RGB(222, 222, 222);
        s->corner    = 5;
        break;

    case CBT_WIN11DARK:
        s->barTop    = RGB( 43,  43,  43);
        s->barBot    = RGB( 39,  39,  39);
        s->barBorder = RGB( 58,  58,  58);
        s->text      = RGB(240, 240, 240);
        s->textDis   = RGB(130, 130, 130);
        s->accent    = RGB( 76, 148, 222);
        s->menuBack  = RGB( 44,  44,  44);
        s->menuBorder= RGB( 64,  64,  64);
        s->editBack  = RGB( 56,  56,  56);
        s->dark      = true;
        s->corner    = 5;
        break;

    case CBT_SLATEDARK:
        s->barTop    = RGB( 38,  45,  54);
        s->barBot    = RGB( 32,  38,  46);
        s->barBorder = RGB( 56,  66,  78);
        s->text      = RGB(226, 232, 240);
        s->textDis   = RGB(118, 130, 145);
        s->accent    = RGB( 82, 148, 214);
        s->menuBack  = RGB( 35,  42,  51);
        s->menuBorder= RGB( 58,  68,  80);
        s->editBack  = RGB( 27,  33,  40);
        s->dark      = true;
        s->corner    = 4;
        break;
    }
}

void CBBuildPalette(const CBSeed& s, COLORREF* c)
{
    const float hotT   = s.dark ? 0.30f : 0.16f;
    const float hotB   = s.dark ? 0.55f : 0.40f;
    const float prsT   = s.dark ? 0.50f : 0.30f;
    const float prsB   = s.dark ? 0.78f : 0.58f;
    const float chkT   = s.dark ? 0.42f : 0.24f;
    const float chkB   = s.dark ? 0.72f : 0.52f;

    c[CBC_BARBACK]        = s.barTop;
    c[CBC_BARBACK2]       = s.barBot;
    c[CBC_BARBORDER]      = s.barBorder;

    c[CBC_ITEMTEXT]       = s.text;
    c[CBC_ITEMTEXTDIS]    = s.textDis;
    c[CBC_HOTBACK]        = CBBlend(s.barTop, s.accent, hotT);
    c[CBC_HOTBORDER]      = CBBlend(s.barTop, s.accent, hotB);
    c[CBC_PRESSBACK]      = CBBlend(s.barTop, s.accent, prsT);
    c[CBC_PRESSBORDER]    = CBBlend(s.barTop, s.accent, prsB);
    c[CBC_CHECKBACK]      = CBBlend(s.barTop, s.accent, chkT);
    c[CBC_CHECKBORDER]    = CBBlend(s.barTop, s.accent, chkB);
    c[CBC_ITEMTEXTHOT]    = CBContrast(s.text, c[CBC_HOTBACK], 90.0);

    c[CBC_SEPARATOR]      = CBBlend(s.barTop, s.text, 0.22f);
    c[CBC_GRIPPER]        = CBBlend(s.barTop, s.text, 0.34f);

    c[CBC_MENUBACK]       = s.menuBack;
    c[CBC_MENUBORDER]     = s.menuBorder;
    c[CBC_MENUGUTTER]     = s.dark ? CBLighten(s.menuBack, 0.06f)
                                   : CBDarken (s.menuBack, 0.045f);
    c[CBC_MENUHOT]        = CBBlend(s.menuBack, s.accent, hotT);
    c[CBC_MENUHOTBORDER]  = CBBlend(s.menuBack, s.accent, hotB);
    c[CBC_MENUTEXT]       = CBContrast(s.text, s.menuBack, 90.0);
    c[CBC_MENUTEXTHOT]    = CBContrast(s.text, c[CBC_MENUHOT], 90.0);
    c[CBC_MENUTEXTDIS]    = s.textDis;
    c[CBC_MENUSHORTCUT]   = CBBlend(c[CBC_MENUTEXT], s.menuBack, 0.42f);
    c[CBC_MENUSEP]        = CBBlend(s.menuBack, s.text, 0.20f);
    c[CBC_MENUCHECK]      = CBContrast(s.accent, s.menuBack, 70.0);

    c[CBC_CAPTIONBACK]    = CBBlend(s.barTop, s.accent, s.dark ? 0.38f : 0.24f);
    c[CBC_CAPTIONTEXT]    = CBContrast(s.text, c[CBC_CAPTIONBACK], 95.0);
    c[CBC_FLOATBORDER]    = CBBlend(s.barBorder, s.accent, 0.28f);
    c[CBC_CHEVRON]        = CBContrast(s.text, s.barTop, 80.0);

    c[CBC_EDITBACK]       = s.editBack;
    c[CBC_EDITBORDER]     = CBBlend(s.editBack, s.text, s.dark ? 0.38f : 0.30f);
    c[CBC_EDITTEXT]       = CBContrast(s.text, s.editBack, 95.0);
    c[CBC_EDITSEL]        = CBBlend(s.editBack, s.accent, 0.45f);

    c[CBC_TIPBACK]        = s.dark ? RGB(52, 52, 55) : RGB(255, 255, 255);
    c[CBC_TIPBORDER]      = CBBlend(c[CBC_TIPBACK], s.text, 0.34f);
    c[CBC_TIPTEXT]        = CBContrast(s.text, c[CBC_TIPBACK], 95.0);

    c[CBC_ACCENT]         = s.accent;
}

void CBApplyTheme(CBManager* m, int theme)
{
    if (theme == CBT_CUSTOM) { m->theme = theme; return; }
    CBGetSeed(theme, &m->seed);
    CBBuildPalette(m->seed, m->col);
    m->metric[CBM_CORNER] = m->seed.corner;
    m->theme = theme;
}

void CBApplyAccent(CBManager* m, COLORREF accent)
{
    m->seed.accent = accent;
    CBBuildPalette(m->seed, m->col);
}

/*=====================================================================
  4.  Fonts and text
  =====================================================================*/
static void ReleaseFormat(CBFont& f)
{
    if (f.fmt)     { f.fmt->Release();     f.fmt = NULL; }
    if (f.fmtBold) { f.fmtBold->Release(); f.fmtBold = NULL; }
}

void CBReleaseFonts(CBManager* m)
{
    for (int i = 1; i <= 4; ++i) ReleaseFormat(m->font[i]);
}

IDWriteTextFormat* CBGetFormat(CBManager* m, int slot, bool bold)
{
    if (slot < 1 || slot > 4) slot = CBF_ITEM;
    CBFont& f = m->font[slot];
    IDWriteTextFormat** pp = bold ? &f.fmtBold : &f.fmt;
    if (*pp) return *pp;
    if (!g_dw) return NULL;

    const wchar_t* face = f.face.empty() ? L"Segoe UI" : f.face.c_str();
    float sizeDip = (float)f.sizePt * 96.0f / 72.0f * m->dpiScale;
    if (sizeDip < 4.0f) sizeDip = 4.0f;

    DWRITE_FONT_WEIGHT w = (bold || f.bold) ? DWRITE_FONT_WEIGHT_SEMI_BOLD
                                            : DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE  st = f.italic ? DWRITE_FONT_STYLE_ITALIC
                                     : DWRITE_FONT_STYLE_NORMAL;

    HRESULT hr = g_dw->CreateTextFormat(face, NULL, w, st,
                                        DWRITE_FONT_STRETCH_NORMAL,
                                        sizeDip, L"", pp);
    if (FAILED(hr) && wcscmp(face, L"Segoe UI") != 0)
        hr = g_dw->CreateTextFormat(L"Segoe UI", NULL, w, st,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    sizeDip, L"", pp);
    if (FAILED(hr)) return NULL;

    (*pp)->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return *pp;
}

/* One line, laid out.  The caller releases. */
static IDWriteTextLayout* MakeLayout(CBManager* m, int slot, bool bold,
                                     const std::wstring& s, float maxW,
                                     float maxH, int underline,
                                     DWRITE_TEXT_ALIGNMENT align,
                                     bool trim)
{
    IDWriteTextFormat* fmt = CBGetFormat(m, slot, bold);
    if (!fmt) return NULL;
    IDWriteTextLayout* lay = NULL;
    if (FAILED(g_dw->CreateTextLayout(s.c_str(), (UINT32)s.size(), fmt,
                                      maxW, maxH, &lay)) || !lay)
        return NULL;

    lay->SetTextAlignment(align);
    lay->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (underline >= 0 && underline < (int)s.size())
    {
        DWRITE_TEXT_RANGE r;
        r.startPosition = (UINT32)underline;
        r.length        = 1;
        lay->SetUnderline(TRUE, r);
    }
    if (trim)
    {
        DWRITE_TRIMMING t;
        t.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
        t.delimiter = 0;
        t.delimiterCount = 0;
        IDWriteInlineObject* sign = NULL;
        if (SUCCEEDED(g_dw->CreateEllipsisTrimmingSign(fmt, &sign)) && sign)
        {
            lay->SetTrimming(&t, sign);
            sign->Release();
        }
    }
    return lay;
}

void CBMeasure(CBManager* m, int slot, bool bold, const std::wstring& text,
               float* w, float* h)
{
    if (w) *w = 0;
    if (h) *h = 0;
    if (text.empty())
    {
        if (h) *h = (float)CBFontHeight(m, slot);
        return;
    }
    IDWriteTextLayout* lay = MakeLayout(m, slot, bold, text, 100000.0f,
                                        1000.0f, -1,
                                        DWRITE_TEXT_ALIGNMENT_LEADING, false);
    if (!lay) return;
    DWRITE_TEXT_METRICS tm;
    if (SUCCEEDED(lay->GetMetrics(&tm)))
    {
        if (w) *w = tm.widthIncludingTrailingWhitespace;
        if (h) *h = tm.height;
    }
    lay->Release();
}

int CBFontHeight(CBManager* m, int slot)
{
    float h = 0;
    CBMeasure(m, slot, false, L"Wg", NULL, &h);
    if (h < 8) h = 12.0f * m->dpiScale;
    return (int)(h + 0.5f);
}

/* Draw one line inside rc, vertically centred. */
static void DrawLine1(CBContainer* c, int slot, bool bold,
                      const std::wstring& s, int underline,
                      const RECT& rc, COLORREF color,
                      DWRITE_TEXT_ALIGNMENT align, bool trim)
{
    if (s.empty() || !c->rt || !c->brush) return;
    float w = (float)(rc.right - rc.left);
    float h = (float)(rc.bottom - rc.top);
    if (w <= 0 || h <= 0) return;

    IDWriteTextLayout* lay = MakeLayout(c->mgr, slot, bold, s, w, h,
                                        underline, align, trim);
    if (!lay) return;
    c->brush->SetColor(CBColor(color));
    c->rt->DrawTextLayout(D2D1::Point2F((float)rc.left, (float)rc.top),
                          lay, c->brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    lay->Release();
}

/*=====================================================================
  5.  Images
  =====================================================================*/
static bool WicToImage(IWICBitmapSource* src, CBImage* out)
{
    if (!g_wic || !src) return false;
    IWICFormatConverter* conv = NULL;
    if (FAILED(g_wic->CreateFormatConverter(&conv)) || !conv) return false;

    bool ok = false;
    if (SUCCEEDED(conv->Initialize(src, GUID_WICPixelFormat32bppPBGRA,
                                   WICBitmapDitherTypeNone, NULL, 0.0,
                                   WICBitmapPaletteTypeMedianCut)))
    {
        UINT w = 0, h = 0;
        conv->GetSize(&w, &h);
        if (w && h && w < 4096 && h < 4096)
        {
            out->w = w;
            out->h = h;
            out->px.resize((size_t)w * h * 4);
            ok = SUCCEEDED(conv->CopyPixels(NULL, w * 4,
                                            (UINT)out->px.size(), &out->px[0]));
        }
    }
    conv->Release();
    return ok;
}

/*  Where a Clarion app actually keeps its icons.

    An image is named in the template by file name alone - "NEW.ICO" - and
    the raw name only opens if it happens to sit in the CURRENT DIRECTORY,
    which is wherever the app was started from rather than where it lives.
    So a bare name is looked for next to the EXE and in an images folder
    beside it as well.  A name with a path in it is taken as given. */
static bool CBTryOpenImage(const wchar_t* file, IWICBitmapDecoder** dec)
{
    return SUCCEEDED(g_wic->CreateDecoderFromFilename(
               file, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, dec))
           && *dec;
}

static bool CBOpenImageFile(const wchar_t* file, IWICBitmapDecoder** dec)
{
    *dec = NULL;
    if (CBTryOpenImage(file, dec)) return true;

    if (wcschr(file, L'\\') || wcschr(file, L'/') || wcschr(file, L':'))
        return false;                       /* a path was given - trust it */

    wchar_t exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (!n || n >= MAX_PATH) return false;
    wchar_t* slash = wcsrchr(exe, L'\\');
    if (!slash) return false;
    *(slash + 1) = 0;

    std::wstring beside = std::wstring(exe) + file;
    if (CBTryOpenImage(beside.c_str(), dec)) return true;

    std::wstring inImages = std::wstring(exe) + L"images\\" + file;
    if (CBTryOpenImage(inImages.c_str(), dec)) return true;

    return false;
}

/*  An image ADDED TO THE CLARION PROJECT is linked into the EXE, not left
    on disk, so there is no file to open at all.  Clarion names those
    resources after the file: NEW.ICO goes in as an RT_GROUP_ICON called
    NEW_ICO - upper case, the dot turned into an underscore.  Read off a
    real application:

        CLOSED_ICO  DELETE_ICO  EDIT_ICO  FIND_ICO  HELP_ICO
        INSERT_ICO  OPEN_ICO    VCRFIRST_ICO  VCRNEXT_ICO  MARK_ICO

    which is why a plain "NEW.ICO" in the template is enough: add the icon
    to the project and it is found here whatever the working directory. */
static bool CBLoadResourceImage(const wchar_t* file, CBImage* out)
{
    const wchar_t* name = file;
    for (const wchar_t* p = file; *p; ++p)
        if (*p == L'\\' || *p == L'/' || *p == L':') name = p + 1;
    if (!*name) return false;

    std::wstring res;
    bool icon = false;
    for (const wchar_t* p = name; *p; ++p)
    {
        wchar_t c = *p;
        if (c == L'.') c = L'_';
        res += (wchar_t)towupper(c);
    }
    if (res.size() > 4 && res.compare(res.size() - 4, 4, L"_ICO") == 0) icon = true;

    HMODULE app = GetModuleHandleW(NULL);
    if (!app) return false;
    /* RT_GROUP_ICON / RT_BITMAP are ANSI macros - spell them wide. */
    LPCWSTR kind = icon ? MAKEINTRESOURCEW(14) : MAKEINTRESOURCEW(2);
    if (!FindResourceW(app, res.c_str(), kind))
        return false;

    HANDLE h = LoadImageW(app, res.c_str(),
                          icon ? IMAGE_ICON : IMAGE_BITMAP,
                          0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!h) return false;
    return CBDecodeHandle(h, icon, out);
}

bool CBDecodeFile(const wchar_t* file, CBImage* out)
{
    if (!g_wic || !file || !*file) return false;
    IWICBitmapDecoder* dec = NULL;
    if (!CBOpenImageFile(file, &dec))
        return CBLoadResourceImage(file, out);
    IWICBitmapFrameDecode* frame = NULL;
    bool ok = false;
    if (SUCCEEDED(dec->GetFrame(0, &frame)) && frame)
    {
        ok = WicToImage(frame, out);
        frame->Release();
    }
    dec->Release();
    return ok;
}

bool CBDecodeStrip(const wchar_t* file, int cx, std::vector<CBImage>* out)
{
    CBImage whole;
    if (!CBDecodeFile(file, &whole)) return false;
    if (cx <= 0) cx = (int)whole.h;
    if (cx <= 0) return false;

    int n = (int)whole.w / cx;
    if (n < 1) return false;

    for (int i = 0; i < n; ++i)
    {
        CBImage part;
        part.w = (UINT)cx;
        part.h = whole.h;
        part.px.resize((size_t)cx * whole.h * 4);
        for (UINT y = 0; y < whole.h; ++y)
        {
            const BYTE* srcRow = &whole.px[(size_t)y * whole.w * 4 + (size_t)i * cx * 4];
            BYTE*       dstRow = &part.px[(size_t)y * cx * 4];
            memcpy(dstRow, srcRow, (size_t)cx * 4);
        }
        out->push_back(part);
    }
    return true;
}

bool CBDecodeHandle(HANDLE h, bool isIcon, CBImage* out)
{
    if (!g_wic || !h) return false;
    IWICBitmap* bmp = NULL;
    HRESULT hr;
    if (isIcon)
        hr = g_wic->CreateBitmapFromHICON((HICON)h, &bmp);
    else
        hr = g_wic->CreateBitmapFromHBITMAP((HBITMAP)h, NULL,
                                            WICBitmapUsePremultipliedAlpha, &bmp);
    if (FAILED(hr) || !bmp) return false;
    bool ok = WicToImage(bmp, out);
    bmp->Release();
    return ok;
}

/* Scale src down to size x size with WIC's Fant filter.  This matters:
   toolbar art is usually 32px line drawings, and letting D2D bilinearly
   halve them at draw time turns every one-pixel stroke into pale grey.
   Fant is a proper windowed filter and holds the contrast. */
static bool ScaleImage(const CBImage& src, int size, CBImage* out)
{
    if (!g_wic || !size || src.px.empty()) return false;
    if ((int)src.w == size && (int)src.h == size) return false;  /* nothing to do */

    IWICBitmap* mem = NULL;
    if (FAILED(g_wic->CreateBitmapFromMemory(src.w, src.h,
                                             GUID_WICPixelFormat32bppPBGRA,
                                             src.w * 4, (UINT)src.px.size(),
                                             (BYTE*)&src.px[0], &mem)) || !mem)
        return false;

    IWICBitmapScaler* sc = NULL;
    bool ok = false;
    if (SUCCEEDED(g_wic->CreateBitmapScaler(&sc)) && sc)
    {
        if (SUCCEEDED(sc->Initialize(mem, (UINT)size, (UINT)size,
                                     WICBitmapInterpolationModeFant)))
        {
            out->w = (UINT)size;
            out->h = (UINT)size;
            out->px.resize((size_t)size * size * 4);
            ok = SUCCEEDED(sc->CopyPixels(NULL, size * 4,
                                          (UINT)out->px.size(), &out->px[0]));
        }
        sc->Release();
    }
    mem->Release();
    return ok;
}

ID2D1Bitmap* CBGetBitmap(CBContainer* c, int image, int size)
{
    CBManager* m = c->mgr;
    if (!c->rt || image < 1 || image >= (int)m->images.size()) return NULL;
    if (size < 1) size = 16;

    /* One container draws at one icon size, so the whole cache is keyed
       on it: change the size and the cache is rebuilt. */
    if (c->bmpSize != size)
    {
        CBDiscardBitmaps(c);
        c->bmpSize = size;
    }
    if ((int)c->bmp.size() <= image) c->bmp.resize(m->images.size(), NULL);
    if (c->bmp[image]) return c->bmp[image];

    CBImage& img = m->images[image];
    if (!img.w || !img.h || img.px.empty()) return NULL;

    CBImage scaled;
    const CBImage* use = &img;
    if (ScaleImage(img, size, &scaled)) use = &scaled;

    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f);
    ID2D1Bitmap* b = NULL;
    if (FAILED(c->rt->CreateBitmap(D2D1::SizeU(use->w, use->h), &use->px[0],
                                   use->w * 4, bp, &b)))
        return NULL;
    c->bmp[image] = b;
    return b;
}

void CBDiscardBitmaps(CBContainer* c)
{
    for (size_t i = 0; i < c->bmp.size(); ++i)
        if (c->bmp[i]) { c->bmp[i]->Release(); c->bmp[i] = NULL; }
    c->bmp.clear();
    c->bmpSize = 0;
}

/*=====================================================================
  6.  Render targets
  =====================================================================*/
bool CBEnsureRT(CBContainer* c)
{
    if (!g_d2d || !c->hwnd) return false;
    if (c->rt) return true;

    RECT rc;
    GetClientRect(c->hwnd, &rc);
    UINT w = (UINT)(rc.right - rc.left);
    UINT h = (UINT)(rc.bottom - rc.top);
    if (!w) w = 1;
    if (!h) h = 1;

    HRESULT hr = g_d2d->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(c->hwnd, D2D1::SizeU(w, h),
                                         D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &c->rt);
    if (FAILED(hr) || !c->rt) { c->rt = NULL; return false; }

    /* One DIP = one pixel: every metric below is in real pixels. */
    c->rt->SetDpi(96.0f, 96.0f);
    if (FAILED(c->rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                            &c->brush)))
    {
        c->rt->Release();
        c->rt = NULL;
        return false;
    }
    return true;
}

void CBDiscardRT(CBContainer* c)
{
    CBDiscardBitmaps(c);
    if (c->brush) { c->brush->Release(); c->brush = NULL; }
    if (c->rt)    { c->rt->Release();    c->rt = NULL; }
}

/*=====================================================================
  7.  Primitive drawing helpers
  =====================================================================*/
static D2D1_RECT_F RF(const RECT& r)
{
    return D2D1::RectF((float)r.left, (float)r.top,
                       (float)r.right, (float)r.bottom);
}

/* A crisp 1px outline sits on half-pixel centres. */
static D2D1_RECT_F RFEdge(const RECT& r)
{
    return D2D1::RectF((float)r.left + 0.5f, (float)r.top + 0.5f,
                       (float)r.right - 0.5f, (float)r.bottom - 0.5f);
}

static void FillBox(CBContainer* c, const RECT& r, COLORREF col, float radius)
{
    c->brush->SetColor(CBColor(col));
    if (radius > 0.5f)
        c->rt->FillRoundedRectangle(D2D1::RoundedRect(RF(r), radius, radius),
                                    c->brush);
    else
        c->rt->FillRectangle(RF(r), c->brush);
}

static void StrokeBox(CBContainer* c, const RECT& r, COLORREF col, float radius)
{
    c->brush->SetColor(CBColor(col));
    if (radius > 0.5f)
        c->rt->DrawRoundedRectangle(D2D1::RoundedRect(RFEdge(r), radius, radius),
                                    c->brush, 1.0f);
    else
        c->rt->DrawRectangle(RFEdge(r), c->brush, 1.0f);
}

static void HLine(CBContainer* c, int x1, int x2, int y, COLORREF col)
{
    c->brush->SetColor(CBColor(col));
    c->rt->DrawLine(D2D1::Point2F((float)x1, y + 0.5f),
                    D2D1::Point2F((float)x2, y + 0.5f), c->brush, 1.0f);
}

static void VLine(CBContainer* c, int x, int y1, int y2, COLORREF col)
{
    c->brush->SetColor(CBColor(col));
    c->rt->DrawLine(D2D1::Point2F(x + 0.5f, (float)y1),
                    D2D1::Point2F(x + 0.5f, (float)y2), c->brush, 1.0f);
}

/* dir: 0 = down, 1 = right, 2 = up, 3 = left.  Drawn as two strokes,
   which stays sharp at any DPI - a filled triangle does not. */
static void DrawArrow(CBContainer* c, float cx, float cy, float size,
                      COLORREF col, int dir)
{
    c->brush->SetColor(CBColor(col));
    float s = size * 0.5f;
    float w = size * 0.28f;
    D2D1_POINT_2F a, b, d;
    switch (dir)
    {
    case 1:  a = D2D1::Point2F(cx - w, cy - s); b = D2D1::Point2F(cx + w, cy);
             d = D2D1::Point2F(cx - w, cy + s); break;
    case 2:  a = D2D1::Point2F(cx - s, cy + w); b = D2D1::Point2F(cx, cy - w);
             d = D2D1::Point2F(cx + s, cy + w); break;
    case 3:  a = D2D1::Point2F(cx + w, cy - s); b = D2D1::Point2F(cx - w, cy);
             d = D2D1::Point2F(cx + w, cy + s); break;
    default: a = D2D1::Point2F(cx - s, cy - w); b = D2D1::Point2F(cx, cy + w);
             d = D2D1::Point2F(cx + s, cy - w); break;
    }
    float sw = size <= 8.0f ? 1.3f : 1.5f;
    c->rt->DrawLine(a, b, c->brush, sw);
    c->rt->DrawLine(b, d, c->brush, sw);
}

static void DrawCheck(CBContainer* c, const RECT& box, COLORREF col)
{
    c->brush->SetColor(CBColor(col));
    float w = (float)(box.right - box.left);
    float x = (float)box.left, y = (float)box.top;
    float sw = w < 14.0f ? 1.6f : 2.0f;
    c->rt->DrawLine(D2D1::Point2F(x + w * 0.22f, y + w * 0.52f),
                    D2D1::Point2F(x + w * 0.42f, y + w * 0.72f), c->brush, sw);
    c->rt->DrawLine(D2D1::Point2F(x + w * 0.42f, y + w * 0.72f),
                    D2D1::Point2F(x + w * 0.78f, y + w * 0.28f), c->brush, sw);
}

static void DrawRadioDot(CBContainer* c, const RECT& box, COLORREF col)
{
    c->brush->SetColor(CBColor(col));
    float cx = (box.left + box.right) * 0.5f;
    float cy = (box.top + box.bottom) * 0.5f;
    float r  = (box.right - box.left) * 0.18f;
    c->rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), c->brush);
}

static void DrawImageAt(CBContainer* c, int image, int x, int y, int size,
                        bool enabled)
{
    ID2D1Bitmap* b = CBGetBitmap(c, image, size);
    if (!b) return;
    D2D1_RECT_F dst = D2D1::RectF((float)x, (float)y,
                                  (float)(x + size), (float)(y + size));
    c->rt->DrawBitmap(b, dst, enabled ? 1.0f : 0.32f,
                      D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

static void DrawGripper(CBContainer* c, const RECT& r, bool vertical,
                        COLORREF col)
{
    c->brush->SetColor(CBColor(col));
    if (vertical)
    {
        float y = (r.top + r.bottom) * 0.5f - 1.5f;
        for (int i = 0; i < 2; ++i)
            c->rt->FillRectangle(
                D2D1::RectF((float)r.left + 2, y + i * 3.0f,
                            (float)r.right - 2, y + i * 3.0f + 1.5f), c->brush);
    }
    else
    {
        float x = (r.left + r.right) * 0.5f - 1.5f;
        for (int i = 0; i < 2; ++i)
            c->rt->FillRectangle(
                D2D1::RectF(x + i * 3.0f, (float)r.top + 2,
                            x + i * 3.0f + 1.5f, (float)r.bottom - 2), c->brush);
    }
}

/*=====================================================================
  8.  Painting a bar
  =====================================================================*/
static void PaintBarBackground(CBManager* m, CBContainer* c, const RECT& rc)
{
    COLORREF t = m->col[CBC_BARBACK], b = m->col[CBC_BARBACK2];
    bool vertical = (c->dock == CBD_LEFT || c->dock == CBD_RIGHT);

    if (t == b)
    {
        c->rt->Clear(CBColor(t));
        return;
    }
    c->rt->Clear(CBColor(t));

    D2D1_GRADIENT_STOP g[2];
    g[0].position = 0.0f; g[0].color = CBColor(t);
    g[1].position = 1.0f; g[1].color = CBColor(b);

    ID2D1GradientStopCollection* stops = NULL;
    if (FAILED(c->rt->CreateGradientStopCollection(g, 2, D2D1_GAMMA_2_2,
                                                   D2D1_EXTEND_MODE_CLAMP,
                                                   &stops)) || !stops)
        return;

    D2D1_POINT_2F p0 = D2D1::Point2F(0, 0);
    D2D1_POINT_2F p1 = vertical ? D2D1::Point2F((float)rc.right, 0)
                                : D2D1::Point2F(0, (float)rc.bottom);

    ID2D1LinearGradientBrush* lg = NULL;
    if (SUCCEEDED(c->rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(p0, p1), stops, &lg)) && lg)
    {
        c->rt->FillRectangle(RF(rc), lg);
        lg->Release();
    }
    stops->Release();
}

/* The visual state of one item, resolved once. */
struct ItemState { bool hot, hotArrow, pressed, pressArrow, open, disabled; };

static ItemState ResolveState(CBContainer* c, CBItem* it)
{
    ItemState s;
    s.disabled   = !it->enabled;
    s.hot        = (c->hotItem == it->id) && !s.disabled;
    s.hotArrow   = s.hot && c->hotZone == CBHIT_ARROW;
    s.pressed    = (c->pressItem == it->id) && !s.disabled;
    s.pressArrow = s.pressed && c->pressZone == CBHIT_ARROW;
    s.open       = (c->openItem == it->id);
    return s;
}

static void DrawItemBackground(CBManager* m, CBContainer* c, CBItem* it,
                               const ItemState& st, float radius)
{
    if (st.disabled) return;

    bool active = st.pressed || st.open;
    if (active)
    {
        if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
        {
            /* The two halves light up independently, exactly like a
               Codejock split button. */
            RECT cmd = it->rc; cmd.right = it->arrow.left;
            const RECT& arr = it->arrow;
            bool arrowSide = st.pressArrow || st.open;
            FillBox(c, arrowSide ? arr : cmd,
                    m->col[CBC_PRESSBACK], radius);
            FillBox(c, arrowSide ? cmd : arr,
                    m->col[CBC_HOTBACK], radius);
            StrokeBox(c, it->rc, m->col[CBC_PRESSBORDER], radius);
            VLine(c, it->arrow.left, it->rc.top + 3, it->rc.bottom - 3,
                  m->col[CBC_PRESSBORDER]);
            return;
        }
        FillBox(c, it->rc, m->col[CBC_PRESSBACK], radius);
        StrokeBox(c, it->rc, m->col[CBC_PRESSBORDER], radius);
        return;
    }
    if (it->checked)
    {
        FillBox(c, it->rc, st.hot ? m->col[CBC_PRESSBACK] : m->col[CBC_CHECKBACK],
                radius);
        StrokeBox(c, it->rc, m->col[CBC_CHECKBORDER], radius);
        return;
    }
    if (st.hot)
    {
        if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
        {
            RECT cmd = it->rc; cmd.right = it->arrow.left;
            FillBox(c, st.hotArrow ? it->arrow : cmd, m->col[CBC_HOTBACK], radius);
            StrokeBox(c, it->rc, m->col[CBC_HOTBORDER], radius);
            VLine(c, it->arrow.left, it->rc.top + 3, it->rc.bottom - 3,
                  m->col[CBC_HOTBORDER]);
            return;
        }
        FillBox(c, it->rc, m->col[CBC_HOTBACK], radius);
        StrokeBox(c, it->rc, m->col[CBC_HOTBORDER], radius);
    }
}

static void DrawBarItem(CBManager* m, CBContainer* c, CBItem* it)
{
    if (!it->visible || it->overflow) return;
    if (IsRectEmpty(&it->rc)) return;

    /* On a ribbon the LARGE size is chosen per item, not per bar: an
       image-over-text button is the big one and everything beside it in
       the same group stays small. */
    const bool bigItem = (c->style & CBBS_LARGEICONS) ||
                         ((c->style & CBBS_RIBBON) && (it->style & CBIS_TEXTBELOW));
    const int  icon   = bigItem ? CBMetric(m, CBM_LARGEICON)
                                : CBMetric(m, CBM_ICONSIZE);
    const int  padX   = CBMetric(m, CBM_PADX);
    const bool vert   = (c->dock == CBD_LEFT || c->dock == CBD_RIGHT);
    const float radius= (float)CBMetric(m, CBM_CORNER);
    ItemState  st     = ResolveState(c, it);

    COLORREF textCol = st.disabled ? m->col[CBC_ITEMTEXTDIS]
                     : ((st.hot && (m->style & CBS_HOTTEXT))
                            ? m->col[CBC_ITEMTEXTHOT] : m->col[CBC_ITEMTEXT]);

    switch (it->type)
    {
    case CBI_SEPARATOR:
    {
        int cx = (it->rc.left + it->rc.right) / 2;
        int cy = (it->rc.top + it->rc.bottom) / 2;
        if (vert) HLine(c, it->rc.left + 4, it->rc.right - 4, cy, m->col[CBC_SEPARATOR]);
        else      VLine(c, cx, it->rc.top + 3, it->rc.bottom - 3, m->col[CBC_SEPARATOR]);
        return;
    }
    case CBI_SPACE:
        return;

    case CBI_LABEL:
    {
        RECT tr = it->rc;
        tr.left += padX / 2;
        tr.right -= padX / 2;
        DrawLine1(c, CBF_ITEM, false, it->text, -1, tr, textCol,
                  DWRITE_TEXT_ALIGNMENT_LEADING, true);
        return;
    }
    case CBI_EDIT:
    case CBI_COMBO:
    {
        FillBox(c, it->rc, m->col[CBC_EDITBACK], radius > 0 ? 2.0f : 0.0f);
        StrokeBox(c, it->rc,
                  st.hot ? m->col[CBC_HOTBORDER] : m->col[CBC_EDITBORDER],
                  radius > 0 ? 2.0f : 0.0f);
        RECT tr = it->rc;
        tr.left += 5;
        tr.right -= (it->type == CBI_COMBO) ? 18 : 4;
        /* While the overlay EDIT is up it draws the text itself. */
        if (!(m->editItem == it->id && m->editWnd))
            DrawLine1(c, CBF_ITEM, false, it->value, -1, tr,
                      st.disabled ? m->col[CBC_ITEMTEXTDIS] : m->col[CBC_EDITTEXT],
                      DWRITE_TEXT_ALIGNMENT_LEADING, true);
        if (it->type == CBI_COMBO)
            DrawArrow(c, (float)(it->rc.right - 10),
                      (float)((it->rc.top + it->rc.bottom) / 2), 7.0f,
                      st.disabled ? m->col[CBC_ITEMTEXTDIS] : m->col[CBC_EDITTEXT], 0);
        return;
    }
    default:
        break;
    }

    DrawItemBackground(m, c, it, st, radius);

    /* ---- content ---- */
    RECT content = it->rc;
    if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
        content.right = it->arrow.left;
    content.left  += padX;
    content.right -= padX;

    bool wantIcon = it->image > 0 && !(it->style & CBIS_TEXTONLY);
    bool wantText = !it->text.empty() && !(it->style & CBIS_ICONONLY);

    if (it->type == CBI_CHECKBOX)
    {
        int boxSz = (int)(13 * m->dpiScale);
        RECT bx;
        bx.left   = content.left;
        bx.top    = (it->rc.top + it->rc.bottom) / 2 - boxSz / 2;
        bx.right  = bx.left + boxSz;
        bx.bottom = bx.top + boxSz;
        FillBox(c, bx, m->col[CBC_EDITBACK], 2.0f);
        StrokeBox(c, bx, st.disabled ? m->col[CBC_ITEMTEXTDIS]
                                     : m->col[CBC_EDITBORDER], 2.0f);
        if (it->checked)
            DrawCheck(c, bx, st.disabled ? m->col[CBC_ITEMTEXTDIS]
                                         : m->col[CBC_ACCENT]);
        content.left = bx.right + (int)(5 * m->dpiScale);
        if (wantText)
            DrawLine1(c, CBF_ITEM, false, it->text, it->underline, content,
                      textCol, DWRITE_TEXT_ALIGNMENT_LEADING, true);
        return;
    }

    if (it->style & CBIS_TEXTBELOW)
    {
        int ix = (content.left + content.right) / 2 - icon / 2;
        int iy = it->rc.top + CBMetric(m, CBM_PADY);
        if (wantIcon) DrawImageAt(c, it->image, ix, iy, icon, !st.disabled);
        RECT tr = content;
        tr.top = iy + (wantIcon ? icon + 2 : 0);
        if (wantText)
            DrawLine1(c, CBF_ITEM, false, it->text, it->underline, tr, textCol,
                      DWRITE_TEXT_ALIGNMENT_CENTER, true);
    }
    else
    {
        int ix = content.left;
        int iy = (it->rc.top + it->rc.bottom) / 2 - icon / 2;
        if (wantIcon)
        {
            DrawImageAt(c, it->image, ix, iy, icon, !st.disabled);
            content.left += icon + (wantText ? (int)(5 * m->dpiScale) : 0);
        }
        if (it->type == CBI_COLOR)
        {
            /* The swatch is the point of the item: a solid band under
               the glyph, or the whole glyph slot when there is none. */
            RECT sw;
            sw.left   = wantIcon ? ix : content.left;
            sw.right  = sw.left + icon;
            sw.bottom = iy + icon;
            sw.top    = wantIcon ? sw.bottom - (int)(4 * m->dpiScale) : iy;
            FillBox(c, sw, it->color, 0.0f);
            StrokeBox(c, sw, CBBlend(it->color, m->col[CBC_ITEMTEXT], 0.35f), 0.0f);
            if (!wantIcon) content.left += icon + (wantText ? (int)(5 * m->dpiScale) : 0);
        }
        if (wantText)
            DrawLine1(c, CBF_ITEM, false, it->text, it->underline, content,
                      textCol,
                      (it->type == CBI_MENU && !wantIcon)
                          ? DWRITE_TEXT_ALIGNMENT_CENTER
                          : DWRITE_TEXT_ALIGNMENT_LEADING, true);
    }

    /* ---- the drop arrow ---- */
    if (it->type == CBI_DROPDOWN)
    {
        DrawArrow(c, (float)(it->rc.right - CBMetric(m, CBM_PADX) - 3),
                  (float)((it->rc.top + it->rc.bottom) / 2), 7.0f, textCol, 0);
    }
    else if (it->type == CBI_SPLIT || it->type == CBI_COLOR)
    {
        DrawArrow(c, (float)((it->arrow.left + it->arrow.right) / 2),
                  (float)((it->rc.top + it->rc.bottom) / 2), 7.0f, textCol, 0);
        if (!st.hot && !st.pressed && !st.open && !it->checked)
            VLine(c, it->arrow.left, it->rc.top + 4, it->rc.bottom - 4,
                  m->col[CBC_SEPARATOR]);
    }
}

/*=====================================================================
  Painting a ribbon

  Two surfaces: the tab strip keeps the bar background, and the content
  panel below it is a lighter page that the ACTIVE tab is visually
  joined to - which is the whole trick that makes a ribbon read as
  tabbed pages rather than as a toolbar with buttons above it.
  =====================================================================*/
void CBPaintRibbon(CBManager* m, CBContainer* c)
{
    if (!CBEnsureRT(c)) return;

    RECT rc;
    GetClientRect(c->hwnd, &rc);

    const int tabH  = CBFontHeight(m, CBF_ITEM) + (int)(9 * m->dpiScale);
    const int padY  = CBMetric(m, CBM_BARPADY);
    const int capH  = CBFontHeight(m, CBF_ITEM) + (int)(4 * m->dpiScale);
    const float rad = (float)CBMetric(m, CBM_CORNER);

    c->rt->BeginDraw();
    PaintBarBackground(m, c, rc);

    /* ---- the content page ---- */
    RECT page = rc;
    page.top = padY + tabH;
    FillBox(c, page, m->col[CBC_MENUBACK], 0.0f);
    HLine(c, page.left, page.right, page.top, m->col[CBC_BARBORDER]);
    if (!(c->style & CBBS_NOBORDER))
        HLine(c, rc.left, rc.right, rc.bottom - 1, m->col[CBC_BARBORDER]);

    /* ---- the tab strip ---- */
    for (size_t t = 0; t < c->items.size(); ++t)
    {
        CBContainer* tab = CBFindContainer(m, c->items[t]);
        if (!tab || tab->kind != CBK_TAB) continue;
        if (IsRectEmpty(&tab->tabRc)) continue;

        bool active = (tab->id == c->activeTab);
        bool hot    = (tab->id == c->hotTab) && !active;

        RECT tr = tab->tabRc;
        if (active)
        {
            /* joined to the page: fill down over the page's top line */
            RECT fill = tr;
            fill.bottom = page.top + 1;
            FillBox(c, fill, m->col[CBC_MENUBACK], 0.0f);
            VLine(c, fill.left,      fill.top + 1, fill.bottom, m->col[CBC_BARBORDER]);
            VLine(c, fill.right - 1, fill.top + 1, fill.bottom, m->col[CBC_BARBORDER]);
            HLine(c, fill.left, fill.right, fill.top, m->col[CBC_ACCENT]);
        }
        else if (hot)
        {
            RECT fill = tr;
            InflateRect(&fill, -1, -1);
            FillBox(c, fill, m->col[CBC_HOTBACK], rad);
        }

        DrawLine1(c, CBF_ITEM, active, tab->caption, -1, tr,
                  active ? m->col[CBC_ITEMTEXT]
                         : (hot ? m->col[CBC_ITEMTEXTHOT] : m->col[CBC_ITEMTEXT]),
                  DWRITE_TEXT_ALIGNMENT_CENTER, true);
    }

    /* ---- the collapse button at the end of the strip ---- */
    if (!IsRectEmpty(&c->minRc))
    {
        if (c->hotMin)
            FillBox(c, c->minRc, m->col[CBC_HOTBACK], rad);
        const float cx = (float)((c->minRc.left + c->minRc.right) / 2);
        const float cy = (float)((c->minRc.top + c->minRc.bottom) / 2);
        /*  Pointing up while the ribbon is open - press it and the ribbon
            goes up - and down once it is collapsed. */
        DrawArrow(c, cx, cy, 8.0f,
                  c->hotMin ? m->col[CBC_ITEMTEXTHOT] : m->col[CBC_CHEVRON],
                  c->minimized ? 0 : 2);
    }

    /* ---- the active tab's groups ---- */
    CBContainer* active = CBFindContainer(m, c->activeTab);
    if (active && active->kind == CBK_TAB)
    {
        for (size_t g = 0; g < active->items.size(); ++g)
        {
            CBContainer* grp = CBFindContainer(m, active->items[g]);
            if (!grp || grp->kind != CBK_GROUP) continue;
            if (IsRectEmpty(&grp->groupRc)) continue;

            RECT cap = grp->groupRc;
            cap.top    = cap.bottom - capH;
            cap.bottom = grp->groupRc.bottom - 2;
            DrawLine1(c, CBF_ITEM, false, grp->caption, -1, cap,
                      m->col[CBC_MENUSHORTCUT], DWRITE_TEXT_ALIGNMENT_CENTER, true);

            /* the divider between this group and the next */
            VLine(c, grp->groupRc.right - 1,
                  grp->groupRc.top + (int)(4 * m->dpiScale),
                  grp->groupRc.bottom - capH,
                  m->col[CBC_SEPARATOR]);
        }
    }

    for (size_t i = 0; i < c->laid.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->laid[i]);
        if (it) DrawBarItem(m, c, it);
    }

    HRESULT hr = c->rt->EndDraw();
    if (hr == (HRESULT)D2DERR_RECREATE_TARGET) CBDiscardRT(c);
}

void CBPaintBar(CBManager* m, CBContainer* c)
{
    if (c->style & CBBS_RIBBON) { CBPaintRibbon(m, c); return; }
    if (!CBEnsureRT(c)) return;

    RECT rc;
    GetClientRect(c->hwnd, &rc);

    c->rt->BeginDraw();
    PaintBarBackground(m, c, rc);

    bool floating = (c->dock == CBD_FLOAT);
    int  left = 0, top = 0;

    if (floating)
    {
        int cap = CBMetric(m, CBM_ITEMHEIGHT) > 0
                      ? (int)(17 * m->dpiScale) : (int)(17 * m->dpiScale);
        RECT cr = rc;
        cr.bottom = cr.top + cap;
        FillBox(c, cr, m->col[CBC_CAPTIONBACK], 0.0f);
        RECT tr = cr;
        tr.left += 6;
        tr.right -= 20;
        DrawLine1(c, CBF_CAPTION, false, c->title, -1, tr,
                  m->col[CBC_CAPTIONTEXT], DWRITE_TEXT_ALIGNMENT_LEADING, true);
        /* close box */
        float cx = (float)(rc.right - 11), cy = (float)(cr.top + cap / 2);
        c->brush->SetColor(CBColor(m->col[CBC_CAPTIONTEXT]));
        c->rt->DrawLine(D2D1::Point2F(cx - 3.5f, cy - 3.5f),
                        D2D1::Point2F(cx + 3.5f, cy + 3.5f), c->brush, 1.3f);
        c->rt->DrawLine(D2D1::Point2F(cx + 3.5f, cy - 3.5f),
                        D2D1::Point2F(cx - 3.5f, cy + 3.5f), c->brush, 1.3f);
        StrokeBox(c, rc, m->col[CBC_FLOATBORDER], 0.0f);
    }
    else if (!(c->style & CBBS_NOBORDER))
    {
        switch (c->dock)
        {
        case CBD_TOP:    HLine(c, rc.left, rc.right, rc.bottom - 1, m->col[CBC_BARBORDER]); break;
        case CBD_BOTTOM: HLine(c, rc.left, rc.right, rc.top,        m->col[CBC_BARBORDER]); break;
        case CBD_LEFT:   VLine(c, rc.right - 1, rc.top, rc.bottom,  m->col[CBC_BARBORDER]); break;
        case CBD_RIGHT:  VLine(c, rc.left, rc.top, rc.bottom,       m->col[CBC_BARBORDER]); break;
        }
    }

    if ((c->style & CBBS_GRIPPER) && !floating)
    {
        RECT g = rc;
        bool vert = (c->dock == CBD_LEFT || c->dock == CBD_RIGHT);
        if (vert) { g.bottom = g.top + (int)(8 * m->dpiScale); g.left += 2; g.right -= 2; }
        else      { g.right  = g.left + (int)(8 * m->dpiScale); g.top  += 2; g.bottom -= 2; }
        DrawGripper(c, g, vert, m->col[CBC_GRIPPER]);
    }

    for (size_t i = 0; i < c->laid.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->laid[i]);
        if (it) DrawBarItem(m, c, it);
    }

    if (c->hasChevron)
    {
        bool hot = (c->hotItem == -1);
        if (hot) FillBox(c, c->chevronRc, m->col[CBC_HOTBACK],
                         (float)CBMetric(m, CBM_CORNER));
        float cx = (float)((c->chevronRc.left + c->chevronRc.right) / 2);
        float cy = (float)((c->chevronRc.top + c->chevronRc.bottom) / 2);
        DrawArrow(c, cx, cy - 3.0f, 7.0f, m->col[CBC_CHEVRON], 0);
        DrawArrow(c, cx, cy + 2.0f, 7.0f, m->col[CBC_CHEVRON], 0);
    }

    HRESULT hr = c->rt->EndDraw();
    if (hr == (HRESULT)D2DERR_RECREATE_TARGET) CBDiscardRT(c);
    (void)left; (void)top;
}

/*=====================================================================
  9.  Painting a popup menu
  =====================================================================*/
void CBPaintMenu(CBManager* m, CBContainer* c)
{
    if (!CBEnsureRT(c)) return;

    RECT rc;
    GetClientRect(c->hwnd, &rc);
    const int gutter = c->gutterW;

    c->rt->BeginDraw();
    c->rt->Clear(CBColor(m->col[CBC_MENUBACK]));

    if ((m->style & CBS_MENUICONS) && gutter > 0)
    {
        RECT g = rc;
        g.right = g.left + gutter;
        g.top += 1;
        g.bottom -= 1;
        FillBox(c, g, m->col[CBC_MENUGUTTER], 0.0f);
    }
    StrokeBox(c, rc, m->col[CBC_MENUBORDER], 0.0f);

    const int icon = CBMetric(m, CBM_ICONSIZE);

    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it || !it->visible || IsRectEmpty(&it->rc)) continue;

        if (it->type == CBI_SEPARATOR)
        {
            int y = (it->rc.top + it->rc.bottom) / 2;
            HLine(c, it->rc.left + gutter + 2, it->rc.right - 4, y,
                  m->col[CBC_MENUSEP]);
            continue;
        }

        bool sel = ((int)i == c->selIndex) && it->enabled;
        COLORREF tcol = !it->enabled ? m->col[CBC_MENUTEXTDIS]
                      : (sel ? m->col[CBC_MENUTEXTHOT] : m->col[CBC_MENUTEXT]);

        if (sel)
        {
            RECT hr2 = it->rc;
            InflateRect(&hr2, -2, 0);
            FillBox(c, hr2, m->col[CBC_MENUHOT], (float)CBMetric(m, CBM_CORNER));
            StrokeBox(c, hr2, m->col[CBC_MENUHOTBORDER],
                      (float)CBMetric(m, CBM_CORNER));
        }

        /* gutter: the check mark wins over the image */
        RECT gb;
        gb.left   = it->rc.left + 3;
        gb.top    = (it->rc.top + it->rc.bottom) / 2 - icon / 2;
        gb.right  = gb.left + icon;
        gb.bottom = gb.top + icon;

        if (it->checked)
        {
            RECT box = gb;
            InflateRect(&box, 2, 2);
            FillBox(c, box, m->col[CBC_MENUHOT], 2.0f);
            StrokeBox(c, box, m->col[CBC_MENUCHECK], 2.0f);
            if (it->style & CBIS_RADIO) DrawRadioDot(c, gb, m->col[CBC_MENUCHECK]);
            else                        DrawCheck(c, gb, m->col[CBC_MENUCHECK]);
        }
        else if (it->image > 0)
        {
            DrawImageAt(c, it->image, gb.left, gb.top, icon, it->enabled);
        }

        RECT tr = it->rc;
        tr.left  = it->rc.left + gutter + 6;
        tr.right = it->rc.right - 8 - c->shortcutW - (it->menu ? 14 : 0);
        DrawLine1(c, CBF_MENU, (it->style & CBIS_DEFAULT) != 0, it->text,
                  it->underline, tr, tcol, DWRITE_TEXT_ALIGNMENT_LEADING, true);

        if (!it->shortcut.empty())
        {
            RECT sr = it->rc;
            sr.right = it->rc.right - 8 - (it->menu ? 14 : 0);
            sr.left  = sr.right - c->shortcutW;
            DrawLine1(c, CBF_MENU, false, it->shortcut, -1, sr,
                      it->enabled ? m->col[CBC_MENUSHORTCUT]
                                  : m->col[CBC_MENUTEXTDIS],
                      DWRITE_TEXT_ALIGNMENT_TRAILING, false);
        }
        if (it->menu)
            DrawArrow(c, (float)(it->rc.right - 11),
                      (float)((it->rc.top + it->rc.bottom) / 2), 7.0f, tcol, 1);
    }

    HRESULT hr = c->rt->EndDraw();
    if (hr == (HRESULT)D2DERR_RECREATE_TARGET) CBDiscardRT(c);
}

/*=====================================================================
  10.  Tooltip
  =====================================================================*/
void CBHideTip(CBManager* m)
{
    if (m->tipWnd)
    {
        ShowWindow(m->tipWnd, SW_HIDE);
        m->tipText.clear();
    }
}

void CBShowTip(CBManager* m, CBContainer* c, CBItem* it)
{
    if (!(m->style & CBS_TOOLTIPS)) return;
    std::wstring txt = it->tooltip.empty() ? it->text : it->tooltip;
    if (txt.empty()) return;

    if (!m->tipWnd)
    {
        m->tipWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST |
                                    WS_EX_NOACTIVATE,
                                    CBWC_TIP, L"", WS_POPUP,
                                    0, 0, 10, 10, NULL, NULL, g_inst, NULL);
        if (!m->tipWnd) return;
        SetWindowLongPtrW(m->tipWnd, GWLP_USERDATA, (LONG_PTR)m);
    }
    m->tipText = txt;

    float tw = 0, th = 0;
    CBMeasure(m, CBF_TOOLTIP, false, txt, &tw, &th);
    int w = (int)tw + (int)(16 * m->dpiScale);
    int h = (int)th + (int)(10 * m->dpiScale);

    POINT pt;
    GetCursorPos(&pt);
    pt.y += (int)(20 * m->dpiScale);

    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi))
    {
        if (pt.x + w > mi.rcWork.right)  pt.x = mi.rcWork.right - w;
        if (pt.y + h > mi.rcWork.bottom) pt.y = mi.rcWork.bottom - h - 4;
        if (pt.x < mi.rcWork.left)       pt.x = mi.rcWork.left;
    }
    SetWindowPos(m->tipWnd, HWND_TOPMOST, pt.x, pt.y, w, h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(m->tipWnd, NULL, FALSE);
    (void)c;
}

static void PaintTip(CBManager* m, HWND hwnd)
{
    if (!g_d2d) return;
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (!m->tipRt)
    {
        if (FAILED(g_d2d->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(
                    hwnd, D2D1::SizeU(rc.right, rc.bottom)), &m->tipRt)))
            return;
        m->tipRt->SetDpi(96.0f, 96.0f);
    }
    m->tipRt->Resize(D2D1::SizeU(rc.right, rc.bottom));

    ID2D1SolidColorBrush* br = NULL;
    if (FAILED(m->tipRt->CreateSolidColorBrush(CBColor(m->col[CBC_TIPTEXT]), &br)))
        return;

    m->tipRt->BeginDraw();
    m->tipRt->Clear(CBColor(m->col[CBC_TIPBACK]));
    br->SetColor(CBColor(m->col[CBC_TIPBORDER]));
    m->tipRt->DrawRectangle(RFEdge(rc), br, 1.0f);

    IDWriteTextLayout* lay = MakeLayout(m, CBF_TOOLTIP, false, m->tipText,
                                        (float)rc.right, (float)rc.bottom, -1,
                                        DWRITE_TEXT_ALIGNMENT_CENTER, false);
    if (lay)
    {
        br->SetColor(CBColor(m->col[CBC_TIPTEXT]));
        m->tipRt->DrawTextLayout(D2D1::Point2F(0, 0), lay, br,
                                 D2D1_DRAW_TEXT_OPTIONS_CLIP);
        lay->Release();
    }
    br->Release();
    if (m->tipRt->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET)
    {
        m->tipRt->Release();
        m->tipRt = NULL;
    }
}

LRESULT CALLBACK CBTipProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CBManager* m = (CBManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_PAINT:
        if (m) PaintTip(m, hwnd);
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/*=====================================================================
  11.  Overlay EDIT control for CBI_EDIT items
  =====================================================================*/
static LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CBManager* m = (CBManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!m) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_KEYDOWN:
        if (wp == VK_RETURN) { CBEndEdit(m, true);  return 0; }
        if (wp == VK_ESCAPE) { CBEndEdit(m, false); return 0; }
        break;
    case WM_CHAR:
        /* swallow the beep the default EDIT makes on Enter / Escape */
        if (wp == VK_RETURN || wp == VK_ESCAPE) return 0;
        break;
    case WM_KILLFOCUS:
        CBEndEdit(m, true);
        return 0;
    }
    return CallWindowProcW(m->editOldProc, hwnd, msg, wp, lp);
}

void CBBeginEdit(CBManager* m, CBContainer* c, CBItem* it)
{
    CBEndEdit(m, true);

    RECT r = it->rc;
    InflateRect(&r, -2, -2);

    m->editWnd = CreateWindowExW(0, L"EDIT", it->value.c_str(),
                                 WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 r.left, r.top, r.right - r.left,
                                 r.bottom - r.top,
                                 c->hwnd, NULL, g_inst, NULL);
    if (!m->editWnd) return;

    if (!m->editFont)
    {
        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = -(LONG)(m->font[CBF_ITEM].sizePt * 96.0f / 72.0f * m->dpiScale);
        lf.lfWeight = m->font[CBF_ITEM].bold ? FW_SEMIBOLD : FW_NORMAL;
        lf.lfItalic = (BYTE)(m->font[CBF_ITEM].italic ? TRUE : FALSE);
        lf.lfCharSet = DEFAULT_CHARSET;
        lstrcpynW(lf.lfFaceName,
                  m->font[CBF_ITEM].face.empty() ? L"Segoe UI"
                                                 : m->font[CBF_ITEM].face.c_str(),
                  LF_FACESIZE);
        m->editFont = CreateFontIndirectW(&lf);
    }
    if (m->editFont)
        SendMessageW(m->editWnd, WM_SETFONT, (WPARAM)m->editFont, TRUE);

    m->editItem    = it->id;
    m->editOldProc = (WNDPROC)SetWindowLongPtrW(m->editWnd, GWLP_WNDPROC,
                                                (LONG_PTR)EditSubclass);
    SetWindowLongPtrW(m->editWnd, GWLP_USERDATA, (LONG_PTR)m);
    SendMessageW(m->editWnd, EM_SETSEL, 0, -1);
    SetFocus(m->editWnd);
    InvalidateRect(c->hwnd, NULL, FALSE);
}

void CBEndEdit(CBManager* m, bool commit)
{
    if (!m->editWnd) return;

    HWND    w    = m->editWnd;
    int     itId = m->editItem;
    WNDPROC old  = m->editOldProc;

    /* Clear the state FIRST: destroying the control fires WM_KILLFOCUS,
       which would otherwise re-enter this function. */
    m->editWnd    = NULL;
    m->editItem   = 0;
    m->editOldProc= NULL;

    if (old) SetWindowLongPtrW(w, GWLP_WNDPROC, (LONG_PTR)old);

    CBItem* it = CBFindItem(m, itId);
    if (commit && it)
    {
        wchar_t buf[1024];
        int n = GetWindowTextW(w, buf, 1023);
        buf[n < 0 ? 0 : n] = 0;
        std::wstring nv = buf;
        if (nv != it->value)
        {
            it->value = nv;
            CBQueue(m, it->id, it->cmd, CBE_TEXTCHANGED, 0);
        }
    }
    DestroyWindow(w);

    if (it)
    {
        CBContainer* c = CBFindContainer(m, it->container);
        if (c && c->hwnd) InvalidateRect(c->hwnd, NULL, FALSE);
    }
}

/*=====================================================================
  12.  Popup menus - windows, hit testing and the tracking loop
  =====================================================================*/
static CBContainer* MenuOf(CBManager* m, int id) { return CBFindContainer(m, id); }

static void DestroyMenuWindow(CBContainer* c)
{
    CBDiscardRT(c);
    if (c->hwnd) { DestroyWindow(c->hwnd); c->hwnd = NULL; }
    c->selIndex = -1;
}

static void CloseChainFrom(CBManager* m, size_t index)
{
    while (m->menuChain.size() > index)
    {
        CBContainer* c = MenuOf(m, m->menuChain.back());
        m->menuChain.pop_back();
        if (c) DestroyMenuWindow(c);
    }
}

void CBCloseMenus(CBManager* m) { CloseChainFrom(m, 0); }

/* Places the popup so it stays on the monitor.  When it will not fit
   below, it flips above the exclude rect rather than covering it. */
static HWND CreateMenuWindow(CBManager* m, CBContainer* c, int x, int y,
                             const RECT* exclude)
{
    CBLayoutMenu(m, c);

    int w = c->measW, h = c->measH;
    POINT pt;
    pt.x = x;
    pt.y = y;
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi))
    {
        if (x + w > mi.rcWork.right)  x = mi.rcWork.right - w;
        if (x < mi.rcWork.left)       x = mi.rcWork.left;
        if (y + h > mi.rcWork.bottom)
        {
            if (exclude && exclude->top - h >= mi.rcWork.top) y = exclude->top - h;
            else                                              y = mi.rcWork.bottom - h;
        }
        if (y < mi.rcWork.top)        y = mi.rcWork.top;
    }

    c->hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                              CBWC_MENU, L"", WS_POPUP,
                              x, y, w, h, m->parent, NULL, g_inst, NULL);
    if (!c->hwnd) return NULL;
    SetWindowLongPtrW(c->hwnd, GWLP_USERDATA, (LONG_PTR)c);
    ShowWindow(c->hwnd, SW_SHOWNOACTIVATE);
    return c->hwnd;
}

/* index into c->items, or -1 */
static int MenuHitTest(CBManager* m, CBContainer* c, POINT p)
{
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it || !it->visible) continue;
        if (PtInRect(&it->rc, p)) return (int)i;
    }
    return -1;
}

static int ChainIndexAt(CBManager* m, POINT screenPt)
{
    for (int i = (int)m->menuChain.size() - 1; i >= 0; --i)
    {
        CBContainer* c = MenuOf(m, m->menuChain[i]);
        if (!c || !c->hwnd) continue;
        RECT r;
        GetWindowRect(c->hwnd, &r);
        if (PtInRect(&r, screenPt)) return i;
    }
    return -1;
}

/* First selectable row at or after `from`, walking by `step`. */
static int NextSelectable(CBManager* m, CBContainer* c, int from, int step)
{
    int n = (int)c->items.size();
    if (n == 0) return -1;
    for (int k = 0; k < n; ++k)
    {
        int i = from + k * step;
        while (i < 0)  i += n;
        while (i >= n) i -= n;
        CBItem* it = CBFindItem(m, c->items[i]);
        if (it && it->visible && it->enabled && it->type != CBI_SEPARATOR)
            return i;
    }
    return -1;
}

static void OpenSubmenuAt(CBManager* m, size_t chainIdx, int rowIdx)
{
    CBContainer* parent = MenuOf(m, m->menuChain[chainIdx]);
    if (!parent) return;
    CBItem* it = CBFindItem(m, parent->items[rowIdx]);
    if (!it || !it->menu || !it->enabled) return;
    CBContainer* sub = MenuOf(m, it->menu);
    if (!sub || sub->kind != CBK_MENU) return;

    CBQueue(m, it->id, it->cmd, CBE_DROPDOWN, it->menu);

    RECT pr;
    GetWindowRect(parent->hwnd, &pr);
    RECT ex;
    ex.left   = pr.left + it->rc.left;
    ex.top    = pr.top  + it->rc.top;
    ex.right  = pr.left + it->rc.right;
    ex.bottom = pr.top  + it->rc.bottom;

    CBLayoutMenu(m, sub);
    int x = pr.right - 3;
    int y = ex.top - 3;

    /* flip to the left of the parent when there is no room on the right */
    HMONITOR mon = MonitorFromWindow(parent->hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi) && x + sub->measW > mi.rcWork.right)
        x = pr.left - sub->measW + 3;

    sub->ownerItem = it->id;
    if (CreateMenuWindow(m, sub, x, y, &ex))
        m->menuChain.push_back(sub->id);
}

/* Match a typed character against the '&' accelerators of one menu. */
static int AccelIndex(CBManager* m, CBContainer* c, wchar_t ch)
{
    wchar_t up = (wchar_t)towupper(ch);
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (!it || !it->visible || !it->enabled) continue;
        if (it->underline >= 0 && it->underline < (int)it->text.size() &&
            (wchar_t)towupper(it->text[it->underline]) == up)
            return (int)i;
    }
    return -1;
}

long CBTrackPopup(CBManager* m, int menu, int x, int y, int ownerItem,
                  int ownerBar, const RECT* exclude)
{
    CBContainer* root = MenuOf(m, menu);
    if (!root || root->kind != CBK_MENU || root->items.empty()) return 0;

    /* Let a callback host fill the menu before it is measured. */
    CBItem* owner = CBFindItem(m, ownerItem);
    CBQueue(m, ownerItem, owner ? owner->cmd : 0, CBE_DROPDOWN, menu);

    CBCloseMenus(m);
    m->menuResult     = 0;
    m->menuResultItem = 0;
    m->menuSwitchItem = 0;
    m->menuCancelled  = false;

    root->ownerItem = ownerItem;
    HWND hRoot = CreateMenuWindow(m, root, x, y, exclude);
    if (!hRoot) return 0;
    m->menuChain.push_back(root->id);

    SetCapture(hRoot);

    bool done    = false;
    bool sawDown = false;
    MSG  msg;

    while (!done)
    {
        if (GetCapture() != hRoot) break;
        if (!GetMessageW(&msg, NULL, 0, 0)) { PostQuitMessage((int)msg.wParam); break; }

        switch (msg.message)
        {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(msg.lParam);
            pt.y = GET_Y_LPARAM(msg.lParam);
            ClientToScreen(hRoot, &pt);

            int ci = ChainIndexAt(m, pt);

            if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN ||
                msg.message == WM_LBUTTONDBLCLK)
            {
                sawDown = true;
                if (ci < 0)
                {
                    /* A press on the button that opened us closes the
                       menu and is swallowed, so it reads as a toggle. */
                    done = true;
                    break;
                }
            }

            if (ci >= 0)
            {
                CBContainer* mc = MenuOf(m, m->menuChain[ci]);
                POINT lp = pt;
                ScreenToClient(mc->hwnd, &lp);
                int idx = MenuHitTest(m, mc, lp);

                if (msg.message == WM_MOUSEMOVE)
                {
                    if (mc->selIndex != idx)
                    {
                        CloseChainFrom(m, (size_t)ci + 1);
                        mc->selIndex = idx;
                        InvalidateRect(mc->hwnd, NULL, FALSE);
                        if (idx >= 0)
                        {
                            CBItem* it = CBFindItem(m, mc->items[idx]);
                            if (it && it->menu && it->enabled)
                                OpenSubmenuAt(m, (size_t)ci, idx);
                        }
                    }
                }
                else if (msg.message == WM_LBUTTONUP)
                {
                    if (idx >= 0)
                    {
                        CBItem* it = CBFindItem(m, mc->items[idx]);
                        if (it && it->enabled && it->type != CBI_SEPARATOR)
                        {
                            if (it->menu)
                            {
                                if (mc->selIndex != idx || m->menuChain.size() == (size_t)ci + 1)
                                {
                                    CloseChainFrom(m, (size_t)ci + 1);
                                    mc->selIndex = idx;
                                    OpenSubmenuAt(m, (size_t)ci, idx);
                                }
                            }
                            else if (sawDown)
                            {
                                m->menuResult     = it->cmd;
                                m->menuResultItem = it->id;
                                done = true;
                            }
                        }
                    }
                    else if (sawDown) done = true;
                }
            }
            else if (msg.message == WM_MOUSEMOVE && ownerBar)
            {
                /* Sliding along a menu bar switches menus, the way a
                   real menu bar behaves. */
                CBContainer* bar = CBFindContainer(m, ownerBar);
                if (bar && bar->hwnd)
                {
                    POINT lp = pt;
                    ScreenToClient(bar->hwnd, &lp);
                    RECT br;
                    GetClientRect(bar->hwnd, &br);
                    if (PtInRect(&br, lp))
                    {
                        int zone = 0;
                        int hit = CBItemHitTest(m, bar, lp, &zone);
                        if (hit > 0 && hit != ownerItem)
                        {
                            CBItem* hi = CBFindItem(m, hit);
                            if (hi && hi->menu && hi->enabled &&
                                (hi->type == CBI_MENU || hi->type == CBI_DROPDOWN))
                            {
                                m->menuSwitchItem = hit;
                                done = true;
                            }
                        }
                    }
                }
            }
            else if (msg.message == WM_LBUTTONUP && sawDown)
            {
                done = true;
            }
            break;
        }

        case WM_KEYDOWN:
        {
            size_t last = m->menuChain.size() - 1;
            CBContainer* mc = MenuOf(m, m->menuChain[last]);
            if (!mc) { done = true; break; }

            switch (msg.wParam)
            {
            case VK_ESCAPE:
                if (m->menuChain.size() > 1) CloseChainFrom(m, last);
                else done = true;
                break;
            case VK_DOWN:
                mc->selIndex = NextSelectable(m, mc, mc->selIndex + 1, 1);
                InvalidateRect(mc->hwnd, NULL, FALSE);
                break;
            case VK_UP:
                mc->selIndex = NextSelectable(m, mc,
                                   mc->selIndex < 0 ? -1 : mc->selIndex - 1, -1);
                InvalidateRect(mc->hwnd, NULL, FALSE);
                break;
            case VK_RIGHT:
                if (mc->selIndex >= 0)
                {
                    CBItem* it = CBFindItem(m, mc->items[mc->selIndex]);
                    if (it && it->menu && it->enabled)
                    {
                        OpenSubmenuAt(m, last, mc->selIndex);
                        CBContainer* sub = MenuOf(m, m->menuChain.back());
                        if (sub && sub != mc)
                        {
                            sub->selIndex = NextSelectable(m, sub, 0, 1);
                            InvalidateRect(sub->hwnd, NULL, FALSE);
                        }
                        break;
                    }
                }
                /* fall through to the bar when there is no submenu */
            case VK_LEFT:
                if (msg.wParam == VK_LEFT && m->menuChain.size() > 1)
                {
                    CloseChainFrom(m, last);
                    break;
                }
                if (ownerBar)
                {
                    CBContainer* bar = CBFindContainer(m, ownerBar);
                    if (bar)
                    {
                        int cur = -1;
                        std::vector<int> menus;
                        for (size_t i = 0; i < bar->items.size(); ++i)
                        {
                            CBItem* bi = CBFindItem(m, bar->items[i]);
                            if (bi && bi->visible && bi->enabled && bi->menu &&
                                (bi->type == CBI_MENU || bi->type == CBI_DROPDOWN))
                            {
                                if (bi->id == ownerItem) cur = (int)menus.size();
                                menus.push_back(bi->id);
                            }
                        }
                        if (!menus.empty() && cur >= 0)
                        {
                            int nxt = (msg.wParam == VK_LEFT) ? cur - 1 : cur + 1;
                            if (nxt < 0) nxt = (int)menus.size() - 1;
                            if (nxt >= (int)menus.size()) nxt = 0;
                            m->menuSwitchItem = menus[nxt];
                            done = true;
                        }
                    }
                }
                break;
            case VK_RETURN:
                if (mc->selIndex >= 0)
                {
                    CBItem* it = CBFindItem(m, mc->items[mc->selIndex]);
                    if (it && it->enabled && it->type != CBI_SEPARATOR)
                    {
                        if (it->menu) OpenSubmenuAt(m, last, mc->selIndex);
                        else
                        {
                            m->menuResult     = it->cmd;
                            m->menuResultItem = it->id;
                            done = true;
                        }
                    }
                }
                break;
            }
            break;
        }

        case WM_CHAR:
        {
            CBContainer* mc = MenuOf(m, m->menuChain.back());
            if (mc)
            {
                int idx = AccelIndex(m, mc, (wchar_t)msg.wParam);
                if (idx >= 0)
                {
                    CBItem* it = CBFindItem(m, mc->items[idx]);
                    mc->selIndex = idx;
                    if (it && it->menu) OpenSubmenuAt(m, m->menuChain.size() - 1, idx);
                    else if (it)
                    {
                        m->menuResult     = it->cmd;
                        m->menuResultItem = it->id;
                        done = true;
                    }
                }
            }
            break;
        }

        default:
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            break;
        }
    }

    if (GetCapture() == hRoot) ReleaseCapture();
    CBCloseMenus(m);

    if (m->menuResultItem)
    {
        CBItem* it = CBFindItem(m, m->menuResultItem);
        if (it)
        {
            if (it->style & CBIS_AUTOCHECK)
            {
                it->checked = !it->checked;
                CBQueue(m, it->id, it->cmd, CBE_TOGGLED, it->checked ? 1 : 0);
            }
            CBQueue(m, it->id, it->cmd, CBE_COMMAND, 0);
        }
    }
    return m->menuResult;
}

/*=====================================================================
  13.  The menu window procedure
  =====================================================================*/
LRESULT CALLBACK CBMenuProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CBContainer* c = (CBContainer*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_PAINT:
        if (c && c->mgr) CBPaintMenu(c->mgr, c);
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SIZE:
        if (c && c->rt) c->rt->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        return 0;
    case WM_DESTROY:
        if (c) CBDiscardRT(c);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/*=====================================================================
  Dragging a bar to a new dock, or off into a floating frame

  The gripper is the handle on a docked bar and the caption is the
  handle on a floating one.  Nothing moves while the mouse is down: a
  translucent hint window shows the strip the bar would land in, and the
  drop is applied afterwards.  Showing the target instead of dragging
  the real bar is what makes docking to an edge readable at all - and it
  avoids re-laying the whole window out on every mouse move.
  =====================================================================*/

/* The drag handle on a docked bar - the same strip PaintBar draws the
   gripper dots in. */
bool CBGripperRect(CBManager* m, CBContainer* c, RECT* out)
{
    if (!c || c->kind != CBK_BAR) return false;
    if (!(c->style & CBBS_GRIPPER)) return false;
    if (c->dock == CBD_FLOAT || c->dock == CBD_FIXED) return false;

    RECT rc;
    GetClientRect(c->hwnd, &rc);
    *out = rc;
    bool vert = (c->dock == CBD_LEFT || c->dock == CBD_RIGHT);
    if (vert) out->bottom = out->top + (int)(8 * m->dpiScale);
    else      out->right  = out->left + (int)(8 * m->dpiScale);
    return true;
}

/* Which row of `dock` a point falls in, as an INSERTION index. */
static int CBRowAt(CBManager* m, int dock, POINT cp, CBContainer* skip)
{
    const bool horiz = (dock == CBD_TOP || dock == CBD_BOTTOM);
    std::vector<int> rows, lo, hi;

    std::map<int, CBContainer*>::iterator i;
    for (i = m->containers.begin(); i != m->containers.end(); ++i)
    {
        CBContainer* c = i->second;
        if (c->kind != CBK_BAR || c == skip) continue;
        if (!c->visible || c->dock != dock || !c->hwnd) continue;

        RECT wr;
        GetWindowRect(c->hwnd, &wr);
        POINT tl;
        tl.x = wr.left;
        tl.y = wr.top;
        POINT br;
        br.x = wr.right;
        br.y = wr.bottom;
        ScreenToClient(m->parent, &tl);
        ScreenToClient(m->parent, &br);
        int a = horiz ? tl.y : tl.x;
        int b = horiz ? br.y : br.x;

        size_t k = 0;
        for (; k < rows.size(); ++k) if (rows[k] == c->dockRow) break;
        if (k == rows.size())
        {
            rows.push_back(c->dockRow);
            lo.push_back(a);
            hi.push_back(b);
        }
        else
        {
            if (a < lo[k]) lo[k] = a;
            if (b > hi[k]) hi[k] = b;
        }
    }
    if (rows.empty()) return 0;

    for (size_t a = 0; a + 1 < rows.size(); ++a)          /* by row number */
        for (size_t b = a + 1; b < rows.size(); ++b)
            if (rows[b] < rows[a])
            {
                int t = rows[a]; rows[a] = rows[b]; rows[b] = t;
                t = lo[a];  lo[a] = lo[b];  lo[b] = t;
                t = hi[a];  hi[a] = hi[b];  hi[b] = t;
            }

    const int pos = horiz ? cp.y : cp.x;
    /* Rows are numbered from the docked edge inward, so on TOP/LEFT they
       run one way down the screen and on BOTTOM/RIGHT the other. */
    const bool inward = (dock == CBD_TOP || dock == CBD_LEFT);
    for (size_t k = 0; k < rows.size(); ++k)
    {
        int mid = (lo[k] + hi[k]) / 2;
        if (inward ? (pos < mid) : (pos > mid)) return rows[k];
    }
    return rows[rows.size() - 1] + 1;
}

/* Where would the drop land?  Fills dock/row and the hint rect (screen). */
static void CBDragTarget(CBManager* m, POINT sp, int* dock, int* row, RECT* hint)
{
    *dock = CBD_FLOAT;
    *row  = 0;

    CBContainer* c = CBFindContainer(m, m->dragBar);
    if (!c) { SetRectEmpty(hint); return; }

    RECT pc;
    GetClientRect(m->parent, &pc);
    POINT cp = sp;
    ScreenToClient(m->parent, &cp);

    const int edge = (int)(44 * m->dpiScale);
    RECT cr = m->clientRc;
    int d = -1;

    if (PtInRect(&pc, cp))
    {
        if      (cp.y < cr.top    + edge) d = CBD_TOP;
        else if (cp.y > cr.bottom - edge) d = CBD_BOTTOM;
        else if (cp.x < cr.left   + edge) d = CBD_LEFT;
        else if (cp.x > cr.right  - edge) d = CBD_RIGHT;
    }

    /* A bar that may not float has nowhere else to go, so a drop in the
       middle keeps it where it is rather than doing nothing visible. */
    if (d < 0 && !(c->style & CBBS_FLOATABLE) && c->dock != CBD_FLOAT)
        d = c->dock;

    RECT r;
    if (d < 0)                                   /* float at the pointer */
    {
        int w = c->measW > 0 ? c->measW : (int)(160 * m->dpiScale);
        int h = c->measH > 0 ? c->measH : (int)(40  * m->dpiScale);
        r.left   = sp.x - m->dragOff.x;
        r.top    = sp.y - m->dragOff.y;
        r.right  = r.left + w;
        r.bottom = r.top + h;
        *hint = r;
        return;
    }

    *dock = d;
    *row  = CBRowAt(m, d, cp, c);

    int th = m->dragThick[d];
    if (th < 1) th = (int)(26 * m->dpiScale);
    r = cr;
    switch (d)
    {
    case CBD_TOP:    r.bottom = r.top + th;    break;
    case CBD_BOTTOM: r.top    = r.bottom - th; break;
    case CBD_LEFT:   r.right  = r.left + th;   break;
    default:         r.left   = r.right - th;  break;
    }
    POINT tl;
    tl.x = r.left;
    tl.y = r.top;
    POINT br;
    br.x = r.right;
    br.y = r.bottom;
    ClientToScreen(m->parent, &tl);
    ClientToScreen(m->parent, &br);
    hint->left = tl.x; hint->top = tl.y; hint->right = br.x; hint->bottom = br.y;
}

static void CBShowHint(CBManager* m, const RECT* r)
{
    if (!m->dragHint)
    {
        m->dragHint = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                      WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                                      WS_EX_TOPMOST,
                                      CBWC_HINT, L"", WS_POPUP,
                                      0, 0, 10, 10, NULL, NULL, g_inst, NULL);
        if (!m->dragHint) return;
        SetLayeredWindowAttributes(m->dragHint, 0, 110, LWA_ALPHA);
    }
    SetWindowLongPtrW(m->dragHint, GWLP_USERDATA, (LONG_PTR)m->col[CBC_ACCENT]);
    SetWindowPos(m->dragHint, HWND_TOPMOST, r->left, r->top,
                 r->right - r->left, r->bottom - r->top,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(m->dragHint, NULL, TRUE);
}

/* How thick this bar would be if it were docked on `dock`.  Laying it
   out for the target orientation is the only honest answer: the same
   toolbar is 26px tall across the top and ~90px wide down the side. */
static int CBMeasureFor(CBManager* m, CBContainer* c, int dock)
{
    RECT cr = m->clientRc;
    int  save = c->dock;
    int  th;

    c->dock = dock;
    if (dock == CBD_LEFT || dock == CBD_RIGHT)
    {
        CBLayoutBar(m, c, 0, cr.bottom - cr.top);
        th = c->measW;
    }
    else
    {
        CBLayoutBar(m, c, cr.right - cr.left, 0);
        th = c->measH;
    }
    c->dock = save;
    return th;
}

void CBBeginDrag(CBManager* m, CBContainer* c, POINT screenPt)
{
    if (!c || c->kind != CBK_BAR) return;
    if (c->style & CBBS_LOCKED) return;

    for (int d = CBD_TOP; d <= CBD_RIGHT; ++d)
        m->dragThick[d] = CBMeasureFor(m, c, d);
    /* put the bar's own layout back the way the drag found it */
    {
        RECT wrc;
        GetClientRect(c->hwnd, &wrc);
        CBLayoutBar(m, c, wrc.right, wrc.bottom);
    }

    RECT wr;
    GetWindowRect(c->hwnd, &wr);
    m->dragBar    = c->id;
    m->dragOff.x  = screenPt.x - wr.left;
    m->dragOff.y  = screenPt.y - wr.top;
    m->dragStart  = screenPt;
    m->dragActive = false;
    m->dragDock   = c->dock;
    m->dragRow    = c->dockRow;
    SetCapture(c->hwnd);
}

void CBUpdateDrag(CBManager* m, POINT screenPt)
{
    if (!m->dragBar) return;

    if (!m->dragActive)
    {
        int dx = screenPt.x - m->dragStart.x;
        int dy = screenPt.y - m->dragStart.y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        int th = (int)(4 * m->dpiScale);
        if (dx < th && dy < th) return;            /* just a click so far */
        m->dragActive = true;
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { CBEndDrag(m, false); return; }

    RECT hint;
    CBDragTarget(m, screenPt, &m->dragDock, &m->dragRow, &hint);
    if (!IsRectEmpty(&hint)) CBShowHint(m, &hint);
}

void CBEndDrag(CBManager* m, bool apply)
{
    if (!m->dragBar) return;

    /* Clear the state FIRST.  ReleaseCapture() below sends
       WM_CAPTURECHANGED straight back to the bar window, whose handler
       calls this function again to cancel the drag - and that re-entry
       used to wipe the drop before it had been posted. */
    int  bar = m->dragBar;
    bool act = m->dragActive;
    m->dragBar    = 0;
    m->dragActive = false;

    CBContainer* c = CBFindContainer(m, bar);
    if (c && c->hwnd && GetCapture() == c->hwnd) ReleaseCapture();

    if (m->dragHint)
    {
        DestroyWindow(m->dragHint);
        m->dragHint = NULL;
    }
    if (!apply || !act) return;

    /* Applying can destroy and recreate the bar's window, which must not
       happen inside that window's own message handler. */
    m->dragApply = bar;
    PostMessageW(m->parent, CBMSG_APPLYDRAG, 0, 0);
}

void CBApplyDrag(CBManager* m)
{
    int bar = m->dragApply;
    m->dragApply = 0;
    if (!bar) return;

    CBContainer* c = CBFindContainer(m, bar);
    if (!c) return;

    if (m->dragDock == CBD_FLOAT)
    {
        if (!(c->style & CBBS_FLOATABLE)) return;
        POINT sp;
        GetCursorPos(&sp);
        CB_FloatBar(m, bar, sp.x - m->dragOff.x, sp.y - m->dragOff.y);
        return;
    }
    CBInsertBarRow(m, bar, m->dragDock, m->dragRow);
}

LRESULT CALLBACK CBHintProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_ERASEBKGND)
    {
        COLORREF col = (COLORREF)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH b = CreateSolidBrush(col);
        FillRect((HDC)wp, &rc, b);
        DeleteObject(b);
        return 1;
    }
    if (msg == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/*=====================================================================
  14.  The bar window procedure
  =====================================================================*/
/* Screen rect of one item, for the exclude rect of its popup. */
static RECT ItemScreenRect(CBContainer* c, CBItem* it)
{
    RECT r = it->rc;
    POINT tl;
    tl.x = r.left;
    tl.y = r.top;
    POINT br;
    br.x = r.right;
    br.y = r.bottom;
    ClientToScreen(c->hwnd, &tl);
    ClientToScreen(c->hwnd, &br);
    r.left = tl.x; r.top = tl.y; r.right = br.x; r.bottom = br.y;
    return r;
}

static void OpenItemMenu(CBManager* m, CBContainer* c, CBItem* it)
{
    while (it && it->menu)
    {
        RECT ex = ItemScreenRect(c, it);
        int x, y;
        if (c->dock == CBD_LEFT || c->dock == CBD_RIGHT) { x = ex.right; y = ex.top; }
        else                                             { x = ex.left;  y = ex.bottom; }

        c->openItem = it->id;
        InvalidateRect(c->hwnd, NULL, FALSE);
        UpdateWindow(c->hwnd);

        CBTrackPopup(m, it->menu, x, y, it->id, c->id, &ex);

        c->openItem = 0;
        InvalidateRect(c->hwnd, NULL, FALSE);

        /* The mouse slid onto a neighbouring menu title - reopen there. */
        int sw = m->menuSwitchItem;
        m->menuSwitchItem = 0;
        it = sw ? CBFindItem(m, sw) : NULL;
        if (it) { c->hotItem = it->id; c->hotZone = CBHIT_ITEM; }
    }
}

/* The chevron rebuilds its menu from whatever overflowed. */
static void OpenChevron(CBManager* m, CBContainer* c)
{
    if (!c->chevronMenu) return;
    CBContainer* mc = CBFindContainer(m, c->chevronMenu);
    if (!mc) return;

    mc->items.clear();
    for (size_t i = 0; i < c->items.size(); ++i)
    {
        CBItem* it = CBFindItem(m, c->items[i]);
        if (it && it->visible && it->overflow) mc->items.push_back(it->id);
    }
    if (mc->items.empty()) return;

    RECT ex;
    POINT tl;
    tl.x = c->chevronRc.left;
    tl.y = c->chevronRc.top;
    POINT br;
    br.x = c->chevronRc.right;
    br.y = c->chevronRc.bottom;
    ClientToScreen(c->hwnd, &tl);
    ClientToScreen(c->hwnd, &br);
    ex.left = tl.x; ex.top = tl.y; ex.right = br.x; ex.bottom = br.y;

    /* Overflowed items are drawn as menu rows for the duration; their
       rects belong to the popup while it is up, and the next layout
       pass puts them back. */
    CBTrackPopup(m, c->chevronMenu, ex.left, ex.bottom, 0, 0, &ex);
    mc->items.clear();
    CBRelayout(m);
}

static void ActivateItem(CBManager* m, CBContainer* c, CBItem* it, int zone)
{
    switch (it->type)
    {
    case CBI_TOGGLE:
    case CBI_CHECKBOX:
        if (it->style & CBIS_AUTOCHECK)
        {
            it->checked = !it->checked;
            InvalidateRect(c->hwnd, NULL, FALSE);
            CBQueue(m, it->id, it->cmd, CBE_TOGGLED, it->checked ? 1 : 0);
        }
        CBQueue(m, it->id, it->cmd, CBE_COMMAND, 0);
        return;

    case CBI_COLOR:
        if (zone == CBHIT_ARROW)
        {
            CHOOSECOLORW cc;
            static COLORREF custom[16];
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize  = sizeof(cc);
            cc.hwndOwner    = m->parent;
            cc.rgbResult    = it->color;
            cc.lpCustColors = custom;
            cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc))
            {
                it->color = cc.rgbResult;
                InvalidateRect(c->hwnd, NULL, FALSE);
                CBQueue(m, it->id, it->cmd, CBE_COLORCHANGED, (long)it->color);
            }
            return;
        }
        CBQueue(m, it->id, it->cmd, CBE_COMMAND, (long)it->color);
        return;

    case CBI_EDIT:
        CBBeginEdit(m, c, it);
        return;

    case CBI_COMBO:
    {
        /* A drop list built on the fly from the item's own strings. */
        if (it->combo.empty()) return;
        int tmp = CB_CreateMenu(m);
        CBContainer* mc = CBFindContainer(m, tmp);
        if (!mc) return;
        for (size_t i = 0; i < it->combo.size(); ++i)
        {
            std::string a = CBToAnsi(it->combo[i]);
            int row = CB_AddItem(m, tmp, CBI_BUTTON, (long)(i + 1), a.c_str(), 0);
            CBItem* ri = CBFindItem(m, row);
            if (ri && (int)i == it->comboSel) ri->checked = true;
        }
        RECT ex = ItemScreenRect(c, it);
        c->openItem = it->id;
        InvalidateRect(c->hwnd, NULL, FALSE);

        /* The rows exist only for the duration of the drop - the host
           hears about the SELECTION, never about the rows. */
        m->suppressContainer = tmp;
        long pick = CBTrackPopup(m, tmp, ex.left, ex.bottom, it->id, 0, &ex);
        m->suppressContainer = 0;
        c->openItem = 0;

        CB_DestroyContainer(m, tmp);

        if (pick > 0)
        {
            it->comboSel = (int)pick - 1;
            it->value    = it->combo[(size_t)it->comboSel];
            CBQueue(m, it->id, it->cmd, CBE_SELCHANGED, (long)it->comboSel);
            CBQueue(m, it->id, it->cmd, CBE_TEXTCHANGED, 0);
        }
        InvalidateRect(c->hwnd, NULL, FALSE);
        return;
    }

    case CBI_LABEL:
    case CBI_SEPARATOR:
    case CBI_SPACE:
        return;

    default:
        CBQueue(m, it->id, it->cmd, CBE_COMMAND, 0);
        return;
    }
}

LRESULT CALLBACK CBBarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CBContainer* c = (CBContainer*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    CBManager*   m = c ? c->mgr : NULL;
    if (!c || !m) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg)
    {
    case WM_PAINT:
        CBPaintBar(m, c);
        ValidateRect(hwnd, NULL);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEACTIVATE:
        /* Never steal focus from the host's own controls. */
        return MA_NOACTIVATE;

    case WM_SIZE:
        if (c->rt) c->rt->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_MOUSEMOVE:
    {
        POINT p;
        p.x = GET_X_LPARAM(lp);
        p.y = GET_Y_LPARAM(lp);

        if (m->dragBar == c->id)
        {
            POINT sp = p;
            ClientToScreen(hwnd, &sp);
            CBUpdateDrag(m, sp);
            return 0;
        }

        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = 0;
        TrackMouseEvent(&tme);

        if (c->style & CBBS_RIBBON)
        {
            int ht = CBTabHitTest(m, c, p);
            if (ht != c->hotTab)
            {
                c->hotTab = ht;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            const bool onMin = CBRibbonMinHit(c, p);
            if (onMin != c->hotMin)
            {
                c->hotMin = onMin;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        int zone = 0;
        int hit  = CBItemHitTest(m, c, p, &zone);
        if (hit != c->hotItem || zone != c->hotZone)
        {
            c->hotItem = hit;
            c->hotZone = zone;
            InvalidateRect(hwnd, NULL, FALSE);

            CBHideTip(m);
            KillTimer(hwnd, CBTIMER_TIPSHOW);
            if (hit > 0 && (m->style & CBS_TOOLTIPS))
            {
                c->tipItem = hit;
                SetTimer(hwnd, CBTIMER_TIPSHOW, CB_TIPDELAY, NULL);
            }
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        c->hotItem = 0;
        c->hotZone = CBHIT_NONE;
        c->hotTab  = 0;
        c->hotMin  = false;
        KillTimer(hwnd, CBTIMER_TIPSHOW);
        CBHideTip(m);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_TIMER:
        if (wp == CBTIMER_TIPSHOW)
        {
            KillTimer(hwnd, CBTIMER_TIPSHOW);
            CBItem* it = CBFindItem(m, c->tipItem);
            if (it && c->hotItem == it->id && !m->menuChain.size())
            {
                CBShowTip(m, c, it);
                SetTimer(hwnd, CBTIMER_TIPHIDE, CB_TIPDURATION, NULL);
            }
        }
        else if (wp == CBTIMER_TIPHIDE)
        {
            KillTimer(hwnd, CBTIMER_TIPHIDE);
            CBHideTip(m);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        /* double-clicking a floating bar's caption sends it home */
        if (c->dock == CBD_FLOAT && GET_Y_LPARAM(lp) < (int)(17 * m->dpiScale))
        {
            CB_SetBarDock(m, c->id, CBD_TOP, c->dockRow, c->dockOffset);
            return 0;
        }
        /*  Double-clicking the tab strip of a ribbon collapses it to the
            tabs alone, and opens it again - the gesture people already
            know from Office. */
        if (c->style & CBBS_RIBBON)
        {
            POINT dp;
            dp.x = GET_X_LPARAM(lp);
            dp.y = GET_Y_LPARAM(lp);
            if (CBTabHitTest(m, c, dp))
            {
                CB_SetRibbonMinimized(m, c->id, c->minimized ? 0 : 1);
                return 0;
            }
        }
        /* fall through */
    case WM_LBUTTONDOWN:
    {
        POINT p;
        p.x = GET_X_LPARAM(lp);
        p.y = GET_Y_LPARAM(lp);
        CBHideTip(m);
        KillTimer(hwnd, CBTIMER_TIPSHOW);

        if (c->dock == CBD_FLOAT && p.y < (int)(17 * m->dpiScale))
        {
            RECT wr;
            GetWindowRect(hwnd, &wr);
            if (p.x > (wr.right - wr.left) - 22)   /* close box */
            {
                CB_SetBarVisible(m, c->id, 0);
                return 0;
            }
            POINT sp = p;
            ClientToScreen(hwnd, &sp);
            CBBeginDrag(m, c, sp);                 /* caption = drag handle */
            return 0;
        }

        {   /* the gripper is the drag handle on a DOCKED bar */
            RECT gr;
            if (CBGripperRect(m, c, &gr) && PtInRect(&gr, p))
            {
                POINT sp = p;
                ClientToScreen(hwnd, &sp);
                CBBeginDrag(m, c, sp);
                return 0;
            }
        }

        if (CBRibbonMinHit(c, p))
        {
            CB_SetRibbonMinimized(m, c->id, c->minimized ? 0 : 1);
            return 0;
        }

        if (c->style & CBBS_RIBBON)
        {
            int ht = CBTabHitTest(m, c, p);
            if (ht)
            {
                CB_SetActiveTab(m, c->id, ht);
                /* clicking a tab on a collapsed ribbon opens it again */
                if (c->minimized) CB_SetRibbonMinimized(m, c->id, 0);
                return 0;
            }
        }

        int zone = 0;
        int hit  = CBItemHitTest(m, c, p, &zone);
        if (hit == -1) { OpenChevron(m, c); return 0; }
        if (hit <= 0) return 0;

        CBItem* it = CBFindItem(m, hit);
        if (!it || !it->enabled) return 0;

        bool opensMenu = it->menu &&
                         (it->type == CBI_DROPDOWN || it->type == CBI_MENU ||
                          (zone == CBHIT_ARROW &&
                           (it->type == CBI_SPLIT || it->type == CBI_COLOR)));

        if (it->type == CBI_COLOR && zone == CBHIT_ARROW && !it->menu)
        {
            ActivateItem(m, c, it, zone);
            return 0;
        }
        if (opensMenu) { OpenItemMenu(m, c, it); return 0; }
        if (it->type == CBI_COMBO || it->type == CBI_EDIT)
        {
            ActivateItem(m, c, it, zone);
            return 0;
        }

        c->pressItem = hit;
        c->pressZone = zone;
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (m->dragBar == c->id)
        {
            CBEndDrag(m, true);
            return 0;
        }
        POINT p;
        p.x = GET_X_LPARAM(lp);
        p.y = GET_Y_LPARAM(lp);

        int press = c->pressItem, pzone = c->pressZone;
        c->pressItem = 0;
        c->pressZone = CBHIT_NONE;
        if (GetCapture() == hwnd) ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        if (!press) return 0;
        int zone = 0;
        if (CBItemHitTest(m, c, p, &zone) != press) return 0;

        CBItem* it = CBFindItem(m, press);
        if (it && it->enabled) ActivateItem(m, c, it, pzone);
        return 0;
    }

    case WM_RBUTTONUP:
    {
        POINT p;
        p.x = GET_X_LPARAM(lp);
        p.y = GET_Y_LPARAM(lp);
        int zone = 0;
        int hit = CBItemHitTest(m, c, p, &zone);
        if (hit > 0)
        {
            CBItem* it = CBFindItem(m, hit);
            if (it) CBQueue(m, it->id, it->cmd, CBE_RCLICK, 0);
        }
        return 0;
    }

    case WM_SETCURSOR:
    {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hwnd, &p);
        RECT gr;
        if (!(c->style & CBBS_LOCKED) && CBGripperRect(m, c, &gr) &&
            PtInRect(&gr, p))
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        else
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    case WM_CAPTURECHANGED:
        if (m->dragBar == c->id && (HWND)lp != hwnd) CBEndDrag(m, false);
        return 0;

    case WM_DESTROY:
        if (m->dragBar == c->id) { m->dragBar = 0; m->dragActive = false; }
        CBDiscardRT(c);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/*=====================================================================
  15.  Window class registration
  =====================================================================*/
LRESULT CALLBACK CBHintProc(HWND, UINT, WPARAM, LPARAM);

void CBRegisterClasses(void)
{
    if (g_classesDone) return;

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = g_inst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = CBBarProc;
    wc.lpszClassName = CBWC_BAR;
    RegisterClassExW(&wc);

    wc.style         = CS_DROPSHADOW | CS_SAVEBITS;
    wc.lpfnWndProc   = CBMenuProc;
    wc.lpszClassName = CBWC_MENU;
    RegisterClassExW(&wc);

    wc.style         = CS_SAVEBITS;
    wc.lpfnWndProc   = CBTipProc;
    wc.lpszClassName = CBWC_TIP;
    RegisterClassExW(&wc);

    wc.style         = 0;
    wc.lpfnWndProc   = CBHintProc;
    wc.lpszClassName = CBWC_HINT;
    RegisterClassExW(&wc);

    g_classesDone = true;
}
