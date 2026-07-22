#include "imgui.h"
#include "implot.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "Gui.h"
#include "Logger.h"
#include <dbghelp.h>
#include <stdexcept>
#include <tlhelp32.h>
#include <string>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string GetModuleNameFromAddress(DWORD64 address) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &me)) {
            do {
                DWORD64 start = (DWORD64)me.modBaseAddr;
                DWORD64 end = start + me.modBaseSize;
                if (address >= start && address < end) {
                    char buf[512];
                    sprintf_s(buf, "%s+0x%llX", me.szModule, address - start);
                    CloseHandle(hSnapshot);
                    return buf;
                }
            } while (Module32Next(hSnapshot, &me));
        }
        CloseHandle(hSnapshot);
    }
    return "UnknownModule";
}

void PrintStack(EXCEPTION_POINTERS* ep) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymInitialize(process, nullptr, TRUE);

    STACKFRAME64 stackFrame = {0};
    DWORD machineType;
    CONTEXT context = *ep->ContextRecord;

#ifdef _M_IX86
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#else
    LogMessage("Stack trace: Unsupported architecture.");
    SymCleanup(process);
    return;
#endif

    LogMessage("--- CALL STACK ---");
    int frameNum = 0;
    while (StackWalk64(machineType, process, thread, &stackFrame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
        DWORD64 displacement = 0;
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)] = {0};
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        char logLine[512];
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
            // Get line number
            IMAGEHLP_LINE64 line = {0};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisplacement = 0;
            if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &line)) {
                sprintf_s(logLine, "[%d] %s (%s:%d)", frameNum, symbol->Name, line.FileName, line.LineNumber);
            } else {
                sprintf_s(logLine, "[%d] %s (Displacement: %lld)", frameNum, symbol->Name, displacement);
            }
        } else {
            std::string resolved = GetModuleNameFromAddress(stackFrame.AddrPC.Offset);
            sprintf_s(logLine, "[%d] %s (Address: 0x%llX)", frameNum, resolved.c_str(), stackFrame.AddrPC.Offset);
        }
        LogMessage(logLine);
        frameNum++;
    }
    LogMessage("------------------");
    SymCleanup(process);
}

LONG WINAPI OurUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    LogMessage("!!! CRASH DETECTED (SEH Exception) !!!");
    char buffer[256];
    sprintf_s(buffer, "Exception Code: 0x%08X, Exception Address: 0x%p", 
              exceptionInfo->ExceptionRecord->ExceptionCode, 
              exceptionInfo->ExceptionRecord->ExceptionAddress);
    LogMessage(buffer);
    
    std::string crashLocation = GetModuleNameFromAddress((DWORD64)exceptionInfo->ExceptionRecord->ExceptionAddress);
    LogMessage("Crash Module Location: " + crashLocation);
    
    // Print stack trace directly to log
    try {
        PrintStack(exceptionInfo);
    } catch (...) {
        LogMessage("Failed to print stack trace.");
    }
    
    HANDLE hFile = CreateFileA("crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = TRUE;
        
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, nullptr, nullptr);
        CloseHandle(hFile);
        LogMessage("Minidump written to crash.dmp");
    }
    
    MessageBoxA(nullptr, "The application crashed. A debug log (debug.log) and minidump (crash.dmp) have been created in the application directory.", "Application Crash", MB_ICONERROR | MB_OK);
    return EXCEPTION_EXECUTE_HANDLER;
}

// Main code
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    SetUnhandledExceptionFilter(OurUnhandledExceptionFilter);
    LogMessage("=== Application started ===");

    try {
        LogMessage("Registering window class...");
        // Create application window
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"VRTreadmill Class", nullptr };
        ::RegisterClassExW(&wc);
        
        LogMessage("Creating application window...");
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"VR Treadmill Controller", WS_OVERLAPPEDWINDOW, 100, 100, 1200, 900, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd) {
            throw std::runtime_error("Failed to create application window.");
        }

        LogMessage("Initializing Direct3D device...");
        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            throw std::runtime_error("Failed to initialize Direct3D 11 device.");
        }

        LogMessage("Showing application window...");
        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        LogMessage("Setting up ImGui and ImPlot contexts...");
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        LogMessage("Loading font (C:\\Windows\\Fonts\\arial.ttf)...");
        // Load Cyrillic font (Arial)
        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
        if (font == nullptr) {
            LogMessage("Arial font not found, falling back to default...");
            // Fallback if Arial is not found
            io.Fonts->AddFontDefault();
        }

        LogMessage("Initializing ImGui platform backends...");
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        LogMessage("Constructing Gui controller class...");
        // App state
        Gui appGui;

        LogMessage("Entering main message loop...");
        // Main loop
        bool done = false;
        while (!done)
    {
        // Poll and handle messages
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        appGui.render();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

        // Cleanup
        LogMessage("Shutting down ImGui backends...");
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        LogMessage("Cleaning up Direct3D device...");
        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        LogMessage("=== Application finished gracefully ===");
    }
    catch (const std::exception& e) {
        std::string err = std::string("FATAL ERROR (std::exception): ") + e.what();
        LogMessage(err);
        MessageBoxA(nullptr, err.c_str(), "Fatal Error (std::exception)", MB_ICONERROR | MB_OK);
        return 1;
    }
    catch (...) {
        LogMessage("FATAL ERROR: Unknown Exception");
        MessageBoxA(nullptr, "Unknown Exception occurred.", "Fatal Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
