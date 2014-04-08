/*******************************************************************
* reportassistantpage.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "reportassistantpage.h"
#include "reportinterface.h"

ReportAssistantPage::ReportAssistantPage(ReportAssistantDialog * parent)
    : QWidget(parent), m_assistant(parent)
{
}

bool ReportAssistantPage::isComplete()
{
    return true;
}

bool ReportAssistantPage::showNextPage()
{
    return true;
}

ReportInterface * ReportAssistantPage::reportInterface() const
{
    return m_assistant->reportInterface();
}

BugzillaManager * ReportAssistantPage::bugzillaManager() const
{
    return reportInterface()->bugzillaManager();
}

ReportAssistantDialog * ReportAssistantPage::assistant() const
{
    return m_assistant;
}

void ReportAssistantPage::emitCompleteChanged()
{
    emit completeChanged(this, isComplete());
}
