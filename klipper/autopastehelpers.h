/*
    SPDX-FileCopyrightText: 2026 Guillermo Steren

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <config-X11.h>

#include <QList>
#include <Qt>

#if defined(__linux__) || defined(__FreeBSD__)
namespace KWayland::Client {
class FakeInput;
}
#endif

namespace KlipperAutoPaste
{

struct PasteChord {
    Qt::KeyboardModifiers modifiers = Qt::ControlModifier;
    Qt::Key key = Qt::Key_V;
};

/** Paste shortcut from the user's standard key bindings (first binding wins). */
[[nodiscard]] PasteChord pasteChordFromStandard();

#if defined(__linux__) || defined(__FreeBSD__)
void appendModifierEvdev(Qt::KeyboardModifiers modifiers, QList<int> *out);
#endif

#if HAVE_X11
bool x11AutoPasteInjectionAvailable();
void injectPasteShortcutX11(void *display);
#endif

#if defined(__linux__) || defined(__FreeBSD__)
void injectPasteShortcutWaylandFakeInput(KWayland::Client::FakeInput *fakeInput, const PasteChord &chord);
#endif

} // namespace KlipperAutoPaste
