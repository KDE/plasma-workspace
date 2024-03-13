/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QProcess>
#include <QSignalSpy>

namespace MprisInterface
{
QProcess *startPlayer(QObject *parent)
{
    auto process = new QProcess(parent);
    process->setProgram(QStringLiteral("python3"));
    process->setArguments({QFINDTESTDATA(QStringLiteral("../../appiumtests/applets/mediacontrollertest/mediaplayer.py")), //
                           QFINDTESTDATA(QStringLiteral("../../appiumtests/applets/mediacontrollertest/player_b.json"))});
    QSignalSpy startedSpy(process, &QProcess::started);
    QSignalSpy readyReadSpy(process, &QProcess::readyReadStandardOutput);
    process->setReadChannel(QProcess::StandardOutput);
    process->start(QIODeviceBase::ReadOnly);
    if (process->state() != QProcess::Running) {
        Q_ASSERT(startedSpy.wait());
    }

    bool registered = false;
    for (int i = 0; i < 10; ++i) {
        if (process->isReadable() && process->readAllStandardError().contains("MPRIS registered")) {
            registered = true;
            break;
        }
        readyReadSpy.wait();
    }

    qDebug() << process->state();
    qDebug() << process->readAllStandardError();
    Q_ASSERT(registered);

    return process;
}

void stopPlayer(QProcess *process)
{
    QSignalSpy finishedSpy(process, &QProcess::finished);
    process->terminate();
    if (process->state() == QProcess::Running) {
        QVERIFY(finishedSpy.wait());
    }
    delete process;
}

}