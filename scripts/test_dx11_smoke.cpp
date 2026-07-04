
#include <windows.h>
#include <stdio.h>
#include <d3d11.h>

int main() {
    HMODULE h = LoadLibraryA("C:\\Program Files (x86)\\EA Games\\Motor City Online\\dx8z.dll");
    if (!h) { printf("LOAD FAILED: %lu\n", GetLastError()); return 1; }
    printf("Loaded dx8z.dll (DX11 backend in disguise) at %p\n", h);

    typedef BOOL (*CreateWindowFn)(HWND);
    CreateWindowFn pCreate = (CreateWindowFn)GetProcAddress(h, "_THRASH_createwindow");
    typedef BOOL (*DestroyWindowFn)(void);
    DestroyWindowFn pDestroy = (DestroyWindowFn)GetProcAddress(h, "_THRASH_destroywindow");
    typedef BOOL (*ClearWindowFn)(void);
    ClearWindowFn pClear = (ClearWindowFn)GetProcAddress(h, "_THRASH_clearwindow");
    typedef BOOL (*SyncWindowFn)(void);
    SyncWindowFn pSync = (SyncWindowFn)GetProcAddress(h, "_THRASH_sync");

    if (!pCreate || !pDestroy || !pClear || !pSync) {
        printf("MISSING entry points\n");
        FreeLibrary(h);
        return 1;
    }

    // Create a real window to give CreateGameWindow a valid HWND
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "Dx11Test";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(0, "Dx11Test", "DX11 Test", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) {
        printf("CreateWindowEx failed: %lu\n", GetLastError());
        FreeLibrary(h);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOW);
    printf("Created test HWND %p\n", hwnd);

    // Call CreateGameWindow — this creates a real D3D11 device + swap chain
    printf("Calling THRASH_createwindow...\n");
    BOOL ok = pCreate(hwnd);
    printf("Result: %s\n", ok ? "TRUE (D3D11 device + swap chain created)" : "FALSE (failed)");

    if (ok) {
        // Call ClearGameWindow and SyncGameWindow
        printf("Calling THRASH_clearwindow...\n");
        BOOL cOk = pClear();
        printf("Clear result: %s\n", cOk ? "TRUE" : "FALSE");

        printf("Calling THRASH_sync (present)...\n");
        BOOL sOk = pSync();
        printf("Sync result: %s\n", sOk ? "TRUE" : "FALSE");

        // Sleep briefly to see the window
        Sleep(100);

        // Tear down
        printf("Calling THRASH_destroywindow...\n");
        BOOL dOk = pDestroy();
        printf("Destroy result: %s\n", dOk ? "TRUE" : "FALSE");
    }

    DestroyWindow(hwnd);
    UnregisterClassA("Dx11Test", wc.hInstance);
    FreeLibrary(h);
    return 0;
}
