/*
    SPDX-FileCopyrightText: 2007, 2008 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "executable.h"

#include <KLocalizedString>
#include <KNotification>

using namespace Qt::StringLiterals;

ExecutableContainer::ExecutableContainer(const QString &command, QObject *parent)
    : Plasma5Support::DataContainer(parent)
{
    setObjectName(command);
    connect(this, &Plasma5Support::DataContainer::updateRequested, this, &ExecutableContainer::aboutToExec);
    aboutToExec();
}

ExecutableContainer::~ExecutableContainer()
{
    if (m_process) {
        m_process->disconnect(this);
        delete m_process;
    }
}

void ExecutableContainer::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setData(QStringLiteral("exit code"), exitCode);
    setData(QStringLiteral("exit status"), exitStatus);
    setData(QStringLiteral("stdout"), QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    setData(QStringLiteral("stderr"), QString::fromLocal8Bit(m_process->readAllStandardError()));
    checkForUpdate();
}

void ExecutableContainer::aboutToExec()
{
    if (qEnvironmentVariableIsSet("PLASMA_DATAENGINE_DISABLE_COMMANDS")) {
        return;
    }

    if (qEnvironmentVariableIntValue("PLASMA_DATAENGINE_ALWAYS_ALLOW_COMMANDS") == 1) {
        exec();
        return;
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        return;
    }

    if (m_notification) [[unlikely]] {
        m_notification->close(); // Auto-delete is on
    }

    m_notification = KNotification::event(u"aboutToRunCommand"_s, i18nc("@title", "A widget is trying to run a command"), objectName(), u"dialog-warning"_s);
    m_notification->setComponentName(u"plasma_dataengine_executable"_s);

    KNotificationAction *allowAction = m_notification->addAction(i18nc("@action:button", "Allow"));
    connect(allowAction, &KNotificationAction::activated, this, &ExecutableContainer::exec);
    KNotificationAction *denyAction = m_notification->addAction(i18nc("@action:button", "Deny"));
    connect(denyAction, &KNotificationAction::activated, this, &ExecutableContainer::deny);

    m_notification->sendEvent();
}

void ExecutableContainer::exec()
{
    if (!m_process) {
        m_process = new KProcess();
        connect(m_process, &QProcess::finished, this, &ExecutableContainer::finished);
        m_process->setOutputChannelMode(KProcess::SeparateChannels);
        m_process->setShellCommand(objectName());
    }

    m_process->start();
}

void ExecutableContainer::deny()
{
    if (m_process) {
        delete m_process;
        m_process = nullptr;
    }
}

ExecutableEngine::ExecutableEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
{
    setMinimumPollingInterval(1000);
}

bool ExecutableEngine::sourceRequestEvent(const QString &source)
{
    addSource(new ExecutableContainer(source, this));
    return true;
}

K_PLUGIN_CLASS_WITH_JSON(ExecutableEngine, "plasma-dataengine-executable.json")

#include "executable.moc"
