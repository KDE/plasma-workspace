/*
 *   Copyright © 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KUISERVERENGINE_H
#define KUISERVERENGINE_H

#include <QDBusObjectPath>
#include <QBasicTimer>
#include <QTimer>

#include <Plasma/DataContainer>
#include <Plasma/DataEngine>

class JobView;

namespace Plasma
{
    class Service;
} // namespace Plasma

class KuiserverEngine : public Plasma::DataEngine
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.JobViewServer")

public:
    KuiserverEngine(QObject* parent, const QVariantList& args);
    ~KuiserverEngine() override;

    void init();

    QDBusObjectPath requestView(const QString &appName, const QString &appIconName,
                                int capabilities);
    Plasma::Service* serviceForSource(const QString& source) override;

private Q_SLOTS:
    void processPendingJobs();

private:
    QTimer m_pendingJobsTimer;
    QList<JobView *> m_pendingJobs;
};

class JobView : public Plasma::DataContainer
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.JobViewV2")

public:
    enum State {
                 UnknownState = -1,
                 Running = 0,
                 Suspended = 1,
                 Stopped = 2
               };

    JobView(QObject *parent = 0);
    ~JobView() override;

    uint jobId() const;
    JobView::State state();

    void setTotalAmount(qlonglong amount, const QString &unit);
    QString totalAmountSize() const;
    QString totalAmountFiles() const;

    void setProcessedAmount(qlonglong amount, const QString &unit);

    void setSpeed(qlonglong bytesPerSecond);
    QString speedString() const;

    void setInfoMessage(const QString &infoMessage);
    QString infoMessage() const;

    bool setDescriptionField(uint number, const QString &name, const QString &value);
    void clearDescriptionField(uint number);

    void setAppName(const QString &appName);
    void setAppIconName(const QString &appIconName);
    void setCapabilities(int capabilities);
    void setPercent(uint percent);
    void setSuspended(bool suspended);
    void setError(uint errorCode);

    //vestigal, required to implement this dbus interface
    void setDestUrl(const QDBusVariant &destUrl);

    void terminate(const QString &errorMessage);

    QDBusObjectPath objectPath() const;

    void requestStateChange(State state);

public Q_SLOTS:
    void finished();

Q_SIGNALS:
    void suspendRequested();
    void resumeRequested();
    void cancelRequested();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void scheduleUpdate();
    void updateEta();
    int unitId(const QString &unit);

    QDBusObjectPath m_objectPath;
    QBasicTimer m_updateTimer;

    uint m_capabilities;
    uint m_percent;
    uint m_jobId;

    // for ETA calculation we cache these values
    qlonglong m_speed;
    qlonglong m_totalBytes;
    qlonglong m_processedBytes;

    State m_state;

    QMap<QString, int> m_unitMap;
    int m_bytesUnitId;
    int m_unitId;

    static uint s_jobId;
};

#endif
