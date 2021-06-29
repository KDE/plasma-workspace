/*
    SPDX-FileCopyrightText: 2008 Rob Scheepmaker <r.scheepmaker@student.utwente.nl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QVector>

#include <Plasma/DataContainer>
#include <Plasma/DataEngine>

#include "jobsmodel.h"

namespace NotificationManager
{
class Job;
}

namespace Plasma
{
class Service;
} // namespace Plasma

class KuiserverEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    KuiserverEngine(QObject *parent, const QVariantList &args);
    ~KuiserverEngine() override;

    void init();

    Plasma::Service *serviceForSource(const QString &source) override;

    static QString sourceName(NotificationManager::Job *job);
    static uint jobId(const QString &sourceName);

private:
    template<typename T, typename signal>
    void connectJobField(NotificationManager::Job *job, T (NotificationManager::Job::*getter)() const, signal changeSignal, const QString &targetFieldName)
    {
        // Set value initially in case we missed the first change
        const QString source = sourceName(job);
        setData(source, targetFieldName, ((job)->*getter)());
        // and then listen for changes
        connect(job, changeSignal, this, [=] {
            setData(source, targetFieldName, ((job)->*getter)());
        });
    }

    void updateDescriptionField(NotificationManager::Job *job,
                                int number,
                                QString (NotificationManager::Job::*labelGetter)() const,
                                QString (NotificationManager::Job::*valueGetter)() const);

    void updateUnit(NotificationManager::Job *job,
                    int number,
                    const QString &unit,
                    qulonglong (NotificationManager::Job::*processedGetter)() const,
                    qulonglong (NotificationManager::Job::*totalGetter)() const);

    void registerJob(NotificationManager::Job *job);
    void removeJob(NotificationManager::Job *job);

    static QString speedString(qulonglong speed);

    void updateState(NotificationManager::Job *job);
    void updateSpeed(NotificationManager::Job *job);
    void updateEta(NotificationManager::Job *job);

    NotificationManager::JobsModel::Ptr m_jobsModel;

    QVector<NotificationManager::Job *> m_jobs;
};
