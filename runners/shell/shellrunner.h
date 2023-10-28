/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KRunner/Action>
#include <QIcon>
#include <optional>

class ShellRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    ShellRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    std::optional<QString> parseShellCommand(const QString &query, QStringList &envs);
    const KRunner::Actions m_actionList;
    const QIcon m_matchIcon;
};
