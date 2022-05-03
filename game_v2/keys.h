#ifndef KEYS_H
#define KEYS_H

#define JUST_PRESSED(KEY) (keys[KEY] == KS_PRESSED && keys_last_frame[KEY] == KS_RELEASED)
#define JUST_RELEASED(KEY) (keys[KEY] == KS_RELEASED && keys_last_frame[KEY] == KS_PRESSED)

enum Keys {
    K_SPACE, K_ENTER, K_TAB, K_BACKTICK, K_ESCAPE, K_BACKSPACE,
    K_A, K_B, K_C, K_D, K_E, K_F, K_G, K_H, K_I, K_J, K_K, K_L, K_M, K_N, K_O, K_P, K_Q, K_R, K_S, K_T, K_U, K_V, K_W, K_X, K_Y, K_Z,
    K_0, K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9,
    K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12,
    K_LAST
};

enum KeyModifiers {
    KM_CONTROL,
    KM_ALT,
    KM_SHIFT
};

enum KeyState {
    KS_RELEASED,
    KS_PRESSED
};

#endif /* KEYS_H */
