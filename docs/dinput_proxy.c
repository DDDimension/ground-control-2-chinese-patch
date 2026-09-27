/*
 * dinput_proxy.c -- Ground Control II Chinese localisation proxy DLL (32-bit)
 *
 * Role (v5, fullscreen baseline)
 * -----------------------------
 *   1. Forward every dinput.dll export to the real dinput.dll, renamed to
 *      dinput_orig.dll in the game folder.
 *   2. From DllMain (DLL_PROCESS_ATTACH) load LG_Data\LG_GC2.dll, whose own
 *      DllMain installs the ASM hooks that translate the game's text.
 *   3. Write a small log (gc2cn_proxy.log) so the load order can be verified.
 *
 * ¨€¨€ v5 change: windowed mode removed ¨€¨€
 *   v2-v4 also forced DXVK_FORCE_WINDOWED=1 and pre-loaded d3d9.dll, in an
 *   attempt to run the game in a window.  That experiment is over: once DXVK
 *   is windowed, its swapchain extent equals the window size while the D3D9
 *   back buffer stays at the size the game asked for (1024x768), and DXVK
 *   blits that 1:1 into one corner -- leaving large black areas.  Every
 *   workaround (inline-hooking the D3D9 entry points so the back buffer
 *   matched the window, resizing the window from a helper thread, adding a
 *   caption / minimise box) either produced the same black block or crashed.
 *
 *   So the proxy is back to doing exactly one thing: localisation.  Nothing
 *   here touches the display mode, the environment, or d3d9.dll any more.
 *   The game runs in its native fullscreen and the Chinese UI works through
 *   LG_GC2.dll alone.
 *
 * Notes
 *   - The game's own files are never modified.
 *   - Layout = original DLL + original dinput.dll kept as dinput_orig.dll.
 *   - Paths are resolved from GetModuleFileNameW, never from the CWD.
 */

#include <windows.h>
#include <stdio.h>

/* real DLL, placed next to this one as dinput_orig.dll */
#define ORIG_DLL_NAME   L"dinput_orig.dll"
/* the localisation hook DLL */
#define LG_DLL_REL      L"LG_Data\\LG_GC2.dll"
/* log file */
#define LOG_NAME        L"gc2cn_proxy.log"

static HMODULE g_hOrig = NULL;

static void log_line(const wchar_t *msg)
{
    wchar_t path[MAX_PATH];
    wchar_t dir[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return;
    wchar_t *p = wcsrchr(dir, L'\\');
    if (!p) return;
    *(p + 1) = 0;
    _snwprintf(path, MAX_PATH, L"%s%s", dir, LOG_NAME);

    HANDLE h = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[1024];
    int len = WideCharToMultiByte(CP_UTF8, 0, msg, -1, buf, sizeof(buf) - 1, NULL, NULL);
    char line[1200];
    int hl = _snprintf(line, sizeof(line),
                       "[%04d-%02d-%02d %02d:%02d:%02d] %s\r\n",
                       st.wYear, st.wMonth, st.wDay,
                       st.wHour, st.wMinute, st.wSecond,
                       (len > 0) ? buf : "?");
    DWORD wrote = 0;
    WriteFile(h, line, (DWORD)hl, &wrote, NULL);
    CloseHandle(h);
}

static void get_self_dir(wchar_t *out, DWORD cch)
{
    DWORD n = GetModuleFileNameW(NULL, out, cch);
    if (n == 0) { out[0] = 0; return; }
    wchar_t *p = wcsrchr(out, L'\\');
    if (p) *(p + 1) = 0;
}

/* load the real dinput.dll */
static void load_orig(void)
{
    wchar_t dir[MAX_PATH], full[MAX_PATH];
    get_self_dir(dir, MAX_PATH);
    _snwprintf(full, MAX_PATH, L"%s%s", dir, ORIG_DLL_NAME);
    g_hOrig = LoadLibraryW(full);
    log_line(g_hOrig ? L"proxy: dinput_orig.dll loaded OK"
                     : L"proxy: FAILED to load dinput_orig.dll");
}

/* load LG_GC2.dll, which installs the localisation hooks */
static void load_lg(void)
{
    wchar_t dir[MAX_PATH], full[MAX_PATH];
    get_self_dir(dir, MAX_PATH);
    _snwprintf(full, MAX_PATH, L"%s%s", dir, LG_DLL_REL);
    HMODULE h = LoadLibraryW(full);
    if (h) {
        log_line(L"proxy: LG_Data\\LG_GC2.dll injected OK");
    } else {
        wchar_t msg[512];
        _snwprintf(msg, 512, L"proxy: FAILED to load LG_GC2.dll (err=%lu)", GetLastError());
        log_line(msg);
    }
}

static void* get_proc(const char *name)
{
    if (!g_hOrig) load_orig();
    if (!g_hOrig) return NULL;
    return (void*)GetProcAddress(g_hOrig, name);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)hinst; (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        log_line(L"proxy: DllMain PROCESS_ATTACH (v5 fullscreen, no DXVK tweaks)");
        load_orig();
        load_lg();
    }
    return TRUE;
}

/* ---------- forward dinput.dll exports ---------- */

typedef HRESULT (WINAPI *PFN_CreateA)(HINSTANCE, DWORD, const IID*, LPVOID*, LPUNKNOWN);
typedef HRESULT (WINAPI *PFN_CreateW)(HINSTANCE, DWORD, const IID*, LPVOID*, LPUNKNOWN);
typedef HRESULT (WINAPI *PFN_CreateEx)(HINSTANCE, DWORD, const IID*, LPVOID*, LPUNKNOWN);

HRESULT WINAPI DirectInputCreateA(HINSTANCE h, DWORD v, const IID *riid, LPVOID *pp, LPUNKNOWN pu)
{
    PFN_CreateA f = (PFN_CreateA)get_proc("DirectInputCreateA");
    return f ? f(h, v, riid, pp, pu) : E_FAIL;
}
HRESULT WINAPI DirectInputCreateW(HINSTANCE h, DWORD v, const IID *riid, LPVOID *pp, LPUNKNOWN pu)
{
    PFN_CreateW f = (PFN_CreateW)get_proc("DirectInputCreateW");
    return f ? f(h, v, riid, pp, pu) : E_FAIL;
}
HRESULT WINAPI DirectInputCreateEx(HINSTANCE h, DWORD v, const IID *riid, LPVOID *pp, LPUNKNOWN pu)
{
    PFN_CreateEx f = (PFN_CreateEx)get_proc("DirectInputCreateEx");
    return f ? f(h, v, riid, pp, pu) : E_FAIL;
}

/* ---------- COM entry points ---------- */

typedef HRESULT (WINAPI *PFN_DllCanUnloadNow)(void);
typedef HRESULT (WINAPI *PFN_DllGetClassObject)(REFCLSID, REFIID, LPVOID*);
typedef HRESULT (WINAPI *PFN_DllRegisterServer)(void);
typedef HRESULT (WINAPI *PFN_DllUnregisterServer)(void);

HRESULT WINAPI DllCanUnloadNow(void)
{
    PFN_DllCanUnloadNow f = (PFN_DllCanUnloadNow)get_proc("DllCanUnloadNow");
    return f ? f() : S_FALSE;
}
HRESULT WINAPI DllGetClassObject(REFCLSID a, REFIID b, LPVOID *c)
{
    PFN_DllGetClassObject f = (PFN_DllGetClassObject)get_proc("DllGetClassObject");
    return f ? f(a, b, c) : E_FAIL;
}
HRESULT WINAPI DllRegisterServer(void)
{
    PFN_DllRegisterServer f = (PFN_DllRegisterServer)get_proc("DllRegisterServer");
    return f ? f() : E_FAIL;
}
HRESULT WINAPI DllUnregisterServer(void)
{
    PFN_DllUnregisterServer f = (PFN_DllUnregisterServer)get_proc("DllUnregisterServer");
    return f ? f() : E_FAIL;
}

/* ---------- linker stubs for the decorated stdcall names ---------- */

HRESULT WINAPI _Stub_DllCanUnloadNow(void)
{
    return DllCanUnloadNow();
}
HRESULT WINAPI _Stub_DllGetClassObject(void *a, void *b, void *c)
{
    return DllGetClassObject((REFCLSID)a, (REFIID)b, (LPVOID*)c);
}
HRESULT WINAPI _Stub_DllRegisterServer(void)
{
    return DllRegisterServer();
}
HRESULT WINAPI _Stub_DllUnregisterServer(void)
{
    return DllUnregisterServer();
}

/* export decorated stdcall stubs via linker pragma */
#pragma comment(linker, "/export:_Stub_DllCanUnloadNow@0=__Stub_DllCanUnloadNow@0,@8")
#pragma comment(linker, "/export:_Stub_DllGetClassObject@12=__Stub_DllGetClassObject@12,@9")
#pragma comment(linker, "/export:_Stub_DllRegisterServer@0=__Stub_DllRegisterServer@0,@10")
#pragma comment(linker, "/export:_Stub_DllUnregisterServer@0=__Stub_DllUnregisterServer@0,@11")
