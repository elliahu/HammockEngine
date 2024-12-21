#include "HmckWindow.h"

#include "utils/HmckLogger.h"
#include "HmckKeycodes.h"

#if defined(_WIN32)
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Hmck::Window *window = reinterpret_cast<Hmck::Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
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
    // Step 2: Register the window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc; // Window procedure function
    wc.hInstance = GetModuleHandle(nullptr); // Handle to the application instance
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); // Default arrow cursor
    wc.lpszClassName = _windowName.c_str(); // Window class name

    if (!RegisterClassEx(&wc)) {
        Logger::log(LOG_LEVEL_ERROR, "Failed to register window class");
        throw std::runtime_error("Failed to register window class");
    }

    hWnd = CreateWindowEx(
        0, // Optional window styles
        wc.lpszClassName, // Window class name
        _windowName.c_str(), // Window title
        WS_OVERLAPPEDWINDOW, // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Position (x, y)
        windowWidth, windowHeight, // Size (width, height)
        nullptr, // Parent window
        nullptr, // Menu
        wc.hInstance, // Application instance
        nullptr // Additional application data
    );

    if (!hWnd) {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create window");
        throw std::runtime_error("Failed to create window");
    }

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
    vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);

#if defined(_WIN32)
    if (hWnd) {
        DestroyWindow(hWnd);
    }
    UnregisterClass("VulkanWindowClass", GetModuleHandle(nullptr));
#endif
}


Hmck::KeyState Hmck::Window::getKeyState(Keycode key) {
    if (keymap.contains(key)) return keymap[key];
    return KeyState::NONE;
}

bool Hmck::Window::shouldClose() const {
#if defined(_WIN32)
    return msg.message == WM_QUIT;
#endif
}

void Hmck::Window::pollEvents() {
#if defined(_WIN32)
    keymap.clear();
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
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
