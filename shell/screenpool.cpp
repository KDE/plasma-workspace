/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "screenpool.h"
#include "outputorderwatcher.h"
#include "screenpool-debug.h"

#include <KWindowSystem>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>

/*
 * The design is as follows:
 *  - screenRemoval is always handled instantly, it must be processed before QScreen::~QScreen happens
 *  - scren insertion or other changes is batched until everything is in sync
 */

ScreenPool::ScreenPool(QObject *parent)
    : QObject(parent)
    , m_outputOrderWatcher(new OutputOrderWatcher(this))
{
    qRegisterMetaType<QList<QScreen *>>("QList<QScreen *>");

    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ScreenPool::handleScreenRemoved);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ScreenPool::handleUpdate);
    connect(m_outputOrderWatcher, &OutputOrderWatcher::outputOrderChanged, this, &ScreenPool::handleUpdate);
}

ScreenPool::~ScreenPool() = default;

int ScreenPool::idForName(const QString &connector) const
{
    int i = 0;
    for (auto *s : m_availableScreens) {
        if (s->name() == connector) {
            return i;
        }
        ++i;
    }
    return -1;
}

QList<QScreen *> ScreenPool::screenOrder() const
{
    return m_availableScreens;
}

QScreen *ScreenPool::primaryScreen() const
{
    if (m_availableScreens.isEmpty()) {
        return nullptr;
    }
    return m_availableScreens.first();
}

void ScreenPool::handleScreenRemoved(QScreen *screen)
{
    if (m_availableScreens.contains(screen)) {
        Q_EMIT screenRemoved(screen);
        m_availableScreens.removeOne(screen);
        Q_EMIT screenOrderChanged(m_availableScreens);
    }
}

void ScreenPool::handleUpdate()
{
    QList<QScreen *> screens = qApp->screens();

    // Ignore all fake screens
    screens.removeIf([](QScreen *screen) {
        return screen->name().isEmpty();
    });

    const QStringList outputOrder = m_outputOrderWatcher->outputOrder();

    auto screenNames = screens | std::views::transform(&QScreen::name) | std::ranges::to<QSet>();

    qCDebug(SCREENPOOL) << "Trying refresh, screens:" << screenNames << "output order:" << outputOrder;

    if (screenNames != QSet<QString>(outputOrder.cbegin(), outputOrder.cend())) {
        qCDebug(SCREENPOOL) << "Output order doesn't match the screens we have yet, ignoring it";
        return;
    }

    QHash<QString, int> order;
    for (auto [i, name] : std::views::enumerate(outputOrder)) {
        order.insert(name, i);
    }

    QList<QScreen *> newAvailableScreens = screens;
    std::ranges::sort(newAvailableScreens, {}, [&](QScreen *screen) {
        return order.value(screen->name());
    });

    qCDebug(SCREENPOOL) << "Resolved to:" << newAvailableScreens;

    if (m_availableScreens != newAvailableScreens) {
        m_availableScreens = newAvailableScreens;
        Q_EMIT screenOrderChanged(m_availableScreens);
    }
}

QScreen *ScreenPool::screenForId(int id) const
{
    if (id < 0 || m_availableScreens.size() <= id) {
        return nullptr;
    }

    return m_availableScreens[id];
}

int ScreenPool::idForScreen(const QScreen *screen) const
{
    return m_availableScreens.indexOf(screen);
}

QDebug operator<<(QDebug debug, const ScreenPool *pool)
{
    debug << pool->metaObject()->className() << '(' << static_cast<const void *>(pool) << ") Internal state:\n";
    debug << "All screen that QGuiApplication knows:\t" << qGuiApp->screens() << '\n';
    debug << "Screen order from outputOrderWatcher" << pool->m_outputOrderWatcher->outputOrder() << '\n';
    debug << "Sorted list " << pool->m_availableScreens;
    return debug;
}

#include "moc_screenpool.cpp"
