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
    void match(Plasma::RunnerContext *context);
    void openDevice(const QString &udi);

private:
    KFilePlacesModel m_places;
    QString m_pendingUdi;
};

class PlacesRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    PlacesRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~PlacesRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

Q_SIGNALS:
    void doMatch(Plasma::RunnerContext *context);

private:
    PlacesRunnerHelper *m_helper;
};
