/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <ranges>

#include "sessiontrack.h"
#include "signalhandler.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <signal.h>

SessionTrack::SessionTrack(std::vector<QProcess *> &&processes)
    : m_processes(std::move(processes))
{
    SignalHandler::self()->addSignal(SIGTERM);
    connect(SignalHandler::self(), &SignalHandler::signalReceived, QCoreApplication::instance(), [](int signal) {
        if (signal == SIGTERM) {
            QCoreApplication::instance()->exit(0);
        }
    });

    for (auto process : m_processes) {
        connect(process, &QProcess::finished, this, [this] {
            std::erase(m_processes, sender());
        });
    }

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &SessionTrack::deleteLater);
}

SessionTrack::~SessionTrack()
{
    disconnect(this, nullptr, QCoreApplication::instance(), nullptr);

    // copy before the loop as we remove finished processes from the vector
    const std::vector<QProcess *> processesCopy = std::move(m_processes);
    for (auto process : processesCopy) {
        process->terminate();
    }

    for (QProcess *process : processesCopy | std::ranges::views::filter([](QProcess *p) {
                                 return p->state() == QProcess::Running;
                             })) {
        if (!process->waitForFinished(500)) {
            process->kill();
        }
    }
    qDeleteAll(processesCopy);
}
