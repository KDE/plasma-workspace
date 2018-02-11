/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef ACTIVITYINFO_H
#define ACTIVITYINFO_H

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

#endif
