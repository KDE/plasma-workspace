/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020-2021 Alexander Lohnau <alexander.lonau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "shellrunner.h"

#include <fstream>
#include <iostream>

#include <KAuthorized>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KShell>
#include <KTerminalLauncherJob>
#include <QAction>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>

#include <KIO/CommandLauncherJob>
#include <kterminallauncherjob.h>

K_PLUGIN_CLASS_WITH_JSON(ShellRunner, "plasma-runner-shell.json")

ShellRunner::ShellRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("Command"));
    // The results from the services runner are preferred, consequently we set a low priority
    setPriority(AbstractRunner::LowestPriority);
    // If the runner is not authorized we can suspend it
    bool enabled = KAuthorized::authorize(QStringLiteral("run_command")) && KAuthorized::authorize(KAuthorized::SHELL_ACCESS);
    suspendMatching(!enabled);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds commands that match :q:, using common shell syntax")));
    m_actionList = {new QAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18n("Run in Terminal Window"), this)};
    m_matchIcon = QIcon::fromTheme(QStringLiteral("system-run"));
}

ShellRunner::~ShellRunner()
{
}

void ShellRunner::match(Plasma::RunnerContext &context)
{
    QStringList envs;
    std::optional<QString> parsingResult = parseShellCommand(context.query(), envs);
    if (parsingResult.has_value()) {
        const QString command = parsingResult.value();
        Plasma::QueryMatch match(this);
        match.setId(QStringLiteral("exec://") + command);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setIcon(m_matchIcon);
        match.setText(i18n("Run %1", context.query()));
        match.setData(QVariantList({command, envs}));
        match.setRelevance(0.7);
        match.setActions(m_actionList);
        context.addMatch(match);
    }
}

void ShellRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    const QVariantList data = match.data().toList();
    const QStringList list = data.at(1).toStringList();
    const QString command = data.at(0).toString();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto &str : list) {
        const int pos = str.indexOf('=');
        env.insert(str.left(pos), str.mid(pos + 1));
    }

    if (match.selectedAction()) {
        auto job = new KTerminalLauncherJob(command);
        job->setProcessEnvironment(env);
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();
    } else {
        auto job = new KIO::CommandLauncherJob(command);
        job->setProcessEnvironment(env);
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();
    }
}

std::optional<QString> ShellRunner::parseShellCommand(const QString &query, QStringList &envs)
{
    const static QRegularExpression envRegex = QRegularExpression(QStringLiteral("^.+=.+$"));
    const QStringList split = KShell::splitArgs(query);
    for (const auto &entry : split) {
        const QString aliasExpansion = findAlias(entry);
        const QString executablePath = QStandardPaths::findExecutable(KShell::tildeExpand(entry));
        if (!aliasExpansion.isEmpty()) {
            return parseShellCommand(aliasExpansion + query.mid(query.indexOf(entry) + entry.length()), envs);
        } else if (!executablePath.isEmpty()) {
            QStringList executableParts{executablePath};
            executableParts << split.mid(split.indexOf(entry) + 1);
            return KShell::joinArgs(executableParts);
        } else if (envRegex.match(entry).hasMatch()) {
            envs.append(entry);
        } else {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

QString ShellRunner::findAlias(const QString &entry)
{
    QString expansion;
    std::string l;
    std::ifstream shellConfigFile("/home/natalie/.zshrc");
    if (shellConfigFile.is_open()) {
        while (getline(shellConfigFile, l)) {
            QString line = l.c_str();
            if (line.startsWith("alias " + entry + "=")) {
                expansion = line.mid(8 + entry.length(), line.length() - entry.length() - 9);
                break;
            }
        }
        shellConfigFile.close();
    }
    return expansion;
}

#include "shellrunner.moc"
