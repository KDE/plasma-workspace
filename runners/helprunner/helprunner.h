/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KRunner/RunnerManager>

#include <KPluginMetaData>
#include <QAction>
#include <QObject>

using namespace Plasma;

class HelpRunner : public AbstractRunner
{
public:
    HelpRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args);

    void match(RunnerContext &context) override;
    void run(const RunnerContext &context, const QueryMatch &match) override;

private:
    RunnerManager *m_manager;
    QList<QAction *> m_actionList;
};
