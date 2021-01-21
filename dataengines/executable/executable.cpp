/*
 *   Copyright (C) 2007, 2008 Petri Damsten <damu@iki.fi>
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

#include "executable.h"
#include <QDebug>
ExecutableContainer::ExecutableContainer(const QString &command, QObject *parent)
    : Plasma::DataContainer(parent)
    , m_process(nullptr)
{
    setObjectName(command);
    connect(this, &Plasma::DataContainer::updateRequested, this, &ExecutableContainer::exec);
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
    if (!m_process) {
        m_process = new KProcess();
        connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
        m_process->setOutputChannelMode(KProcess::SeparateChannels);
        m_process->setShellCommand(objectName());
    }

    if (m_process->state() == QProcess::NotRunning) {
        m_process->start();
    } else {
        qDebug() << "Process" << objectName() << "already running. Pid:" << m_process->pid();
    }
}

ExecutableEngine::ExecutableEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    setMinimumPollingInterval(1000);
}

bool ExecutableEngine::sourceRequestEvent(const QString &source)
{
    addSource(new ExecutableContainer(source, this));
    return true;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(executable, ExecutableEngine, "plasma-dataengine-executable.json")

#include "executable.moc"
