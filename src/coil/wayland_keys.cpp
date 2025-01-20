module;

#include <linux/input-event-codes.h>
#include <cstdint>

module coil.core.wayland;

namespace Coil
{
  InputKey WaylandWindowSystem::ConvertKey(uint32_t code)
  {
    InputKey key;
    switch(code)
    {
#define C(c, k) case KEY_##c: key = InputKey::k; break

    C(BACKSPACE, BackSpace);
    C(TAB, Tab);
    C(LINEFEED, LineFeed);
    C(CLEAR, Clear);
    C(ENTER, Return);
    C(PAUSE, Pause);
    C(SCROLLLOCK, ScrollLock);
    // C(SysReq, SysReq);
    C(ESC, Escape);
    C(INSERT, Insert);
    C(DELETE, Delete);

    C(SPACE, Space);

    C(COMMA, Comma);
    C(MINUS, Hyphen);
    C(DOT, Period);
    C(SLASH, Slash);

    C(0, _0);
    C(1, _1);
    C(2, _2);
    C(3, _3);
    C(4, _4);
    C(5, _5);
    C(6, _6);
    C(7, _7);
    C(8, _8);
    C(9, _9);

    C(A, A);
    C(B, B);
    C(C, C);
    C(D, D);
    C(E, E);
    C(F, F);
    C(G, G);
    C(H, H);
    C(I, I);
    C(J, J);
    C(K, K);
    C(L, L);
    C(M, M);
    C(N, N);
    C(O, O);
    C(P, P);
    C(Q, Q);
    C(R, R);
    C(S, S);
    C(T, T);
    C(U, U);
    C(V, V);
    C(W, W);
    C(X, X);
    C(Y, Y);
    C(Z, Z);

    C(HOME, Home);
    C(LEFT, Left);
    C(UP, Up);
    C(RIGHT, Right);
    C(DOWN, Down);
    C(PAGEUP, PageUp);
    C(PAGEDOWN, PageDown);
    C(END, End);
    // C(BEGIN, Begin);
    C(GRAVE, Grave);

    C(NUMLOCK, NumLock);
    // C(SPACE, NumPadSpace);
    // C(TAB, NumPadTab);
    // C(ENTER, NumPadEnter);
    // C(F1, NumPadF1);
    // C(F2, NumPadF2);
    // C(F3, NumPadF3);
    // C(F4, NumPadF4);
    // C(HOME, NumPadHome);
    // C(LEFT, NumPadLeft);
    // C(UP, NumPadUp);
    // C(RIGHT, NumPadRight);
    // C(DOWN, NumPadDown);
    // C(PAGEUP, NumPadPageUp);
    // C(PAGEDOWN, NumPadPageDown);
    // C(END, NumPadEnd);
    // C(BEGIN, NumPadBegin);
    // C(INSERT, NumPadInsert);
    // C(DELETE, NumPadDelete);
    // C(EQUAL, NumPadEqual);
    // C(STAR, NumPadMultiply);
    // C(ADD, NumPadAdd);
    // C(SEPARATOR, NumPadSeparator);
    // C(SUBTRACT, NumPadSubtract);
    // C(DECIMAL, NumPadDecimal);
    // C(DIVIDE, NumPadDivide);

    C(KP0, NumPad0);
    C(KP1, NumPad1);
    C(KP2, NumPad2);
    C(KP3, NumPad3);
    C(KP4, NumPad4);
    C(KP5, NumPad5);
    C(KP6, NumPad6);
    C(KP7, NumPad7);
    C(KP8, NumPad8);
    C(KP9, NumPad9);

    C(F1, F1);
    C(F2, F2);
    C(F3, F3);
    C(F4, F4);
    C(F5, F5);
    C(F6, F6);
    C(F7, F7);
    C(F8, F8);
    C(F9, F9);
    C(F10, F10);
    C(F11, F11);
    C(F12, F12);

    C(LEFTSHIFT, ShiftL);
    C(RIGHTSHIFT, ShiftR);
    C(LEFTCTRL, ControlL);
    C(RIGHTCTRL, ControlR);
    C(CAPSLOCK, CapsLock);
    C(LEFTALT, AltL);
    C(RIGHTALT, AltR);
    C(LEFTMETA, SuperL);
    C(RIGHTMETA, SuperR);

#undef C

    default:
      key = InputKey::_Unknown;
      break;
    }

    return key;
  }
}
