/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksproxymodeliface.h"

#include <KDescendantsProxyModel>

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short A proxy tasks model for flattening a tree-structured tasks model
 * into a list-structured tasks model.
 *
 * This proxy model is a subclass of KDescendantsProxyModel implementing
 * AbstractTasksModelIface.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT FlattenTaskGroupsProxyModel : public KDescendantsProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

public:
    explicit FlattenTaskGroupsProxyModel(QObject *parent = nullptr);
    ~FlattenTaskGroupsProxyModel() override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

protected:
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}
