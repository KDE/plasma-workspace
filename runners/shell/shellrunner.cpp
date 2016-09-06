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
#include <QDebug>
#include <KLocalizedString>
#include <KRun>
#include <KShell>
#include <QStandardPaths>
#include <KToolInvocation>

#include <Plasma/Theme>

#include "shell_config.h"

K_EXPORT_PLASMA_RUNNER(shell, ShellRunner)

ShellRunner::ShellRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    setObjectName( QLatin1String("Command" ));
    setPriority(AbstractRunner::HighestPriority);
    setHasRunOptions(true);
    m_enabled = KAuthorized::authorize(QStringLiteral("run_command")) && KAuthorized::authorize(QStringLiteral("shell_access"));
    setIgnoredTypes(Plasma::RunnerContext::Directory | Plasma::RunnerContext::File |
                    Plasma::RunnerContext::NetworkLocation | Plasma::RunnerContext::UnknownType |
                    Plasma::RunnerContext::Help);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds commands that match :q:, using common shell syntax")));
}

ShellRunner::~ShellRunner()
{
}

void ShellRunner::match(Plasma::RunnerContext &context)
{
    if (!m_enabled) {
        return;
    }

    if (context.type() == Plasma::RunnerContext::Executable ||
        context.type() == Plasma::RunnerContext::ShellCommand)  {
        const QString term = context.query();
        Plasma::QueryMatch match(this);
        match.setId(term);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setIconName(QStringLiteral("system-run"));
        match.setText(i18n("Run %1", term));
        match.setRelevance(0.7);
        context.addMatch(match);
    }
}

void ShellRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(match);

    if (m_enabled) {
        if (match.selectedAction() && match.selectedAction() == actions().value(QStringLiteral("runInTerminal"))) {
            KToolInvocation::invokeTerminal(context.query());
        } else {
            KRun::runCommand(context.query(), NULL);
        }
    }
}

QList<QAction *> ShellRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    const QString runInTerminalId = QStringLiteral("runInTerminal");

    QAction *terminalAction = action(runInTerminalId);
    if (!terminalAction) {
        terminalAction = addAction(runInTerminalId, QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18n("Run in Terminal Window"));
    };

    return {terminalAction};
}

#include "shellrunner.moc"
