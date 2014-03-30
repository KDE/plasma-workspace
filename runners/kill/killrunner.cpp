/* Copyright 2009  Jan Gerrit Marker <jangerrit@weiler-marker.com>
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
#include <KIcon>
#include <KProcess>
#include <KUser>
#include <kauth.h>

#include "processcore/processes.h"
#include "processcore/process.h"

#include "killrunner_config.h"

KillRunner::KillRunner(QObject *parent, const QVariantList& args)
        : Plasma::AbstractRunner(parent, args),
          m_processes(0)
{
    Q_UNUSED(args);
    setObjectName( QLatin1String("Kill Runner") );
    reloadConfiguration();

    connect(this, SIGNAL(prepare()), this, SLOT(prep()));
    connect(this, SIGNAL(teardown()), this, SLOT(cleanup()));

    m_delayedCleanupTimer.setInterval(50);
    m_delayedCleanupTimer.setSingleShot(true);
    connect(&m_delayedCleanupTimer, SIGNAL(timeout()), this, SLOT(cleanup()));
}

KillRunner::~KillRunner()
{
}


void KillRunner::reloadConfiguration()
{
    KConfigGroup grp = config();
    m_triggerWord.clear();
    if (grp.readEntry(CONFIG_USE_TRIGGERWORD, true)) {
        m_triggerWord = grp.readEntry(CONFIG_TRIGGERWORD, i18n("kill")) + ' ';
    }

    m_sorting = (KillRunnerConfig::Sort) grp.readEntry(CONFIG_SORTING, 0);
    QList<Plasma::RunnerSyntax> syntaxes;
    syntaxes << Plasma::RunnerSyntax(m_triggerWord + ":q:",
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
        m_processes = 0;

        m_prepLock.unlock();
    } else {
        m_delayedCleanupTimer.stop();
    }
}

void KillRunner::match(Plasma::RunnerContext &context)
{
    QString term = context.query();
    const bool hasTrigger = !m_triggerWord.isEmpty();
    if (hasTrigger && !term.startsWith(m_triggerWord, Qt::CaseInsensitive)) {
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
    foreach (const KSysGuard::Process *process, processlist) {
        if (!context.isValid()) {
            return;
        }

        const QString name = process->name;
        if (!name.contains(term, Qt::CaseInsensitive)) {
            //Process doesn't match the search term
            continue;
        }

        const quint64 pid = process->pid;
        const qlonglong uid = process->uid;
        const QString user = getUserName(uid);

        QVariantList data;
        data << pid << user;

        Plasma::QueryMatch match(this);
        match.setText(i18n("Terminate %1", name));
        match.setSubtext(i18n("Process ID: %1\nRunning as user: %2", QString::number(pid), user));
        match.setIcon(KIcon("application-exit"));
        match.setData(data);
        match.setId(name);

        // Set the relevance
        switch (m_sorting) {
        case KillRunnerConfig::CPU:
            match.setRelevance((process->userUsage + process->sysUsage) / 100);
            break;
        case KillRunnerConfig::CPUI:
            match.setRelevance(1 - (process->userUsage + process->sysUsage) / 100);
            break;
        case KillRunnerConfig::NONE:
            match.setRelevance(name.compare(term, Qt::CaseInsensitive) == 0 ? 1 : 9);
            break;
        }

        matches << match;
    }

    qDebug() << "match count is" << matches.count();
    context.addMatches(term, matches);
}

void KillRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    QVariantList data = match.data().value<QVariantList>();
    quint64 pid = data[0].toUInt();
    QString user = data[1].toString();

    int signal;
    if (match.selectedAction() != NULL) {
        signal = match.selectedAction()->data().toInt();
    } else {
        signal = 9; //default: SIGKILL
    }

    QStringList args;
    args << QString("-%1").arg(signal) << QString("%1").arg(pid);
    KProcess *process = new KProcess(this);
    int returnCode = process->execute("kill", args);

    if (returnCode == 0)
    {
        return;
    }

    KAuth::Action killAction = QString("org.kde.ksysguard.processlisthelper.sendsignal");
    killAction.setHelperID("org.kde.ksysguard.processlisthelper");
    killAction.addArgument("pid0", pid);
    killAction.addArgument("pidcount", 1);
    killAction.addArgument("signal", signal);
    killAction.execute();
}

QList<QAction*> KillRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    QList<QAction*> ret;

    if (!action("SIGTERM")) {
        (addAction("SIGTERM", KIcon("application-exit"), i18n("Send SIGTERM")))->setData(15);
        (addAction("SIGKILL", KIcon("process-stop"), i18n("Send SIGKILL")))->setData(9);
    }
    ret << action("SIGTERM") << action("SIGKILL");
    return ret;
}

QString KillRunner::getUserName(qlonglong uid)
{
    KUser user(uid);
    if (user.isValid()) {
        return user.loginName();
    }
    qDebug() << QString("No user with UID %1 was found").arg(uid);
    return "root";//No user with UID uid was found, so root is used
}

#include "killrunner.moc"
