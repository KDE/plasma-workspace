/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "shellrunner.h"

#include <KAuthorized>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KToolInvocation>
#include <KShell>
#include <QRegularExpression>
#include <QStandardPaths>

#include <KIO/CommandLauncherJob>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(ShellRunner, "plasma-runner-shell.json")

ShellRunner::ShellRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args),
    m_bashCompatibleStrings({})
{
    setObjectName(QStringLiteral("Command"));
    setPriority(AbstractRunner::HighestPriority);
    // If the runner is not authorized we can suspend it
    bool enabled = KAuthorized::authorize(QStringLiteral("run_command")) && KAuthorized::authorize(QStringLiteral("shell_access"));
    suspendMatching(!enabled);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds commands that match :q:, using common shell syntax")));
    m_actionList = {addAction(QStringLiteral("runInTerminal"),
                            QIcon::fromTheme(QStringLiteral("utilities-terminal")),
                            i18n("Run in Terminal Window"))};
    m_matchIcon = QIcon::fromTheme(QStringLiteral("system-run"));
    // We only want to read the bash aliases/functions if the configured shell is bash
    if (QFileInfo(qEnvironmentVariable("SHELL")).fileName() == QLatin1String("bash")) {
        suspendMatching(true);
        fetchBashAliasesAndFunctions();
        qWarning() << Q_FUNC_INFO;
    }
}

ShellRunner::~ShellRunner()
{
}

void ShellRunner::match(Plasma::RunnerContext &context)
{
    qWarning() << Q_FUNC_INFO;
    QStringList envs;
    QString command = context.query();
    if (parseShellCommand(context.query(), envs, command)) {
        const QString term = context.query();
        Plasma::QueryMatch match(this);
        match.setId(term);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setIcon(m_matchIcon);
        match.setText(i18n("Run %1", term));
        match.setData(QVariantList({command, envs}));
        match.setRelevance(0.7);
        match.setActions(m_actionList);
        context.addMatch(match);
    }
}

void ShellRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    if (match.selectedAction()) {
        const QVariantList data = match.data().toList();
        KToolInvocation::invokeTerminal(data.at(0).toString(), data.at(1).toStringList());
        return;
    }

    auto *job = new KIO::CommandLauncherJob(context.query()); // The job can handle the env parameters
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    job->start();
}

bool ShellRunner::parseShellCommand(const QString &query, QStringList &envs, QString &command)
{
    const static QRegularExpression envRegex = QRegularExpression(QStringLiteral("^.+=.+$"));
    const QStringList split = KShell::splitArgs(query);
    for (const auto &entry : split) {
        if (!QStandardPaths::findExecutable(KShell::tildeExpand(entry)).isEmpty()) {
            command = KShell::joinArgs(split.mid(split.indexOf(entry)));
            return true;
        } else if (m_bashCompatibleStrings.contains(entry)) {
            command = QStringLiteral("bash -i -c %1").arg(KShell::joinArgs(split.mid(split.indexOf(entry))));
        } else if (envRegex.match(entry).hasMatch()) {
            envs.append(entry);
        } else {
            return false;
        }
    }
    return false;
}

void ShellRunner::fetchBashAliasesAndFunctions()
{
    QProcess *aliasesProcess = new QProcess();
    QProcess *functionProcess = new QProcess();
    int finishedProcesses = 0;
    auto processDone = [this, aliasesProcess, functionProcess, &finishedProcesses] () {
        ++finishedProcesses;
        if (finishedProcesses == 2) {
            suspendMatching(false);
            aliasesProcess->deleteLater();
            functionProcess->deleteLater();
        }
    };

    // Bash aliases
    aliasesProcess->start(QStringLiteral("bash"), QStringList{"-c", "-i", "alias"});
    connect(aliasesProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        [this, aliasesProcess, processDone](int, QProcess::ExitStatus) {
        const QStringList aliasOutputLines = QString(aliasesProcess->readAll()).split('\n');
        for (const auto &alias : aliasOutputLines) {
            QString prefixRemoved = QString(alias).remove(0, strlen("alias "));
            m_bashCompatibleStrings << prefixRemoved.left(prefixRemoved.indexOf('='));
        }
        processDone();
    });

    // Bash functions
    functionProcess->start(QStringLiteral("bash"), QStringList{"-c", "-i", "declare -F"});
    connect(functionProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        [this, functionProcess, processDone](int, QProcess::ExitStatus) {
        const QStringList functionOutputLines = QString(functionProcess->readAll()).split('\n');
        for (const auto &function : functionOutputLines) {
            m_bashCompatibleStrings << QString(function).remove(0, strlen("declare -f "));
        }
        processDone();
    });
}

#include "shellrunner.moc"
