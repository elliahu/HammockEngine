#include "HmckWindow.h"

#include "utils/HmckLogger.h"
#include "HmckKeycodes.h"
#include "backends/imgui_impl_vulkan.h"

#if defined(_WIN32)
#include "win32/resources.h"
#endif

#if defined(_WIN32)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Hmck::Window *window = reinterpret_cast<Hmck::Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CLOSE:
        if (window)
        {
            window->Win32_onClose();
        }
        return 0;
    case WM_DESTROY:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0); // Clear user data
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        return 0;
    case WM_KEYDOWN:
        if (window)
        {
            window->Win32_onKeyDown(wParam);
        }
        return 0;
    case WM_KEYUP:
        if (window)
        {
            window->Win32_onKeyUp(wParam);
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
        if (window && wParam != SIZE_MINIMIZED)
        {
            if ((window->resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
            {
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
        if (window)
        {
            window->Win32_onDpiChange(hWnd, wParam, lParam);
        }
        return 0;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}
#endif

#if defined(__linux__)

extern const struct wl_keyboard_listener keyboard_listener;
extern const struct wl_seat_listener seat_listener;

void registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Hmck::Window *self = static_cast<Hmck::Window *>(data);
  
    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor = static_cast<wl_compositor *>(
            wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat = static_cast<wl_seat *>(
            wl_registry_bind(registry, id, &wl_seat_interface, 1));
        wl_seat_add_listener(self->seat, &seat_listener, self);
    }
}

void keyboard_key_handler(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    Hmck::Window *self = static_cast<Hmck::Window *>(data);
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        self->keymap[key] = Hmck::KeyState::DOWN;
    }
    else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        self->keymap[key] = Hmck::KeyState::UP;
    }
}

void seat_capabilities_handler(void *data, struct wl_seat *seat, uint32_t capabilities)
{
    Hmck::Window *self = static_cast<Hmck::Window *>(data);
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        self->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(self->keyboard, &keyboard_listener, self);
    }
}

const struct wl_registry_listener registry_listener = {
    .global = registry_handler,
    .global_remove = nullptr,
};

const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities_handler,
    .name = nullptr,
};

const struct wl_keyboard_listener keyboard_listener = {
    .enter = nullptr,
    .leave = nullptr,
    .key = keyboard_key_handler,
    .modifiers = nullptr,
    .repeat_info = nullptr,
};

#endif

Hmck::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
                     int windowHeight) : instance{instance}
{
    width = windowWidth;
    height = windowHeight;
    windowName = _windowName;

#if defined(_WIN32)
    SetProcessDPIAware();

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = _windowName.c_str();

    // Load the icon
    wc.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_HAMMOCK_ICON));
    if (!wc.hIcon)
    {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Load default icon if custom icon fails
    }

    if (!RegisterClassEx(&wc))
    {
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
        this);

    if (!hWnd)
    {
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

    if (vkCreateWin32SurfaceKHR(instance.getInstance(), &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS)
    {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create Vulkan surface");
        throw std::runtime_error("Failed to create Vulkan surface");
    }

#endif

#if defined(__linux__)
    display = wl_display_connect(NULL);
    if (!display)
    {
        Logger::log(LOG_LEVEL_ERROR, "Failed to connect to Wayland display\n");
        throw std::runtime_error("Failed to connect to Wayland display");
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (!compositor)
    {
        throw std::runtime_error("Failed to get Wayland compositor");
    }

    _surface = wl_compositor_create_surface(compositor);
    if (!_surface)
    {
        throw std::runtime_error("Failed to create Wayland surface");
    }

    // Create Vulkan Wayland surface
    VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.display = display;
    surfaceCreateInfo.surface = _surface;

    if (vkCreateWaylandSurfaceKHR(instance.getInstance(), &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan Wayland surface");
    }
#endif
}

Hmck::Window::~Window()
{
    // Clear any resources that depend on the window
    keymap.clear();

    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

#if defined(_WIN32)
    // Clear message queue for this window
    MSG msg;
    while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hWnd)
    {
        // Don't destroy if already being destroyed
        if (IsWindow(hWnd))
        {
            // This triggers WM_DESTROY
            DestroyWindow(hWnd);
        }
        hWnd = nullptr;
    }

    // Only unregister if we're the last window using this class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    if (GetClassInfoEx(GetModuleHandle(nullptr), windowName.c_str(), &wc))
    {
        UnregisterClass(windowName.c_str(), GetModuleHandle(nullptr));
    }
#endif
}

// You might also want to add this helper method to your Window class

Hmck::KeyState Hmck::Window::getKeyState(Keycode key)
{
    if (keymap.contains(key))
        return keymap[key];
    return KeyState::NONE;
}

bool Hmck::Window::shouldClose() const
{
    return _shouldClose;
}

void Hmck::Window::pollEvents()
{
#if defined(_WIN32)
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

#if defined(_WIN32)
void Hmck::Window::Win32_onKeyDown(WPARAM key)
{
    keymap[key] = KeyState::DOWN;
}

void Hmck::Window::Win32_onKeyUp(WPARAM key)
{
    keymap[key] = KeyState::UP;
}

void Hmck::Window::Win32_onClose()
{
    // Set should close first
    _shouldClose = true;

    // Wait for any pending messages
    MSG msg;
    while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Hmck::Window::Win32_onDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UINT dpi = HIWORD(wParam);
    RECT *const pRect = reinterpret_cast<RECT *>(lParam);

    if (pRect)
    {
        SetWindowPos(hWnd,
                     nullptr,
                     pRect->left,
                     pRect->top,
                     pRect->right - pRect->left,
                     pRect->bottom - pRect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

#endif
