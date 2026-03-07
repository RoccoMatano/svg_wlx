
#pragma once

#ifndef IMPORT_PLUGIN
#define PLUGIN_API extern "C" __declspec(dllexport)
#else
#define PLUGIN_API extern "C" __declspec(dllimport)
#endif

#define lc_copy             1
#define lc_newparams        2
#define lc_selectall        3
#define lc_setpercent       4

#define lcp_wraptext        1
#define lcp_fittowindow     2
#define lcp_ansi            4
#define lcp_ascii           8
#define lcp_variable        12
#define lcp_forceshow       16
#define lcp_fitlargeronly   32
#define lcp_center          64
#define lcp_darkmode        128
#define lcp_darkmodenative  256

#define lcs_findfirst       1
#define lcs_matchcase       2
#define lcs_wholewords      4
#define lcs_backwards       8

#define itm_percent         0xFFFE
#define itm_fontstyle       0xFFFD
#define itm_wrap            0xFFFC
#define itm_fit             0xFFFB
#define itm_next            0xFFFA
#define itm_center          0xFFF9

#define LISTPLUGIN_OK       0
#define LISTPLUGIN_ERROR    1

struct ListDefaultParamStruct
{
    INT size;
    DWORD PluginInterfaceVersionLow;
    DWORD PluginInterfaceVersionHi;
    CHAR DefaultIniName[MAX_PATH];
};

PLUGIN_API HWND WINAPI ListLoadW(HWND parent, PCWSTR file_name, INT flags);
PLUGIN_API INT  WINAPI ListSendCommand(HWND plugin,INT command,INT param);
PLUGIN_API VOID WINAPI ListCloseWindow(HWND plugin);
PLUGIN_API VOID WINAPI ListGetDetectString(PSTR detect, INT maxlen);
PLUGIN_API INT  WINAPI ListSearchDialog(HWND plugin,INT find_next);
PLUGIN_API VOID WINAPI ListSetDefaultParams(ListDefaultParamStruct* dps);
PLUGIN_API INT  WINAPI ListLoadNextW(
    HWND parent,
    HWND plugin,
    PCWSTR file_name,
    INT flags
    );
PLUGIN_API INT WINAPI ListSearchTextW(
    HWND plugin,
    PCWSTR search,
    INT search_param
    );
PLUGIN_API HBITMAP WINAPI ListGetPreviewBitmapW(
    PCWSTR file_name,
    INT width,
    INT height,
    BYTE* preview,
    INT preview_len
    );
PLUGIN_API INT WINAPI ListNotificationReceived(
    HWND plugin,
    UINT msg,
    WPARAM wp,
    LPARAM lp
    );
PLUGIN_API INT WINAPI ListPrintW(
    HWND plugin,
    PCWSTR file_name,
    PCWSTR def_printer,
    INT flags,
    RECT* margins
    );
