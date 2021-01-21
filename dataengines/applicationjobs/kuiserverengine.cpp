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

#include "kuiserverengine.h"
#include "jobcontrol.h"

#include <QUrl>

#include <KFormat>
#include <KJob>
#include <KLocalizedString>

#include "notifications.h"

#include <algorithm>

using namespace NotificationManager;

KuiserverEngine::KuiserverEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    init();
}

KuiserverEngine::~KuiserverEngine()
{
}

QString KuiserverEngine::sourceName(Job *job)
{
    return QStringLiteral("Job %1").arg(job->id());
}

uint KuiserverEngine::jobId(const QString &sourceName)
{
    return sourceName.midRef(4 /*length of Job + space*/).toUInt();
}

Plasma::Service *KuiserverEngine::serviceForSource(const QString &source)
{
    const uint id = jobId(source);
    if (!id) {
        return DataEngine::serviceForSource(source);
    }

    auto it = std::find_if(m_jobs.constBegin(), m_jobs.constBegin(), [&id](Job *job) {
        return job->id() == id;
    });

    if (it == m_jobs.constEnd()) {
        return DataEngine::serviceForSource(source);
    }

    return new JobControl(this, *it);
}

void KuiserverEngine::init()
{
    m_jobsModel = JobsModel::createJobsModel();
    // TODO see if this causes any issues when/if other processes are using applicationjobs engine, e.g. Latte Dock
    m_jobsModel->init();

    connect(m_jobsModel.data(), &Notifications::rowsInserted, this, [this](const QModelIndex &parent, int first, int last) {
        for (int i = first; i <= last; ++i) {
            const QModelIndex idx = m_jobsModel->index(first, 0, parent);
            Job *job = idx.data(Notifications::JobDetailsRole).value<Job *>();
            registerJob(job);
        }
    });

    connect(m_jobsModel.data(), &Notifications::rowsAboutToBeRemoved, this, [this](const QModelIndex &parent, int first, int last) {
        for (int i = first; i <= last; ++i) {
            const QModelIndex idx = m_jobsModel->index(first, 0, parent);
            Job *job = idx.data(Notifications::JobDetailsRole).value<Job *>();
            removeJob(job);
        }
    });
}

void KuiserverEngine::updateDescriptionField(Job *job, int number, QString (Job::*labelGetter)() const, QString (Job::*valueGetter)() const)
{
    const QString source = sourceName(job);
    const QString labelString = QStringLiteral("label%1").arg(number);
    const QString labelNameString = QStringLiteral("labelName%1").arg(number);
    const QString labelFileNameString = QStringLiteral("labelFileName%1").arg(number);

    const QString label = ((job)->*labelGetter)();
    const QString value = ((job)->*valueGetter)();

    if (label.isEmpty() && value.isEmpty()) {
        setData(source, labelString, QVariant());
        setData(source, labelNameString, QVariant());
        setData(source, labelFileNameString, QVariant());
    } else {
        setData(source, labelNameString, label);
        setData(source, labelString, value);

        const QUrl url = QUrl::fromUserInput(value, QString(), QUrl::AssumeLocalFile);
        setData(source, labelFileNameString, url.toString(QUrl::PreferLocalFile | QUrl::RemoveFragment | QUrl::RemoveQuery));
    }
    setData(source, labelString);
}

void KuiserverEngine::updateUnit(Job *job,
                                 int number,
                                 const QString &unit,
                                 qulonglong (NotificationManager::Job::*processedGetter)() const,
                                 qulonglong (NotificationManager::Job::*totalGetter)() const)
{
    const QString source = sourceName(job);

    setData(source, QStringLiteral("totalUnit%1").arg(number), unit);
    setData(source, QStringLiteral("totalAmount%1").arg(number), ((job)->*totalGetter)());
    setData(source, QStringLiteral("processedUnit%1").arg(number), unit);
    setData(source, QStringLiteral("processedAmount%1").arg(number), ((job)->*processedGetter)());
}

void KuiserverEngine::registerJob(Job *job)
{
    if (m_jobs.contains(job)) { // shouldn't really happen
        return;
    }

    const QString source = sourceName(job);

    setData(source, QStringLiteral("appName"), job->desktopEntry()); // job->applicationName());
    setData(source, QStringLiteral("appIconName"), job->applicationIconName());
    setData(source, QStringLiteral("suspendable"), job->suspendable());
    setData(source, QStringLiteral("killable"), job->killable());
    updateState(job);

    connect(job, &Job::stateChanged, this, [this, job] {
        updateState(job);
    });
    connect(job, &Job::speedChanged, this, [this, job] {
        updateEta(job);
    });

    connectJobField(job, &Job::summary, &Job::summaryChanged, QStringLiteral("infoMessage"));
    connectJobField(job, &Job::percentage, &Job::percentageChanged, QStringLiteral("percentage"));
    connectJobField(job, &Job::error, &Job::errorChanged, QStringLiteral("error"));
    connectJobField(job, &Job::errorText, &Job::errorTextChanged, QStringLiteral("errorText"));
    connectJobField(job, &Job::destUrl, &Job::destUrlChanged, QStringLiteral("destUrl"));

    static const struct {
        int number;
        QString (Job::*labelGetter)() const;
        void (Job::*labelSignal)();
        QString (Job::*valueGetter)() const;
        void (Job::*valueSignal)();
    } s_descriptionFields[] = {
        {0, &Job::descriptionLabel1, &Job::descriptionLabel1Changed, &Job::descriptionValue1, &Job::descriptionValue1Changed},
        {1, &Job::descriptionLabel2, &Job::descriptionLabel2Changed, &Job::descriptionValue2, &Job::descriptionValue2Changed},
    };

    for (auto fields : s_descriptionFields) {
        updateDescriptionField(job, fields.number, fields.labelGetter, fields.valueGetter);
        connect(job, fields.labelSignal, this, [=] {
            updateDescriptionField(job, fields.number, fields.labelGetter, fields.valueGetter);
        });
        connect(job, fields.valueSignal, this, [=] {
            updateDescriptionField(job, fields.number, fields.labelGetter, fields.valueGetter);
        });
    }

    static const struct {
        // Previously the dataengine counted units up but for simplicity a fixed number is assigned to each unit
        int number;
        QString unit;
        qulonglong (Job::*processedGetter)() const;
        void (Job::*processedSignal)();
        qulonglong (Job::*totalGetter)() const;
        void (Job::*totalSignal)();
    } s_unitsFields[] = {
        {0, QStringLiteral("bytes"), &Job::processedBytes, &Job::processedBytesChanged, &Job::totalBytes, &Job::totalBytesChanged},
        {1, QStringLiteral("files"), &Job::processedFiles, &Job::processedFilesChanged, &Job::totalFiles, &Job::totalFilesChanged},
        {2, QStringLiteral("dirs"), &Job::processedDirectories, &Job::processedDirectoriesChanged, &Job::totalDirectories, &Job::totalDirectoriesChanged}};

    for (auto fields : s_unitsFields) {
        updateUnit(job, fields.number, fields.unit, fields.processedGetter, fields.totalGetter);
        connect(job, fields.processedSignal, this, [=] {
            updateUnit(job, fields.number, fields.unit, fields.processedGetter, fields.totalGetter);
        });
        connect(job, fields.totalSignal, this, [=] {
            updateUnit(job, fields.number, fields.unit, fields.processedGetter, fields.totalGetter);
        });
    }

    m_jobs.append(job);
}

void KuiserverEngine::removeJob(Job *job)
{
    if (!job || !m_jobs.contains(job)) {
        return;
    }

    m_jobs.removeOne(job);

    const QString source = sourceName(job);
    removeSource(source);
}

QString KuiserverEngine::speedString(qulonglong speed)
{
    return i18nc("Bytes per second", "%1/s", KFormat().formatByteSize(speed));
}

void KuiserverEngine::updateState(Job *job)
{
    const QString source = sourceName(job);

    QString stateString;
    switch (job->state()) {
    case Notifications::JobStateRunning:
        stateString = QStringLiteral("running");
        updateSpeed(job);
        break;
    case Notifications::JobStateSuspended:
        stateString = QStringLiteral("suspended");
        setData(source, QStringLiteral("speed"), QVariant());
        setData(source, QStringLiteral("numericSpeed"), QVariant());
        break;
    case Notifications::JobStateStopped:
        stateString = QStringLiteral("stopped");
        break;
    }

    setData(source, QStringLiteral("state"), stateString);

    if (job->state() == Notifications::JobStateStopped) {
        removeJob(job);
    }
}

void KuiserverEngine::updateSpeed(Job *job)
{
    const QString source = sourceName(job);
    setData(source, QStringLiteral("speed"), speedString(job->speed()));
    setData(source, QStringLiteral("numericSpeed"), job->speed());
    updateEta(job);
}

void KuiserverEngine::updateEta(Job *job)
{
    const QString source = sourceName(job);

    if (job->speed() < 1 || job->totalBytes() < 1) {
        setData(source, QStringLiteral("eta"), 0);
        return;
    }

    const qlonglong remaining = 1000 * (job->totalBytes() - job->processedBytes());
    setData(source, QStringLiteral("eta"), remaining / job->speed());
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(kuiserver, KuiserverEngine, "plasma-dataengine-applicationjobs.json")

#include "kuiserverengine.moc"
