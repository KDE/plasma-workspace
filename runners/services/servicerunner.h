/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KService>

#include <KActivities/Consumer>
#include <KActivities/Stats/ResultWatcher>
#include <QMap>

using namespace KActivities::Stats;

/**
 * This class looks for matches in the set of .desktop files installed by
 * applications. This way the user can type exactly what they see in the
 * applications menu and have it start the appropriate app. Essentially anything
 * that KService knows about, this runner can launch
 */

class ServiceRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    ServiceRunner(QObject *parent, const KPluginMetaData &metaData);
    ~ServiceRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

    struct ActivityFavourite {
        QStringList linkedActivities;
        bool isGlobal;
    };
    QMap<QString, ActivityFavourite> m_favourites;

protected:
    void setupMatch(const KService::Ptr &service, KRunner::QueryMatch &action);

private:
    void processActivitiesResults(const ResultSet &results);
    const Query m_kactivitiesQuery;
    const ResultWatcher m_kactivitiesWatcher;
    const KActivities::Consumer m_activitiesConsuer;
};
