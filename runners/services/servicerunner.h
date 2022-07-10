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

class ServiceRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    ServiceRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~ServiceRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;

protected:
    void setupMatch(const KService::Ptr &service, Plasma::QueryMatch &action);
};
