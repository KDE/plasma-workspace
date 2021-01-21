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

#ifndef CLIPCOMMANDPROCESS_H
#define CLIPCOMMANDPROCESS_H

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

#endif // CLIPCOMMANDPROCESS_H
