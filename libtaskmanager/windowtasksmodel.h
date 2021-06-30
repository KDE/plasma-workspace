/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QIdentityProxyModel>

#include "abstracttasksproxymodeliface.h"

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short A window tasks model.
 *
 * This model presents tasks sourced from window data retrieved from the
 * windowing server the host process is connected to. The underlying
 * windowing system is abstracted away.
 *
 * @see WaylandTasksModel
 * @see XWindowTasksModel
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT WindowTasksModel : public QIdentityProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

public:
    explicit WindowTasksModel(QObject *parent = nullptr);
    ~WindowTasksModel() override;

    QHash<int, QByteArray> roleNames() const override;

protected:
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}
