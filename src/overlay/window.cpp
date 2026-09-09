#pragma once
#include <windows.h>
#include <iostream>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Dark background
            HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 40));
            FillRect(hdc, &ps.rcPaint, bgBrush);
            DeleteObject(bgBrush);

            // A simple accent bar at the top
            RECT topBar = { 0, 0, 400, 4 };
            HBRUSH accentBrush = CreateSolidBrush(RGB(100, 180, 255));
            FillRect(hdc, &topBar, accentBrush);
            DeleteObject(accentBrush);

            // Text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(220, 220, 220));
            RECT textRect = { 16, 16, 384, 180 };
            DrawTextW(hdc, L"Stardew Notion Widget\nLoading...", -1, &textRect, DT_LEFT | DT_WORDBREAK);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void createOverlayWindow() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSW wc      = {};
    wc.lpfnWndProc    = WndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = L"StardewNotionOverlay";
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        std::cerr << "Failed to register window class\n";
        return;
    }

    // Position in the top-right corner (adjust x/y to taste)
    int width  = 400;
    int height = 200;
    int x      = GetSystemMetrics(SM_CXSCREEN) - width - 20;
    int y      = 20;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,   // always on top + transparency support
        L"StardewNotionOverlay",
        L"Stardew Notion Widget",
        WS_POPUP,                          // no title bar or borders
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        std::cerr << "Failed to create window. Error: " << GetLastError() << "\n";
        return;
    }

    // 230/255 opacity — mostly opaque but slightly see-through
    SetLayeredWindowAttributes(hwnd, 0, 230, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    std::cout << "Overlay window created. Press ESC to close.\n";

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}