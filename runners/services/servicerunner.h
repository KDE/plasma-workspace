/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KService>

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

protected:
    void setupMatch(const KService::Ptr &service, KRunner::QueryMatch &action);
};
