/*
    SPDX-FileCopyrightText: 2007 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "hddtemp.h"

#include <QTcpSocket>

#include <QTimerEvent>

#include <QDebug>

HddTemp::HddTemp(QObject *parent)
    : QObject(parent)
{
    updateData();
}

HddTemp::~HddTemp()
{
}

QStringList HddTemp::sources()
{
    updateData();
    return m_data.keys();
}

void HddTemp::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    m_cacheValid = false;
}

bool HddTemp::updateData()
{
    if (m_cacheValid) {
        return true;
    }

    if (m_failCount > 4) {
        return false;
    }

    QTcpSocket socket;
    QString data;

    socket.connectToHost(QStringLiteral("localhost"), 7634);
    if (socket.waitForConnected(500)) {
        while (data.length() < 1024) {
            if (!socket.waitForReadyRead(500)) {
                if (data.length() > 0) {
                    break;
                } else {
                    // qDebug() << socket.errorString();
                    return false;
                }
            }
            data += QString(socket.readAll());
        }
        socket.disconnectFromHost();
        // on success retry fail count
        m_failCount = 0;
    } else {
        m_failCount++;
        // qDebug() << socket.errorString();
        return false;
    }
    const QStringList list = data.split('|');
    int i = 1;
    m_data.clear();
    while (i + 4 < list.size()) {
        m_data[list[i]].append(list[i + 2]);
        m_data[list[i]].append(list[i + 3]);
        i += 5;
    }
    m_cacheValid = true;
    startTimer(0);

    return true;
}

QVariant HddTemp::data(const QString source, const DataType type) const
{
    return m_data[source][type];
}
