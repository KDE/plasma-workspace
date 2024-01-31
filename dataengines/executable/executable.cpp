/*
    SPDX-FileCopyrightText: 2007, 2008 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "executable.h"
#include <QDebug>
ExecutableContainer::ExecutableContainer(const QString &command, QObject *parent)
    : Plasma5Support::DataContainer(parent)
    , m_process(nullptr)
{
    setObjectName(command);
    connect(this, &Plasma5Support::DataContainer::updateRequested, this, &ExecutableContainer::exec);
    exec();
}

ExecutableContainer::~ExecutableContainer()
{
    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
    }
    delete m_process;
}

void ExecutableContainer::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setData(QStringLiteral("exit code"), exitCode);
    setData(QStringLiteral("exit status"), exitStatus);
    setData(QStringLiteral("stdout"), QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    setData(QStringLiteral("stderr"), QString::fromLocal8Bit(m_process->readAllStandardError()));
    checkForUpdate();
}

void ExecutableContainer::exec()
{
    if (qEnvironmentVariableIsSet("PLASMA_DATAENGINE_DISABLE_COMMANDS")) {
        return;
    }

    if (!m_process) {
        m_process = new KProcess();
        connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
        m_process->setOutputChannelMode(KProcess::SeparateChannels);
        m_process->setShellCommand(objectName());
    }

    if (m_process->state() == QProcess::NotRunning) {
        m_process->start();
    } else {
        qDebug() << "Process" << objectName() << "already running. Pid:" << m_process->processId();
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
