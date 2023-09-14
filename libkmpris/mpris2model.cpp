/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "mpris2model.h"

#include <QUrl>

#include "mpris2filterproxymodel.h"
#include "mpris2sourcemodel.h"
#include "multiplexermodel.h"
#include "playercontainer.h"

Mpris2Model::Mpris2Model(QObject *parent)
    : QConcatenateTablesProxyModel(parent)
    , m_multiplexerModel(MultiplexerModel::self())
    , m_mprisModel(Mpris2FilterProxyModel::self())
{
    addSourceModel(m_multiplexerModel.get());
    addSourceModel(m_mprisModel.get());

    connect(this, &QConcatenateTablesProxyModel::dataChanged, this, &Mpris2Model::onDataChanged);
    connect(this, &QConcatenateTablesProxyModel::rowsAboutToBeRemoved, this, &Mpris2Model::onRowsAboutToBeRemoved);
    connect(this, &QConcatenateTablesProxyModel::rowsRemoved, this, &Mpris2Model::onRowsRemoved);
    connect(this, &QConcatenateTablesProxyModel::rowsInserted, this, &Mpris2Model::onRowsInserted);

    if (rowCount() > 0) {
        m_currentPlayer = index(m_currentIndex, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
        Q_EMIT currentPlayerChanged();
    }
}

Mpris2Model::~Mpris2Model()
{
}

QHash<int, QByteArray> Mpris2Model::roleNames() const
{
    return m_mprisModel->roleNames();
}

unsigned Mpris2Model::currentIndex() const
{
    return m_currentIndex;
}

void Mpris2Model::setCurrentIndex(unsigned _index)
{
    if (m_currentIndex == _index) {
        return;
    }

    if (rowCount() > 0) {
        m_currentIndex = std::clamp<unsigned>(_index, 0, rowCount() - 1);
        m_currentPlayer = index(m_currentIndex, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    } else {
        m_currentIndex = 0;
        m_currentPlayer = nullptr;
    }
    Q_EMIT currentPlayerChanged();
    Q_EMIT currentIndexChanged();
}

PlayerContainer *Mpris2Model::currentPlayer() const
{
    return m_currentPlayer;
}

PlayerContainer *Mpris2Model::playerForLauncherUrl(const QUrl &launcherUrl, unsigned pid) const
{
    if (launcherUrl.isEmpty()) {
        return nullptr;
    }

    // MPRIS spec explicitly mentions that "DesktopEntry" is with .desktop extension trimmed
    // Moreover, remove URL parameters, like wmClass (part after the question mark)
    QStringList result = launcherUrl.toString().split(QLatin1Char('/'));
    if (result.empty()) {
        return nullptr;
    }

    result = result.crbegin()->split(QLatin1Char('?'));
    if (result.empty()) {
        return nullptr;
    }

    QString &desktopFileName = result.begin()->remove(QLatin1String(".desktop"));
    if (desktopFileName.contains(QLatin1String("applications:"))) {
        desktopFileName.remove(0, 13);
    }

    // Find in PID
    QModelIndex idx, fallbackIdx;
    for (int i = 0, size = m_mprisModel->rowCount(); i < size; ++i) {
        QModelIndex _idx = m_mprisModel->index(i, 0);
        if (_idx.data(Mpris2SourceModel::InstancePidRole).toUInt() == pid) {
            idx = _idx;
            break;
        } else if (_idx.data(Mpris2SourceModel::KDEPidRole).toUInt() == pid) {
            idx = _idx;
            break;
        } else if (_idx.data(Mpris2SourceModel::DesktopEntryRole).toString() == desktopFileName) {
            fallbackIdx = _idx;
        }
    }

    if (idx.isValid()) {
        return idx.data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    } else if (fallbackIdx.isValid()) {
        // If PID match fails, return fallbackSource.
        return fallbackIdx.data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    } else {
        return nullptr;
    }
}

void Mpris2Model::onRowsInserted(const QModelIndex &, int first, int)
{
    if (first == 0) {
        Q_ASSUME(m_currentIndex == 0);
        m_currentPlayer = index(0, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
        Q_EMIT currentPlayerChanged();
    }
}

void Mpris2Model::onRowsAboutToBeRemoved(const QModelIndex &, int first, int)
{
    if (m_currentIndex < static_cast<unsigned>(first)) {
        return;
    }
    unsigned currentIndex = m_currentIndex;
    const PlayerContainer *const oldPlayer = m_currentPlayer;
    if (currentIndex == static_cast<unsigned>(first)) {
        currentIndex = 0; // Reset to Multiplexer
    }
    if (rowCount() - 1 /* Multiplexer */ >= 2 /* At least two players */) {
        m_currentPlayer = index(currentIndex, 0).data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    } else {
        m_currentPlayer = nullptr;
    }
    if (oldPlayer != m_currentPlayer) {
        Q_EMIT currentPlayerChanged();
    }

    // m_currentIndex is updated in onRowsRemoved(...)
}

void Mpris2Model::onRowsRemoved(const QModelIndex &, int first, int)
{
    if (m_currentIndex < static_cast<unsigned>(first)) {
        return;
    }
    if (m_currentIndex == static_cast<unsigned>(first)) {
        m_currentIndex = 0; // Reset to Multiplexer
    } else if (m_currentIndex > 0) {
        --m_currentIndex;
    }
    Q_EMIT currentIndexChanged();
}

void Mpris2Model::onDataChanged(const QModelIndex &topLeft, const QModelIndex &, const QList<int> &roles)
{
    if (m_currentIndex != static_cast<unsigned>(topLeft.row()) || (!roles.empty() && !roles.contains(Mpris2SourceModel::ContainerRole))) {
        return;
    }
    m_currentPlayer = topLeft.data(Mpris2SourceModel::ContainerRole).value<PlayerContainer *>();
    Q_EMIT currentPlayerChanged();
}

#include "moc_mpris2model.cpp"