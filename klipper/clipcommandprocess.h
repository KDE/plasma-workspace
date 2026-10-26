/*
    SPDX-FileCopyrightText: 2009 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KProcess>
#include <memory>

#include "klipper_export.h"

class ClipAction;
struct ClipCommand;
class HistoryItem;
class HistoryModel;
class SystemClipboard;

class KLIPPER_EXPORT ClipCommandProcess : public KProcess
{
    Q_OBJECT
public:
    ClipCommandProcess(const QStringList &actionCapturedTexts, const ClipCommand &command, const QString &clip, const QString &uuid);
public Q_SLOTS:
    void slotStdOutputAvailable();
    void slotFinished(int exitCode, QProcess::ExitStatus newState);

private:
    std::shared_ptr<HistoryModel> m_model;
    std::shared_ptr<SystemClipboard> m_clip;
    QString m_newhistoryItem;
    QString m_historyItemUuid;
};
