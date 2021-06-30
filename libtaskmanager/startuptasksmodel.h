/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "abstracttasksmodel.h"
#include "taskmanager_export.h"

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

class TASKMANAGER_EXPORT StartupTasksModel : public AbstractTasksModel
{
    Q_OBJECT

public:
    explicit StartupTasksModel(QObject *parent = nullptr);
    ~StartupTasksModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Request an additional instance of the application backing the
     * startup notification at the given index.
     *
     * @param index An index in this startup tasks model.
     **/
    void requestNewInstance(const QModelIndex &index) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

}
