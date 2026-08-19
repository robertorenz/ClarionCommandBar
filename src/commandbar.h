/*=====================================================================
  commandbar.h  -  ClaCommandBar: Direct2D command bars for Clarion
  ---------------------------------------------------------------------
  Flat C API, all functions __stdcall (Clarion PASCAL), ANSI strings
  (converted to UTF-16 internally with CP_ACP).  Designed to be called
  from Clarion 32-bit applications via COMMANDBAR.DLL.

  The model, in one paragraph:

    ONE MANAGER (HCB) is attached to one parent window.  The manager
    owns CONTAINERS and ITEMS.  A container is either a BAR (a strip of
    items docked to an edge of the parent, or floating) or a MENU (a
    popup that appears under a dropdown item, or as a submenu).  Bars
    and menus share one id space, so CB_AddItem() takes either.  Every
    item has an item id (unique) and a command id (the caller's number,
    which several items may share - a toolbar Save and a menu Save
    normally carry the same one).  User actions report both.
  =====================================================================*/
#ifndef CLACOMMANDBAR_H
#define CLACOMMANDBAR_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef COMMANDBAR_EXPORTS
#define CBAPI __declspec(dllexport) __stdcall
#else
#define CBAPI __stdcall
#endif

typedef void* HCB;          /* command-bar manager instance handle */

/* ---- item types --------------------------------------------------- */
#define CBI_BUTTON      1   /* plain push button                       */
#define CBI_TOGGLE      2   /* stays down until clicked again          */
#define CBI_DROPDOWN    3   /* whole button drops its menu             */
#define CBI_SPLIT       4   /* left = command, right arrow = menu      */
#define CBI_SEPARATOR   5   /* divider line                            */
#define CBI_LABEL       6   /* static text, no interaction             */
#define CBI_EDIT        7   /* in-bar type-in field                    */
#define CBI_COMBO       8   /* in-bar drop list (CB_AddComboItem)      */
#define CBI_CHECKBOX    9   /* box + text, on/off                      */
#define CBI_COLOR      10   /* swatch button + colour picker           */
#define CBI_MENU       11   /* top-level menu title on a menu bar      */
#define CBI_SPACE      12   /* flexible gap: pushes what follows right */

/* ---- item style flags (CB_SetItemStyle) --------------------------- */
#define CBIS_TEXTONLY   0x0001  /* never draw the image               */
#define CBIS_ICONONLY   0x0002  /* never draw the text                */
#define CBIS_TEXTBELOW  0x0004  /* large button: image over text      */
#define CBIS_BEGINGROUP 0x0008  /* draw a separator before this item  */
#define CBIS_WRAP       0x0010  /* start a new row after this item    */
#define CBIS_RIGHTALIGN 0x0020  /* push this item to the far end      */
#define CBIS_DEFAULT    0x0040  /* menu default item - drawn bold     */
#define CBIS_RADIO      0x0080  /* menu check mark is a radio dot     */
#define CBIS_AUTOCHECK  0x0100  /* toggle/checkbox flips itself       */
#define CBIS_STRETCH    0x0200  /* edit/combo eats the leftover width */

/* ---- dock sides (CB_SetBarDock) ----------------------------------- */
#define CBD_TOP         0
#define CBD_BOTTOM      1
#define CBD_LEFT        2
#define CBD_RIGHT       3
#define CBD_FLOAT       4

/* ---- bar style flags (CB_AddBar) ---------------------------------- */
#define CBBS_MENUBAR    0x0001  /* this bar is the menu bar           */
#define CBBS_GRIPPER    0x0002  /* draw the drag gripper              */
#define CBBS_FLOATABLE  0x0004  /* user may drag it off into a frame  */
#define CBBS_LOCKED     0x0008  /* no dragging at all                 */
#define CBBS_NOBORDER   0x0010  /* no edge line                       */
#define CBBS_LARGEICONS 0x0020  /* 32px images on this bar            */

/* ---- manager style flags (CB_Create) ------------------------------ */
#define CBS_TOOLTIPS    0x0001  /* show tooltips                      */
#define CBS_CHEVRON     0x0002  /* overflow chevron when too narrow   */
#define CBS_FLATLOOK    0x0004  /* flat buttons, hot-tracked          */
#define CBS_HOTTEXT     0x0008  /* item text recolours on hover       */
#define CBS_MENUICONS   0x0010  /* icon gutter down the popup menus   */

/* ---- built-in themes (CB_SetTheme) -------------------------------- */
/* Every one of these is a professional, business-application palette. */
#define CBT_STEELBLUE   1   /* default: slate + steel blue            */
#define CBT_OFFICE2003  2   /* classic blue gradient                  */
#define CBT_OFFICE2007  3   /* glossy silver-blue                     */
#define CBT_OFFICE2010  4   /* muted silver, flat-ish                 */
#define CBT_OFFICE2013  5   /* flat white, thin lines                 */
#define CBT_OFFICE2016  6   /* flat, coloured title band              */
#define CBT_VS2012LIGHT 7   /* light grey, minimal                    */
#define CBT_VS2012DARK  8   /* dark grey, light text                  */
#define CBT_WIN11LIGHT  9   /* rounded, soft grey, subtle shadow      */
#define CBT_WIN11DARK  10   /* rounded, near-black, soft accents      */
#define CBT_SLATEDARK  11   /* dark slate + steel blue accents        */
#define CBT_CUSTOM     99   /* keep the current colours; change them  */
                            /* one at a time with CB_SetColor         */

/* ---- colour slots (CB_SetColor) ----------------------------------- */
/* All COLORREF (0x00BBGGRR) - exactly what Clarion's COLOR: equates   */
/* and its colour picker produce.                                     */
#define CBC_BARBACK        1   /* bar background, top of the gradient */
#define CBC_BARBACK2       2   /* bar background, bottom (=top: flat) */
#define CBC_BARBORDER      3   /* line along the bar's docked edge    */
#define CBC_ITEMTEXT       4   /* item text, normal                   */
#define CBC_ITEMTEXTHOT    5   /* item text under the mouse           */
#define CBC_ITEMTEXTDIS    6   /* item text, disabled                 */
#define CBC_HOTBACK        7   /* hovered item fill                   */
#define CBC_HOTBORDER      8   /* hovered item outline                */
#define CBC_PRESSBACK      9   /* pressed item fill                   */
#define CBC_PRESSBORDER   10   /* pressed item outline                */
#define CBC_CHECKBACK     11   /* toggled-on item fill                */
#define CBC_CHECKBORDER   12   /* toggled-on item outline             */
#define CBC_SEPARATOR     13   /* separator line                      */
#define CBC_GRIPPER       14   /* drag gripper dots                   */
#define CBC_MENUBACK      15   /* popup menu background               */
#define CBC_MENUBORDER    16   /* popup menu outline                  */
#define CBC_MENUGUTTER    17   /* popup menu icon column              */
#define CBC_MENUHOT       18   /* highlighted menu row fill           */
#define CBC_MENUHOTBORDER 19   /* highlighted menu row outline        */
#define CBC_MENUTEXT      20   /* menu row text                       */
#define CBC_MENUTEXTHOT   21   /* highlighted menu row text           */
#define CBC_MENUTEXTDIS   22   /* disabled menu row text              */
#define CBC_MENUSHORTCUT  23   /* the Ctrl+S column                   */
#define CBC_MENUSEP       24   /* menu separator line                 */
#define CBC_MENUCHECK     25   /* check mark / radio dot              */
#define CBC_CAPTIONBACK   26   /* floating bar caption bar            */
#define CBC_CAPTIONTEXT   27   /* floating bar caption text           */
#define CBC_FLOATBORDER   28   /* floating bar frame                  */
#define CBC_CHEVRON       29   /* overflow chevron glyph              */
#define CBC_EDITBACK      30   /* in-bar edit / combo background      */
#define CBC_EDITBORDER    31   /* in-bar edit / combo outline         */
#define CBC_EDITTEXT      32   /* in-bar edit / combo text            */
#define CBC_EDITSEL       33   /* selection inside an edit            */
#define CBC_TIPBACK       34   /* tooltip background                  */
#define CBC_TIPBORDER     35   /* tooltip outline                     */
#define CBC_TIPTEXT       36   /* tooltip text                        */
#define CBC_ACCENT        37   /* the one accent colour: focus rings, */
                               /* the Office2016 band, drop arrows    */
#define CBC_COUNT         37

/* ---- font slots (CB_SetFont) -------------------------------------- */
#define CBF_ITEM        1   /* bar item text                          */
#define CBF_MENU        2   /* popup menu text                        */
#define CBF_CAPTION     3   /* floating bar caption                   */
#define CBF_TOOLTIP     4   /* tooltip text                           */

/* ---- metrics (CB_SetMetric) --------------------------------------- */
#define CBM_ITEMHEIGHT   1  /* button height, px (0 = from the font)  */
#define CBM_ICONSIZE     2  /* small image size, px      (default 16) */
#define CBM_LARGEICON    3  /* CBBS_LARGEICONS size, px  (default 32) */
#define CBM_PADX         4  /* horizontal padding inside a button     */
#define CBM_PADY         5  /* vertical padding inside a button       */
#define CBM_GAP          6  /* gap between two items                  */
#define CBM_BARPADX      7  /* inset at each end of a bar             */
#define CBM_BARPADY      8  /* inset at the top/bottom of a bar       */
#define CBM_SEPWIDTH     9  /* width a separator occupies             */
#define CBM_CORNER      10  /* button corner radius, px  (0 = square) */
#define CBM_MENUWIDTH   11  /* minimum popup menu width               */
#define CBM_GUTTERWIDTH 12  /* popup menu icon column width           */
#define CBM_ROWGAP      13  /* gap between two rows of one bar        */

/* ---- event types returned by CB_PollEvent ------------------------- */
#define CBE_COMMAND      1  /* button pressed / menu item chosen      */
#define CBE_TOGGLED      2  /* toggle or checkbox flipped (param=0|1) */
#define CBE_DROPDOWN     3  /* a menu is about to open - fill it now  */
#define CBE_TEXTCHANGED  4  /* an edit / combo text was committed     */
#define CBE_SELCHANGED   5  /* combo selection changed (param = index)*/
#define CBE_COLORCHANGED 6  /* colour item changed (param = COLORREF) */
#define CBE_LAYOUT       7  /* bars re-laid out: re-read the client   */
                            /* rect and move your own controls        */
#define CBE_RCLICK       8  /* right button released on an item       */

/* ---- lifetime ----------------------------------------------------- */
int   CBAPI CB_Initialize(void);            /* once per process       */
void  CBAPI CB_Shutdown(void);
/* hwndParent is the window the bars dock inside.                      */
HCB   CBAPI CB_Create(HWND hwndParent, unsigned long style);
void  CBAPI CB_Destroy(HCB cb);
/* Re-run the whole dock layout - call it after the parent resizes.    */
void  CBAPI CB_Layout(HCB cb);
/* The client area left over once every docked bar has taken its       */
/* space, in parent client coordinates.  Any out pointer may be NULL.  */
void  CBAPI CB_GetClientRect(HCB cb, int* x, int* y, int* w, int* h);
void  CBAPI CB_Redraw(HCB cb);

/* ---- theming ------------------------------------------------------ */
/* Load one of the CBT_ palettes wholesale.  Do this FIRST: it         */
/* overwrites every colour slot.  CB_SetColor afterwards tweaks one.   */
void  CBAPI CB_SetTheme(HCB cb, int theme);
int   CBAPI CB_GetTheme(HCB cb);
void  CBAPI CB_SetColor(HCB cb, int slot, COLORREF color);
COLORREF CBAPI CB_GetColor(HCB cb, int slot);
void  CBAPI CB_SetFont(HCB cb, int slot, const char* face, int sizePt,
                       int bold, int italic);
void  CBAPI CB_SetMetric(HCB cb, int metric, int value);
int   CBAPI CB_GetMetric(HCB cb, int metric);
/* Recolour the whole palette around one accent colour, keeping the    */
/* current theme's light/dark character.  The one-call way to make the */
/* bars match a house style.                                           */
void  CBAPI CB_SetAccent(HCB cb, COLORREF accent);

/* ---- images ------------------------------------------------------- */
/* Load one image file (PNG / JPG / GIF / BMP / ICO, alpha honoured)   */
/* and return its image index (>= 1), 0 on failure.                    */
int   CBAPI CB_AddImage(HCB cb, const char* fileName);
/* Load a horizontal strip of cx-wide images and return the index of   */
/* the FIRST one; the rest follow consecutively.  The count goes into  */
/* *count.  A 16x16 strip of 8 icons gives you indices n .. n+7.       */
int   CBAPI CB_AddImageStrip(HCB cb, const char* fileName, int cx,
                             int* count);
/* Take an HICON / HBITMAP the caller already owns (Clarion's ICON()   */
/* resources arrive this way).  The DLL copies the pixels; the caller  */
/* keeps ownership of the handle it passed in.                         */
int   CBAPI CB_AddImageHandle(HCB cb, HANDLE hIconOrBitmap, int isIcon);
int   CBAPI CB_GetImageCount(HCB cb);

/* ---- containers: bars and menus ----------------------------------- */
/* A bar docked on `dock` (CBD_*).  Returns a container id (>= 1).     */
int   CBAPI CB_AddBar(HCB cb, const char* title, int dock,
                      unsigned long barStyle);
/* An empty popup menu.  Returns a container id (>= 1).  Attach it to  */
/* a CBI_DROPDOWN / CBI_SPLIT / CBI_MENU item with CB_SetItemMenu, or  */
/* to a menu row as a submenu with the same call.                      */
int   CBAPI CB_CreateMenu(HCB cb);
void  CBAPI CB_DestroyContainer(HCB cb, int container);
void  CBAPI CB_ClearContainer(HCB cb, int container);
void  CBAPI CB_SetBarDock(HCB cb, int bar, int dock, int row, int offset);
int   CBAPI CB_GetBarDock(HCB cb, int bar);
void  CBAPI CB_SetBarVisible(HCB cb, int bar, int visible);
int   CBAPI CB_GetBarVisible(HCB cb, int bar);
/* Float the bar at screen position x,y (dock becomes CBD_FLOAT).      */
void  CBAPI CB_FloatBar(HCB cb, int bar, int x, int y);
/* Pop a menu up at screen x,y - the classic context menu.  Blocks     */
/* until the user picks or cancels; the choice ALSO arrives as a normal */
/* CBE_COMMAND event, so either poll for it or use the return value.   */
/* Returns the command id chosen, or 0 if the user cancelled.          */
long  CBAPI CB_TrackMenu(HCB cb, int menu, int x, int y);

/* ---- items -------------------------------------------------------- */
/* container = a bar id or a menu id.  cmdId is the caller's number,   */
/* reported back on every event; several items may share one.  text    */
/* may carry a '&' accelerator.  Returns the item id (>= 1).           */
int   CBAPI CB_AddItem(HCB cb, int container, int type, long cmdId,
                       const char* text, int image);
/* Insert before `beforeItem` instead of appending.                    */
int   CBAPI CB_InsertItem(HCB cb, int container, int beforeItem, int type,
                          long cmdId, const char* text, int image);
void  CBAPI CB_RemoveItem(HCB cb, int item);
void  CBAPI CB_SetItemText(HCB cb, int item, const char* text);
int   CBAPI CB_GetItemText(HCB cb, int item, char* buf, int bufLen);
void  CBAPI CB_SetItemImage(HCB cb, int item, int image);
void  CBAPI CB_SetItemStyle(HCB cb, int item, unsigned long styleFlags);
unsigned long CBAPI CB_GetItemStyle(HCB cb, int item);
void  CBAPI CB_SetItemEnabled(HCB cb, int item, int enabled);
int   CBAPI CB_GetItemEnabled(HCB cb, int item);
void  CBAPI CB_SetItemChecked(HCB cb, int item, int checked);
int   CBAPI CB_GetItemChecked(HCB cb, int item);
void  CBAPI CB_SetItemVisible(HCB cb, int item, int visible);
int   CBAPI CB_GetItemVisible(HCB cb, int item);
void  CBAPI CB_SetItemTooltip(HCB cb, int item, const char* text);
/* The right-hand column of a menu row: "Ctrl+S".  Cosmetic - use      */
/* CB_AddAccelerator to make the key actually work.                    */
void  CBAPI CB_SetItemShortcut(HCB cb, int item, const char* text);
/* Fixed width in px for EDIT / COMBO / LABEL / SPACE (0 = automatic). */
void  CBAPI CB_SetItemWidth(HCB cb, int item, int px);
/* Hang a popup on a DROPDOWN / SPLIT / MENU item, or a submenu on a   */
/* menu row.  menu = 0 detaches.                                       */
void  CBAPI CB_SetItemMenu(HCB cb, int item, int menu);
int   CBAPI CB_GetItemMenu(HCB cb, int item);
void  CBAPI CB_SetItemCmd(HCB cb, int item, long cmdId);
long  CBAPI CB_GetItemCmd(HCB cb, int item);
/* First item carrying this command id, 0 if there is none.  The way   */
/* to grey out "Save" everywhere at once without keeping item ids.     */
int   CBAPI CB_FindItem(HCB cb, long cmdId);
/* Apply enable / check to EVERY item carrying this command id.        */
void  CBAPI CB_EnableCmd(HCB cb, long cmdId, int enabled);
void  CBAPI CB_CheckCmd(HCB cb, long cmdId, int checked);
int   CBAPI CB_GetItemCount(HCB cb, int container);
int   CBAPI CB_GetItemAt(HCB cb, int container, int index); /* 0-based */

/* ---- edit / combo / colour item values ---------------------------- */
void  CBAPI CB_SetItemValue(HCB cb, int item, const char* text);
int   CBAPI CB_GetItemValue(HCB cb, int item, char* buf, int bufLen);
void  CBAPI CB_AddComboItem(HCB cb, int item, const char* text);
void  CBAPI CB_ClearComboItems(HCB cb, int item);
void  CBAPI CB_SetComboSel(HCB cb, int item, int index);   /* 0-based  */
int   CBAPI CB_GetComboSel(HCB cb, int item);              /* -1 none  */
void  CBAPI CB_SetItemColor(HCB cb, int item, COLORREF color);
COLORREF CBAPI CB_GetItemColor(HCB cb, int item);

/* ---- keyboard ------------------------------------------------------ */
/* Make a key fire a command.  key is a virtual key code ('S' = VK for */
/* the S key), mods is any of the CBK_ flags below.  Feed keystrokes   */
/* in with CB_TranslateKey.                                            */
#define CBK_CTRL   0x0001
#define CBK_SHIFT  0x0002
#define CBK_ALT    0x0004
void  CBAPI CB_AddAccelerator(HCB cb, long cmdId, int key, int mods);
void  CBAPI CB_ClearAccelerators(HCB cb);
/* Returns 1 if the key matched an accelerator (a CBE_COMMAND was      */
/* queued) or opened a menu, 0 if the host should handle it normally.  */
int   CBAPI CB_TranslateKey(HCB cb, int key, int mods);

/* ---- events -------------------------------------------------------- */
/* Poll the queue.  Returns 1 and fills the out params while there is  */
/* anything queued, else 0.  Pump it from a Clarion TIMER.  Any out    */
/* pointer may be NULL.                                                */
int   CBAPI CB_PollEvent(HCB cb, int* item, long* cmdId, int* evType,
                         long* param);
/* Optional immediate callback:                                        */
/*   void __stdcall cb(long userData, int item, long cmdId,            */
/*                     int evType, long param)                         */
typedef void (__stdcall *CB_EVENTPROC)(long userData, int item, long cmdId,
                                       int evType, long param);
void  CBAPI CB_SetCallback(HCB cb, CB_EVENTPROC proc, long userData);

/* ---- odds and ends ------------------------------------------------- */
/* The mouse position in SCREEN coordinates - what CB_TrackMenu wants   */
/* for a context menu.  Here so a caller does not have to bind to       */
/* user32 itself just to pop a menu at the pointer.                     */
void  CBAPI CB_GetCursorPos(int* x, int* y);

#ifdef __cplusplus
}
#endif
#endif /* CLACOMMANDBAR_H */
