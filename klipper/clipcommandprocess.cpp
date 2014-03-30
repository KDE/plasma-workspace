/*
    Copyright 2009 Esben Mose Hansen <kde@mosehansen.dk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include "clipcommandprocess.h"

#include <KCharMacroExpander>

#include "history.h"
#include "historystringitem.h"
#include "urlgrabber.h"

ClipCommandProcess::ClipCommandProcess(const ClipAction& action, const ClipCommand& command, const QString& clip, History* history, const HistoryItem* original_item) :
    KProcess(),
    m_history(history),
    m_historyItem(original_item),
    m_newhistoryItem()
{
    QHash<QChar,QString> map;
    map.insert( 's', clip );

    // support %u, %U (indicates url param(s)) and %f, %F (file param(s))
    map.insert( 'u', clip );
    map.insert( 'U', clip );
    map.insert( 'f', clip );
    map.insert( 'F', clip );

    const QStringList matches = action.regExpMatches();
    // support only %0 and the first 9 matches...
    const int numMatches = qMin(10, matches.count());
    for ( int i = 0; i < numMatches; ++i ) {
        map.insert( QChar( '0' + i ), matches.at( i ) );
    }

    setOutputChannelMode(OnlyStdoutChannel);
    setShellCommand(KMacroExpander::expandMacrosShellQuote( command.command, map ).trimmed());

    connect(this, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(slotFinished(int,QProcess::ExitStatus)));
    if (command.output != ClipCommand::IGNORE) {
        connect(this, SIGNAL(readyRead()), SLOT(slotStdOutputAvailable()));
    }
    if (command.output != ClipCommand::REPLACE) {
        m_historyItem = 0L; // Don't replace
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
            m_history->insert(new HistoryStringItem(m_newhistoryItem));
        }
    }
    deleteLater();
}

void ClipCommandProcess::slotStdOutputAvailable()
{
    m_newhistoryItem.append(QString::fromLocal8Bit(this->readAllStandardOutput().data()));
}


#include "clipcommandprocess.moc"
