/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "mpris2sourcemodel.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QStringList>

#include "libkmpris_debug.h"
#include "playercontainer.h"

std::shared_ptr<Mpris2SourceModel> Mpris2SourceModel::self()
{
    static std::weak_ptr<Mpris2SourceModel> s_model;
    if (s_model.expired()) {
        std::shared_ptr<Mpris2SourceModel> ptr{new Mpris2SourceModel};
        s_model = ptr;
        return ptr;
    }

    return s_model.lock();
}

Mpris2SourceModel::Mpris2SourceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    auto watcher =
        new QDBusServiceWatcher(QStringLiteral("org.mpris.MediaPlayer2*"), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &Mpris2SourceModel::onServiceOwnerChanged);

    fetchServiceNames();
}

Mpris2SourceModel::~Mpris2SourceModel()
{
}

QVariant Mpris2SourceModel::dataFromPlayer(PlayerContainer *container, int role)
{
    switch (role) {
    case TrackRole:
        return container->track();
    case CanControlRole:
        return container->canControl();
    case CanGoNextRole:
        return container->canGoNext();
    case CanGoPreviousRole:
        return container->canGoPrevious();
    case CanPlayRole:
        return container->canPlay();
    case CanPauseRole:
        return container->canPause();
    case CanStopRole:
        return container->canStop();
    case CanSeekRole:
        return container->canSeek();
    case LoopStatusRole:
        return container->loopStatus();
    case PlaybackStatusRole:
        return container->playbackStatus();
    case PositionRole:
        return container->position();
    case RateRole:
        return container->rate();
    case ShuffleRole:
        return container->shuffle();
    case VolumeRole:
        return container->volume();
    case ArtUrlRole:
        return container->artUrl();
    case ArtistRole:
        return container->artist();
    case AlbumRole:
        return container->album();
    case LengthRole:
        return container->length();
    case CanQuitRole:
        return container->canQuit();
    case CanRaiseRole:
        return container->canRaise();
    case CanSetFullscreenRole:
        return container->canSetFullscreen();
    case DesktopEntryRole:
        return container->desktopEntry();
    case IdentityRole:
        return container->identity();
    case IconNameRole:
        return container->iconName();
    case InstancePidRole:
        return container->instancePid();
    case KDEPidRole:
        return container->kdePid();
    case ContainerRole:
        return QVariant::fromValue(container);
    default:
        return {};
    }
}

QVariant Mpris2SourceModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    return dataFromPlayer(m_containers.at(index.row()), role);
}

bool Mpris2SourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    bool ok = false;
    switch (PlayerContainer *const container = m_containers.at(index.row()); role) {
    case LoopStatusRole:
        Q_ASSERT(value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(LoopStatus::Track));
        if (value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(LoopStatus::Track)) {
            container->setLoopStatus(static_cast<LoopStatus::Status>(value.toUInt(&ok)));
            return ok;
        }
        break;
    case PlaybackStatusRole:
        Q_ASSERT(value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(PlaybackStatus::Paused));
        if (value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(PlaybackStatus::Paused)) {
            container->setPlaybackStatus(static_cast<PlaybackStatus::Status>(value.toUInt(&ok)));
            return ok;
        }
        break;
    case PositionRole:
        container->setPosition(value.toLongLong(&ok));
        return ok;
    case ShuffleRole:
        Q_ASSERT(value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(ShuffleStatus::On));
        if (value.toUInt() >= 0 && value.toUInt() <= qToUnderlying(ShuffleStatus::On)) {
            container->setShuffle(static_cast<ShuffleStatus::Status>(value.toUInt(&ok)));
            return ok;
        }
        break;
    case VolumeRole:
        Q_ASSERT(value.toDouble() >= 0.0 && value.toDouble() <= 1.0);
        if (value.toDouble() >= 0.0 && value.toDouble() <= 1.0) {
            container->setVolume(value.toDouble(&ok));
            return ok;
        }
        break;
    default:
        break;
    }

    return false;
}

int Mpris2SourceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_containers.size();
}

QHash<int, QByteArray> Mpris2SourceModel::roleNames() const
{
    return {
        {TrackRole, QByteArrayLiteral("track")},
        {CanControlRole, QByteArrayLiteral("canControl")},
        {CanGoNextRole, QByteArrayLiteral("canGoNext")},
        {CanGoPreviousRole, QByteArrayLiteral("canGoPrevious")},
        {CanPlayRole, QByteArrayLiteral("canPlay")},
        {CanPauseRole, QByteArrayLiteral("canPause")},
        {CanStopRole, QByteArrayLiteral("canStop")},
        {CanSeekRole, QByteArrayLiteral("canSeek")},
        {LoopStatusRole, QByteArrayLiteral("loopStatus")},
        {PlaybackStatusRole, QByteArrayLiteral("playbackStatus")},
        {PositionRole, QByteArrayLiteral("position")},
        {RateRole, QByteArrayLiteral("rate")},
        {ShuffleRole, QByteArrayLiteral("shuffle")},
        {VolumeRole, QByteArrayLiteral("volume")},
        {ArtUrlRole, QByteArrayLiteral("artUrl")},
        {ArtistRole, QByteArrayLiteral("artist")},
        {AlbumRole, QByteArrayLiteral("album")},
        {LengthRole, QByteArrayLiteral("length")},
        {CanQuitRole, QByteArrayLiteral("canQuit")},
        {CanRaiseRole, QByteArrayLiteral("canRaise")},
        {CanSetFullscreenRole, QByteArrayLiteral("canSetFullscreen")},
        {DesktopEntryRole, QByteArrayLiteral("desktopEntry")},
        {IdentityRole, QByteArrayLiteral("identity")},
        {IconNameRole, QByteArrayLiteral("iconName")},
        {KDEPidRole, QByteArrayLiteral("kdePid")},
        {ContainerRole, QByteArrayLiteral("container")},
    };
}

void Mpris2SourceModel::onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    if (!m_listReady || !serviceName.startsWith(MPRIS2_PREFIX)) {
        return;
    }

    const QString sourceName = serviceName.sliced(MPRIS2_PATH.size());

    if (!oldOwner.isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS service" << serviceName << "just went offline";
        removeMediaPlayer(sourceName);
    }

    if (!newOwner.isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS service" << serviceName << "just came online";
        addMediaPlayer(serviceName, sourceName);
    }
}

void Mpris2SourceModel::onServiceNamesFeteched(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(MPRIS2) << "Could not get list of available D-Bus services";
    } else {
        for (const QStringList names = propsReply.value(); const QString &serviceName : names) {
            if (!serviceName.startsWith(MPRIS2_PREFIX)) {
                continue;
            }

            qCDebug(MPRIS2) << "Found MPRIS2 service" << serviceName;
            // watch out for race conditions; the media player could
            // have appeared between starting the service watcher and
            // this call being dealt with
            // NB: _disappearing_ between sending this call and doing
            // this processing is fine
            const QString sourceName = serviceName.mid(23);
            const bool exist = std::any_of(m_containers.cbegin(), m_containers.cend(), [&sourceName](PlayerContainer *c) {
                return c->objectName() == sourceName;
            });
            if (!exist) {
                qCDebug(MPRIS2) << "Haven't already seen" << serviceName;
                addMediaPlayer(serviceName, sourceName);
            }
        }
    }

    m_listReady = true;
}

void Mpris2SourceModel::onInitialFetchFinished(PlayerContainer *container)
{
    qCDebug(MPRIS2) << "Props fetch for" << container->objectName() << "finished; adding";

    // don't let future refreshes trigger this
    disconnect(container, &PlayerContainer::initialFetchFinished, this, &Mpris2SourceModel::onInitialFetchFinished);
    disconnect(container, &PlayerContainer::initialFetchFailed, this, &Mpris2SourceModel::onInitialFetchFailed);

    // Check if the player follows the specification dutifully.
    if (container->identity().isEmpty()) {
        qCDebug(MPRIS2) << "MPRIS2 service" << container->objectName() << "isn't standard-compliant, ignoring";
        return;
    }

    const int row = m_containers.size();
    beginInsertRows(QModelIndex(), row, row);

    m_containers.push_back(container);

    endInsertRows();

    // Property bindings
    connect(container, &AbstractPlayerContainer::canControlChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanControlRole});
    });
    connect(container, &AbstractPlayerContainer::trackChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {TrackRole});
    });
    connect(container, &AbstractPlayerContainer::canGoNextChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanGoNextRole});
    });
    connect(container, &AbstractPlayerContainer::canGoPreviousChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanGoPreviousRole});
    });
    connect(container, &AbstractPlayerContainer::canPlayChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanPlayRole});
    });
    connect(container, &AbstractPlayerContainer::canPauseChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanPauseRole});
    });
    connect(container, &AbstractPlayerContainer::canStopChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanStopRole});
    });
    connect(container, &AbstractPlayerContainer::canSeekChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanSeekRole});
    });
    connect(container, &AbstractPlayerContainer::loopStatusChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {LoopStatusRole});
    });
    connect(container, &AbstractPlayerContainer::playbackStatusChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {PlaybackStatusRole});
    });
    connect(container, &AbstractPlayerContainer::positionChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {PositionRole});
    });
    connect(container, &AbstractPlayerContainer::rateChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {RateRole});
    });
    connect(container, &AbstractPlayerContainer::shuffleChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {ShuffleRole});
    });
    connect(container, &AbstractPlayerContainer::volumeChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {VolumeRole});
    });
    connect(container, &AbstractPlayerContainer::artUrlChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {ArtUrlRole});
    });
    connect(container, &AbstractPlayerContainer::artistChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {ArtistRole});
    });
    connect(container, &AbstractPlayerContainer::albumChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {AlbumRole});
    });
    connect(container, &AbstractPlayerContainer::lengthChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {LengthRole});
    });
    connect(container, &AbstractPlayerContainer::canQuitChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanQuitRole});
    });
    connect(container, &AbstractPlayerContainer::canRaiseChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanRaiseRole});
    });
    connect(container, &AbstractPlayerContainer::canSetFullscreenChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {CanSetFullscreenRole});
    });
    connect(container, &AbstractPlayerContainer::desktopEntryChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {DesktopEntryRole, IconNameRole});
    });
    connect(container, &AbstractPlayerContainer::identityChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {IdentityRole});
    });
    connect(container, &AbstractPlayerContainer::kdePidChanged, this, [this] {
        const int row = std::distance(m_containers.cbegin(), std::find(m_containers.cbegin(), m_containers.cend(), sender()));
        Q_EMIT dataChanged(index(row, 0), index(row, 0), {KDEPidRole});
    });
}

void Mpris2SourceModel::onInitialFetchFailed(PlayerContainer *container)
{
    qCWarning(MPRIS2) << "Failed to find a working MPRIS2 interface for" << container->dbusAddress();
    delete container;
}

void Mpris2SourceModel::fetchServiceNames()
{
    QDBusPendingCall call = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &Mpris2SourceModel::onServiceNamesFeteched);
}

void Mpris2SourceModel::addMediaPlayer(const QString &serviceName, const QString &sourceName)
{
    PlayerContainer *const container = new PlayerContainer(serviceName, this);
    container->setObjectName(sourceName);

    connect(container, &PlayerContainer::initialFetchFinished, this, &Mpris2SourceModel::onInitialFetchFinished);
    connect(container, &PlayerContainer::initialFetchFailed, this, &Mpris2SourceModel::onInitialFetchFailed);
}

void Mpris2SourceModel::removeMediaPlayer(const QString &sourceName)
{
    auto it = std::find_if(m_containers.begin(), m_containers.end(), [&sourceName](PlayerContainer *c) {
        return c->objectName() == sourceName;
    });

    PlayerContainer *container = *it;
    disconnect(container, nullptr, this, nullptr);
    const int row = std::distance(m_containers.begin(), it);
    beginRemoveRows(QModelIndex(), row, row);
    m_containers.erase(it);
    endRemoveRows();

    delete container;
}

#include "moc_mpris2sourcemodel.cpp"