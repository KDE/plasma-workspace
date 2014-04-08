/*******************************************************************
* reportassistantpage.h
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

#ifndef REPORTASSISTANTPAGE__H
#define REPORTASSISTANTPAGE__H

#include <QWidget>

#include "reportassistantdialog.h"

class BugzillaManager;

/** BASE interface which implements some signals, and
**  aboutTo(Show|Hide) functions (also reimplements QWizard behaviour) **/
class ReportAssistantPage: public QWidget
{
    Q_OBJECT

public:
    explicit ReportAssistantPage(ReportAssistantDialog * parent);

    /** Load the widget data if empty **/
    virtual void aboutToShow() {}
    /** Save the widget data **/
    virtual void aboutToHide() {}
    /** Tells the KAssistantDialog to enable the Next button **/
    virtual bool isComplete();

    /** Last time checks to see if you can turn the page **/
    virtual bool showNextPage();

    ReportInterface *reportInterface() const;
    BugzillaManager *bugzillaManager() const;
    ReportAssistantDialog * assistant() const;

public Q_SLOTS:
    void emitCompleteChanged();

Q_SIGNALS:
    /** Tells the KAssistantDialog that the isComplete function changed value **/
    void completeChanged(ReportAssistantPage*, bool);

private:
    ReportAssistantDialog * const m_assistant;
};

#endif
