#/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "multiplexer.h"

#include "mpris2filterproxymodel.h"
#include "mpris2sourcemodel.h"
#include "playercontainer.h"

std::shared_ptr<Multiplexer> Multiplexer::self()
{
    static std::weak_ptr<Multiplexer> s_multiplexer;
    if (s_multiplexer.expired()) {
        auto ptr = std::make_shared<Multiplexer>();
        s_multiplexer = ptr;
        return ptr;
    }

    return s_multiplexer.lock();
}

Multiplexer::Multiplexer(QObject *parent)
    : QObject(parent)
    , m_filterModel(Mpris2FilterProxyModel::self())
{
    for (int i = 0, size = m_filterModel->rowCount(); i < size; ++i) {
        PlayerContainer *const container = m_filterModel->index(i, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
        connect(container, &PlayerContainer::playbackStatusChanged, this, &Multiplexer::onPlaybackStatusChanged);
    }

    connect(m_filterModel.get(), &QAbstractListModel::rowsInserted, this, &Multiplexer::onRowsInserted);
    connect(m_filterModel.get(), &QAbstractListModel::rowsAboutToBeRemoved, this, &Multiplexer::onRowsAboutToBeRemoved);
    // rowsRemoved also triggers updates in MultiplexerModel, but we want to update MultiplexerModel after rowsRemoved
    connect(m_filterModel.get(), &QAbstractListModel::rowsRemoved, this, &Multiplexer::onRowsRemoved);
}

Multiplexer::~Multiplexer()
{
}

QBindable<int> Multiplexer::activePlayerIndex() const
{
    return &m_activePlayerIndex;
}

QBindable<PlayerContainer *> Multiplexer::activePlayer() const
{
    return &m_activePlayer;
}

void Multiplexer::onRowsInserted(const QModelIndex &, int first, int)
{
    PlayerContainer *const container = m_filterModel->index(first, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    connect(container, &PlayerContainer::playbackStatusChanged, this, &Multiplexer::onPlaybackStatusChanged);

    if (m_activePlayer && m_activePlayer->playbackStatus() == PlaybackStatus::Playing) {
        // Keep the current player
        // No need to update index as the new item is inserted at the end
        return;
    }

    if (!m_activePlayer || container->playbackStatus() == PlaybackStatus::Playing) {
        // Use the new player
        m_activePlayer = container;
        m_activePlayerIndex = first;
    }
}

void Multiplexer::onRowsAboutToBeRemoved(const QModelIndex &, int first, int)
{
    Q_ASSERT(m_activePlayer);
    PlayerContainer *const container = m_filterModel->index(first, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    // Need to manually disconnect from the container because the source can be filtered out but not gone (e.g. a browser)
    disconnect(container, &PlayerContainer::playbackStatusChanged, this, &Multiplexer::onPlaybackStatusChanged);
    if (m_activePlayerIndex == first) {
        m_activePlayer = nullptr;
        // Index is updated in evaluatePlayers() later
    }
}

void Multiplexer::onRowsRemoved(const QModelIndex &, int, int)
{
    if (!m_activePlayer) {
        evaluatePlayers();
    } else {
        // Only update index
        updateIndex();
    }
}

void Multiplexer::onPlaybackStatusChanged()
{
    // m_activePlayer can't be nullptr here, otherwise something is wrong
    if (m_activePlayer->playbackStatus() == PlaybackStatus::Playing) {
        // Keep the current player
        return;
    }

    PlayerContainer *container = static_cast<PlayerContainer *>(sender());
    if (container->playbackStatus() == PlaybackStatus::Playing) {
        // Use the new player
        m_activePlayer = container;
        updateIndex();
    } else {
        evaluatePlayers();
    }
}

void Multiplexer::updateIndex()
{
    const auto sourceModel = static_cast<Mpris2SourceModel *>(m_filterModel->sourceModel());
    const int rawRow =
        std::distance(sourceModel->m_containers.cbegin(), std::find(sourceModel->m_containers.cbegin(), sourceModel->m_containers.cend(), m_activePlayer));
    const QModelIndex idx = m_filterModel->mapFromSource(sourceModel->index(rawRow, 0));
    // Q_ASSERT_X(idx.isValid(), Q_FUNC_INFO, qUtf8Printable(QStringLiteral("m_activePlayer: %1").arg(m_activePlayer->identity())));
    m_activePlayerIndex = idx.row();
}

void Multiplexer::evaluatePlayers()
{
    PlayerContainer *container = nullptr;
    for (int i = 0, size = m_filterModel->rowCount(); i < size; ++i) {
        PlayerContainer *c = m_filterModel->index(i, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
        if (c->playbackStatus() == PlaybackStatus::Playing) {
            container = c;
            break;
        }
    }

    if (container) {
        // Has an active player
        m_activePlayer = container;
        updateIndex();
    } else if (!m_activePlayer && m_filterModel->rowCount() > 0) {
        // No active player, use the first player
        m_activePlayer = m_filterModel->index(0, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
        m_activePlayerIndex = 0;
    } else if (m_filterModel->rowCount() == 0) {
        m_activePlayer = nullptr;
        m_activePlayerIndex = -1;
    }

    // If there was an active player and currently there is no player that is playing, keep the previous selection
}
