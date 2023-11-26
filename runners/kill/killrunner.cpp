/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "killrunner.h"

#include <QAction>
#include <QDebug>
#include <QIcon>

#include <KAuth/Action>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KProcess>

#include <processcore/process.h>
#include <processcore/processes.h>

K_PLUGIN_CLASS_WITH_JSON(KillRunner, "plasma-runner-kill.json")

KillRunner::KillRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_actionList({
          KRunner::Action(QString::number(15), QStringLiteral("application-exit"), i18n("Send SIGTERM")),
          KRunner::Action(QString::number(9), QStringLiteral("process-stop"), i18n("Send SIGKILL")),
      })
    , m_processes(new KSysGuard::Processes(QString(), this))
{
    connect(this, &KRunner::AbstractRunner::prepare, m_processes, [this]() {
        m_needsRefresh = true;
    });
}

void KillRunner::reloadConfiguration()
{
    KConfigGroup grp = config();
    m_triggerWord.clear();
    if (grp.readEntry(CONFIG_USE_TRIGGERWORD, true)) {
        m_triggerWord = grp.readEntry(CONFIG_TRIGGERWORD, i18n("kill")) + QLatin1Char(' ');
    }
    m_hasTrigger = !m_triggerWord.isEmpty();

    m_sorting = (Sort)grp.readEntry(CONFIG_SORTING, static_cast<int>(Sort::NONE));
    QList<KRunner::RunnerSyntax> syntaxes;
    syntaxes << KRunner::RunnerSyntax(m_triggerWord + QStringLiteral(":q:"), i18n("Terminate running applications whose names match the query."));
    setSyntaxes(syntaxes);
    if (m_hasTrigger) {
        setTriggerWords({m_triggerWord});
        setMinLetterCount(minLetterCount() + 2); // Requires two characters after trigger word
    } else {
        setMinLetterCount(2);
        setMatchRegex(QRegularExpression());
    }
}

void KillRunner::match(KRunner::RunnerContext &context)
{
    // Only refresh the matches when we are matching. If we were to call it in the prepare slot, we would waste resources
    // because very likely the runner will not be used during the current match session
    if (m_needsRefresh) {
        m_processes->updateAllProcesses();
        if (!context.isValid()) {
            return;
        }
    }
    QString term = context.query();
    term = term.right(term.length() - m_triggerWord.length());

    QList<KRunner::QueryMatch> matches;
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
        KRunner::QueryMatch match(this);
        match.setText(i18n("Terminate %1", name));
        match.setSubtext(i18n("Process ID: %1", QString::number(pid)));
        match.setIconName(QStringLiteral("application-exit"));
        match.setData(pid);
        match.setId(name);
        match.setActions(m_actionList);

        // Set the relevance
        switch (m_sorting) {
        case Sort::CPU:
            match.setRelevance((process->userUsage() + process->sysUsage()) / 100.0);
            break;
        case Sort::CPUI:
            match.setRelevance(1.0 - (process->userUsage() + process->sysUsage()) / 100.0);
            break;
        case Sort::NONE:
            match.setRelevance(name.compare(term, Qt::CaseInsensitive) == 0 ? 1 : 9);
            break;
        }

        matches << match;
    }

    context.addMatches(matches);
}

void KillRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const quint64 pid = match.data().toUInt();
    const int signal = match.selectedAction() ? match.selectedAction().id().toInt() : 9; // default: SIGKILL

    int returnCode = KProcess::execute(QStringLiteral("kill"), {QStringLiteral("-%1").arg(signal), QString::number(pid)});
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

#include "killrunner.moc"
