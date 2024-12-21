#include "HmckWindow.h"

#include "utils/HmckLogger.h"
#include "HmckKeycodes.h"

#if defined(_WIN32)
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Hmck::Window *window = reinterpret_cast<Hmck::Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CLOSE:
            window->_shouldClose = true;
            DestroyWindow(hWnd); // This will trigger WM_DESTROY
            return 0;
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0); // Clear user data
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            ValidateRect(hWnd, NULL);
            return 0;
        case WM_KEYDOWN:
            if (window) {
                window->onKeyDown(wParam);
            }
            return 0;
        case WM_KEYUP:
            if (window) {
                window->onKeyUp(wParam);
            }
            return 0;
        case WM_LBUTTONDOWN: // mouse left down
            return 0;
        case WM_RBUTTONDOWN: // mouse right down
            return 0;
        case WM_MBUTTONDOWN: // mouse middle down
            return 0;
        case WM_LBUTTONUP: // mouse left up
            return 0;
        case WM_RBUTTONUP: // mouse right up
            return 0;
        case WM_MBUTTONUP: // mouse middle up
            return 0;
        case WM_MOUSEWHEEL:
            return 0;
        case WM_MOUSEMOVE:
            return 0;
        case WM_SIZE: // size changed
            if (window && wParam != SIZE_MINIMIZED) {
                if ((window->resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
                    window->width = LOWORD(lParam);
                    window->height = HIWORD(lParam);
                    window->framebufferResized = true;
                }
            }
            return 0;
        case WM_ENTERSIZEMOVE: // resizing started
            window->resizing = true;
            return 0;
        case WM_EXITSIZEMOVE: // resizing stopped
            window->resizing = false;
            return 0;
        case WM_DROPFILES:
            return 0;
        case WM_DPICHANGED:
            if (window) {
                window->HandleDpiChange(hWnd, wParam, lParam);
            }
            return 0;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}
#endif

Hmck::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
                     int windowHeight) : instance{instance} {
    width = windowWidth;
    height = windowHeight;
    windowName = _windowName;

#if defined(_WIN32)
    SetProcessDPIAware();

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = _windowName.c_str();

    if (!RegisterClassEx(&wc)) {
        Logger::log(LOG_LEVEL_ERROR, "Failed to register window class");
        throw std::runtime_error("Failed to register window class");
    }

    // Calculate window rect to account for borders and title bar
    RECT windowRect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int adjustedWidth = windowRect.right - windowRect.left;
    int adjustedHeight = windowRect.bottom - windowRect.top;

    hWnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        _windowName.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        adjustedWidth, adjustedHeight, // Use adjusted dimensions
        nullptr,
        nullptr,
        wc.hInstance,
        this
    );

    if (!hWnd) {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create window");
        throw std::runtime_error("Failed to create window");
    }

    // Store the actual client area dimensions
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;

    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = hWnd;
    surfaceCreateInfo.hinstance = wc.hInstance;

    if (vkCreateWin32SurfaceKHR(instance.getInstance(), &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create Vulkan surface");
        throw std::runtime_error("Failed to create Vulkan surface");
    }


#endif
}

Hmck::Window::~Window() {
    // Clear any resources that depend on the window
    keymap.clear();
    resizing = false;
    framebufferResized = false;

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

#if defined(_WIN32)
    // Clear message queue for this window
    MSG msg;
    while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hWnd) {
        // Don't destroy if already being destroyed
        if (IsWindow(hWnd)) {
            // This triggers WM_DESTROY
            DestroyWindow(hWnd);
        }
        hWnd = nullptr;
    }

    // Only unregister if we're the last window using this class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    if (GetClassInfoEx(GetModuleHandle(nullptr), windowName.c_str(), &wc)) {
        UnregisterClass(windowName.c_str(), GetModuleHandle(nullptr));
    }
#endif
}

// You might also want to add this helper method to your Window class
void Hmck::Window::HandleDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    UINT dpi = HIWORD(wParam);
    RECT *const pRect = reinterpret_cast<RECT *>(lParam);

    if (pRect) {
        SetWindowPos(hWnd,
                     nullptr,
                     pRect->left,
                     pRect->top,
                     pRect->right - pRect->left,
                     pRect->bottom - pRect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}


Hmck::KeyState Hmck::Window::getKeyState(Keycode key) {
    if (keymap.contains(key)) return keymap[key];
    return KeyState::NONE;
}

bool Hmck::Window::shouldClose() const {
return _shouldClose;
}

void Hmck::Window::pollEvents() {
#if defined(_WIN32)
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

#if defined(_WIN32)
void Hmck::Window::onKeyDown(WPARAM key) {
    keymap[key] = KeyState::DOWN;
}
#endif

#if defined(_WIN32)
void Hmck::Window::onKeyUp(WPARAM key) {
    keymap[key] = KeyState::UP;
}
#endif
