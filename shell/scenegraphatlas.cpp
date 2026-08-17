/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "scenegraphatlas.h"

#include <QQuickWindow>
#include <QSGRendererInterface>

namespace
{
// How many of the shell's windows are waiting for their scene graph to read the cap. Only ever touched from
// the GUI thread, where windows are created and destroyed.
int s_waiting = 0;

// One sheet a window instead of one sized from the screen. 2048 is the smallest size measured to hold
// everything the shell asks of a sheet: the busiest window of a session filled 4.66 MB of it, and a
// 1024x1024 sheet, at 4.0 MB, turned images away.
constexpr char AtlasSide[] = "2048";

void put()
{
    if (s_waiting++ == 0) {
        qputenv("QSG_ATLAS_WIDTH", AtlasSide);
        qputenv("QSG_ATLAS_HEIGHT", AtlasSide);
    }
}

void take()
{
    if (--s_waiting == 0) {
        qunsetenv("QSG_ATLAS_WIDTH");
        qunsetenv("QSG_ATLAS_HEIGHT");
    }
}

/*
 * Holds the cap in the environment for one window, from its own construction to its own destruction.
 *
 * A child of the window, so a window which never renders releases the cap by dying, and a window which
 * does releases it by having this deleted. Exactly one release either way, which matters: the count is
 * what decides whether the variables are in the environment at all, and letting it go negative would
 * leave every later window uncapped.
 */
class CapWhileComingUp : public QObject
{
public:
    explicit CapWhileComingUp(QQuickWindow *window)
        : QObject(window)
    {
        put();
    }

    ~CapWhileComingUp() override
    {
        take();
    }
};
}

void SceneGraphAtlas::capFor(QQuickWindow *window)
{
    // The software renderer has no atlas to size.
    if (window->rendererInterface() && window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        return;
    }

    auto *cap = new CapWhileComingUp(window);

    // The atlas is made while the scene graph starts and this signal is emitted at the end of that, so the
    // cap cannot come out before the window has read it. The signal comes from the render thread, and the
    // receiver belongs to the GUI thread, so it arrives queued and the count stays on one thread.
    QObject::connect(window, &QQuickWindow::sceneGraphInitialized, cap, [cap]() {
        delete cap;
    });
}
