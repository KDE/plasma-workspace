/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "killrunner.h"

#include <QAction>
#include <QDebug>
#include <QIcon>

#include <KAuth>
#include <KLocalizedString>
#include <KProcess>

#include <processcore/process.h>
#include <processcore/processes.h>

K_PLUGIN_CLASS_WITH_JSON(KillRunner, "plasma-runner-kill.json")

KillRunner::KillRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
    , m_processes(nullptr)
{
    setObjectName(QStringLiteral("Kill Runner"));

    auto *sigterm = new QAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Send SIGTERM"), this);
    sigterm->setData(15);
    auto *sigkill = new QAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18n("Send SIGKILL"), this);
    sigkill->setData(9);
    m_actionList = {sigterm, sigkill};

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

    m_sorting = (Sort)grp.readEntry(CONFIG_SORTING, static_cast<int>(Sort::NONE));
    QList<Plasma::RunnerSyntax> syntaxes;
    syntaxes << Plasma::RunnerSyntax(m_triggerWord + QStringLiteral(":q:"), i18n("Terminate running applications whose names match the query."));
    setSyntaxes(syntaxes);
    if (m_hasTrigger) {
        setTriggerWords({m_triggerWord});
        setMinLetterCount(minLetterCount() + 2); // Requires two characters after trigger word
    } else {
        setMinLetterCount(2);
        setMatchRegex(QRegularExpression());
    }
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
        match.setActions(m_actionList);

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
        signal = 9; // default: SIGKILL
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

#include "killrunner.moc"
