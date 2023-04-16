/*
    SPDX-FileCopyrightText: 2008 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <kfileplacesmodel.h>
#include <krunner/abstractrunner.h>

class PlacesRunner;

class PlacesRunnerHelper : public QObject
{
    Q_OBJECT

public:
    explicit PlacesRunnerHelper(PlacesRunner *runner);

public Q_SLOTS:
    void match(KRunner::RunnerContext *context);
    void openDevice(const QString &udi);

private:
    KFilePlacesModel m_places;
    QString m_pendingUdi;
};

class PlacesRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    PlacesRunner(QObject *parent, const KPluginMetaData &metaData);
    ~PlacesRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

Q_SIGNALS:
    void doMatch(KRunner::RunnerContext *context);

private:
    PlacesRunnerHelper *m_helper;
};
