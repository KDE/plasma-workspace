/*******************************************************************
* parsebugbacktraces.h
* Copyright 2011 Matthias Fuchs <mat69@gmx.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#ifndef PARSE_BUG_BACKTRACES_H
#define PARSE_BUG_BACKTRACES_H

#include "parser/backtraceline.h"
#include "bugzillalib.h"

class BacktraceParser;

/**
 * Parses a Bugreport to find all the backtraces listed there
 * NOTE it assumes that the backtraces provided were created
 * by gdb
 */
class ParseBugBacktraces : QObject
{
    Q_OBJECT
    public:
        explicit ParseBugBacktraces(const BugReport &bug, QObject *parent = 0);

        void parse();

        enum DuplicateRating {
            PerfectDuplicate,//functionnames and stackframe numer match
            MostLikelyDuplicate,//functionnames and stackframe numer match >=90%
            MaybeDuplicate,//functionnames and stackframe numer match >=60%
            NoDuplicate//functionnames and stackframe numer match <60%
        };

        DuplicateRating findDuplicate(const QList<BacktraceLine> &backtrace);

    Q_SIGNALS:
        void starting();
        void newLine(const QString &line);

    private:
        void parse(const QString &comment);

    private:
        BacktraceParser *m_parser;
        const BugReport m_bug;
        QList<QList<BacktraceLine> > m_backtraces;
};

#endif
