#include "pch.h"
#include "bmp_from_svg.h"

////////////////////////////////////////////////////////////////////////////////

static const COLORREF BG_COLOR = RGB(0xff, 0xff, 0xff);
extern "C" int _fltused = 0;
static ATOM wnd_class = 0;


////////////////////////////////////////////////////////////////////////////////

extern "C" BYTE __ImageBase;

inline HINSTANCE hinst()
{
    return reinterpret_cast<HINSTANCE>(&__ImageBase);
}

BOOL WINAPI entry_point(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH && wnd_class)
    {
        UnregisterClass(MAKEINTRESOURCE(wnd_class), hinst());
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

struct SvgCtxt
{
    HBITMAP bitmap;
    LONG    width;
    LONG    height;
};

inline SvgCtxt* get_ctxt(HWND hwnd)
{
    return reinterpret_cast<SvgCtxt*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;

    auto* ctxt = get_ctxt(hwnd);

    switch (msg)
    {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
        if (ctxt && ctxt->bitmap)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            HGDIOBJ oldBmp = SelectObject(memDC, ctxt->bitmap);

            auto wwidth = rc.right - rc.left;
            auto wheight = rc.bottom - rc.top;
            LONG dx, dy, stretched_w, stretched_h;
            if (wwidth >= ctxt->width && wheight >= ctxt->height)
            {
                // center image
                dx = (rc.right - ctxt->width) / 2;
                dy = (rc.bottom - ctxt->height) / 2;
                stretched_w = ctxt->width;
                stretched_h = ctxt->height;
            }
            else
            {
                auto scaleX = float(wwidth) / ctxt->width;
                auto scaleY = float(wheight) / ctxt->height;
                if (scaleX < scaleY)
                {
                    stretched_w = LONG(ctxt->width * scaleX + 0.5f);
                    stretched_h = LONG(ctxt->height * scaleX + 0.5f);
                    dx = rc.left;
                    dy = (rc.bottom - stretched_h) / 2;
                }
                else
                {
                    stretched_w = LONG(ctxt->width * scaleY + 0.5f);
                    stretched_h = LONG(ctxt->height * scaleY + 0.5f);
                    dx = (rc.right - stretched_w) / 2;
                    dy = rc.top;
                }
            }
            StretchBlt(
                hdc,
                dx,
                dy,
                stretched_w,
                stretched_h,
                memDC,
                0,
                0,
                ctxt->width,
                ctxt->height,
                SRCCOPY
                );
            SelectObject(memDC, oldBmp);
            DeleteDC(memDC);
        }
        else
        {
            DrawText(
                hdc,
                L"SVG unavailable",
                -1,
                &rc,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE
                );
        }
        EndPaint(hwnd, &ps);
        return 0;

    case WM_ERASEBKGND:
        return 1;  // handled in WM_PAINT

    case WM_DESTROY:
        if (ctxt)
        {
            if (ctxt->bitmap)
            {
                DeleteObject(ctxt->bitmap);
            }
            free(ctxt);
        }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

////////////////////////////////////////////////////////////////////////////////

PLUGIN_API void WINAPI ListGetDetectString(PSTR detect_str, int maxlen)
{
    static const CHAR ds[] = "EXT=\"SVG\" | EXT=\"SVGZ\"";
    strncpy(detect_str, ds, maxlen - 1);
    detect_str[maxlen - 1] = '\0';
}

////////////////////////////////////////////////////////////////////////////////

PLUGIN_API HBITMAP WINAPI ListGetPreviewBitmapW(
    PCWSTR fname,
    int width,
    int height,
    BYTE*,
    int
    )
{
    LONG dummy_w, dummy_h;
    return bitmap_from_svg(fname, width, height, BG_COLOR, dummy_w, dummy_h);
}

////////////////////////////////////////////////////////////////////////////////

PLUGIN_API HWND WINAPI ListLoadW(HWND parent, PCWSTR fname, int)
{
    auto* ctxt = static_cast<SvgCtxt*>(malloc(sizeof(SvgCtxt)));
    if (!ctxt)
    {
        return nullptr;
    }

    if (wnd_class == 0)
    {
        WNDCLASS wc{};
        wc.lpfnWndProc    = wnd_proc;
        wc.hInstance      = hinst();
        wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground  = HBRUSH(GetStockObject(WHITE_BRUSH));
        wc.lpszClassName  = L"svg_wlx";
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wnd_class = RegisterClass(&wc);
        if (wnd_class == 0)
        {
            return nullptr;
        }
    }

    RECT rc;
    GetClientRect(parent, &rc);
    auto width = rc.right - rc.left;
    auto height = rc.bottom - rc.top;
    auto hwnd = CreateWindow(
        MAKEINTRESOURCE(wnd_class),
        L"",
        WS_CHILD | WS_VISIBLE,
        0,
        0,
        width,
        height,
        parent,
        nullptr,
        hinst(),
        nullptr
        );
    if (hwnd)
    {
        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(ctxt)
            );
        ctxt->bitmap = bitmap_from_svg(
            fname,
            width,
            height,
            BG_COLOR,
            ctxt->width,
            ctxt->height
            );
    }
    return hwnd;
}

////////////////////////////////////////////////////////////////////////////////

PLUGIN_API void WINAPI ListCloseWindow(HWND list_wnd)
{
    DestroyWindow(list_wnd);
}

////////////////////////////////////////////////////////////////////////////////

PLUGIN_API int WINAPI ListLoadNextW(HWND, HWND svg_wnd, PCWSTR fname, int)
{
    auto* ctxt = get_ctxt(svg_wnd);
    if (!ctxt)
    {
        return LISTPLUGIN_ERROR;
    }

    if (ctxt->bitmap)
    {
        DeleteObject(ctxt->bitmap);
        ctxt->bitmap = nullptr;
    }

    RECT rc;
    GetClientRect(svg_wnd, &rc);
    ctxt->bitmap = bitmap_from_svg(
        fname,
        rc.right - rc.left,
        rc.bottom - rc.top,
        BG_COLOR,
        ctxt->width,
        ctxt->height
        );
    InvalidateRect(svg_wnd, nullptr, true);
    return ctxt->bitmap ? LISTPLUGIN_OK : LISTPLUGIN_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
