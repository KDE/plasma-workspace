/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <QIcon>
#include <optional>

/**
 * This class runs programs using the literal name of the binary, much as one
 * would use at a shell prompt.
 */
class ShellRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    ShellRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~ShellRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

private:
    QStringList parseShellCommands(const QString &query, QStringList &envs);
    std::optional<QString> parseShellCommand(QString &line, QStringList &envs);
    QString findAlias(const QString &entry);
    QList<QAction *> m_actionList;
    QIcon m_matchIcon;
};
