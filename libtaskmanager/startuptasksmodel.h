/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksproxymodeliface.h"
#include "taskmanager_export.h"

#include <QIdentityProxyModel>

namespace TaskManager
{
/**
 * @short A tasks model for startup notifications.
 *
 * This model presents tasks sourced from startup notifications.
 *
 * Startup notifications are given a timeout sourced from klaunchrc, falling
 * back to a default of 5 seconds if no configuration is present.
 *
 * Startup tasks are removed 500 msec after cancellation or timeout to
 * overlap with window tasks appearing in window task models.
 *
 * @author Eike Hein <hein@kde.org>
 */

class TASKMANAGER_EXPORT StartupTasksModel : public QIdentityProxyModel, public AbstractTasksProxyModelIface
{
    Q_OBJECT

public:
    explicit StartupTasksModel(QObject *parent = nullptr);
    ~StartupTasksModel() override;

    QHash<int, QByteArray> roleNames() const override;

protected:
    QModelIndex mapIfaceToSource(const QModelIndex &index) const override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}
