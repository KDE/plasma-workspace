/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "funnelmodel.h"

FunnelModel::FunnelModel(QObject *parent)
    : ForwardingModel(parent)
{
}

FunnelModel::~FunnelModel()
{
}

void FunnelModel::setSourceModel(QAbstractItemModel *model)
{
    if (model && m_sourceModel == model) {
        return;
    }

    if (!model) {
        reset();

        return;
    }

    connect(model, SIGNAL(destroyed(QObject *)), this, SLOT(reset()));

    if (!m_sourceModel) {
        beginResetModel();

        m_sourceModel = model;

        connectSignals();

        endResetModel();

        Q_EMIT countChanged();

        Q_EMIT sourceModelChanged();
        Q_EMIT descriptionChanged();

        return;
    }

    int oldCount = m_sourceModel->rowCount();
    int newCount = model->rowCount();

    auto setNewModel = [this, model]() {
        disconnectSignals();
        m_sourceModel = model;
        connectSignals();
    };

    if (newCount > oldCount) {
        beginInsertRows(QModelIndex(), oldCount, newCount - 1);
        setNewModel();
        endInsertRows();
    } else if (newCount < oldCount) {
        if (newCount == 0) {
            beginResetModel();
            setNewModel();
            endResetModel();
        } else {
            beginRemoveRows(QModelIndex(), newCount, oldCount - 1);
            setNewModel();
            endRemoveRows();
        }
    } else {
        setNewModel();
    }

    if (newCount > 0) {
        Q_EMIT dataChanged(index(0, 0), index(qMin(oldCount, newCount) - 1, 0));
    }

    if (oldCount != newCount) {
        Q_EMIT countChanged();
    }

    Q_EMIT sourceModelChanged();
    Q_EMIT descriptionChanged();
}
