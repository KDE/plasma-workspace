/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "multiplexermodel.h"

#include <KLocalizedString>

#include "mpris2sourcemodel.h"
#include "multiplexer.h"
#include "multiplexermodel.h"
#include "playercontainer.h"

MultiplexerModel::MultiplexerModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_multiplexer(new Multiplexer(this))
{
    m_multiplexer->preferredPlayer().setBinding([this] {
        return m_preferredPlayer.value();
    });

    updateActivePlayer();
    /*
        # Why is Qt::QueuedConnection used?
        If there are only one player remaining and the last player is closed:
        1. Mpris2SourceModel::rowsRemoved -> Mpris2FilterProxyModel::rowsRemoved
        3. Mpris2FilterProxyModel::rowsRemoved -> Multiplexer::onRowsRemoved
        4. In Multiplexer::onRowsRemoved, activePlayerIndex is updated -> Multiplexer::activePlayerIndexChanged
        5. Multiplexer::activePlayerIndexChanged -> **Qt::QueuedConnection**, so MultiplexerModel::updateActivePlayer() is not called in this context。
            ⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️⚠️
            Can't call updateActivePlayer() in the current context because updateActivePlayer() will emit rowsRemoved, which confuses Mpris2Model
            because at this moment Mpris2FilterProxyModel::rowsRemoved is not finished!!! Without Qt::QueuedConnection there will be a CRASH!
            💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣💣
        6. After Mpris2SourceModel::rowsRemoved, the container is deleted -> PlayerContainer::destroyed
        7. PlayerContainer::destroyed -> MultiplexerModel::updateActivePlayer
        8. MultiplexerModel::updateActivePlayer -> (no player now) MultiplexerModel::rowsRemoved
        9. The queued connection now calls MultiplexerModel::updateActivePlayer(), but there is no player now so it just simply returns.

        # Why is this connection needed?
        If there is more than one player, and the active player changes to another player:
        1. activePlayerIndex is updated -> Multiplexer::activePlayerIndexChanged
        2. Multiplexer::activePlayerIndexChanged -> **Qt::QueuedConnection**, so MultiplexerModel::updateActivePlayer() is not called in this context
        3. The queued connection now calls MultiplexerModel::updateActivePlayer(), and (m_multiplexer->activePlayer().value() != m_activePlayer) is satisfied,
           so a new player becomes the starred player.
     */
    connect(m_multiplexer, &Multiplexer::activePlayerIndexChanged, this, &MultiplexerModel::updateActivePlayer, Qt::QueuedConnection);
}

MultiplexerModel::~MultiplexerModel()
{
}

QVariant MultiplexerModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || !m_activePlayer) {
        return {};
    }

    switch (role) {
    case Mpris2SourceModel::IdentityRole:
        return i18nc("@action:button", "Choose player automatically");
    case Mpris2SourceModel::IconNameRole:
        return QStringLiteral("emblem-favorite");
    default:
        return Mpris2SourceModel::dataFromPlayer(m_activePlayer, role);
    }
}

int MultiplexerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_activePlayer ? 1 : 0;
}

QHash<int, QByteArray> MultiplexerModel::roleNames() const
{
    return Mpris2SourceModel::self()->roleNames();
}

QBindable<QString> MultiplexerModel::preferredPlayer()
{
    return &m_preferredPlayer;
}

void MultiplexerModel::updateActivePlayer()
{
    if (m_activePlayer && !m_multiplexer->activePlayer().value()) {
        beginRemoveRows({}, 0, 0);
        disconnect(m_activePlayer, nullptr, this, nullptr);
        m_activePlayer = nullptr;
        endRemoveRows();
    } else if (m_multiplexer->activePlayer().value() != m_activePlayer) {
        if (!m_activePlayer) {
            beginInsertRows({}, 0, 0);
            m_activePlayer = m_multiplexer->activePlayer().value();
            endInsertRows();
        } else {
            disconnect(m_activePlayer, nullptr, this, nullptr);
            m_activePlayer = m_multiplexer->activePlayer().value();
            Q_EMIT dataChanged(index(0, 0), index(0, 0));
        }

        connect(m_activePlayer, &QObject::destroyed, this, &MultiplexerModel::updateActivePlayer);

        // Property bindings
        connect(m_activePlayer, &AbstractPlayerContainer::canControlChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanControlRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::trackChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::TrackRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canGoNextChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanGoNextRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canGoPreviousChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanGoPreviousRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canPlayChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanPlayRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canPauseChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanPauseRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canStopChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanStopRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canSeekChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanSeekRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::loopStatusChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::LoopStatusRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::playbackStatusChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::PlaybackStatusRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::positionChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::PositionRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::rateChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::RateRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::shuffleChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::ShuffleRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::volumeChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::VolumeRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::artUrlChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::ArtUrlRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::artistChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::ArtistRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::albumChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::AlbumRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::lengthChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::LengthRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canQuitChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanQuitRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canRaiseChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanRaiseRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::canSetFullscreenChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::CanSetFullscreenRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::desktopEntryChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::DesktopEntryRole, Mpris2SourceModel::IconNameRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::identityChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::IdentityRole});
        });
        connect(m_activePlayer, &AbstractPlayerContainer::kdePidChanged, this, [this] {
            Q_EMIT dataChanged(index(0, 0), index(0, 0), {Mpris2SourceModel::KDEPidRole});
        });
    }
}

#include "moc_multiplexermodel.cpp"
