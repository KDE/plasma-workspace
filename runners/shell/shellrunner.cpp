/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *   Copyright (C) 2020-2021 Alexander Lohnau <alexander.lonau@gmx.de>
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
#include <KShell>
#include <KToolInvocation>
#include <QAction>
#include <QRegularExpression>
#include <QStandardPaths>

#include <KIO/CommandLauncherJob>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(ShellRunner, "plasma-runner-shell.json")

ShellRunner::ShellRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("Command"));
    // The results from the services runner are preferred, consequently we set a low priority
    setPriority(AbstractRunner::LowestPriority);
    // If the runner is not authorized we can suspend it
    bool enabled = KAuthorized::authorize(QStringLiteral("run_command")) && KAuthorized::authorize(QStringLiteral("shell_access"));
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
    if (match.selectedAction()) {
        const QVariantList data = match.data().toList();
        KToolInvocation::invokeTerminal(data.at(0).toString(), data.at(1).toStringList());
        return;
    }

    auto *job = new KIO::CommandLauncherJob(context.query()); // The job can handle the env parameters
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
    job->start();
}

std::optional<QString> ShellRunner::parseShellCommand(const QString &query, QStringList &envs)
{
    const static QRegularExpression envRegex = QRegularExpression(QStringLiteral("^.+=.+$"));
    const QStringList split = KShell::splitArgs(query);
    for (const auto &entry : split) {
        const QString executablePath = QStandardPaths::findExecutable(KShell::tildeExpand(entry));
        if (!executablePath.isEmpty()) {
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

#include "shellrunner.moc"
