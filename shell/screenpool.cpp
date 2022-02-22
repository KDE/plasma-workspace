/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "screenpool.h"
#include "primaryoutputwatcher.h"
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

ScreenPool::ScreenPool(const KSharedConfig::Ptr &config, QObject *parent)
    : QObject(parent)
    , m_configGroup(KConfigGroup(config, QStringLiteral("ScreenConnectors")))
    , m_primaryWatcher(new PrimaryOutputWatcher(this))
{
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ScreenPool::handleScreenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ScreenPool::handleScreenRemoved);
    connect(m_primaryWatcher, &PrimaryOutputWatcher::primaryOutputNameChanged, this, &ScreenPool::handlePrimaryOutputNameChanged);

    m_reconsiderOutputsTimer.setSingleShot(true);
    m_reconsiderOutputsTimer.setInterval(250);
    connect(&m_reconsiderOutputsTimer, &QTimer::timeout, this, &ScreenPool::reconsiderOutputs);

    m_configSaveTimer.setSingleShot(true);
    connect(&m_configSaveTimer, &QTimer::timeout, this, [this]() {
        m_configGroup.sync();
    });
}

void ScreenPool::load()
{
    QScreen *primary = m_primaryWatcher->primaryScreen();
    m_primaryConnector = QString();
    m_connectorForId.clear();
    m_idForConnector.clear();

    if (primary) {
        m_primaryConnector = primary->name();
        if (!m_primaryConnector.isEmpty()) {
            m_connectorForId[0] = m_primaryConnector;
            m_idForConnector[m_primaryConnector] = 0;
        }
    }

    // restore the known ids to connector mappings
    const auto keys = m_configGroup.keyList();
    for (const QString &key : keys) {
        QString connector = m_configGroup.readEntry(key, QString());
        const int currentId = key.toInt();

        if (!key.isEmpty() && !connector.isEmpty() && !m_connectorForId.contains(currentId) && !m_idForConnector.contains(connector)) {
            m_connectorForId[currentId] = connector;
            m_idForConnector[connector] = currentId;
        } else if (m_idForConnector.value(connector) != currentId) {
            m_configGroup.deleteEntry(key);
        }
    }

    // Populate allthe screen based on what's connected at startup
    for (QScreen *screen : qGuiApp->screens()) {
        // On some devices QGuiApp::screenAdded is always emitted for some screens at startup so at this point that screen would already be managed
        if (!m_allSortedScreens.contains(screen)) {
            handleScreenAdded(screen);
        } else if (!m_idForConnector.contains(screen->name())) {
            insertScreenMapping(firstAvailableId(), screen->name());
        }
    }
    CHECK_SCREEN_INVARIANTS
}

ScreenPool::~ScreenPool()
{
    m_configGroup.sync();
}

QString ScreenPool::primaryConnector() const
{
    return m_primaryConnector;
}

void ScreenPool::setPrimaryConnector(const QString &primary)
{
    if (m_primaryConnector == primary) {
        return;
    }

    int oldIdForPrimary = m_idForConnector.value(primary, -1);
    if (oldIdForPrimary == -1) {
        // move old primary to new free id
        oldIdForPrimary = firstAvailableId();
    }

    m_idForConnector[primary] = 0;
    m_connectorForId[0] = primary;
    m_idForConnector[m_primaryConnector] = oldIdForPrimary;
    m_connectorForId[oldIdForPrimary] = m_primaryConnector;
    m_primaryConnector = primary;
    save();
}

void ScreenPool::save()
{
    QMap<int, QString>::const_iterator i;
    for (i = m_connectorForId.constBegin(); i != m_connectorForId.constEnd(); ++i) {
        m_configGroup.writeEntry(QString::number(i.key()), i.value());
    }
    // write to disck every 30 seconds at most
    m_configSaveTimer.start(30000);
}

void ScreenPool::insertScreenMapping(int id, const QString &connector)
{
    Q_ASSERT(!m_connectorForId.contains(id) || m_connectorForId.value(id) == connector);
    Q_ASSERT(!m_idForConnector.contains(connector) || m_idForConnector.value(connector) == id);

    if (id == 0) {
        m_primaryConnector = connector;
    }

    m_connectorForId[id] = connector;
    m_idForConnector[connector] = id;
    save();
}

int ScreenPool::id(const QString &connector) const
{
    return m_idForConnector.value(connector, -1);
}

QString ScreenPool::connector(int id) const
{
    Q_ASSERT(m_connectorForId.contains(id));

    return m_connectorForId.value(id);
}

int ScreenPool::firstAvailableId() const
{
    int i = 0;
    // find the first integer not stored in m_connectorForId
    // m_connectorForId is the only map, so the ids are sorted
    foreach (int existingId, m_connectorForId.keys()) {
        if (i != existingId) {
            return i;
        }
        ++i;
    }

    return i;
}

QList<int> ScreenPool::knownIds() const
{
    return m_connectorForId.keys();
}

QList<QScreen *> ScreenPool::screens() const
{
    return m_availableScreens;
}

QScreen *ScreenPool::primaryScreen() const
{
    QScreen *primary = m_primaryWatcher->primaryScreen();
    if (m_redundantScreens.contains(primary)) {
        return m_redundantScreens[primary];
    } else {
        return primary;
    }
}

QScreen *ScreenPool::screenForId(int id) const
{
    if (!m_connectorForId.contains(id)) {
        return nullptr;
    }

    // TODO: do QScreen bookeeping completely in screenpool, cache also available QScreens
    const QString name = m_connectorForId.value(id);
    for (QScreen *screen : m_availableScreens) {
        if (screen->name() == name) {
            return screen;
        }
    }
    return nullptr;
}

QScreen *ScreenPool::screenForConnector(const QString &connector)
{
    for (QScreen *screen : m_availableScreens) {
        if (screen->name() == connector) {
            return screen;
        }
    }
    return nullptr;
}

bool ScreenPool::noRealOutputsConnected() const
{
    if (qApp->screens().count() > 1) {
        return false;
    }

    return isOutputFake(m_primaryWatcher->primaryScreen());
}

bool ScreenPool::isOutputFake(QScreen *screen) const
{
    Q_ASSERT(screen);
    // On X11 the output named :0.0 is fake (the geometry is usually valid and whatever the geometry
    // of the last connected screen was), on wayland the fake output has no name and no geometry
    const bool fake = screen->name() == QStringLiteral(":0.0") || screen->geometry().isEmpty() || screen->name().isEmpty();
    // If there is a fake output we can only have one screen left (the fake one)
    //    Q_ASSERT(!fake || fake == (qGuiApp->screens().count() == 1));
    return fake;
}

QScreen *ScreenPool::outputRedundantTo(QScreen *screen) const
{
    Q_ASSERT(screen);
    // Manage separatedly fake screens
    if (isOutputFake(screen)) {
        return nullptr;
    }
    const QRect thisGeometry = screen->geometry();

    const int thisId = id(screen->name());

    // FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    // so this ultra inefficient heuristic has to stay until we have a slightly better api
    // logic is:
    // a screen is redundant if:
    //* its geometry is contained in another one
    //* if their resolutions are different, the "biggest" one wins
    //* if they have the same geometry, the one with the lowest id wins (arbitrary, but gives reproducible behavior and makes the primary screen win)
    for (QScreen *s : m_allSortedScreens) {
        // don't compare with itself
        if (screen == s) {
            continue;
        }

        const QRect otherGeometry = s->geometry();

        if (otherGeometry.isNull()) {
            continue;
        }

        const int otherId = id(s->name());

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

void ScreenPool::reconsiderOutputs()
{
    QScreen *oldPrimaryScreen = primaryScreen();
    for (QScreen *screen : m_allSortedScreens) {
        if (m_redundantScreens.contains(screen)) {
            if (QScreen *toScreen = outputRedundantTo(screen)) {
                // Insert again, redndantTo may have changed
                m_fakeScreens.remove(screen);
                m_redundantScreens.insert(screen, toScreen);
            } else {
                qCDebug(SCREENPOOL) << "not redundant anymore" << screen << (isOutputFake(screen) ? "but is a fake screen" : "");
                Q_ASSERT(!m_availableScreens.contains(screen));
                m_redundantScreens.remove(screen);
                if (isOutputFake(screen)) {
                    m_fakeScreens.insert(screen);
                } else {
                    m_fakeScreens.remove(screen);
                    m_availableScreens.append(screen);
                    if (!m_idForConnector.contains(screen->name())) {
                        insertScreenMapping(firstAvailableId(), screen->name());
                    }
                    Q_EMIT screenAdded(screen);
                    QScreen *newPrimaryScreen = primaryScreen();
                    if (newPrimaryScreen != oldPrimaryScreen) {
                        // Primary screen was redundant, not anymore
                        setPrimaryConnector(newPrimaryScreen->name());
                        Q_EMIT primaryScreenChanged(oldPrimaryScreen, newPrimaryScreen);
                    }
                }
            }
        } else if (QScreen *toScreen = outputRedundantTo(screen)) {
            qCDebug(SCREENPOOL) << "new redundant screen" << screen << "with primary screen" << m_primaryWatcher->primaryScreen();

            m_fakeScreens.remove(screen);
            m_redundantScreens.insert(screen, toScreen);
            if (!m_idForConnector.contains(screen->name())) {
                insertScreenMapping(firstAvailableId(), screen->name());
            }
            if (m_availableScreens.contains(screen)) {
                QScreen *newPrimaryScreen = primaryScreen();
                if (newPrimaryScreen != oldPrimaryScreen) {
                    // Primary screen became redundant
                    setPrimaryConnector(newPrimaryScreen->name());
                    Q_EMIT primaryScreenChanged(oldPrimaryScreen, newPrimaryScreen);
                }
                m_availableScreens.removeAll(screen);
                Q_EMIT screenRemoved(screen);
            }
        } else if (isOutputFake(screen)) {
            // NOTE: order of operations is important
            qCDebug(SCREENPOOL) << "new fake screen" << screen;
            m_redundantScreens.remove(screen);
            m_fakeScreens.insert(screen);
            if (m_availableScreens.contains(screen)) {
                QScreen *newPrimaryScreen = primaryScreen();
                if (newPrimaryScreen != oldPrimaryScreen) {
                    // Primary screen became fake
                    setPrimaryConnector(newPrimaryScreen->name());
                    Q_EMIT primaryScreenChanged(oldPrimaryScreen, newPrimaryScreen);
                }
                m_availableScreens.removeAll(screen);
                Q_EMIT screenRemoved(screen);
            }
        } else if (m_fakeScreens.contains(screen)) {
            Q_ASSERT(!m_availableScreens.contains(screen));
            m_fakeScreens.remove(screen);
            m_availableScreens.append(screen);
            if (!m_idForConnector.contains(screen->name())) {
                insertScreenMapping(firstAvailableId(), screen->name());
            }
            Q_EMIT screenAdded(screen);
            QScreen *newPrimaryScreen = primaryScreen();
            if (newPrimaryScreen != oldPrimaryScreen) {
                // Primary screen was redundant, not anymore
                setPrimaryConnector(newPrimaryScreen->name());
                Q_EMIT primaryScreenChanged(oldPrimaryScreen, newPrimaryScreen);
            }
        } else {
            qCDebug(SCREENPOOL) << "fine screen" << screen;
        }
    }

    // updateStruts();

    CHECK_SCREEN_INVARIANTS
}

void ScreenPool::insertSortedScreen(QScreen *screen)
{
    if (m_allSortedScreens.contains(screen)) {
        // This should happen only when a fake screen isn't anymore
        return;
    }
    auto before = std::find_if(m_allSortedScreens.begin(), m_allSortedScreens.end(), [this, screen](QScreen *otherScreen) {
        return (screen->geometry().width() > otherScreen->geometry().width() && screen->geometry().height() > otherScreen->geometry().height())
            || id(screen->name()) < id(otherScreen->name());
    });
    m_allSortedScreens.insert(before, screen);
}

void ScreenPool::handleScreenAdded(QScreen *screen)
{
    qCDebug(SCREENPOOL) << "handleScreenAdded" << screen << screen->geometry();
    connect(
        screen,
        &QScreen::geometryChanged,
        this,
        [this, screen]() {
            m_allSortedScreens.removeAll(screen);
            insertSortedScreen(screen);
            m_reconsiderOutputsTimer.start();
        },
        Qt::UniqueConnection);
    insertSortedScreen(screen);

    if (isOutputFake(screen)) {
        m_fakeScreens.insert(screen);
        return;
    } else if (!m_idForConnector.contains(screen->name())) {
        insertScreenMapping(firstAvailableId(), screen->name());
    }

    if (QScreen *toScreen = outputRedundantTo(screen)) {
        m_redundantScreens.insert(screen, toScreen);
        return;
    }

    if (m_fakeScreens.contains(screen)) {
        qCDebug(SCREENPOOL) << "not fake anymore" << screen;
        m_fakeScreens.remove(screen);
    }

    m_reconsiderOutputsTimer.start();
    Q_ASSERT(!m_availableScreens.contains(screen));
    m_availableScreens.append(screen);
    Q_EMIT screenAdded(screen);
}

void ScreenPool::handleScreenRemoved(QScreen *screen)
{
    qCDebug(SCREENPOOL) << "handleScreenRemoved" << screen;
    m_allSortedScreens.removeAll(screen);
    if (m_redundantScreens.contains(screen)) {
        Q_ASSERT(!m_fakeScreens.contains(screen));
        Q_ASSERT(!m_availableScreens.contains(screen));
        m_redundantScreens.remove(screen);
    } else if (m_fakeScreens.contains(screen)) {
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_availableScreens.contains(screen));
        m_fakeScreens.remove(screen);
    } else if (isOutputFake(screen)) {
        // This happens when an output is recicled because it was the last one and became fake
        Q_ASSERT(m_availableScreens.contains(screen));
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_fakeScreens.contains(screen));
        Q_ASSERT(m_allSortedScreens.isEmpty());
        m_allSortedScreens.append(screen);
        m_availableScreens.removeAll(screen);
        m_fakeScreens.insert(screen);
    } else {
        Q_ASSERT(m_availableScreens.contains(screen));
        Q_ASSERT(!m_redundantScreens.contains(screen));
        Q_ASSERT(!m_fakeScreens.contains(screen));
        m_availableScreens.removeAll(screen);
        reconsiderOutputs();
        Q_EMIT screenRemoved(screen);
    }
    CHECK_SCREEN_INVARIANTS
}

void ScreenPool::handlePrimaryOutputNameChanged(const QString &oldOutputName, const QString &newOutputName)
{
    // when the appearance of a new primary screen *moves*
    // the position of the now secondary, the two screens will appear overlapped for an instant, and a spurious output redundant would happen here if checked
    // immediately
    m_reconsiderOutputsTimer.start();

    QScreen *oldPrimary = screenForConnector(oldOutputName);
    QScreen *newPrimary = m_primaryWatcher->primaryScreen();
    // First check if the data arrived is correct, then set the new peimary considering redundants
    Q_ASSERT(newPrimary && newPrimary->name() == newOutputName);
    newPrimary = primaryScreen();

    // This happens when a screen that was primary because the real primary was redundant becomes the real primary
    if (m_primaryConnector == newPrimary->name()) {
        return;
    }

    if (!newPrimary || newPrimary == oldPrimary || newPrimary->geometry().isNull()) {
        return;
    }

    // On X11 we get fake screens as primary

    // Special case: we are in "no connectors" mode, there is only a (recycled) QScreen instance which is not attached to any output. Treat this as a screen
    // removed This happens only on X, wayland doesn't seem to be getting fake screens
    if (noRealOutputsConnected()) {
        qCDebug(SCREENPOOL) << "EMITTING SCREEN REMOVED" << newPrimary;
        handleScreenRemoved(newPrimary);
        return;
        // On X11, the output named :0.0 is fake
    } else if (oldOutputName == ":0.0" || oldOutputName.isEmpty()) {
        setPrimaryConnector(newOutputName);
        // NOTE: when we go from 0 to 1 screen connected, screens can be renamed in those two followinf cases
        // * last output connected/disconnected -> we go between the fake screen and the single output, renamed
        // * external screen connected to a closed lid laptop, disconnecting the qscreen instance will be recycled from external output to internal
        // In the latter case m_availableScreens will aready contain newPrimary
        // We'll go here also once at startup, for which we don't need to do anything besides setting internally the primary conector name
        handleScreenAdded(newPrimary);
        return;
    } else {
        Q_ASSERT(newPrimary);
        qCDebug(SCREENPOOL) << "PRIMARY CHANGED" << oldPrimary << "-->" << newPrimary;
        setPrimaryConnector(newOutputName);
        Q_EMIT primaryScreenChanged(oldPrimary, newPrimary);
    }
}

void ScreenPool::screenInvariants()
{
    // Is the primary connector in sync with the actual primaryScreen? The only way it can get out of sync with primaryConnector() is the single fake screen/no
    // real outputs scenario
    Q_ASSERT(noRealOutputsConnected() || primaryScreen()->name() == primaryConnector());
    // Is the primary screen available? TODO: it can be redundant
    // Q_ASSERT(m_availableScreens.contains(primaryScreen()));

    // QScreen bookeeping integrity
    auto allScreens = qGuiApp->screens();
    // Do we actually track every screen?

    Q_ASSERT((m_availableScreens.count() + m_redundantScreens.count() + m_fakeScreens.count()) == allScreens.count());
    Q_ASSERT(allScreens.count() == m_allSortedScreens.count());

    // At most one fake output
    Q_ASSERT(m_fakeScreens.count() <= 1);
    if (m_fakeScreens.count() == 1) {
        // If we have a fake output we can't have anything else
        Q_ASSERT(m_availableScreens.count() == 0);
        Q_ASSERT(m_redundantScreens.count() == 0);
    } else {
        for (QScreen *screen : allScreens) {
            if (m_availableScreens.contains(screen)) {
                // If available can't be redundant
                Q_ASSERT(!m_redundantScreens.contains(screen));
            } else if (m_redundantScreens.contains(screen)) {
                // If redundant can't be available
                Q_ASSERT(!m_availableScreens.contains(screen));
            } else {
                // We can't have a screen unaccounted for
                Q_ASSERT(false);
            }
            // Is every screen mapped to an id?
            Q_ASSERT(m_idForConnector.contains(screen->name()));
            // Are the two maps symmetrical?
            Q_ASSERT(connector(id(screen->name())) == screen->name());
        }
    }
    for (QScreen *screen : m_redundantScreens.keys()) {
        Q_ASSERT(outputRedundantTo(screen) != nullptr);
    }
}

QDebug operator<<(QDebug debug, const ScreenPool *pool)
{
    debug << pool->metaObject()->className() << '(' << static_cast<const void *>(pool) << ") Internal state:\n";
    debug << "Connector Mapping:\n";
    auto it = pool->m_idForConnector.constBegin();
    while (it != pool->m_idForConnector.constEnd()) {
        debug << it.key() << "\t-->\t" << it.value() << '\n';
        it++;
    }
    debug << "Platform primary screen:\t" << pool->m_primaryWatcher->primaryScreen() << '\n';
    debug << "Actual primary screen:\t" << pool->primaryScreen() << '\n';
    debug << "Available screens:\t" << pool->m_availableScreens << '\n';
    debug << "\"Fake\" screens:\t" << pool->m_fakeScreens << '\n';
    debug << "Redundant screens covered by other ones:\t" << pool->m_redundantScreens << '\n';
    debug << "All screens, ordered by size:\t" << pool->m_allSortedScreens << '\n';
    debug << "All screen that QGuiApplication knows:\t" << qGuiApp->screens() << '\n';
    return debug;
}

#include "moc_screenpool.cpp"
