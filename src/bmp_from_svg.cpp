#include "pch.h"
#include "bmp_from_svg.h"

#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"

////////////////////////////////////////////////////////////////////////////////

static NSVGimage* parse_file(PCWSTR fname)
{
    NSVGimage* result = nullptr;
    auto hdl = CreateFile(
        fname,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
        );
    if (hdl != INVALID_HANDLE_VALUE)
    {
        auto size = GetFileSize(hdl, nullptr);
        if (size != 0 && size != INVALID_FILE_SIZE)
        {
            auto* buf = static_cast<PSTR>(malloc(size + 1));
            if (buf)
            {
                if (ReadFile(hdl, buf, size, nullptr, nullptr))
                {
                    buf[size] = 0;
                    result = nsvgParse(buf, "px", 96.0f);
                }
                free(buf);
            }
        }
        CloseHandle(hdl);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////

HBITMAP bitmap_from_svg(
    PCWSTR fname,
    LONG max_width,
    LONG max_height,
    COLORREF back_color,
    LONG& width,
    LONG& height
    )
{
    auto* svg = parse_file(fname);
    if (!svg)
    {
        return nullptr;
    }

    auto scaleX = float(max_width) / svg->width;
    auto scaleY = float(max_height) / svg->height;
    auto scale = (scaleX < scaleY) ? scaleX : scaleY;
    auto w = LONG(svg->width * scale);
    auto h = LONG(svg->height * scale);
    width = w;
    height = h;

    auto* rgba = static_cast<BYTE*>(malloc(w * h * 4));
    if (!rgba)
    {
        nsvgDelete(svg);
        return nullptr;
    }

    auto* rast = nsvgCreateRasterizer();
    if (!rast)
    {
        nsvgDelete(svg);
        return nullptr;
    }

    nsvgRasterize(rast, svg, 0, 0, scale, rgba, w, h, w * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;   // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    auto hdc = GetDC(nullptr);
    BYTE* bgr = nullptr;
    auto bmp = CreateDIBSection(
        hdc,
        &bmi,
        DIB_RGB_COLORS,
        reinterpret_cast<void**>(&bgr),
        nullptr,
        0);
    ReleaseDC(nullptr, hdc);

    const auto BG_R = GetRValue(back_color);
    const auto BG_G = GetGValue(back_color);
    const auto BG_B = GetBValue(back_color);
    const auto row_bytes = ((w * 3 + 3) & ~3);
    if (bmp && bgr)
    {
        for (auto y = 0; y < h; y++)
        {
            auto rss = y * w;
            auto rsd = y * row_bytes;
            for (auto x = 0; x < w; x++)
            {
                auto sidx = (rss + x) * 4;
                auto sr = rgba[sidx + 0];
                auto sg = rgba[sidx + 1];
                auto sb = rgba[sidx + 2];
                auto sa = rgba[sidx + 3];

                auto didx = rsd + x * 3;
                bgr[didx + 0] = (sb * sa + BG_B * (255 - sa)) / 255;
                bgr[didx + 1] = (sg * sa + BG_G * (255 - sa)) / 255;
                bgr[didx + 2] = (sr * sa + BG_R * (255 - sa)) / 255;
            }
        }
    }
    free(rgba);
    return bmp;
}

////////////////////////////////////////////////////////////////////////////////

BOOL save_svg_bitmap(PCWSTR fname, HBITMAP bmp)
{
    BITMAP bm;
    GetObject(bmp, sizeof(bm), &bm);
    if (bm.bmBits == nullptr)
    {
        return false;
    }

    LONG width  = bm.bmWidth;
    LONG height = bm.bmHeight;
    LONG stride = ((width * 3 + 3) & ~3); // DWORD aligned row size
    LONG imageSize = stride * height;

    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = {};

    infoHeader.biSize        = static_cast<DWORD>(sizeof(BITMAPINFOHEADER));
    infoHeader.biWidth       = width;
    infoHeader.biHeight      = -height;
    infoHeader.biPlanes      = 1;
    infoHeader.biBitCount    = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage   = imageSize;

    fileHeader.bfType = 0x4D42; // 'BM'
    fileHeader.bfOffBits = static_cast<DWORD>(
        sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
        );
    fileHeader.bfSize = fileHeader.bfOffBits + imageSize;

    HANDLE file = CreateFile(
        fname,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
        );
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    WriteFile(file, &fileHeader, sizeof(fileHeader), nullptr, nullptr);
    WriteFile(file, &infoHeader, sizeof(infoHeader), nullptr, nullptr);
    WriteFile(file, bm.bmBits, imageSize, nullptr, nullptr);
    CloseHandle(file);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
