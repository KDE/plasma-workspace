/*
    SPDX-FileCopyrightText: 2009 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipcommandprocess.h"

#include <KMacroExpander>

#include "history.h"
#include "historystringitem.h"
#include "urlgrabber.h"

ClipCommandProcess::ClipCommandProcess(const ClipAction &action,
                                       const ClipCommand &command,
                                       const QString &clip,
                                       History *history,
                                       HistoryItemConstPtr original_item)
    : KProcess()
    , m_history(history)
    , m_historyItem(original_item)
    , m_newhistoryItem()
{
    QHash<QChar, QString> map;
    map.insert(QLatin1Char('s'), clip);

    // support %u, %U (indicates url param(s)) and %f, %F (file param(s))
    map.insert(QLatin1Char('u'), clip);
    map.insert(QLatin1Char('U'), clip);
    map.insert(QLatin1Char('f'), clip);
    map.insert(QLatin1Char('F'), clip);

    const QStringList matches = action.actionCapturedTexts();
    // support only %0 and the first 9 matches...
    const int numMatches = qMin(10, matches.count());
    for (int i = 0; i < numMatches; ++i) {
        map.insert(QChar('0' + i), matches.at(i));
    }

    setOutputChannelMode(OnlyStdoutChannel);
    setShellCommand(KMacroExpander::expandMacrosShellQuote(command.command, map).trimmed());

    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotFinished(int, QProcess::ExitStatus)));
    if (command.output != ClipCommand::IGNORE) {
        connect(this, &QIODevice::readyRead, this, &ClipCommandProcess::slotStdOutputAvailable);
    }
    if (command.output != ClipCommand::REPLACE) {
        m_historyItem.clear();
    }
}

void ClipCommandProcess::slotFinished(int /*exitCode*/, QProcess::ExitStatus /*newState*/)
{
    if (m_history) {
        // If an history item was provided, remove it so that the new item can replace it
        if (m_historyItem) {
            m_history->remove(m_historyItem);
        }
        if (!m_newhistoryItem.isEmpty()) {
            m_history->insert(HistoryItemPtr(new HistoryStringItem(m_newhistoryItem)));
        }
    }
    deleteLater();
}

void ClipCommandProcess::slotStdOutputAvailable()
{
    m_newhistoryItem.append(QString::fromLocal8Bit(this->readAllStandardOutput()));
}
