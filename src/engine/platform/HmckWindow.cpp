#include "HmckWindow.h"

#include "utils/HmckLogger.h"
#include "HmckKeycodes.h"
#include "backends/imgui_impl_vulkan.h"
#include "utils/HmckUserInterface.h"

#if defined(_WIN32)
#include "win32/resources.h"
#include "windows.h"
#include <windowsx.h>
#endif
#if defined(__linux__)
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include <sys/mman.h>
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
    case WM_CHAR:
        // Pass the character to ImGui
        Hmck::UserInterface::forwardInputCharacter((unsigned short)wParam);
        return 0;
    case WM_LBUTTONDOWN:
    {
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Left, true);
        if (window)
        {
            window->buttonMap[MOUSE_LEFT] = Hmck::ButtonState::DOWN;
        }
        return 0;
    }
    case WM_RBUTTONDOWN:
    {
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Right, true);
        if (window)
        {
            window->buttonMap[MOUSE_RIGHT] = Hmck::ButtonState::DOWN;
        }
        return 0;
    }
    case WM_MBUTTONDOWN:
    {
        // mouse middle down
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Middle, true);
        if (window)
        {
            window->buttonMap[MOUSE_MIDDLE] = Hmck::ButtonState::DOWN;
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        // mouse left up
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Left, false);
        if (window)
        {
            window->buttonMap[MOUSE_LEFT] = Hmck::ButtonState::UP;
        }
        return 0;
    }
    case WM_RBUTTONUP:
    {
        // mouse right up
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Right, false);
        if (window)
        {
            window->buttonMap[MOUSE_RIGHT] = Hmck::ButtonState::UP;
        }
        return 0;
    }
    case WM_MBUTTONUP:
    {
        // mouse middle up
        Hmck::UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Middle, false);
        if (window)
        {
            window->buttonMap[MOUSE_MIDDLE] = Hmck::ButtonState::UP;
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
        return 0;
    case WM_MOUSEMOVE:
    {
        float xPos = static_cast<float>(GET_X_LPARAM(lParam));
        float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
        Hmck::UserInterface::forwardMousePosition(xPos, yPos);
        if (window)
            window->mousePosition = HmckVec2{xPos, yPos};
        return 0;
    }
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
    return 0;
}
#endif

#if defined(__linux__)

extern const struct wl_keyboard_listener keyboard_listener;
extern const struct wl_seat_listener seat_listener;

void registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Hmck::Window *self = static_cast<Hmck::Window *>(data);

    if (interface == nullptr || data == nullptr)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Failed to get Wayland interface or data");
        return;
    }

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor = static_cast<wl_compositor *>(
            wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    }
    else if (strcmp(interface, "xdg_wm_base") == 0)
    {
        self->xdg_wm_base = static_cast<xdg_wm_base *>(
            wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat = static_cast<wl_seat *>(
            wl_registry_bind(registry, id, &wl_seat_interface, 1));
        wl_seat_add_listener(self->seat, &seat_listener, self);
    }

    if (strcmp(interface, "zxdg_decoration_manager_v1") == 0)
    {
        self->decoration_manager = (struct zxdg_decoration_manager_v1 *)
            wl_registry_bind(registry, id, &zxdg_decoration_manager_v1_interface, 1);
    }
}

void keyboard_key_handler(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    if (!data)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Keyboard handler received null data");
        return;
    }
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
    if (!data)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Seat capabilities handler received null data");
        return;
    }
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

void keyboard_keymap_handler(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
    // Ensure data is valid
    if (!data)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Keyboard keymap handler received null data");
        return;
    }
    Hmck::Window *self = static_cast<Hmck::Window *>(data);

    // Only handle the keymap if it's in the xkb_v1 format
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Unsupported keymap format");
        close(fd);
        return;
    }

    // Read the keymap data from the file descriptor
    char *keymap_string = static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    if (keymap_string == MAP_FAILED)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Failed to map keymap");
        close(fd);
        return;
    }

    // Create an xkb context
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!context)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Failed to create xkb context");
        munmap(keymap_string, size);
        close(fd);
        return;
    }

    // Create the keymap from the string
    struct xkb_keymap *keymap = xkb_keymap_new_from_string(context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(keymap_string, size);
    close(fd);
    if (!keymap)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Failed to create xkb keymap");
        xkb_context_unref(context);
        return;
    }

    // Create the xkb state
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state)
    {
        Hmck::Logger::log(Hmck::LOG_LEVEL_ERROR, "Failed to create xkb state");
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return;
    }

    // Store the xkb state and keymap in the window structure
    self->xkb_context = context;
    self->xkb_keymap = keymap;
    self->xkb_state = state;
}

const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap_handler,
    .enter = nullptr,
    .leave = nullptr,
    .key = keyboard_key_handler,
    .modifiers = nullptr,
    .repeat_info = nullptr,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
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
    wl_registry_add_listener(registry, &registry_listener, this); // Changed NULL to this
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

    // Add XDG shell support
    if (!xdg_wm_base)
    {
        throw std::runtime_error("Failed to get XDG shell");
    }

    // Add the XDG WM base listener
    xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, nullptr);

    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, _surface);
    if (!xdg_surface)
    {
        throw std::runtime_error("Failed to create XDG surface");
    }

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    if (!xdg_toplevel)
    {
        throw std::runtime_error("Failed to create toplevel surface");
    }

    if (decoration_manager)
    {
        toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
            static_cast<zxdg_decoration_manager_v1 *>(decoration_manager), xdg_toplevel);
        if (toplevel_decoration)
        {
            zxdg_decoration_manager_v1_get_toplevel_decoration(
                static_cast<zxdg_decoration_manager_v1 *>(decoration_manager), xdg_toplevel);
        }
    }

    // Set window properties
    xdg_toplevel_set_title(xdg_toplevel, windowName.c_str());
    xdg_toplevel_set_app_id(xdg_toplevel, windowName.c_str());

    // Request window decorations
    struct wl_array decorations;
    wl_array_init(&decorations);

    // Initial commit to apply the configuration
    wl_surface_commit(_surface);

    // Wait for the initial configure event
    wl_display_roundtrip(display);

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

Hmck::KeyState Hmck::Window::getKeyState(Keycode key)
{
    if (keymap.contains(key))
        return keymap[key];
    return KeyState::NONE;
}

Hmck::ButtonState Hmck::Window::getButtonState(Keycode button)
{
    if (buttonMap.contains(button))
        return buttonMap[button];
    return ButtonState::NONE;
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
