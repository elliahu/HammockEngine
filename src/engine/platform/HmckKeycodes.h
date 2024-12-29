#pragma once
#if defined(__linux__)
#include <xkbcommon/xkbcommon.h>
#endif

namespace Hmck{

    typedef unsigned long long Keycode;

    enum class KeyState {
        NONE, DOWN, UP
    };

    enum class ButtonState {
        NONE, DOWN, UP
    };

#define MOUSE_LEFT 0x0
#define MOUSE_RIGHT 0x1
#define MOUSE_MIDDLE 0x2

#if defined(_WIN32)
#define KEY_A 0x41
#define KEY_B 0x42
#define KEY_C 0x43
#define KEY_D 0x44
#define KEY_E 0x45
#define KEY_F 0x46
#define KEY_G 0x47
#define KEY_H 0x48
#define KEY_I 0x49
#define KEY_J 0x4A
#define KEY_K 0x4B
#define KEY_L 0x4C
#define KEY_M 0x4D
#define KEY_N 0x4E
#define KEY_O 0x4F
#define KEY_P 0x50
#define KEY_Q 0x51
#define KEY_R 0x52
#define KEY_S 0x53
#define KEY_T 0x54
#define KEY_U 0x55
#define KEY_V 0x56
#define KEY_W 0x57
#define KEY_X 0x58
#define KEY_Y 0x59
#define KEY_Z 0x5A

#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_F6 VK_F6
#define KEY_F7 VK_F7
#define KEY_F8 VK_F8
#define KEY_F9 VK_F9
#define KEY_F10 VK_F10
#define KEY_F11 VK_F11
#define KEY_F12 VK_F12

#define KEY_ESCAPE VK_ESCAPE
#define KEY_TAB VK_TAB
#define KEY_RETURN VK_RETURN
#define KEY_SHIFT VK_SHIFT
#define KEY_CONTROL VK_CONTROL
#define KEY_MENU VK_MENU
#define KEY_CAPITAL VK_CAPITAL
#define KEY_NUMLOCK VK_NUMLOCK
#define KEY_SCROLL VK_SCROLL
#define KEY_PAUSE VK_PAUSE

#define KEY_UP VK_UP
#define KEY_DOWN VK_DOWN
#define KEY_LEFT VK_LEFT
#define KEY_RIGHT VK_RIGHT

#define KEY_NUMPAD0 VK_NUMPAD0
#define KEY_NUMPAD1 VK_NUMPAD1
#define KEY_NUMPAD2 VK_NUMPAD2
#define KEY_NUMPAD3 VK_NUMPAD3
#define KEY_NUMPAD4 VK_NUMPAD4
#define KEY_NUMPAD5 VK_NUMPAD5
#define KEY_NUMPAD6 VK_NUMPAD6
#define KEY_NUMPAD7 VK_NUMPAD7
#define KEY_NUMPAD8 VK_NUMPAD8
#define KEY_NUMPAD9 VK_NUMPAD9
#define KEY_MULTIPLY VK_MULTIPLY
#define KEY_ADD VK_ADD
#define KEY_SEPARATOR VK_SEPARATOR
#define KEY_SUBTRACT VK_SUBTRACT
#define KEY_DECIMAL VK_DECIMAL
#define KEY_DIVIDE VK_DIVIDE

#define KEY_SPACE VK_SPACE
#define KEY_BACK VK_BACK
#define KEY_DELETE VK_DELETE
#define KEY_INSERT VK_INSERT
#define KEY_HOME VK_HOME
#define KEY_END VK_END
#define KEY_PAGEUP VK_PRIOR
#define KEY_PAGEDOWN VK_NEXT

#define KEY_LSHIFT VK_LSHIFT
#define KEY_RSHIFT VK_RSHIFT
#define KEY_LCONTROL VK_LCONTROL
#define KEY_RCONTROL VK_RCONTROL
#define KEY_LMENU VK_LMENU
#define KEY_RMENU VK_RMENU

#define KEY_OEM_1 VK_OEM_1
#define KEY_OEM_2 VK_OEM_2
#define KEY_OEM_3 VK_OEM_3
#define KEY_OEM_4 VK_OEM_4
#define KEY_OEM_5 VK_OEM_5
#define KEY_OEM_6 VK_OEM_6
#define KEY_OEM_7 VK_OEM_7
#endif

#if defined(__linux__)
#define KEY_A XKB_KEY_a
#define KEY_B XKB_KEY_b
#define KEY_C XKB_KEY_c
#define KEY_D XKB_KEY_d
#define KEY_E XKB_KEY_e
#define KEY_F XKB_KEY_f
#define KEY_G XKB_KEY_g
#define KEY_H XKB_KEY_h
#define KEY_I XKB_KEY_i
#define KEY_J XKB_KEY_j
#define KEY_K XKB_KEY_k
#define KEY_L XKB_KEY_l
#define KEY_M XKB_KEY_m
#define KEY_N XKB_KEY_n
#define KEY_O XKB_KEY_o
#define KEY_P XKB_KEY_p
#define KEY_Q XKB_KEY_q
#define KEY_R XKB_KEY_r
#define KEY_S XKB_KEY_s
#define KEY_T XKB_KEY_t
#define KEY_U XKB_KEY_u
#define KEY_V XKB_KEY_v
#define KEY_W XKB_KEY_w
#define KEY_X XKB_KEY_x
#define KEY_Y XKB_KEY_y
#define KEY_Z XKB_KEY_z

#define KEY_F1 XKB_KEY_F1
#define KEY_F2 XKB_KEY_F2
#define KEY_F3 XKB_KEY_F3
#define KEY_F4 XKB_KEY_F4
#define KEY_F5 XKB_KEY_F5
#define KEY_F6 XKB_KEY_F6
#define KEY_F7 XKB_KEY_F7
#define KEY_F8 XKB_KEY_F8
#define KEY_F9 XKB_KEY_F9
#define KEY_F10 XKB_KEY_F10
#define KEY_F11 XKB_KEY_F11
#define KEY_F12 XKB_KEY_F12

#define KEY_ESCAPE XKB_KEY_Escape
#define KEY_TAB XKB_KEY_Tab
#define KEY_RETURN XKB_KEY_Return
#define KEY_SHIFT XKB_KEY_Shift_L
#define KEY_CONTROL XKB_KEY_Control_L
#define KEY_MENU XKB_KEY_Alt_L
#define KEY_CAPITAL XKB_KEY_Caps_Lock
#define KEY_NUMLOCK XKB_KEY_Num_Lock
#define KEY_SCROLL XKB_KEY_Scroll_Lock
#define KEY_PAUSE XKB_KEY_Pause

#define KEY_UP XKB_KEY_Up
#define KEY_DOWN XKB_KEY_Down
#define KEY_LEFT XKB_KEY_Left
#define KEY_RIGHT XKB_KEY_Right

#define KEY_NUMPAD0 XKB_KEY_KP_0
#define KEY_NUMPAD1 XKB_KEY_KP_1
#define KEY_NUMPAD2 XKB_KEY_KP_2
#define KEY_NUMPAD3 XKB_KEY_KP_3
#define KEY_NUMPAD4 XKB_KEY_KP_4
#define KEY_NUMPAD5 XKB_KEY_KP_5
#define KEY_NUMPAD6 XKB_KEY_KP_6
#define KEY_NUMPAD7 XKB_KEY_KP_7
#define KEY_NUMPAD8 XKB_KEY_KP_8
#define KEY_NUMPAD9 XKB_KEY_KP_9
#define KEY_MULTIPLY XKB_KEY_KP_Multiply
#define KEY_ADD XKB_KEY_KP_Add
#define KEY_SEPARATOR XKB_KEY_KP_Separator
#define KEY_SUBTRACT XKB_KEY_KP_Subtract
#define KEY_DECIMAL XKB_KEY_KP_Decimal
#define KEY_DIVIDE XKB_KEY_KP_Divide

#define KEY_SPACE XKB_KEY_space
#define KEY_BACK XKB_KEY_BackSpace
#define KEY_DELETE XKB_KEY_Delete
#define KEY_INSERT XKB_KEY_Insert
#define KEY_HOME XKB_KEY_Home
#define KEY_END XKB_KEY_End
#define KEY_PAGEUP XKB_KEY_Page_Up
#define KEY_PAGEDOWN XKB_KEY_Page_Down

#define KEY_LSHIFT XKB_KEY_Shift_L
#define KEY_RSHIFT XKB_KEY_Shift_R
#define KEY_LCONTROL XKB_KEY_Control_L
#define KEY_RCONTROL XKB_KEY_Control_R
#define KEY_LMENU XKB_KEY_Alt_L
#define KEY_RMENU XKB_KEY_Alt_R

#define KEY_OEM_1 XKB_KEY_semicolon
#define KEY_OEM_2 XKB_KEY_slash
#define KEY_OEM_3 XKB_KEY_grave
#define KEY_OEM_4 XKB_KEY_bracketleft
#define KEY_OEM_5 XKB_KEY_backslash
#define KEY_OEM_6 XKB_KEY_bracketright
#define KEY_OEM_7 XKB_KEY_apostrophe
#endif

}