#include "hammock/platform/Window.h"

#include "hammock/utils/Logger.h"
#include "hammock/platform/Keycodes.h"
#include "backends/imgui_impl_vulkan.h"
#include "hammock/utils/UserInterface.h"

#if defined(_WIN32)
#include "win32/resources.h"
#include "windows.h"
#include <windowsx.h>
#endif
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
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
            if ((window->Win32_resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
            {
                window->width = LOWORD(lParam);
                window->height = HIWORD(lParam);
                window->framebufferResized = true;
            }
        }
        return 0;
    case WM_ENTERSIZEMOVE: // resizing started
        window->Win32_resizing = true;
        return 0;
    case WM_EXITSIZEMOVE: // resizing stopped
        window->Win32_resizing = false;
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

#endif

Hammock::Window::Window(VulkanInstance &instance, const std::string &_windowName, int windowWidth,
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

    Win32_hWnd = CreateWindowEx(
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

    if (!Win32_hWnd)
    {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create window");
        throw std::runtime_error("Failed to create window");
    }

    // Store the actual client area dimensions
    RECT clientRect;
    GetClientRect(Win32_hWnd, &clientRect);
    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;

    SetWindowLongPtr(Win32_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(Win32_hWnd, SW_SHOW);
    UpdateWindow(Win32_hWnd);

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = Win32_hWnd;
    surfaceCreateInfo.hinstance = wc.hInstance;

    if (vkCreateWin32SurfaceKHR(instance.getInstance(), &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS)
    {
        Logger::log(LOG_LEVEL_ERROR, "Failed to create Vulkan surface");
        throw std::runtime_error("Failed to create Vulkan surface");
    }

#endif

#if defined(__linux__)

    X11_display = XOpenDisplay(nullptr);
    if (!X11_display)
    {
        Logger::log(LOG_LEVEL_ERROR, "Failed to open X display\n");
        throw std::runtime_error("Failed to open X display");
    }

    X11_root = DefaultRootWindow(X11_display);

    XSetWindowAttributes windowAttributes;
    windowAttributes.background_pixel = WhitePixel(X11_display, 0);
    windowAttributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    X11_window = XCreateWindow(X11_display, X11_root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &windowAttributes);

    XStoreName(X11_display, X11_window, windowName.c_str());
    XMapWindow(X11_display, X11_window);
    XFlush(X11_display);

    X11_wmDeleteMessage = XInternAtom(X11_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(X11_display, X11_window, &X11_wmDeleteMessage, 1);

    VkXlibSurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.dpy = X11_display;
    surfaceInfo.window = X11_window;

    VkResult result = vkCreateXlibSurfaceKHR(instance.getInstance(), &surfaceInfo, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan surface\n";
        exit(EXIT_FAILURE);
    }

#endif
}

Hammock::Window::~Window()
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
    while (PeekMessage(&msg, Win32_hWnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (Win32_hWnd)
    {
        // Don't destroy if already being destroyed
        if (IsWindow(Win32_hWnd))
        {
            // This triggers WM_DESTROY
            DestroyWindow(Win32_hWnd);
        }
        Win32_hWnd = nullptr;
    }

    // Only unregister if we're the last window using this class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    if (GetClassInfoEx(GetModuleHandle(nullptr), windowName.c_str(), &wc))
    {
        UnregisterClass(windowName.c_str(), GetModuleHandle(nullptr));
    }
#endif

#if defined(__linux__)
    if (X11_window)
    {
        XDestroyWindow(X11_display, X11_window);
        X11_window = 0;
    }

    if (X11_display)
    {
        XCloseDisplay(X11_display);
        X11_display = nullptr;
    }
#endif
}

Hammock::KeyState Hammock::Window::getKeyState(Keycode key)
{
    if (keymap.contains(key))
        return keymap[key];
    return KeyState::NONE;
}

Hammock::ButtonState Hammock::Window::getButtonState(Keycode button)
{
    if (buttonMap.contains(button))
        return buttonMap[button];
    return ButtonState::NONE;
}

bool Hammock::Window::shouldClose() const
{
    return _shouldClose;
}

void Hammock::Window::pollEvents()
{
#if defined(_WIN32)
    while (PeekMessage(&Win32_msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Win32_msg);
        DispatchMessage(&Win32_msg);
    }
#endif

#if defined(__linux__)
    XEvent event;
    while (XPending(X11_display) > 0)
    {
        XNextEvent(X11_display, &event);
        X11_processEvent(event);
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
    while (PeekMessage(&msg, Win32_hWnd, 0, 0, PM_REMOVE))
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

#if defined(__linux__)
void Hammock::Window::X11_onClose()
{
    _shouldClose = true;

    XEvent event;
    while (XPending(X11_display) > 0)
    {
        XNextEvent(X11_display, &event);
    }
}

void Hammock::Window::X11_processEvent(XEvent event)
{
    switch (event.type)
    {
    case ClientMessage:
    {
        X11_onClose();
        break;
    }
    case KeyPress:
    {
        KeySym keySym = XkbKeycodeToKeysym(X11_display, event.xkey.keycode, 0, 0);
        X11_onKeyDown(keySym);
        break;
    }
    case KeyRelease:
    {
        KeySym keySym = XkbKeycodeToKeysym(X11_display, event.xkey.keycode, 0, 0);
        X11_onKeyUp(keySym);
        break;
    }
    case ButtonPress:
    {
        switch (event.xbutton.button)
        {
        case Button1:
            buttonMap[MOUSE_LEFT] = ButtonState::DOWN;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Left, true);
            break;
        case Button2:
            buttonMap[MOUSE_MIDDLE] = ButtonState::DOWN;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Right, true);
            break;
        case Button3:
            buttonMap[MOUSE_RIGHT] = ButtonState::DOWN;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Middle, true);
            break;
        }
        break;
    }
    case ButtonRelease:
    {
        switch (event.xbutton.button)
        {
        case Button1:
            buttonMap[MOUSE_LEFT] = ButtonState::UP;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Left, false);
            break;
        case Button2:
            buttonMap[MOUSE_MIDDLE] = ButtonState::UP;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Right, false);
            break;
        case Button3:
            buttonMap[MOUSE_RIGHT] = ButtonState::UP;
            UserInterface::forwardButtonDownEvent(ImGuiMouseButton_Middle, false);
            break;
        }
        break;
    }
    case MotionNotify:
    {
        float xPos = static_cast<float>(event.xmotion.x);
        float yPos = static_cast<float>(event.xmotion.y);
        UserInterface::forwardMousePosition(xPos, yPos);
        mousePosition = HmckVec2{xPos, yPos};
        break;
    }
    case ConfigureNotify:
    {
        if (event.xconfigure.width != width || event.xconfigure.height != height)
        {
            width = event.xconfigure.width;
            height = event.xconfigure.height;
            framebufferResized = true;
        }
        break;
    }
    break;
    }
}
void Hammock::Window::X11_onKeyDown(KeySym key)
{
    keymap[key] = KeyState::DOWN;
}
void Hammock::Window::X11_onKeyUp(KeySym key)
{
    keymap[key] = KeyState::UP;
}
#endif
