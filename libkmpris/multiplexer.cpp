#/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "multiplexer.h"

#include "mpris2filterproxymodel.h"
#include "mpris2sourcemodel.h"
#include "playercontainer.h"

#include "libkmpris_debug.h"

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

    connect(m_filterModel.get(), &QAbstractItemModel::rowsInserted, this, &Multiplexer::onRowsInserted);
    connect(m_filterModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &Multiplexer::onRowsAboutToBeRemoved);
    // rowsRemoved also triggers updates in MultiplexerModel, but we want to update MultiplexerModel after rowsRemoved
    connect(m_filterModel.get(), &QAbstractItemModel::rowsRemoved, this, &Multiplexer::onRowsRemoved);
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

    if (!m_activePlayer || (m_activePlayer->playbackStatus() != PlaybackStatus::Playing && container->playbackStatus() == PlaybackStatus::Playing)) {
        m_activePlayer = container;
        m_activePlayerIndex = first;
    } else {
        // Keep the current player
        updateIndex(); // BUG 483027: The index might have changed when the pbi filter is invalidated
    }
}

void Multiplexer::onRowsAboutToBeRemoved(const QModelIndex &, int first, int)
{
    Q_ASSERT_X(m_activePlayer, Q_FUNC_INFO, qUtf8Printable(QString::number(first)));
    PlayerContainer *const container = m_filterModel->index(first, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    // Need to manually disconnect from the container because the source can be filtered out but not gone (e.g. a browser)
    disconnect(container, &PlayerContainer::playbackStatusChanged, this, &Multiplexer::onPlaybackStatusChanged);
    if (m_activePlayerIndex == first) {
        Q_ASSERT_X(m_activePlayer.value() == container,
                   Q_FUNC_INFO,
                   qUtf8Printable(QStringLiteral("Active player %1 does not match active index %2 (%3)")
                                      .arg(m_activePlayer->identity(), QString::number(m_activePlayerIndex.value()), container->identity())));
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
    for (int i = 0, count = m_filterModel->rowCount(); i < count; ++i) {
        if (m_filterModel->index(i, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>() == m_activePlayer.value()) {
            m_activePlayerIndex = i;
            return;
        }
    }

    const auto sourceModel = static_cast<Mpris2SourceModel *>(m_filterModel->sourceModel());
    const auto beginIt = sourceModel->m_containers.cbegin();
    const auto endIt = sourceModel->m_containers.cend();
    qCWarning(MPRIS2) << "Current active player:" << m_activePlayer->identity();
    qCWarning(MPRIS2) << "Available players:" << std::accumulate(beginIt, endIt, QString(), [](QString left, PlayerContainer *right) {
        return std::move(left) + QLatin1Char(',') + right->identity();
    });
    qCWarning(MPRIS2) << "Pending players:"
                      << std::accumulate(sourceModel->m_pendingContainers.cbegin(),
                                         sourceModel->m_pendingContainers.cend(),
                                         QString(),
                                         [](QString left, auto &right) {
                                             return std::move(left) + QLatin1Char(',') + right.first /* sourceName */;
                                         });
    Q_ASSERT_X(false, Q_FUNC_INFO, "Model index is invalid");
    m_activePlayerIndex = -1;
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
