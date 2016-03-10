/*******************************************************************
* reportassistantpages_base.h
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

#ifndef REPORTASSISTANTPAGES__BASE__H
#define REPORTASSISTANTPAGES__BASE__H

#include <QtCore/QPointer>
#include <QtWidgets/QDialog>

#include "reportassistantdialog.h"
#include "reportassistantpage.h"

#include "ui_assistantpage_introduction.h"
#include "ui_assistantpage_bugawareness.h"
#include "ui_assistantpage_conclusions.h"
#include "ui_assistantpage_conclusions_dialog.h"

class BacktraceWidget;

/** Introduction page **/
class IntroductionPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit IntroductionPage(ReportAssistantDialog *);

private:
    Ui::AssistantPageIntroduction   ui;
};

/** Backtrace page **/
class CrashInformationPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit CrashInformationPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;
    bool isComplete() override;
    bool showNextPage() override;

private:
    BacktraceWidget *        m_backtraceWidget;
};

/** Bug Awareness page **/
class BugAwarenessPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugAwarenessPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

private Q_SLOTS:
    void showApplicationDetailsExamples();

    void updateCheckBoxes();

private:
    Ui::AssistantPageBugAwareness   ui;
};

/** Conclusions page **/
class ConclusionPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit ConclusionPage(ReportAssistantDialog *);

    void aboutToShow() override;
    void aboutToHide() override;

    bool isComplete() override;

private Q_SLOTS:
    void finishClicked();

    void openReportInformation();

private:
    Ui::AssistantPageConclusions            ui;

    QPointer<QDialog>                       m_infoDialog;

    bool                                    m_isBKO;
    bool                                    m_needToReport;

Q_SIGNALS:
    void finished(bool);
};

class ReportInformationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReportInformationDialog(const QString & reportText);
    ~ReportInformationDialog() override;

private Q_SLOTS:
    void saveReport();

private:
    Ui::AssistantPageConclusionsDialog ui;
};

#endif
