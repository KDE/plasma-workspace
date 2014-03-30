/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
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

#include <QWidget>
#include <QPushButton>
#include <QIcon>

#include <KAuthorized>
#include <QDebug>
#ifdef Q_OS_UNIX
#include <SuProcess>
#endif
#include <KGlobal>
#include <KLocale>
#include <KRun>
#include <KShell>
#include <KStandardDirs>
#include <KToolInvocation>

#include <Plasma/Theme>

#include "shell_config.h"

K_EXPORT_PLASMA_RUNNER(shell, ShellRunner)

ShellRunner::ShellRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args),
      m_inTerminal(false),
      m_asOtherUser(false)
{
    setObjectName( QLatin1String("Command" ));
    setPriority(AbstractRunner::HighestPriority);
    setHasRunOptions(true);
    m_enabled = KAuthorized::authorizeKAction("run_command") && KAuthorized::authorizeKAction("shell_access");
    setIgnoredTypes(Plasma::RunnerContext::Directory | Plasma::RunnerContext::File |
                    Plasma::RunnerContext::NetworkLocation | Plasma::RunnerContext::UnknownType |
                    Plasma::RunnerContext::Help);

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Finds commands that match :q:, using common shell syntax")));
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
        match.setIcon(QIcon::fromTheme("system-run"));
        match.setText(i18n("Run %1", term));
        match.setRelevance(0.7);
        context.addMatch(match);
    }
}

void ShellRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(match);

    // filter match's id to remove runner's name
    // as this is the command we want to run

    if (m_enabled) {
#ifdef Q_OS_UNIX
        //qDebug() << m_asOtherUser << m_username << m_password;
        if (m_asOtherUser && !m_username.isEmpty()) {
            //TODO: provide some user feedback on failure
            QString exec;
            QString args;
            if (m_inTerminal) {
                // we have to reimplement this from KToolInvocation because we need to use KDESu
                KConfigGroup confGroup( KGlobal::config(), "General" );
                exec = confGroup.readPathEntry("TerminalApplication", "konsole");
                if (!exec.isEmpty()) {
                    if (exec == "konsole") {
                        args += " --noclose";
                    } else if (exec == "xterm") {
                        args += " -hold";
                    }

                    args += " -e " + context.query();
                }
            } else {
                const QStringList commandLine = KShell::splitArgs(context.query(), KShell::TildeExpand);
                if (!commandLine.isEmpty()) {
                    exec = commandLine.at(0);
                }

                args = context.query().right(context.query().size() - commandLine.at(0).length());
            }

            if (!exec.isEmpty()) {
                exec = KStandardDirs::findExe(exec);
                exec.append(args);
                if (!exec.isEmpty()) {
                    KDESu::SuProcess client(m_username.toLocal8Bit(), exec.toLocal8Bit());
                    const QByteArray password = m_password.toLocal8Bit();
                    //TODO handle errors like wrong password via KNotifications in 4.7
                    client.exec(password.constData());
                }
            }
        } else
#endif
        if (m_inTerminal) {
            KToolInvocation::invokeTerminal(context.query());
        } else {
            KRun::runCommand(context.query(), NULL);
        }
    }

    // reset for the next run!
    m_inTerminal = false;
    m_asOtherUser = false;
    m_username.clear();
    m_password.clear();
}

void ShellRunner::createRunOptions(QWidget *parent)
{
    //TODO: for multiple runners?
    //TODO: sync palette on theme changes
    ShellConfig *configWidget = new ShellConfig(config(), parent);

    QPalette pal = configWidget->palette();
    Plasma::Theme *theme = new Plasma::Theme(this);
    pal.setColor(QPalette::Normal, QPalette::Window, theme->color(Plasma::Theme::BackgroundColor));
    pal.setColor(QPalette::Normal, QPalette::WindowText, theme->color(Plasma::Theme::TextColor));
    configWidget->setPalette(pal);

    connect(configWidget->m_ui.cbRunAsOther, SIGNAL(clicked(bool)), this, SLOT(setRunAsOtherUser(bool)));
    connect(configWidget->m_ui.cbRunInTerminal, SIGNAL(clicked(bool)), this, SLOT(setRunInTerminal(bool)));
    connect(configWidget->m_ui.leUsername, SIGNAL(textChanged(QString)), this, SLOT(setUsername(QString)));
    connect(configWidget->m_ui.lePassword, SIGNAL(textChanged(QString)), this, SLOT(setPassword(QString)));
}

void ShellRunner::setRunAsOtherUser(bool asOtherUser)
{
    m_asOtherUser = asOtherUser;
}

void ShellRunner::setRunInTerminal(bool runInTerminal)
{
    m_inTerminal = runInTerminal;
}

void ShellRunner::setUsername(const QString &username)
{
    m_username = username;
}

void ShellRunner::setPassword(const QString &password)
{
    m_password = password;
}

#include "shellrunner.moc"
