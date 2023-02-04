/*
    SPDX-FileCopyrightText: 2009 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KProcess>
#include <memory>

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
                       std::shared_ptr<const HistoryItem> original_item = nullptr);
public Q_SLOTS:
    void slotStdOutputAvailable();
    void slotFinished(int exitCode, QProcess::ExitStatus newState);

private:
    History *m_history;
    std::shared_ptr<const HistoryItem> m_historyItem;
    QString m_newhistoryItem;
};
