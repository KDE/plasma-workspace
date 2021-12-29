/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KService>

//#include <KRunner/AbstractRunner>
#include <krunner/abstractrunner.h>

/**
 * This class looks for matches in the set of .desktop files installed by
 * applications. This way the user can type exactly what they see in the
 * applications menu and have it start the appropriate app. Essentially anything
 * that KService knows about, this runner can launch
 */

class ServiceRunner : public AbstractRunner
{
    Q_OBJECT

public:
    ServiceRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~ServiceRunner() override;

    void match(RunnerContext &context) override;
    void run(const RunnerContext &context, const QueryMatch &match) override;

protected Q_SLOTS:
    QMimeData *mimeDataForMatch(const QueryMatch &match) override;

protected:
    void setupMatch(const KService::Ptr &service, QueryMatch &action);
};
