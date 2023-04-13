/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short Provides basic activity information.
 *
 * This class provides basic information about the activities defined in
 * the system.
 *
 * @NOTE: This is a placeholder, to be moved into KActivities (which it
 * wraps) or the Task Manager applet backend.
 *
 * @see KActivities
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT ActivityInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentActivity READ currentActivity NOTIFY currentActivityChanged)
    Q_PROPERTY(int numberOfRunningActivities READ numberOfRunningActivities NOTIFY numberOfRunningActivitiesChanged)

public:
    explicit ActivityInfo(QObject *parent = nullptr);
    ~ActivityInfo() override;

    /**
     * The currently active virtual desktop.
     *
     * @returns the number of the currently active virtual desktop.
     **/
    QString currentActivity() const;

    /**
     * The number of currently-running activities defined in the session.
     *
     * @returns the number of activities defined in the session.
     **/
    int numberOfRunningActivities() const;

    /**
     * The list of currently-running activities defined in the session.
     *
     * @returns the list of currently-running activities defined in the session.
     **/
    Q_INVOKABLE QStringList runningActivities() const;

    /**
     * The name of the activity of the given id.
     *
     * @param id An activity id string.
     * @returns the name of the activity of the given id.
     **/
    Q_INVOKABLE QString activityName(const QString &id);

    /**
     * The icon of the activity of the given id.
     *
     * @param id An activity id string.
     * @returns the name or file path of the activity of the given id.
     **/
    Q_INVOKABLE QString activityIcon(const QString &id);

Q_SIGNALS:
    void currentActivityChanged() const;
    void numberOfRunningActivitiesChanged() const;

    /**
     * The names of the running activities have changed.
     * @since 5.40.0
     **/
    void namesOfRunningActivitiesChanged() const;

private:
    class Private;
    QScopedPointer<Private> d;
};

}
