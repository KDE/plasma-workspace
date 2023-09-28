/*
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KActivities/Stats/ResultModel>
#include <KRunner/AbstractRunner>
#include <KRunner/Action>

using namespace KActivities::Stats;

class RecentDocuments : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    RecentDocuments(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private:
    const KRunner::Actions m_actions;
    QString m_lastLoadedQuery;
    std::unique_ptr<ResultModel> m_resultsModel;
    const int m_maxResults = 20;
    const int m_minLetterCount = 3;
};
