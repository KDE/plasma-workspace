/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "forwardingmodel.h"

ForwardingModel::ForwardingModel(QObject *parent)
    : AbstractModel(parent)
{
}

ForwardingModel::~ForwardingModel()
{
}

QString ForwardingModel::description() const
{
    if (!m_sourceModel) {
        return QString();
    }

    AbstractModel *abstractModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (!abstractModel) {
        return QString();
    }

    return abstractModel->description();
}

QAbstractItemModel *ForwardingModel::sourceModel() const
{
    return m_sourceModel;
}

void ForwardingModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    disconnectSignals();

    beginResetModel();

    m_sourceModel = sourceModel;

    connectSignals();

    endResetModel();

    emit countChanged();
    emit sourceModelChanged();
    emit descriptionChanged();
}

bool ForwardingModel::canFetchMore(const QModelIndex &parent) const
{
    if (!m_sourceModel) {
        return false;
    }

    return m_sourceModel->canFetchMore(indexToSourceIndex(parent));
}

void ForwardingModel::fetchMore(const QModelIndex &parent)
{
    if (m_sourceModel) {
        m_sourceModel->fetchMore(indexToSourceIndex(parent));
    }
}

QModelIndex ForwardingModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (!m_sourceModel) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex ForwardingModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)

    return QModelIndex();
}

QVariant ForwardingModel::data(const QModelIndex &index, int role) const
{
    if (!m_sourceModel) {
        return QVariant();
    }

    return m_sourceModel->data(indexToSourceIndex(index), role);
}

int ForwardingModel::rowCount(const QModelIndex &parent) const
{
    if (!m_sourceModel) {
        return 0;
    }

    return m_sourceModel->rowCount(indexToSourceIndex(parent));
}

QModelIndex ForwardingModel::indexToSourceIndex(const QModelIndex &index) const
{
    if (!m_sourceModel || !index.isValid()) {
        return QModelIndex();
    }

    return m_sourceModel->index(index.row(), index.column(), index.parent().isValid() ? indexToSourceIndex(index.parent()) : QModelIndex());
}

bool ForwardingModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    if (!m_sourceModel) {
        return false;
    }

    AbstractModel *abstractModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (!abstractModel) {
        return false;
    }

    return abstractModel->trigger(row, actionId, argument);
}

QString ForwardingModel::labelForRow(int row)
{
    if (!m_sourceModel) {
        return QString();
    }

    AbstractModel *abstractModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (!abstractModel) {
        return QString();
    }

    return abstractModel->labelForRow(row);
}

AbstractModel *ForwardingModel::modelForRow(int row)
{
    if (!m_sourceModel) {
        return nullptr;
    }

    AbstractModel *abstractModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (!abstractModel) {
        return nullptr;
    }

    return abstractModel->modelForRow(row);
}

AbstractModel *ForwardingModel::favoritesModel()
{
    AbstractModel *sourceModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (sourceModel) {
        return sourceModel->favoritesModel();
    }

    return AbstractModel::favoritesModel();
}

int ForwardingModel::separatorCount() const
{
    if (!m_sourceModel) {
        return 0;
    }

    AbstractModel *abstractModel = qobject_cast<AbstractModel *>(m_sourceModel);

    if (!abstractModel) {
        return 0;
    }

    return abstractModel->separatorCount();
}

void ForwardingModel::reset()
{
    beginResetModel();
    endResetModel();

    emit countChanged();
    emit separatorCountChanged();
}

void ForwardingModel::connectSignals()
{
    if (!m_sourceModel) {
        return;
    }

    connect(m_sourceModel, SIGNAL(destroyed()), this, SLOT(reset()));
    connect(m_sourceModel.data(), &QAbstractItemModel::dataChanged, this, &QAbstractItemModel::dataChanged, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsAboutToBeInserted, this, &QAbstractItemModel::rowsAboutToBeInserted, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsAboutToBeMoved, this, &QAbstractItemModel::rowsAboutToBeMoved, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &QAbstractItemModel::rowsAboutToBeRemoved, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::layoutAboutToBeChanged, this, &QAbstractItemModel::layoutAboutToBeChanged, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsInserted, this, &QAbstractItemModel::rowsInserted, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsInserted, this, &AbstractModel::countChanged, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsMoved, this, &QAbstractItemModel::rowsMoved, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsRemoved, this, &QAbstractItemModel::rowsRemoved, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::rowsRemoved, this, &AbstractModel::countChanged, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::modelAboutToBeReset, this, &QAbstractItemModel::modelAboutToBeReset, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::modelReset, this, &QAbstractItemModel::modelReset, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::modelReset, this, &AbstractModel::countChanged, Qt::UniqueConnection);
    connect(m_sourceModel.data(), &QAbstractItemModel::layoutChanged, this, &QAbstractItemModel::layoutChanged, Qt::UniqueConnection);
}

void ForwardingModel::disconnectSignals()
{
    if (!m_sourceModel) {
        return;
    }

    disconnect(m_sourceModel, nullptr, this, nullptr);
}
