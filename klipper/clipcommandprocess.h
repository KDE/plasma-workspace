/*
    SPDX-FileCopyrightText: 2009 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KProcess>
#include <QSharedPointer>

class ClipAction;
class History;
struct ClipCommand;
class HistoryItem;

class ClipCommandProcess : public KProcess
{
    Q_OBJECT
public:
    ClipCommandProcess(const ClipAction &action,
                       const ClipCommand &command,
                       const QString &clip,
                       History *history = nullptr,
                       QSharedPointer<const HistoryItem> original_item = QSharedPointer<const HistoryItem>());
public Q_SLOTS:
    void slotStdOutputAvailable();
    void slotFinished(int exitCode, QProcess::ExitStatus newState);

private:
    History *m_history;
    QSharedPointer<const HistoryItem> m_historyItem;
    QString m_newhistoryItem;
};
