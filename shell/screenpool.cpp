/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "screenpool.h"
#include "outputorderwatcher.h"
#include "screenpool-debug.h"

#include <KWindowSystem>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#ifndef NDEBUG
#define CHECK_SCREEN_INVARIANTS screenInvariants();
#else
#define CHECK_SCREEN_INVARIANTS
#endif

#if HAVE_X11
#include <X11/Xlib.h>
#endif

#include <chrono>

using namespace std::chrono_literals;

ScreenPool::ScreenPool(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QList<QScreen *>>("QList<QScreen *>");
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, [this, screen]() {
            handleScreenGeometryChanged(screen);
        });
        handleScreenAdded(screen);
    });

    for (const auto screens = qGuiApp->screens(); auto screen : screens) {
        connect(screen, &QScreen::geometryChanged, this, [this, screen]() {
            handleScreenGeometryChanged(screen);
        });
    }

    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ScreenPool::handleScreenRemoved);

    // Note that the ScreenPool must process the QGuiApplication::screenAdded signal
    // before the primary output watcher.
    m_outputOrderWatcher = OutputOrderWatcher::instance(this);
    connect(m_outputOrderWatcher, &OutputOrderWatcher::outputOrderChanged, this, &ScreenPool::handleOutputOrderChanged);

    reconsiderOutputOrder();
}

ScreenPool::~ScreenPool()
{
}

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

bool ScreenPool::noRealOutputsConnected() const
{
    if (qApp->screens().count() > 1) {
        return false;
    } else if (m_availableScreens.isEmpty()) {
        return true;
    }

    return isOutputFake(m_availableScreens.first());
}

bool ScreenPool::isOutputFake(QScreen *screen) const
{
    Q_ASSERT(screen);
    // On X11 the output named :0.0 is fake (the geometry is usually valid and whatever the geometry
    // of the last connected screen was), on wayland the fake output has no name and no geometry
    bool screenHasDefaultName = false;
#if HAVE_X11
    if (auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        static QString defaultName; // QXcbScreen::defaultName
        if (defaultName.isEmpty()) {
            QByteArray displayName = DisplayString(interface->display());
            int dotPos = displayName.lastIndexOf('.');
            if (dotPos != -1) {
                displayName.truncate(dotPos);
            }
            defaultName = QString::fromLocal8Bit(displayName) + QLatin1String(".0");
        }
        screenHasDefaultName = screen->name() == defaultName;
    }
#endif
    const bool fake = screenHasDefaultName || screen->geometry().isEmpty() || screen->name().isEmpty();
    // If there is a fake output we can only have one screen left (the fake one)
    //    Q_ASSERT(!fake || fake == (qGuiApp->screens().count() == 1));
    return fake;
}

QScreen *ScreenPool::outputRedundantTo(QScreen *screen) const
{
    Q_ASSERT(screen);
    // Manage fake screens separately
    if (isOutputFake(screen)) {
        return nullptr;
    }
    const QRect thisGeometry = screen->geometry();

    // Don't use it as we don't want to consider redundants
    const int thisId = m_outputOrderWatcher->outputOrder().indexOf(screen->name());

    // FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    // so this ultra inefficient heuristic has to stay until we have a slightly better api
    // logic is:
    // a screen is redundant if:
    //* its geometry is contained in another one
    //* if their resolutions are different, the "biggest" one wins
    //* if they have the same geometry, the one with the lowest id wins (arbitrary, but gives reproducible behavior and makes the primary screen win)
    for (QScreen *s : m_sizeSortedScreens) {
        // don't compare with itself
        if (screen == s) {
            continue;
        }

        const QRect otherGeometry = s->geometry();

        if (otherGeometry.isNull()) {
            continue;
        }

        const int otherId = m_outputOrderWatcher->outputOrder().indexOf(s->name());

        if (otherGeometry.contains(thisGeometry, false)
            && ( // since at this point contains is true, if either
                 // measure of othergeometry is bigger, has a bigger area
                otherGeometry.width() > thisGeometry.width() || otherGeometry.height() > thisGeometry.height() ||
                // ids not -1 are considered in descending order of importance
                //-1 means that is a screen not known yet, just arrived and
                // not yet in screenpool: this happens for screens that
                // are hotplugged and weren't known. it does NOT happen
                // at first startup, as screenpool populates on load with all screens connected at the moment before the rest of the shell starts up
                (thisId == -1 && otherId != -1) || (thisId > otherId && otherId != -1))) {
            return s;
        }
    }

    return nullptr;
}

void ScreenPool::insertSortedScreen(QScreen *screen)
{
    if (m_sizeSortedScreens.contains(screen)) {
        // This should happen only when a fake screen isn't anymore
        return;
    }
    auto before = std::find_if(m_sizeSortedScreens.begin(), m_sizeSortedScreens.end(), [this, screen](QScreen *otherScreen) {
        return (screen->geometry().width() > otherScreen->geometry().width() && screen->geometry().height() > otherScreen->geometry().height())
            || idForName(screen->name()) < idForName(otherScreen->name());
    });
    m_sizeSortedScreens.insert(before, screen);
}

void ScreenPool::handleScreenGeometryChanged(QScreen *screen)
{
    m_sizeSortedScreens.removeAll(screen);
    insertSortedScreen(screen);
    reconsiderOutputOrder();
}

void ScreenPool::handleScreenAdded(QScreen *screen)
{
    qCDebug(SCREENPOOL) << "handleScreenAdded" << screen << screen->geometry();
    // handleScreenAdded can be called twice from handleOutputOrderChanged and QGuiApplication::screenAdded when a new screen is added,
    // so there is a chance the screen is already added to m_availableScreens.
    Q_ASSERT_X(m_availableScreens.contains(screen) || m_fakeScreens.contains(screen) || m_redundantScreens.contains(screen)
                   || !m_sizeSortedScreens.contains(screen),
               Q_FUNC_INFO,
               qUtf8Printable(std::invoke([this, screen]() {
                   QString message;
                   QDebug(&message) << this << "Current screen:" << screen;
                   return message;
               })));

    insertSortedScreen(screen);

    if (isOutputFake(screen)) {
        m_fakeScreens.insert(screen);
        return;
    } else if (auto it = m_fakeScreens.constFind(screen); it != m_fakeScreens.cend()) {
        qCDebug(SCREENPOOL) << "not fake anymore" << screen;
        m_fakeScreens.erase(it);
    }

    // TODO: remove?
    if (QScreen *toScreen = outputRedundantTo(screen)) {
        m_redundantScreens.insert(screen, toScreen);
        return;
    }
}

void ScreenPool::handleScreenRemoved(QScreen *screen)
{
    qCDebug(SCREENPOOL) << "handleScreenRemoved" << screen;

    m_sizeSortedScreens.removeAll(screen);
    if (m_redundantScreens.contains(screen)) {
        Q_ASSERT(!m_fakeScreens.contains(screen));
        Q_ASSERT(!m_availableScreens.contains(screen));
        m_redundantScreens.remove(screen);
    } else if (m_fakeScreens.contains(screen)) {
        // If is in fake screens, then must be fake, this will only happen on Wayland
        Q_ASSERT(isOutputFake(screen));
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_availableScreens.contains(screen));
        m_fakeScreens.remove(screen);
    } else if (isOutputFake(screen)) {
        // Fake but not in m_fakeScreens can only happen on X11, where the last output quietly renames itself to ":0.0" without signals
#if HAVE_X11
        Q_ASSERT(KWindowSystem::isPlatformX11());
#else
        qCCritical(SCREENPOOL, "Something wrong happened on Wayland.");
        Q_UNREACHABLE();
#endif
        Q_ASSERT_X(m_availableScreens.contains(screen), Q_FUNC_INFO, qUtf8Printable(std::invoke([this, screen]() {
                       QString message;
                       QDebug(&message) << this << "Current screen:" << screen;
                       return message;
                   })));
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_fakeScreens.contains(screen));
        m_availableScreens.removeAll(screen);
    } else if (m_availableScreens.contains(screen)) {
        // It's possible that the screen was already "removed" by handleOutputOrderChanged
        Q_ASSERT(m_availableScreens.contains(screen));
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_fakeScreens.contains(screen));
        m_orderChangedPendingSignal = true;
        Q_EMIT screenRemoved(screen);
        m_availableScreens.removeAll(screen);
    }

    // We can't call CHECK_SCREEN_INVARIANTS here, but on handleOutputOrderChanged which is about to arrive
}

void ScreenPool::handleOutputOrderChanged(const QStringList &newOrder)
{
    qCDebug(SCREENPOOL) << "handleOutputOrderChanged" << newOrder;
    QHash<QString, QScreen *> connMap;
    for (auto s : qApp->screens()) {
        connMap[s->name()] = s;
    }

    QList<QScreen *> newAvailableScreens;
    m_redundantScreens.clear();

    for (const auto &c : newOrder) {
        // When a screen is removed we reevaluate the order, but the order changed signal may not have happened yet,
        // so filter out outputs which now are invalid
        if (!connMap.contains(c)) {
            continue;
        }
        auto *s = connMap[c];
        if (!m_sizeSortedScreens.contains(s)) {
            // BUG 483432: This can happen when an external monitor is connected, but the internal monitor emits QScreen::geometryChanged
            // before qGuiApp emits QGuiApplication::screenAdded, so the screen is not added to the list yet.
            // Backtrace:
            // QWindowSystemInterface::handleScreenAdded -> 1. QHighDpiScaling::updateHighDpiScaling -> QScreen::geometryChanged(this=internal screen)
            //                                           -> 2. QGuiApplication::screenAdded (screen=new screen)
            handleScreenAdded(s);
        }
        if (isOutputFake(s)) {
            m_fakeScreens.insert(s);
            continue;
        }

        auto *toScreen = outputRedundantTo(s);
        // Never consider redundant an output redundant to something explicitly not included in the output order
        if (toScreen && newOrder.contains(toScreen->name())) {
            m_redundantScreens.insert(s, toScreen);
        } else {
            newAvailableScreens.append(s);
        }
    }

    // always emit removals before updating the sorted list
    for (auto screen : std::as_const(m_availableScreens)) {
        if (!newAvailableScreens.contains(screen)) {
            Q_EMIT screenRemoved(screen);
        }
    }

    if (m_orderChangedPendingSignal || newAvailableScreens != m_availableScreens) {
        qCDebug(SCREENPOOL) << "screenOrderChanged, old order:" << m_availableScreens << "new order:" << newAvailableScreens;
        m_availableScreens = newAvailableScreens;
        Q_EMIT screenOrderChanged(m_availableScreens);
    }

    m_orderChangedPendingSignal = false;

    CHECK_SCREEN_INVARIANTS
    // FIXME Async as the status can be incoherent before a fake screen goes away?
}

void ScreenPool::reconsiderOutputOrder()
{
    // prevent race condition between us and the watcher. depending on who receives signals first,
    // the current screens may not be the ones the watcher knows about -> force an update.
    m_outputOrderWatcher->refresh();
    handleOutputOrderChanged(m_outputOrderWatcher->outputOrder());
}

void ScreenPool::screenInvariants()
{
    if (m_outputOrderWatcher->outputOrder().isEmpty()) {
        return;
    }

    [[maybe_unused]] auto debugMessage = [this] {
        QString message;
        QDebug(&message) << this;
        return message;
    };

    // Is the primary connector in sync with the actual primaryScreen? The only way it can get out of sync with primaryConnector() is the single fake screen/no
    // real outputs scenario
    Q_ASSERT_X(noRealOutputsConnected() || !m_availableScreens.isEmpty(),
               Q_FUNC_INFO,
               qUtf8Printable(debugMessage())); // https://crash-reports.kde.org/organizations/kde/issues/6007/
    // Is the primary screen available? TODO: it can be redundant
    // Q_ASSERT(m_availableScreens.contains(primaryScreen()));

    // QScreen bookeeping integrity
    auto allScreens = qGuiApp->screens();
    // Do we actually track every screen?
    Q_ASSERT_X((m_availableScreens.count() + m_redundantScreens.count()) == m_outputOrderWatcher->outputOrder().count(),
               Q_FUNC_INFO,
               qUtf8Printable(debugMessage())); // https://crash-reports.kde.org/organizations/kde/issues/5249/
    Q_ASSERT_X(allScreens.count() == m_sizeSortedScreens.count(),
               Q_FUNC_INFO,
               qUtf8Printable(debugMessage())); // https://crash-reports.kde.org/organizations/kde/issues/6337/

    // At most one fake output
    Q_ASSERT_X(m_fakeScreens.count() <= 1, Q_FUNC_INFO, qUtf8Printable(debugMessage()));
    for (QScreen *screen : allScreens) {
        // ignore screens filtered out by the output order
        if (!m_outputOrderWatcher->outputOrder().contains(screen->name())) {
            continue;
        }

        if (m_availableScreens.contains(screen)) {
            // If available can't be redundant
            Q_ASSERT(!m_redundantScreens.contains(screen));
        } else if (m_redundantScreens.contains(screen)) {
            // If redundant can't be available
            Q_ASSERT(!m_availableScreens.contains(screen));
        } else if (m_fakeScreens.contains(screen)) {
            // A fake screen can't be anywhere else
            // This branch is quite rare, happens only during the transition from a fake screen to a real one, for a brief moment both screens can be there
            Q_ASSERT(!m_availableScreens.contains(screen));
            Q_ASSERT(!m_redundantScreens.contains(screen));
        } else {
            // We can't have a screen unaccounted for
            Q_ASSERT(false);
        }
    }

    for (QScreen *screen : m_redundantScreens.keys()) {
        Q_ASSERT(outputRedundantTo(screen) != nullptr);
    }
}

QDebug operator<<(QDebug debug, const ScreenPool *pool)
{
    debug << pool->metaObject()->className() << '(' << static_cast<const void *>(pool) << ") Internal state:\n";
    debug << "Screen Order:\t" << pool->m_availableScreens << '\n';
    debug << "\"Fake\" screens:\t" << pool->m_fakeScreens << '\n';
    debug << "Redundant screens covered by other ones:\t" << pool->m_redundantScreens << '\n';
    debug << "All screens, ordered by size:\t" << pool->m_sizeSortedScreens << '\n';
    debug << "All screen that QGuiApplication knows:\t" << qGuiApp->screens() << '\n';
    return debug;
}

#include "moc_screenpool.cpp"
