/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#ifndef PROGRESSLISTMODEL_H
#define PROGRESSLISTMODEL_H

#include "uiserver.h"
#include "jobview.h"

#include <QDBusContext>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(KUISERVER)


class QDBusAbstractInterface;
class QDBusServiceWatcher;

class ProgressListModel: public QAbstractItemModel, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.JobViewServer")


public:

    explicit ProgressListModel(QObject *parent = 0);
    ~ProgressListModel() override;

    QModelIndex parent(const QModelIndex&) const override;


    /**
    * Returns what operations the model/delegate support on the given @p index
    *
    * @param index    the index in which you want to know the allowed operations
    * @return         the allowed operations on the model/delegate
    */
    Qt::ItemFlags flags(const QModelIndex &index) const override;


    /**
    * Returns the data on @p index that @p role contains. The result is
    * a QVariant, so you may need to cast it to the type you want
    *
    * @param index    the index in which you are accessing
    * @param role     the role you want to retrieve
    * @return         the data in a QVariant class
    */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    /**
    * Returns the index for the given @p row. Since it is a list, @p column should
    * be 0, but it will be ignored. @p parent will be ignored as well.
    *
    * @param row      the row you want to get the index
    * @param column   will be ignored
    * @param parent   will be ignored
    * @return         the index for the given @p row as a QModelIndex
    */
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex indexForJob(JobView *jobView) const;


    /**
    * Returns the number of columns
    *
    * @param parent   will be ignored
    * @return         the number of columns. In this case is always 1
    */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;


    /**
    * Returns the number of rows
    *
    * @param parent   will be ignored
    * @return         the number of rows in the model
    */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;


    /**
    * Called by a KJob's DBus connection to this ("JobViewServer").
    * Indicates that the KJob is now in existence (and is a just-created job).
    * Returns a QDBusObjectPath that represents a unique dbus path that is a subset
    * of the current "org.kde.JobView" address(so there can be multiple jobs, and KJob
    * can keep track of them.
    */
    QDBusObjectPath requestView(const QString &appName, const QString &appIconName, int capabilities);

public Q_SLOTS:
    /**
    * Calling this(within "org.kde.kuiserver") results in all of the
    * information that pertains to any KJobs and status updates for
    * them, being sent to this DBus address, at the given
    * @p objectPath
    * Note that you will also receive jobs that existed before this was
    * called
    */
    void registerService(const QString &service, const QString &objectPath);

    /**
    * Forces emission of jobUrlsChanged() signal...exposed to D-BUS (because they need it).
    * @see jobUrlsChanged
    */
    void emitJobUrlsChanged();

    /**
    * Whether or not a JobTracker will be needed. This will occur if there are no useful registered
    * services. For example, this would occur if Plasma has "Show application jobs/file transfers" disabled.
    * In which case, returning true here would be expected. This way KDynamicJobTracker can create a
    * KWidgetJobTracker for each job (shows a dialog for each job).
    * @return if a proper job tracker needs to be created by something.
    */
    bool requiresJobTracker();

    /**
     * Only used over D-BUS merely for debugging purposes.
     * Lets us know which services and at which paths kuiserver
     * thinks should be informed. e.g. Plasma
     * (the applicationjobs dataengine), Dolphin, etc.
     * @return list of currently registered services
     */
    QStringList registeredJobContacts();

private Q_SLOTS:

    void jobFinished(JobView *jobView);
    void jobChanged(uint jobId);


    /**
    * Implemented to handle the case when a client drops out.
    */
    void serviceUnregistered(const QString &name);

Q_SIGNALS:
    void serviceDropped(const QString&);

    /**
    * Emits a list of destination URL's that have
    * jobs pertaining to them(when it changes).
    */
    void jobUrlsChanged(QStringList);

private:

    QDBusObjectPath newJob(const QString &appName, const QString &appIcon, int capabilities);

    ///< desturls
    QStringList gatherJobUrls();

    /**
    * The next available(unused) unique jobId, we can use this one directly,
    * just remember to increment it after you construct a job from it.
    */
    uint m_jobId;

    QList<JobView*> m_jobViews;
    /**
     * Stores a relationship between the process that requested the job and
     * the job itself by using the dbus service and he jobview.
     */
    QHash<QString, JobView*> m_jobViewsOwners;

    /**
     * Contains the list of registered services. In other words, the clients
     * who have "subscribed" to our D-Bus interface so they can get informed
     * about changes to all the jobs.
     */
    QHash<QString, QDBusAbstractInterface*> m_registeredServices;

    UiServer *m_uiServer;
    QDBusServiceWatcher *m_serviceWatcher;
};

//Q_DECLARE_METATYPE(JobView*)

#endif // PROGRESSLISTMODEL_H
