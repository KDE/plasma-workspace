/*
    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <vector>

#include <QEventLoopLocker>
#include <QProcess>

class SessionTrack : public QObject
{
    Q_OBJECT
public:
    SessionTrack(std::vector<QProcess *> &&processes);
    ~SessionTrack() override;

private:
    std::vector<QProcess *> m_processes;
    QEventLoopLocker m_lock;
};
