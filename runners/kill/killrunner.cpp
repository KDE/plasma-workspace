/* Copyright 2009  Jan Gerrit Marker <jangerrit@weiler-marker.com>
 * Copyright 2020  Alexander Lohnau <alexander.lohnau@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "killrunner.h"

#include <QAction>
#include <QDebug>
#include <QIcon>

#include <KProcess>
#include <KAuth>
#include <KLocalizedString>

#include <processcore/processes.h>
#include <processcore/process.h>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(KillRunner, "plasma-runner-kill.json")

KillRunner::KillRunner(QObject *parent, const QVariantList &args)
        : Plasma::AbstractRunner(parent, args), m_processes(nullptr)
{
    setObjectName(QStringLiteral("Kill Runner"));

    addAction(QStringLiteral("SIGTERM"), QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Send SIGTERM"))->setData(15);
    addAction(QStringLiteral("SIGKILL"), QIcon::fromTheme(QStringLiteral("process-stop")), i18n("Send SIGKILL"))->setData(9);
    m_actionList = {action(QStringLiteral("SIGTERM")), action(QStringLiteral("SIGKILL"))};

    connect(this, &Plasma::AbstractRunner::prepare, this, &KillRunner::prep);
    connect(this, &Plasma::AbstractRunner::teardown, this, &KillRunner::cleanup);

    m_delayedCleanupTimer.setInterval(50);
    m_delayedCleanupTimer.setSingleShot(true);
    connect(&m_delayedCleanupTimer, &QTimer::timeout, this, &KillRunner::cleanup);
}

KillRunner::~KillRunner() = default;


void KillRunner::reloadConfiguration()
{
    KConfigGroup grp = config();
    m_triggerWord.clear();
    if (grp.readEntry(CONFIG_USE_TRIGGERWORD, true)) {
        m_triggerWord = grp.readEntry(CONFIG_TRIGGERWORD, i18n("kill")) + QLatin1Char(' ');
    }
    m_hasTrigger = !m_triggerWord.isEmpty();

    m_sorting = (Sort) grp.readEntry(CONFIG_SORTING, static_cast<int>(Sort::NONE));
    QList<Plasma::RunnerSyntax> syntaxes;
    syntaxes << Plasma::RunnerSyntax(m_triggerWord + QStringLiteral(":q:"),
                                     i18n("Terminate running applications whose names match the query."));
    setSyntaxes(syntaxes);
}

void KillRunner::prep()
{
    m_delayedCleanupTimer.stop();
}

void KillRunner::cleanup()
{
    if (!m_processes) {
        return;
    }

    if (m_prepLock.tryLockForWrite()) {
        delete m_processes;
        m_processes = nullptr;

        m_prepLock.unlock();
    } else {
        m_delayedCleanupTimer.stop();
    }
}

void KillRunner::match(Plasma::RunnerContext &context)
{
    QString term = context.query();
    if (m_hasTrigger && !term.startsWith(m_triggerWord, Qt::CaseInsensitive)) {
        return;
    }

    m_prepLock.lockForRead();
    if (!m_processes) {
        m_prepLock.unlock();
        m_prepLock.lockForWrite();
        if (!m_processes) {
            suspendMatching(true);
            m_processes = new KSysGuard::Processes();
            m_processes->updateAllProcesses();
            suspendMatching(false);
        }
    }
    m_prepLock.unlock();

    term = term.right(term.length() - m_triggerWord.length());

    if (term.length() < 2)  {
        return;
    }

    QList<Plasma::QueryMatch> matches;
    const QList<KSysGuard::Process *> processlist = m_processes->getAllProcesses();
    for (const KSysGuard::Process *process : processlist) {
        if (!context.isValid()) {
            return;
        }
        const QString name = process->name();
        if (!name.contains(term, Qt::CaseInsensitive)) {
            continue;
        }

        const quint64 pid = process->pid();
        Plasma::QueryMatch match(this);
        match.setText(i18n("Terminate %1", name));
        match.setSubtext(i18n("Process ID: %1", QString::number(pid)));
        match.setIconName(QStringLiteral("application-exit"));
        match.setData(pid);
        match.setId(name);

        // Set the relevance
        switch (m_sorting) {
        case Sort::CPU:
            match.setRelevance((process->userUsage() + process->sysUsage()) / 100);
            break;
        case Sort::CPUI:
            match.setRelevance(1 - (process->userUsage() + process->sysUsage()) / 100);
            break;
        case Sort::NONE:
            match.setRelevance(name.compare(term, Qt::CaseInsensitive) == 0 ? 1 : 9);
            break;
        }

        matches << match;
    }

    context.addMatches(matches);
}

void KillRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    const quint64 pid = match.data().toUInt();

    int signal;
    if (match.selectedAction()) {
        signal = match.selectedAction()->data().toInt();
    } else {
        signal = 9; //default: SIGKILL
    }

    const QStringList args = {QStringLiteral("-%1").arg(signal), QString::number(pid)};
    int returnCode = KProcess::execute(QStringLiteral("kill"), args);
    if (returnCode == 0) {
        return;
    }

    KAuth::Action killAction = QStringLiteral("org.kde.ksysguard.processlisthelper.sendsignal");
    killAction.setHelperId(QStringLiteral("org.kde.ksysguard.processlisthelper"));
    killAction.addArgument(QStringLiteral("pid0"), pid);
    killAction.addArgument(QStringLiteral("pidcount"), 1);
    killAction.addArgument(QStringLiteral("signal"), signal);
    killAction.execute();
}

QList<QAction*> KillRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    return m_actionList;
}

#include "killrunner.moc"
