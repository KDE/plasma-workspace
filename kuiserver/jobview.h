/*****************************************************************************
*   Copyright (C) 2009 by Shaun Reich <shaun.reich@kdemail.net>              *
*   Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>       *
*   Copyright (C) 2000 Matej Koss <koss@miesto.sk>                           *
*   Copyright (C) 2000 David Faure <faure@kde.org>                           *
*                                                                            *
*   This program is free software; you can redistribute it and/or            *
*   modify it under the terms of the GNU General Public License as           *
*   published by the Free Software Foundation; either version 2 of           *
*   the License, or (at your option) any later version.                      *
*                                                                            *
*   This program is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*   GNU General Public License for more details.                             *
*                                                                            *
*   You should have received a copy of the GNU General Public License        *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*****************************************************************************/


#ifndef JOBVIEW_H
#define JOBVIEW_H

#include <QtDBus/QDBusObjectPath>

#include <kio/global.h>

#include <kuiserversettings.h>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(KUISERVER)

class QDBusAbstractInterface;
class RequestViewCallWatcher;

class JobView : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.JobViewV2")

public:

    enum DataType {
        Capabilities = 33,
        ApplicationName,
        Icon,
        SizeTotal,
        SizeProcessed,
        TimeTotal,
        TimeElapsed,
        Speed,
        Percent,
        InfoMessage,
        DescFields,
        State,
        JobViewRole
    };

    enum JobState {
        Running = 0,
        Suspended = 1,
        Stopped = 2
    };

    JobView(uint jobId, QObject *parent = 0);
    ~JobView() override;

    void terminate(const QString &errorMessage);

    void setSuspended(bool suspended);

    void setTotalAmount(qulonglong amount, const QString &unit);
    QString sizeTotal() const;

    void setProcessedAmount(qulonglong amount, const QString &unit);
    QString sizeProcessed() const;

    void setPercent(uint percent);
    uint percent() const;

    void setSpeed(qulonglong bytesPerSecond);
    QString speed() const;

    void setInfoMessage(const QString &infoMessage);
    QString infoMessage() const;

    bool setDescriptionField(uint number, const QString &name, const QString &value);
    void clearDescriptionField(uint number);

    void setAppName(const QString &appName);
    QString appName() const;

    void setAppIconName(const QString &appIconName);
    QString appIconName() const;

    void setCapabilities(int capabilities);
    int capabilities() const;

    void setError(uint errorCode);
    uint error() const;

    QString errorText() const;

    uint state() const;

    uint jobId() const;

    QDBusObjectPath objectPath() const;



     /**
     * Set the dest Url of the job...
     * sent from the jobtracker (once upon construction)
     * @param destUrl will be a QVariant, likely to have 1
     * dest Url...OR it can be non-existent (only srcUrl would
     * be there), in that case it is a delete a directory job
     * etc..
     */
    void setDestUrl(const QDBusVariant& destUrl);

    QVariant destUrl() const;

    /**
     *  The below methods force an emission of the respective signal.
     *  Only use case is kuiserver's delegate, where we need to control
     *  the state of a job, but can't emit them because they are signals
     *
     *  Note: it isn't used for job's propagating their children jobs
     *  all the way up to KJob, use the other signals in here for that.
     *  (although it does the same thing).
     */
    void requestSuspend();
    void requestResume();
    void requestCancel();


    /**
     * Called by the model.
     * Lets us know that a job at @p objectPath is
     * open for business. @p address is only for data-keeping of our
     * hash, later on. We will remove it when that address drops,
     * due to some signal magic from the model.
     */
    void addJobContact(const QString& objectPath, const QString& address);

    /**
     * Return the list of job contacts (jobs we are currently forwarding information
     * to over the wire). They *should* be valid. If they are not, something is probably
     * fishy.
     * This method is only for D-BUS debug purposes, for his pleasure.
     * So betting on this method and trying to parse it would not be the best of ideas.
     */
    QStringList jobContacts();

    void pendingCallStarted();

public Q_SLOTS:
    void pendingCallFinished(RequestViewCallWatcher *watcher);

Q_SIGNALS:
    void suspendRequested();
    void resumeRequested();
    void cancelRequested();

    void finished(JobView*);

    /**
     * Triggered when an internal data type changes. It is triggered
     * once for each data type of this JobView, that has changed.
     *
     * @param uint unique job identifier
     */
    void changed(uint);

    void destUrlSet();

private Q_SLOTS:

    /**
     * Called when the model finds out that the client that was
     * registered, has just died. Meaning notifications to the
     * given path, are no longer required(remove them from the list).
     */
    void serviceDropped(const QString &address);

private:

    int m_capabilities;        ///< The capabilities of the job

    QString m_applicationName; ///< The application name

    QString m_appIconName;     ///< The icon name

    QString m_sizeTotal;       ///< The total size of the operation

    QString m_sizeProcessed;   ///< The processed size at the moment(amount completed)

    QString m_speed;           ///< The current speed of the operation (human readable, example, "3Mb/s")

    int m_percent;             ///< The current percent completed of the job

    QString m_infoMessage;     ///< The information message to be shown

    uint m_error;              ///< The error code of the job, set when it's terminated

    QString m_errorText;       ///< The error message of the job, set when it's terminated

    QString m_totalUnit;       ///< The unit used in setTotalAmount

    qulonglong m_totalAmount;  ///< The amount used in setTotalAmount

    QString m_processUnit;     ///< The unit used in setProcessedAmount

    qulonglong m_processAmount; ///< The processed amount (setProcessedAmount)

    QHash<uint, QPair<QString, QString> > m_descFields;

    QVariant m_destUrl;

    QDBusObjectPath m_objectPath;

    /**
     * All for the client:
     *   <address name,  <objectPath, interface> >
     */
    QHash<QString, QPair<QString, QDBusAbstractInterface*> > m_objectPaths;

    const uint m_jobId;
    JobState m_state;          ///< Current state of this job

    // if the job has been terminated (but it could still be awaiting a pendingReply)
    bool m_isTerminated;

    // number of pending async calls to "requestView" that progresslistmodel has made.
    // 0 means that this job can be deleted and all is well. Else it has to kind of wait until it comes back.
    int m_currentPendingCalls;
};

#endif //JOBVIEW_H
