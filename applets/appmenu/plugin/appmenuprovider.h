/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "dbusmenumodel.h"
#include "tasksmodel.h"

#include <QRect>
#include <QTimer>
#include <qqmlregistration.h>

class AppMenuProvider : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(DBusMenuModel *model READ model NOTIFY modelChanged)
    Q_PROPERTY(QRect screenGeometry READ screenGeometry WRITE setScreenGeometry NOTIFY screenGeometryChanged)

public:
    explicit AppMenuProvider(QObject *parent = nullptr);

    DBusMenuModel *model() const;

    QRect screenGeometry() const;
    void setScreenGeometry(const QRect &geometry);

Q_SIGNALS:
    void activateRequested(const QModelIndex &index);
    void modelChanged();
    void screenGeometryChanged();

private:
    void onTasksRowsInserted();
    void onTasksRowsRemoved();
    void onTasksModelReset();
    void onTasksDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

    void tryReload();
    void reload(const QString &serviceName, const QString &objectPath);
    void apply();

    std::unique_ptr<DBusMenuModel> m_current;
    std::unique_ptr<DBusMenuModel> m_next;
    std::unique_ptr<TaskManager::TasksModel> m_tasks;
    std::unique_ptr<QTimer> m_timer;
};
