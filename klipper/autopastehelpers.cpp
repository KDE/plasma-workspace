/*
    SPDX-FileCopyrightText: 2026 Guillermo Steren

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autopastehelpers.h"

#include "klipper_debug.h"

#include <QGuiApplication>
#include <QKeyCombination>
#include <QKeySequence>

#if defined(__linux__) || defined(__FreeBSD__)
#include <KWayland/Client/fakeinput.h>
#if defined(__linux__)
#include <linux/input-event-codes.h>
#else
#include "evdevpastekeys.h"
#endif
#endif

#if HAVE_X11
#include <KWindowSystem>
#include <QtGui/qguiapplication_platform.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#undef Bool
#undef Expose
#endif

namespace KlipperAutoPaste
{

PasteChord pasteChordFromStandard()
{
    const QList<QKeySequence> bindings = QKeySequence::keyBindings(QKeySequence::Paste);
    for (const QKeySequence &seq : bindings) {
        const int n = seq.count();
        for (int i = 0; i < n; ++i) {
            const QKeyCombination combo = seq[i];
            const Qt::Key key = combo.key();
            if (key == Qt::Key_unknown || key == Qt::Key(0)) {
                continue;
            }
            return {combo.keyboardModifiers(), key};
        }
    }
    return {};
}

#if defined(__linux__) || defined(__FreeBSD__)
int qtLatinKeyToEvdev(Qt::Key key)
{
    switch (key) {
    case Qt::Key_A:
        return KEY_A;
    case Qt::Key_B:
        return KEY_B;
    case Qt::Key_C:
        return KEY_C;
    case Qt::Key_D:
        return KEY_D;
    case Qt::Key_E:
        return KEY_E;
    case Qt::Key_F:
        return KEY_F;
    case Qt::Key_G:
        return KEY_G;
    case Qt::Key_H:
        return KEY_H;
    case Qt::Key_I:
        return KEY_I;
    case Qt::Key_J:
        return KEY_J;
    case Qt::Key_K:
        return KEY_K;
    case Qt::Key_L:
        return KEY_L;
    case Qt::Key_M:
        return KEY_M;
    case Qt::Key_N:
        return KEY_N;
    case Qt::Key_O:
        return KEY_O;
    case Qt::Key_P:
        return KEY_P;
    case Qt::Key_Q:
        return KEY_Q;
    case Qt::Key_R:
        return KEY_R;
    case Qt::Key_S:
        return KEY_S;
    case Qt::Key_T:
        return KEY_T;
    case Qt::Key_U:
        return KEY_U;
    case Qt::Key_V:
        return KEY_V;
    case Qt::Key_W:
        return KEY_W;
    case Qt::Key_X:
        return KEY_X;
    case Qt::Key_Y:
        return KEY_Y;
    case Qt::Key_Z:
        return KEY_Z;
    default:
        return KEY_V;
    }
}

void appendModifierEvdev(Qt::KeyboardModifiers m, QList<int> *out)
{
    if (m & Qt::MetaModifier) {
        out->append(KEY_LEFTMETA);
    }
    if (m & Qt::ControlModifier) {
        out->append(KEY_LEFTCTRL);
    }
    if (m & Qt::AltModifier) {
        out->append(KEY_LEFTALT);
    }
    if (m & Qt::ShiftModifier) {
        out->append(KEY_LEFTSHIFT);
    }
}

void injectPasteShortcutWaylandFakeInput(KWayland::Client::FakeInput *fakeInput, const PasteChord &chord)
{
    QList<int> mods;
    appendModifierEvdev(chord.modifiers, &mods);
    const int keyEv = qtLatinKeyToEvdev(chord.key);

    for (const int k : mods) {
        fakeInput->requestKeyboardKeyPress(k);
    }
    fakeInput->requestKeyboardKeyPress(keyEv);
    fakeInput->requestKeyboardKeyRelease(keyEv);
    for (qsizetype i = mods.size() - 1; i >= 0; --i) {
        fakeInput->requestKeyboardKeyRelease(mods.at(i));
    }
}

#endif

#if HAVE_X11
KeySym qtLatinKeyToXKeysym(Qt::Key key)
{
    switch (key) {
    case Qt::Key_A:
        return XK_a;
    case Qt::Key_B:
        return XK_b;
    case Qt::Key_C:
        return XK_c;
    case Qt::Key_D:
        return XK_d;
    case Qt::Key_E:
        return XK_e;
    case Qt::Key_F:
        return XK_f;
    case Qt::Key_G:
        return XK_g;
    case Qt::Key_H:
        return XK_h;
    case Qt::Key_I:
        return XK_i;
    case Qt::Key_J:
        return XK_j;
    case Qt::Key_K:
        return XK_k;
    case Qt::Key_L:
        return XK_l;
    case Qt::Key_M:
        return XK_m;
    case Qt::Key_N:
        return XK_n;
    case Qt::Key_O:
        return XK_o;
    case Qt::Key_P:
        return XK_p;
    case Qt::Key_Q:
        return XK_q;
    case Qt::Key_R:
        return XK_r;
    case Qt::Key_S:
        return XK_s;
    case Qt::Key_T:
        return XK_t;
    case Qt::Key_U:
        return XK_u;
    case Qt::Key_V:
        return XK_v;
    case Qt::Key_W:
        return XK_w;
    case Qt::Key_X:
        return XK_x;
    case Qt::Key_Y:
        return XK_y;
    case Qt::Key_Z:
        return XK_z;
    default:
        return XK_v;
    }
}

void appendModifierKeySyms(Qt::KeyboardModifiers m, QList<KeySym> *out)
{
    if (m & Qt::MetaModifier) {
        out->append(XK_Meta_L);
    }
    if (m & Qt::ControlModifier) {
        out->append(XK_Control_L);
    }
    if (m & Qt::AltModifier) {
        out->append(XK_Alt_L);
    }
    if (m & Qt::ShiftModifier) {
        out->append(XK_Shift_L);
    }
}

bool x11AutoPasteInjectionAvailable()
{
    if (KWindowSystem::isPlatformWayland()) {
        return false;
    }
    if (auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        int ev = 0;
        int err = 0;
        int maj = 0;
        int min = 0;
        return XTestQueryExtension(x11->display(), &ev, &err, &maj, &min);
    }
    return false;
}

void injectPasteShortcutX11(void *display)
{
    auto *dpy = static_cast<Display *>(display);
    const PasteChord chord = pasteChordFromStandard();
    QList<KeySym> modSyms;
    appendModifierKeySyms(chord.modifiers, &modSyms);
    const KeySym keySym = qtLatinKeyToXKeysym(chord.key);

    for (const KeySym ks : modSyms) {
        const KeyCode c = XKeysymToKeycode(dpy, ks);
        if (c != 0) {
            XTestFakeKeyEvent(dpy, c, true, CurrentTime);
        }
    }
    const KeyCode keyCode = XKeysymToKeycode(dpy, keySym);
    if (keyCode == 0) {
        qCWarning(KLIPPER_LOG) << "Auto-paste: could not map key for Paste shortcut";
        return;
    }
    XTestFakeKeyEvent(dpy, keyCode, true, CurrentTime);
    XTestFakeKeyEvent(dpy, keyCode, false, CurrentTime);
    for (qsizetype i = modSyms.size() - 1; i >= 0; --i) {
        const KeyCode c = XKeysymToKeycode(dpy, modSyms.at(i));
        if (c != 0) {
            XTestFakeKeyEvent(dpy, c, false, CurrentTime);
        }
    }
    XFlush(dpy);
}
#endif

} // namespace KlipperAutoPaste
