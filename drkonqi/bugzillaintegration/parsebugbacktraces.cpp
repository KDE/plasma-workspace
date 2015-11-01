/*******************************************************************
* parsebugbacktraces.cpp
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

#include "parsebugbacktraces.h"

#include "parser/backtraceparser.h"

typedef QList<BacktraceLine>::const_iterator BacktraceConstIterator;

BacktraceConstIterator findCrashStackFrame(BacktraceConstIterator it, BacktraceConstIterator itEnd)
{
    BacktraceConstIterator result = itEnd;

    //find the beginning of the crash
    for ( ; it != itEnd; ++it) {
        if (it->type() == BacktraceLine::KCrash) {
            result = it;
            break;
        }
    }

    //find the beginning of the stack frame
    for (it = result; it != itEnd; ++it) {
        if (it->type() == BacktraceLine::StackFrame) {
            result = it;
            break;
        }
    }

    return result;
}

//TODO improve this stuff, it is just a HACK
ParseBugBacktraces::DuplicateRating rating(BacktraceConstIterator it, BacktraceConstIterator itEnd, BacktraceConstIterator it2, BacktraceConstIterator itEnd2)
{
    int matches = 0;
    int lines = 0;

    it = findCrashStackFrame(it, itEnd);
    it2 = findCrashStackFrame(it2, itEnd2);

    while (it != itEnd && it2 != itEnd2) {
        if (it->type() == BacktraceLine::StackFrame && it2->type() == BacktraceLine::StackFrame) {
            ++lines;
            if (it->frameNumber() == it2->frameNumber() && it->functionName() == it2->functionName()) {
                ++matches;
            }
            ++it;
            ++it2;
            continue;
        }

        //if iters do not point to emptylines or a stackframe increase them
        if (it->type() != BacktraceLine::StackFrame && it->type() != BacktraceLine::EmptyLine) {
            ++it;
            continue;
        }
        if (it2->type() != BacktraceLine::StackFrame && it2->type() != BacktraceLine::EmptyLine) {
            ++it2;
            continue;
        }

        //one bt is shorter than the other
        if (it->type() == BacktraceLine::StackFrame && it2->type() == BacktraceLine::EmptyLine) {
            ++lines;
            ++it;
            continue;
        }
        if (it2->type() == BacktraceLine::StackFrame && it->type() == BacktraceLine::EmptyLine) {
            ++lines;
            ++it2;
            continue;
        }

        if (it->type() == BacktraceLine::EmptyLine && it2->type() == BacktraceLine::EmptyLine) {
            //done
            break;
        }
    }

    if (!lines) {
        return ParseBugBacktraces::NoDuplicate;
    }

    const int rating = matches * 100 / lines;
    if (rating == 100) {
        return ParseBugBacktraces::PerfectDuplicate;
    } else if (rating >= 90) {
        return ParseBugBacktraces::MostLikelyDuplicate;
    } else if (rating >= 60) {
        return ParseBugBacktraces::MaybeDuplicate;
    } else {
        return ParseBugBacktraces::NoDuplicate;
    }

    return ParseBugBacktraces::NoDuplicate;
}

ParseBugBacktraces::ParseBugBacktraces(const BugReport &bug, QObject *parent)
  : QObject(parent),
    m_bug(bug)
{
    m_parser = BacktraceParser::newParser(QStringLiteral("gdb"), this);
    m_parser->connectToGenerator(this);
}

void ParseBugBacktraces::parse()
{
    parse(m_bug.description());

    QStringList comments = m_bug.comments();
    foreach (const QString &comment, comments) {
        parse(comment);
    }
}

void ParseBugBacktraces::parse(const QString &comment)
{
    emit starting();

    int start = 0;
    int end = -1;
    do {
        start = end + 1;
        end = comment.indexOf('\n', start);
        emit newLine(comment.mid(start, (end != -1 ? end - start + 1 : end)));
    } while (end != -1);

    //accepts anything as backtrace, the start of the backtrace is searched later anyway
    m_backtraces << m_parser->parsedBacktraceLines();
}

ParseBugBacktraces::DuplicateRating ParseBugBacktraces::findDuplicate(const QList<BacktraceLine> &backtrace)
{
    if (m_backtraces.isEmpty() || backtrace.isEmpty()) {
        return NoDuplicate;
    }

    DuplicateRating bestRating = NoDuplicate;
    DuplicateRating currentRating = NoDuplicate;

    QList<QList<BacktraceLine> >::const_iterator itBts;
    QList<QList<BacktraceLine> >::const_iterator itEndBts = m_backtraces.constEnd();
    for (itBts = m_backtraces.constBegin(); itBts != itEndBts; ++itBts) {
        currentRating = rating(backtrace.constBegin(), backtrace.constEnd(), itBts->constBegin(), itBts->constEnd());
        if (currentRating < bestRating) {
            bestRating = currentRating;
        }

        if (bestRating == PerfectDuplicate) {
            return bestRating;
        }
    }

    return bestRating;
}


