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

#include <chrono>

using namespace std::chrono_literals;

bool operator==(const ScreenIdentifier &i1, const ScreenIdentifier &i2)
{
    return i1.edid() == i2.edid() && i1.connector() == i2.connector();
}

uint qHash(const ScreenIdentifier &key, uint seed)
{
    return qHash(key.edid() + key.connector(), seed);
}

ScreenIdentifier::ScreenIdentifier(const QString &edid, const QString &connector)
    : m_edid(edid)
    , m_connector(connector)
{
}

ScreenIdentifier::~ScreenIdentifier()
{
}

ScreenIdentifier ScreenIdentifier::fromScreen(QScreen *screen)
{
    return ScreenIdentifier(edidFromScreen(screen), screen->name());
}

ScreenIdentifier ScreenIdentifier::fromString(const QString &string)
{
    auto arr = string.split(QChar(':'));
    if (arr.size() < 2) {
        // if we don't have edid:connector format, assume we only have the connector
        return ScreenIdentifier(QString(), string);
    } else {
        const auto connector = arr.last();
        arr.pop_back();
        return ScreenIdentifier(arr.join(QChar(':')), connector);
    }
}

QString ScreenIdentifier::toString() const
{
    return m_edid + QChar(':') + m_connector;
}

QString ScreenIdentifier::edidFromScreen(QScreen *screen)
{
    return screen->manufacturer() + QChar('/') + screen->model() + QChar('/') + screen->serialNumber();
}

bool ScreenIdentifier::isValid() const
{
    // Allow empty edids in case of migration of old config and particularly misbehaving monitors
    return !m_connector.isEmpty();
}

QString ScreenIdentifier::edid() const
{
    return m_edid;
}

QString ScreenIdentifier::connector() const
{
    return m_connector;
}

ScreenIdentifier::Match ScreenIdentifier::match(const ScreenIdentifier &other) const
{
    Match m = Match::None;
    if (!m_edid.isEmpty() && m_edid == other.edid()) {
        m = static_cast<Match>(int(m) | int(Match::Edid));
    }
    if (m_connector == other.connector()) {
        m = static_cast<Match>(int(m) | int(Match::Connector));
    }
    return m;
}

ScreenIdentifier::Match ScreenIdentifier::match(QScreen *screen) const
{
    return match(fromScreen(screen));
}

bool ScreenIdentifier::match(QScreen *screen, Match matchMode) const
{
    return match(screen) == matchMode;
}

QDebug operator<<(QDebug debug, const ScreenIdentifier &identifier)
{
    debug << "Edid:" << identifier.edid() << "Connector:" << identifier.connector();
    return debug;
}

ScreenPool::ScreenPool(const KSharedConfig::Ptr &config, QObject *parent)
    : QObject(parent)
    , m_configGroup(KConfigGroup(config, QStringLiteral("ScreenConnectors")))
{
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ScreenPool::handleScreenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ScreenPool::handleScreenRemoved);

    // Note that the ScreenPool must process the QGuiApplication::screenAdded signal
    // before the primary output watcher.
    m_primaryWatcher = new PrimaryOutputWatcher(this);
    connect(m_primaryWatcher, &PrimaryOutputWatcher::primaryOutputNameChanged, this, &ScreenPool::handlePrimaryOutputNameChanged);

    m_reconsiderOutputsTimer.setSingleShot(true);
    m_reconsiderOutputsTimer.setInterval(250ms);
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
    m_identifierForNumber.clear();
    m_numberForIdentifier.clear();

    if (primary) {
        m_primaryConnector = primary->name();
        if (!m_primaryConnector.isEmpty()) {
            auto identifier = ScreenIdentifier::fromScreen(primary);
            m_identifierForNumber[0] = identifier;
            m_numberForIdentifier[identifier] = 0;
        }
    }
    /*
        // Migrating the old configuration TODO Plasma6: remove?
        {
            const auto oldGroup = KConfigGroup(m_configGroup.config(), QStringLiteral("ScreenConnectors"));
            const auto keys = oldGroup.keyList();
            for (const QString &key : keys) {
                const int currentNumber = key.toInt();
                // We already done primary
                if (primary && currentNumber == 0) { //TODO do the actual swapping
                    continue;
                }
                const QString connector = oldGroup.readEntry(key, QString());
                if (connector.isEmpty()) {
                    continue;
                }
                const auto identifier = ScreenIdentifier(QString(), connector);
                m_identifierForNumber[currentNumber] = identifier;
                m_numberForIdentifier[identifier] = currentNumber;
            }
            // oldGroup.deleteGroup(); TODO: enable
        }
    */
    // restore the known ids to identifier mappings
    const auto keys = m_configGroup.keyList();
    for (const QString &key : keys) {
        auto identifier = ScreenIdentifier::fromString(m_configGroup.readEntry(key, QString()));
        const int currentNumber = key.toInt();

        // We already done primary
        if (primary && currentNumber == 0) { // TODO do the actual swapping
            continue;
        }

        // There always must be at most one unique connector in the map
        unmapConnector(identifier.connector());

        /* // Was in the old migrated config with no edid, override with the new one
         auto searchRes = m_identifierForNumber.find(currentNumber);
         if (searchRes != m_identifierForNumber.end() ) {
             qWarning()<<"Found to remove:"<<(*searchRes)<<currentNumber;
             m_numberForIdentifier.remove(*searchRes);
             m_identifierForNumber.erase(searchRes);
         }*/

        qWarning() << "Adding identifier?" << identifier << currentNumber << !m_numberForIdentifier.contains(identifier);
        if (!key.isEmpty() && identifier.isValid() && !m_numberForIdentifier.contains(identifier)) {
            m_identifierForNumber[currentNumber] = identifier;
            m_numberForIdentifier[identifier] = currentNumber;
        } else if (m_numberForIdentifier.value(identifier) != currentNumber) {
            m_configGroup.deleteEntry(key);
        }
    }

    // Populate all the screens based on what's connected at startup
    for (QScreen *screen : qGuiApp->screens()) {
        // On some devices QGuiApp::screenAdded is always emitted for some screens at startup so at this point that screen would already be managed
        qWarning() << "POH?" << screen << m_allSortedScreens;
        if (!m_allSortedScreens.contains(screen)) {
            handleScreenAdded(screen);
        } else {
            int number = numberForConnector(screen->name());
            qWarning() << screen->name() << number;
            if (number < 0) {
                number = firstAvailableId();
            }
            ensureScreenMapping(number, screen);
        }
    }

    qWarning() << "m_identifierForNumber:" << m_identifierForNumber;
    qWarning() << "m_numberForIdentifier:" << m_numberForIdentifier;

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

    int oldIdForPrimary = -1;
    auto searchRes = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), [primary](auto keyVal) {
        return keyVal.first.connector() == primary;
    });
    if (searchRes != m_numberForIdentifier.constKeyValueEnd()) {
        oldIdForPrimary = (*searchRes).second;
    } else {
        // move old primary to new free id
        oldIdForPrimary = firstAvailableId();
    }

    m_numberForIdentifier[(*searchRes).first] = 0;
    m_identifierForNumber[0] = (*searchRes).first;

    searchRes = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), [this](auto keyVal) {
        return keyVal.first.connector() == m_primaryConnector;
    });
    if (searchRes != m_numberForIdentifier.constKeyValueEnd()) {
        m_numberForIdentifier[(*searchRes).first] = oldIdForPrimary;
        m_identifierForNumber[oldIdForPrimary] = (*searchRes).first;
    }

    m_primaryConnector = primary;
    save();
}

void ScreenPool::save()
{
    qWarning() << "SAVING";
    for (auto i = m_identifierForNumber.constBegin(); i != m_identifierForNumber.constEnd(); ++i) {
        m_configGroup.writeEntry(QString::number(i.key()), i.value().toString());
    }
    // write to disk every 30 seconds at most
    m_configSaveTimer.start(30000);
}

void ScreenPool::ensureScreenMapping(int number, QScreen *screen)
{
    const auto connector = screen->name();
    const auto identifier = ScreenIdentifier::fromScreen(screen);

    // if there was already an identifier for the number remap: it changed either the edid or the connector
    auto it = m_identifierForNumber.find(number);
    if (it != m_identifierForNumber.end()) {
        m_numberForIdentifier.remove(*it);
        m_identifierForNumber.erase(it);
    }

    qWarning() << "ensureScreenMapping1 " << number << identifier << m_identifierForNumber.contains(number) << m_identifierForNumber.value(number);
    Q_ASSERT(!m_identifierForNumber.contains(number) || m_identifierForNumber.value(number) == identifier);
    qWarning() << "ensureScreenMapping2 " << number << identifier << "---" << m_numberForIdentifier.contains(identifier)
               << m_numberForIdentifier.value(identifier) << (!m_numberForIdentifier.contains(identifier) || m_numberForIdentifier.value(identifier) == number);
    Q_ASSERT(!m_numberForIdentifier.contains(identifier) || m_numberForIdentifier.value(identifier) == number);

    if (number == 0) {
        m_primaryConnector = connector;
    }

    m_identifierForNumber[number] = identifier;
    m_numberForIdentifier[identifier] = number;

    save();
}

void ScreenPool::unmapConnector(const QString &connector)
{
    if (m_numberForIdentifier.isEmpty()) {
        return;
    }

    auto it = m_numberForIdentifier.begin();
    while (it != m_numberForIdentifier.end()) {
        if (it.key().connector() == connector) {
            m_identifierForNumber.remove(it.value());
            m_numberForIdentifier.erase(it++);
        } else {
            it++;
        }
    }
}

int ScreenPool::id(const QString &connector) const
{
    return numberForConnector(connector);
}

int ScreenPool::numberForConnector(const QString &connector) const
{
    return number(ScreenIdentifier(QString(), connector));
}

QString ScreenPool::connector(int id) const
{
    return identifier(id).connector();
}

ScreenIdentifier ScreenPool::identifier(int number) const
{
    Q_ASSERT(m_identifierForNumber.contains(number));

    return m_identifierForNumber.value(number);
}

int ScreenPool::number(const ScreenIdentifier &identifier) const
{
    const auto callback = [identifier](auto keyVal) {
        return keyVal.first.match(identifier) != ScreenIdentifier::Match::None;
    };
    auto it = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), callback);
    if (it != m_numberForIdentifier.constKeyValueEnd()) {
        return (*it).second;
    }

    return -1;
}

int ScreenPool::firstAvailableId() const
{
    int i = 0;
    // find the first integer not stored in m_identifierForNumber
    // m_identifierForNumber is the only map, so the ids are sorted
    foreach (int existingId, m_identifierForNumber.keys()) {
        if (i != existingId) {
            return i;
        }
        ++i;
    }

    return i;
}

QList<int> ScreenPool::knownIds() const
{
    return m_identifierForNumber.keys();
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
    if (!m_identifierForNumber.contains(id)) {
        return nullptr;
    }

    const auto identifier = m_identifierForNumber.value(id);
    // First try to match everything
    for (QScreen *screen : m_availableScreens) {
        if (identifier.match(screen, ScreenIdentifier::Match::All)) {
            return screen;
        }
    }
    // Then try to match the edid
    for (QScreen *screen : m_availableScreens) {
        if (identifier.match(screen, ScreenIdentifier::Match::Edid)) {
            return screen;
        }
    }
    // Finally, try to match the connector
    for (QScreen *screen : m_availableScreens) {
        if (identifier.match(screen, ScreenIdentifier::Match::Connector)) {
            return screen;
        }
    }
    return nullptr;
}

int ScreenPool::idForScreen(QScreen *screen) const
{
    if (screen == primaryScreen()) {
        return 0;
    }

    auto it = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), [screen](auto keyVal) {
        return keyVal.first.match(screen, ScreenIdentifier::Match::Edid);
    });
    if (it != m_numberForIdentifier.constKeyValueEnd()) {
        return (*it).second;
    }
    // Then try to match the Edid
    it = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), [screen](auto keyVal) {
        return keyVal.first.match(screen, ScreenIdentifier::Match::Edid);
    });
    if (it != m_numberForIdentifier.constKeyValueEnd()) {
        return (*it).second;
    }
    // Finally, try to match the connector
    it = std::find_if(m_numberForIdentifier.constKeyValueBegin(), m_numberForIdentifier.constKeyValueEnd(), [screen](auto keyVal) {
        return keyVal.first.match(screen, ScreenIdentifier::Match::Connector);
    });
    if (it != m_numberForIdentifier.constKeyValueEnd()) {
        return (*it).second;
    }

    return -1;
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
    // Manage fake screens separately
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
                // Insert again, redundantTo may have changed
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
                    if (numberForConnector(screen->name()) < 0) {
                        ensureScreenMapping(firstAvailableId(), screen);
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
            if (numberForConnector(screen->name()) < 0) {
                ensureScreenMapping(firstAvailableId(), screen);
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
            if (numberForConnector(screen->name()) < 0) {
                ensureScreenMapping(firstAvailableId(), screen);
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
    } else {
        int number = numberForConnector(screen->name());
        if (number < 0) {
            number = firstAvailableId();
        }
        ensureScreenMapping(number, screen);
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
        // This happens when an output is recycled because it was the last one and became fake
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
    // First check if the data arrived is correct, then set the new primary considering redundant ones
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
        // In the latter case m_availableScreens will already contain newPrimary
        // We'll go here also once at startup, for which we don't need to do anything besides setting internally the primary connector name
        handleScreenAdded(newPrimary);
        return;
    } else {
        Q_ASSERT(newPrimary);
        qCDebug(SCREENPOOL) << "PRIMARY CHANGED" << oldPrimary << "-->" << newPrimary;
        setPrimaryConnector(newOutputName);
        Q_EMIT primaryScreenChanged(oldPrimary, newPrimary);
    }
}

QString ScreenPool::identifierForScreen(QScreen *screen)
{
    return screen->manufacturer() + QChar('/') + screen->model() + QChar('/') + screen->serialNumber() + QChar(':') + screen->name();
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
            Q_ASSERT(numberForConnector(screen->name()) >= 0);
            // Are the two maps symmetrical?
            qWarning() << "AAAA name:" << screen->name() << "id:" << id(screen->name()) << "connector:" << connector(id(screen->name())) << "identifier"
                       << identifier(id(screen->name())) << ScreenIdentifier::fromScreen(screen);
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
    auto it = pool->m_numberForIdentifier.constBegin();
    while (it != pool->m_numberForIdentifier.constEnd()) {
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
