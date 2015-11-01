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

#include "jobviewadaptor.h"
#include "jobviewserveradaptor.h"
#include "kuiserverengine.h"
#include "jobcontrol.h"

#include <QDBusConnection>

#include <KJob>
#include <KFormat>
#include <klocalizedstring.h>

#include <Plasma/DataEngine>


uint JobView::s_jobId = 0;

static const int UPDATE_INTERVAL = 100;

JobView::JobView(QObject* parent)
    : Plasma::DataContainer(parent),
      m_capabilities(-1),
      m_percent(0),
      m_speed(0),
      m_totalBytes(0),
      m_processedBytes(0),
      m_state(UnknownState),
      m_bytesUnitId(-1),
      m_unitId(0)
{
    m_jobId = ++s_jobId;
    setObjectName(QStringLiteral("Job %1").arg(s_jobId));

    new JobViewV2Adaptor(this);

    m_objectPath.setPath(QStringLiteral("/DataEngine/applicationjobs/JobView_%1").arg(m_jobId));
    QDBusConnection::sessionBus().registerObject(m_objectPath.path(), this);

    setSuspended(false);
}

JobView::~JobView()
{
    QDBusConnection::sessionBus().unregisterObject(m_objectPath.path(), QDBusConnection::UnregisterTree);
}

uint JobView::jobId() const
{
    return m_jobId;
}

void JobView::scheduleUpdate()
{
    if (!m_updateTimer.isActive()) {
        m_updateTimer.start(UPDATE_INTERVAL, this);
    }
}

void JobView::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_updateTimer.timerId()) {
        m_updateTimer.stop();
        checkForUpdate();

        if (m_state == Stopped) {
            emit becameUnused(objectName());
        }
    } else {
        Plasma::DataContainer::timerEvent(event);
    }
}

void JobView::terminate(const QString &errorMessage)
{
    setData(QStringLiteral("errorText"), errorMessage);
    QTimer::singleShot(0, this, &JobView::finished);
}

void JobView::finished()
{
    if (m_state != Stopped) {
        m_state = Stopped;
        setData(QStringLiteral("state"), "stopped");
        setData(QStringLiteral("speed"), QVariant());
        setData(QStringLiteral("numericSpeed"), QVariant());
        scheduleUpdate();
    }
}

JobView::State JobView::state()
{
    return m_state;
}

void JobView::setSuspended(bool suspended)
{
    if (suspended) {
        if (m_state != Suspended) {
            m_state = Suspended;
            setData(QStringLiteral("state"), "suspended");
            setData(QStringLiteral("speed"), QVariant());
            setData(QStringLiteral("numericSpeed"), QVariant());
            scheduleUpdate();
        }
    } else if (m_state != Running) {
        m_state = Running;
        setData(QStringLiteral("state"), "running");
        setData(QStringLiteral("speed"), speedString());
        setData(QStringLiteral("numericSpeed"), m_speed);
        scheduleUpdate();
    }
}

void JobView::setError(uint errorCode)
{
    setData(QStringLiteral("error"), errorCode);
}

int JobView::unitId(const QString &unit)
{
    int id = 0;
    if (m_unitMap.contains(unit)) {
        id = m_unitMap.value(unit);
    } else {
        id = m_unitId;
        setData(QStringLiteral("totalUnit%1").arg(id), unit);
        setData(QStringLiteral("totalAmount%1").arg(id), 0);
        setData(QStringLiteral("processedUnit%1").arg(id), unit);
        setData(QStringLiteral("processedAmount%1").arg(id), 0);
        m_unitMap.insert(unit, m_unitId);

        if (unit == QLatin1String("bytes")) {
            m_bytesUnitId = id;
        }

        ++m_unitId;
        scheduleUpdate();
    }

    return id;
}

void JobView::updateEta()
{
    if (m_speed < 1) {
        setData(QStringLiteral("eta"), 0);
        return;
    }

    if (m_totalBytes < 1) {
        setData(QStringLiteral("eta"), 0);
        return;
    }

    const qlonglong remaining = 1000 * (m_totalBytes - m_processedBytes);
    setData(QStringLiteral("eta"), remaining / m_speed);
}

void JobView::setTotalAmount(qlonglong amount, const QString &unit)
{
    const int id = unitId(unit);
    const QString amountString = QStringLiteral("totalAmount%1").arg(id);
    const qlonglong prevTotal = data().value(amountString).toLongLong();
    if (prevTotal != amount) {
        if (id == m_bytesUnitId) {
            m_totalBytes = amount;
            updateEta();
        }

        setData(amountString, amount);
        scheduleUpdate();
    }
}

void JobView::setProcessedAmount(qlonglong amount, const QString &unit)
{
    const int id = unitId(unit);
    const QString processedString = QStringLiteral("processedAmount%1").arg(id);
    const qlonglong prevTotal = data().value(processedString).toLongLong();
    if (prevTotal != amount) {
        if (id == m_bytesUnitId) {
            m_processedBytes = amount;
            if (!m_totalBytes && m_processedBytes && (m_percent != 0)) {
                m_totalBytes = m_processedBytes / m_percent * 100;
                const QString totalAmountString = QStringLiteral("totalAmount%1").arg(id);
                setData(totalAmountString, m_totalBytes);
            }
            updateEta();
        }

        setData(processedString, amount);
        scheduleUpdate();
    }
}

void JobView::setDestUrl(const QDBusVariant & destUrl)
{
    Q_UNUSED(destUrl);
}

void JobView::setPercent(uint percent)
{
    if (m_percent != percent) {
        m_percent = percent;
        setData(QStringLiteral("percentage"), m_percent);
        scheduleUpdate();
    }
}

void JobView::setSpeed(qlonglong bytesPerSecond)
{
    if (m_speed != bytesPerSecond) {
        m_speed = bytesPerSecond;
        setData(QStringLiteral("speed"), speedString());
        setData(QStringLiteral("numericSpeed"), m_speed);

        if (m_bytesUnitId > -1) {
            updateEta();
        }

        scheduleUpdate();
    }
}

QString JobView::speedString() const
{
    return i18nc("Bytes per second", "%1/s", KFormat().formatByteSize(m_speed));
}

void JobView::setInfoMessage(const QString &infoMessage)
{
    if (data().value(QStringLiteral("infoMessage")) != infoMessage) {
        setData(QStringLiteral("infoMessage"), infoMessage);
        scheduleUpdate();
    }
}

bool JobView::setDescriptionField(uint number, const QString &name, const QString &value)
{
    const QString labelString = QStringLiteral("label%1").arg(number);
    const QString labelNameString = QStringLiteral("labelName%1").arg(number);

    if (!data().contains(labelNameString) || data().value(labelString) != value) {
        setData(labelNameString, name);
        setData(labelString, value);
        scheduleUpdate();
    }
    return true;
}

void JobView::clearDescriptionField(uint number)
{
    const QString labelString = QStringLiteral("label%1").arg(number);
    const QString labelNameString = QStringLiteral("labelName%1").arg(number);

    setData(labelNameString, QVariant());
    setData(labelString, QVariant());
    scheduleUpdate();
}

void JobView::setAppName(const QString &appName)
{
    // don't need to update, this is only set once at creation
    setData(QStringLiteral("appName"), appName);
}

void JobView::setAppIconName(const QString &appIconName)
{
    // don't need to update, this is only set once at creation
    setData(QStringLiteral("appIconName"), appIconName);
}

void JobView::setCapabilities(int capabilities)
{
    if (m_capabilities != uint(capabilities)) {
        m_capabilities = capabilities;
        setData(QStringLiteral("suspendable"), m_capabilities & KJob::Suspendable);
        setData(QStringLiteral("killable"), m_capabilities & KJob::Killable);
        scheduleUpdate();
    }
}

QDBusObjectPath JobView::objectPath() const
{
    return m_objectPath;
}

void JobView::requestStateChange(State state)
{
    switch (state) {
        case Running:
            emit resumeRequested();
            break;
        case Suspended:
            emit suspendRequested();
            break;
        case Stopped:
            emit cancelRequested();
            break;
        default:
            break;
    }
}

KuiserverEngine::KuiserverEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args)
{
    new JobViewServerAdaptor(this);

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerObject(QLatin1String("/DataEngine/applicationjobs/JobWatcher"), this);

    setMinimumPollingInterval(500);

    m_pendingJobsTimer.setSingleShot(true);
    m_pendingJobsTimer.setInterval(500);
    connect(&m_pendingJobsTimer, &QTimer::timeout, this, &KuiserverEngine::processPendingJobs);
    init();
}

KuiserverEngine::~KuiserverEngine()
{
    QDBusConnection::sessionBus()
    .unregisterObject(QLatin1String("/DataEngine/applicationjobs/JobWatcher"), QDBusConnection::UnregisterTree);
    qDeleteAll(m_pendingJobs);
}

QDBusObjectPath KuiserverEngine::requestView(const QString &appName,
                                             const QString &appIconName, int capabilities)
{
    JobView *jobView = new JobView(this);
    jobView->setAppName(appName);
    jobView->setAppIconName(appIconName);
    jobView->setCapabilities(capabilities);
    connect(jobView, &Plasma::DataContainer::becameUnused, this, &KuiserverEngine::removeSource);

    m_pendingJobs << jobView;
    m_pendingJobsTimer.start();

    return jobView->objectPath();
}

void KuiserverEngine::processPendingJobs()
{
    foreach (JobView *jobView, m_pendingJobs) {
        if (jobView->state() == JobView::Stopped) {
            delete jobView;
        } else {
            addSource(jobView);
        }
    }

    m_pendingJobs.clear();
}

Plasma::Service* KuiserverEngine::serviceForSource(const QString& source)
{
    JobView *jobView = qobject_cast<JobView *>(containerForSource(source));
    if (jobView) {
        return new JobControl(this, jobView);
    } else {
        return DataEngine::serviceForSource(source);
    }
}

void KuiserverEngine::init()
{
    // register with the Job UI Serer to receive notifications of jobs becoming available
    QDBusInterface interface(QStringLiteral("org.kde.kuiserver"), QStringLiteral("/JobViewServer")/* object to connect to */,
                              QLatin1String("")/* use the default interface */, QDBusConnection::sessionBus(), this);
    interface.asyncCall(QLatin1String("registerService"), QDBusConnection::sessionBus().baseService(), "/DataEngine/applicationjobs/JobWatcher");
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(kuiserver, KuiserverEngine, "plasma-dataengine-applicationjobs.json")

#include "kuiserverengine.moc"
