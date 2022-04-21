/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksproxymodeliface.h"

#include <QConcatenateTablesProxyModel>

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short A proxy tasks model for concatenating multiple source tasks models.
 *
 * This proxy model is a subclass of \QConcatenateTablesProxyModel implementing
 * AbstractTasksModelIface, forwarding calls to the correct source model.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT ConcatenateTasksProxyModel : public QConcatenateTablesProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

public:
    explicit ConcatenateTasksProxyModel(QObject *parent = nullptr);
    ~ConcatenateTasksProxyModel() override;

protected:
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;
};

}
