/*******************************************************************
* duplicatefinderjob.cpp
* Copyright 2011    Matthias Fuchs <mat69@gmx.net>
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

#include "duplicatefinderjob.h"

#include <QDebug>

#include "backtracegenerator.h"
#include "parser/backtraceparser.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "parsebugbacktraces.h"

DuplicateFinderJob::DuplicateFinderJob(const QList<int> &bugIds, BugzillaManager *manager, QObject *parent)
  : KJob(parent),
    m_manager(manager),
    m_bugIds(bugIds)
{
    qDebug() << "Possible duplicates:" << m_bugIds;
    connect(m_manager, &BugzillaManager::bugReportFetched, this, &DuplicateFinderJob::slotBugReportFetched);
    connect(m_manager, &BugzillaManager::bugReportError, this, &DuplicateFinderJob::slotBugReportError);
}

DuplicateFinderJob::~DuplicateFinderJob()
{
}

void DuplicateFinderJob::start()
{
    analyzeNextBug();
}

DuplicateFinderJob::Result DuplicateFinderJob::result() const
{
    return m_result;
}

void DuplicateFinderJob::analyzeNextBug()
{
    if (m_bugIds.isEmpty()) {
        emitResult();
        return;
    }

    const int bugId = m_bugIds.takeFirst();
    qDebug() << "Fetching:" << bugId;
    m_manager->fetchBugReport(bugId, this);
}

void DuplicateFinderJob::fetchBug(const QString &bugId)
{
    bool ok;
    const int num = bugId.toInt(&ok);
    if (ok) {
        qDebug() << "Fetching:" << bugId;
        m_manager->fetchBugReport(num, this);
    } else {
        qDebug() << "Bug id not valid:" << bugId;
        analyzeNextBug();
    }
}

void DuplicateFinderJob::slotBugReportFetched(const BugReport &bug, QObject *owner)
{
    if (this != owner) {
        return;
    }

    ParseBugBacktraces parse(bug, this);
    parse.parse();

    BacktraceGenerator *btGenerator = DrKonqi::debuggerManager()->backtraceGenerator();
    const ParseBugBacktraces::DuplicateRating rating = parse.findDuplicate(btGenerator->parser()->parsedBacktraceLines());
    qDebug() << "Duplicate rating:" << rating;

    //TODO handle more cases here
    if (rating != ParseBugBacktraces::PerfectDuplicate) {
        qDebug() << "Bug" << bug.bugNumber() << "most likely not a duplicate:" << rating;
        analyzeNextBug();
        return;
    }

    //The Bug is a duplicate, now find out the status and resolution of the existing report
    if (bug.resolutionValue() == BugReport::Duplicate) {
        qDebug() << "Found duplicate is a duplicate itself.";
        if (!m_result.duplicate) {
            m_result.duplicate = bug.bugNumberAsInt();
        }
        fetchBug(bug.markedAsDuplicateOf());
    } else if ((bug.statusValue() == BugReport::UnknownStatus) || (bug.resolutionValue() == BugReport::UnknownResolution)) {
        qDebug() << "Either the status or the resolution is unknown.";
        qDebug() << "Status \"" << bug.bugStatus() << "\" known:" << (bug.statusValue() != BugReport::UnknownStatus);
        qDebug() << "Resolution \"" << bug.resolution() << "\" known:" << (bug.resolutionValue() != BugReport::UnknownResolution);
        analyzeNextBug();
    } else {
        if (!m_result.duplicate) {
            m_result.duplicate = bug.bugNumberAsInt();
        }
        m_result.parentDuplicate = bug.bugNumberAsInt();
        m_result.status = bug.statusValue();
        m_result.resolution = bug.resolutionValue();
        qDebug() << "Found duplicate information (id/status/resolution):" << bug.bugNumber() << bug.bugStatus() << bug.resolution();
        emitResult();
    }
}

void DuplicateFinderJob::slotBugReportError(const QString &message, QObject *owner)
{
    if (this != owner) {
        return;
    }
    qDebug() << "Error fetching bug:" << message;
    analyzeNextBug();
}
