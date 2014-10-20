/*******************************************************************
* reportassistantdialog.cpp
* Copyright 2009,2010    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "reportassistantdialog.h"

#include <QCloseEvent>

#include <KMessageBox>
#include <KLocalizedString>
#include <KConfigGroup>

#include "drkonqi.h"

#include "parser/backtraceparser.h"
#include "debuggermanager.h"
#include "backtracegenerator.h"

#include "crashedapplication.h"
#include "aboutbugreportingdialog.h"
#include "reportassistantpages_base.h"
#include "reportassistantpages_bugzilla.h"
#include "reportassistantpages_bugzilla_duplicates.h"
#include "reportinterface.h"

static const char KDE_BUGZILLA_DESCRIPTION[] = I18N_NOOP("the KDE Bug Tracking System");

ReportAssistantDialog::ReportAssistantDialog(QWidget * parent) :
        KAssistantDialog(parent),
        m_aboutBugReportingDialog(0),
        m_reportInterface(new ReportInterface(this)),
        m_canClose(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    //Set window properties
    setWindowTitle(i18nc("@title:window","Crash Reporting Assistant"));
    setWindowIcon(QIcon::fromTheme("tools-report-bug"));

    connect(this, &ReportAssistantDialog::currentPageChanged, this, &ReportAssistantDialog::currentPageChanged_slot);
    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &ReportAssistantDialog::showHelp);

    //Create the assistant pages

    //-Introduction Page
    KConfigGroup group(KSharedConfig::openConfig(), "ReportAssistant");
    const bool skipIntroduction = group.readEntry("SkipIntroduction", false);

    if (!skipIntroduction) {
        IntroductionPage * m_introduction = new IntroductionPage(this);

        KPageWidgetItem * m_introductionPage = new KPageWidgetItem(m_introduction,
                QLatin1String(PAGE_INTRODUCTION_ID));
        m_pageWidgetMap.insert(QLatin1String(PAGE_INTRODUCTION_ID),m_introductionPage);
        m_introductionPage->setHeader(i18nc("@title","Welcome to the Reporting Assistant"));
        m_introductionPage->setIcon(QIcon::fromTheme("tools-report-bug"));

        addPage(m_introductionPage);
    }

    //-Bug Awareness Page
    BugAwarenessPage * m_awareness = new BugAwarenessPage(this);
    connectSignals(m_awareness);

    KPageWidgetItem * m_awarenessPage = new KPageWidgetItem(m_awareness,
                                                                QLatin1String(PAGE_AWARENESS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_AWARENESS_ID),m_awarenessPage);
    m_awarenessPage->setHeader(i18nc("@title","What do you know about the crash?"));
    m_awarenessPage->setIcon(QIcon::fromTheme("checkbox"));

    //-Crash Information Page
    CrashInformationPage * m_backtrace = new CrashInformationPage(this);
    connectSignals(m_backtrace);

    KPageWidgetItem * m_backtracePage = new KPageWidgetItem(m_backtrace,
                                                        QLatin1String(PAGE_CRASHINFORMATION_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CRASHINFORMATION_ID),m_backtracePage);
    m_backtracePage->setHeader(i18nc("@title","Fetching the Backtrace (Automatic Crash Information)"));
    m_backtracePage->setIcon(QIcon::fromTheme("run-build"));

    //-Results Page
    ConclusionPage * m_conclusions = new ConclusionPage(this);
    connectSignals(m_conclusions);

    KPageWidgetItem * m_conclusionsPage = new KPageWidgetItem(m_conclusions,
                                                                QLatin1String(PAGE_CONCLUSIONS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_CONCLUSIONS_ID),m_conclusionsPage);
    m_conclusionsPage->setHeader(i18nc("@title","Results of the Analyzed Crash Details"));
    m_conclusionsPage->setIcon(QIcon::fromTheme("dialog-information"));
    connect(m_conclusions, &ConclusionPage::finished, this, &ReportAssistantDialog::assistantFinished);

    //-Bugzilla Login
    BugzillaLoginPage * m_bugzillaLogin =  new BugzillaLoginPage(this);
    connectSignals(m_bugzillaLogin);

    KPageWidgetItem * m_bugzillaLoginPage = new KPageWidgetItem(m_bugzillaLogin,
                                                                QLatin1String(PAGE_BZLOGIN_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZLOGIN_ID),m_bugzillaLoginPage);
    m_bugzillaLoginPage->setHeader(i18nc("@title", "Login into %1", i18n(KDE_BUGZILLA_DESCRIPTION)));
    m_bugzillaLoginPage->setIcon(QIcon::fromTheme("user-identity"));
    connect(m_bugzillaLogin, &BugzillaLoginPage::loggedTurnToNextPage, this, &ReportAssistantDialog::loginFinished);

    //-Bugzilla duplicates
    BugzillaDuplicatesPage * m_bugzillaDuplicates =  new BugzillaDuplicatesPage(this);
    connectSignals(m_bugzillaDuplicates);

    KPageWidgetItem * m_bugzillaDuplicatesPage = new KPageWidgetItem(m_bugzillaDuplicates,
                                                            QLatin1String(PAGE_BZDUPLICATES_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDUPLICATES_ID),m_bugzillaDuplicatesPage);
    m_bugzillaDuplicatesPage->setHeader(i18nc("@title","Look for Possible Duplicate Reports"));
    m_bugzillaDuplicatesPage->setIcon(QIcon::fromTheme("repository"));

    //-Bugzilla information
    BugzillaInformationPage * m_bugzillaInformation =  new BugzillaInformationPage(this);
    connectSignals(m_bugzillaInformation);

    KPageWidgetItem * m_bugzillaInformationPage = new KPageWidgetItem(m_bugzillaInformation,
                                                                QLatin1String(PAGE_BZDETAILS_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZDETAILS_ID),m_bugzillaInformationPage);
    m_bugzillaInformationPage->setHeader(i18nc("@title","Enter the Details about the Crash"));
    m_bugzillaInformationPage->setIcon(QIcon::fromTheme("document-edit"));

    //-Bugzilla Report Preview
    BugzillaPreviewPage * m_bugzillaPreview =  new BugzillaPreviewPage(this);

    KPageWidgetItem * m_bugzillaPreviewPage = new KPageWidgetItem(m_bugzillaPreview,
                                                                QLatin1String(PAGE_BZPREVIEW_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZPREVIEW_ID),m_bugzillaPreviewPage);
    m_bugzillaPreviewPage->setHeader(i18nc("@title","Preview the Report"));
    m_bugzillaPreviewPage->setIcon(QIcon::fromTheme("document-preview"));

    //-Bugzilla commit
    BugzillaSendPage * m_bugzillaSend =  new BugzillaSendPage(this);

    KPageWidgetItem * m_bugzillaSendPage = new KPageWidgetItem(m_bugzillaSend,
                                                                    QLatin1String(PAGE_BZSEND_ID));
    m_pageWidgetMap.insert(QLatin1String(PAGE_BZSEND_ID),m_bugzillaSendPage);
    m_bugzillaSendPage->setHeader(i18nc("@title","Sending the Crash Report"));
    m_bugzillaSendPage->setIcon(QIcon::fromTheme("applications-internet"));
    connect(m_bugzillaSend, &BugzillaSendPage::finished, this, &ReportAssistantDialog::assistantFinished);

    //TODO Remember to keep the pages ordered
    addPage(m_awarenessPage);
    addPage(m_backtracePage);
    addPage(m_conclusionsPage);
    addPage(m_bugzillaLoginPage);
    addPage(m_bugzillaDuplicatesPage);
    addPage(m_bugzillaInformationPage);
    addPage(m_bugzillaPreviewPage);
    addPage(m_bugzillaSendPage);

    setMinimumSize(QSize(600, 400));
    resize(minimumSize());
}

ReportAssistantDialog::~ReportAssistantDialog()
{
}

void ReportAssistantDialog::connectSignals(ReportAssistantPage * page)
{
    //React to the changes in the assistant pages
    connect(page, SIGNAL(completeChanged(ReportAssistantPage*,bool)),
             this, SLOT(completeChanged(ReportAssistantPage*,bool)));
}

void ReportAssistantDialog::currentPageChanged_slot(KPageWidgetItem * current , KPageWidgetItem * before)
{
    //Page changed
    buttonBox()->button(QDialogButtonBox::Cancel)->setEnabled(true);
    m_canClose = false;

    //Save data of the previous page
    if (before) {
        ReportAssistantPage* beforePage = dynamic_cast<ReportAssistantPage*>(before->widget());
        beforePage->aboutToHide();
    }

    //Load data of the current(new) page
    if (current) {
        ReportAssistantPage* currentPage = dynamic_cast<ReportAssistantPage*>(current->widget());
        nextButton()->setEnabled(currentPage->isComplete());
        currentPage->aboutToShow();
    }

    //If the current page is the last one, disable all the buttons until the bug is sent
    if (current->name() == QLatin1String(PAGE_BZSEND_ID)) {
        nextButton()->setEnabled(false);
        backButton()->setEnabled(false);
        finishButton()->setEnabled(false);
    }
}

void ReportAssistantDialog::completeChanged(ReportAssistantPage* page, bool isComplete)
{
    if (page == dynamic_cast<ReportAssistantPage*>(currentPage()->widget())) {
        nextButton()->setEnabled(isComplete);
    }
}

void ReportAssistantDialog::assistantFinished(bool showBack)
{
    //The assistant finished: allow the user to close the dialog normally

    nextButton()->setEnabled(false);
    backButton()->setEnabled(showBack);
    finishButton()->setEnabled(true);
    buttonBox()->button(QDialogButtonBox::Cancel)->setEnabled(false);

    m_canClose = true;
}

void ReportAssistantDialog::loginFinished()
{
    //Bugzilla login finished, go to the next page
    if (currentPage()->name() == QLatin1String(PAGE_BZLOGIN_ID)) {
        next();
    }
}

void ReportAssistantDialog::showHelp()
{
    //Show the bug reporting guide dialog
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
    m_aboutBugReportingDialog->showSection(QLatin1String(PAGE_HELP_BEGIN_ID));
    m_aboutBugReportingDialog->showSection(currentPage()->name());
}

//Override KAssistantDialog "next" page implementation
void ReportAssistantDialog::next()
{
    //Allow the widget to Ask a question to the user before changing the page
    ReportAssistantPage * page = dynamic_cast<ReportAssistantPage*>(currentPage()->widget());
    if (page) {
        if (!page->showNextPage()) {
            return;
        }
    }

    const QString name = currentPage()->name();

    //If the information the user can provide is not useful, skip the backtrace page
    if (name == QLatin1String(PAGE_AWARENESS_ID))
    {
        //Force save settings in the current page
        page->aboutToHide();

        if (!(m_reportInterface->isBugAwarenessPageDataUseful()))
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }
    } else if (name == QLatin1String(PAGE_CRASHINFORMATION_ID)){
        //Force save settings in current page
        page->aboutToHide();

        //If the crash is worth reporting and it is BKO, skip the Conclusions page
        if (m_reportInterface->isWorthReporting() &&
            DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla())
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_BZLOGIN_ID)));
            return;
        }
    } else if (name == QLatin1String(PAGE_BZDUPLICATES_ID)) {
        //a duplicate has been found, yet the report is not being attached
        if (m_reportInterface->duplicateId() && !m_reportInterface->attachToBugNumber()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CONCLUSIONS_ID)));
            return;
        }
    }

    KAssistantDialog::next();
}

//Override KAssistantDialog "back"(previous) page implementation
//It has to mirror the custom next() implementation
void ReportAssistantDialog::back()
 {
    if (currentPage()->name() == QLatin1String(PAGE_CONCLUSIONS_ID))
    {
        if (m_reportInterface->duplicateId() && !m_reportInterface->attachToBugNumber()) {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_BZDUPLICATES_ID)));
            return;
        }
        if (!(m_reportInterface->isBugAwarenessPageDataUseful()))
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_AWARENESS_ID)));
            return;
        }
    }

    if (currentPage()->name() == QLatin1String(PAGE_BZLOGIN_ID))
    {
        if (m_reportInterface->isWorthReporting() &&
            DrKonqi::crashedApplication()->bugReportAddress().isKdeBugzilla())
        {
            setCurrentPage(m_pageWidgetMap.value(QLatin1String(PAGE_CRASHINFORMATION_ID)));
            return;
        }
    }

    KAssistantDialog::back();
}

void ReportAssistantDialog::reject()
{
    close();
}

void ReportAssistantDialog::closeEvent(QCloseEvent * event)
{
    //Handle the close event
    if (!m_canClose) {
        //If the assistant didn't finished yet, offer the user the possibilities to
        //Close, Cancel, or Save the bug report and Close"

        KGuiItem closeItem = KStandardGuiItem::close();
        closeItem.setText(i18nc("@action:button", "Close the assistant"));

        KGuiItem keepOpenItem = KStandardGuiItem::cancel();
        keepOpenItem.setText(i18nc("@action:button", "Cancel"));

        BacktraceParser::Usefulness use =
                DrKonqi::debuggerManager()->backtraceGenerator()->parser()->backtraceUsefulness();
        if (use == BacktraceParser::ReallyUseful || use == BacktraceParser::MayBeUseful) {
            //Backtrace is still useful, let the user save it.
            KGuiItem saveBacktraceItem = KStandardGuiItem::save();
            saveBacktraceItem.setText(i18nc("@action:button", "Save information and close"));

            int ret = KMessageBox::questionYesNoCancel(this,
                           xi18nc("@info","Do you really want to close the bug reporting assistant? "
                           "<note>The crash information is still valid, so "
                           "you can save the report before closing if you want.</note>"),
                           i18nc("@title:window","Close the Assistant"),
                           closeItem, saveBacktraceItem, keepOpenItem, QString(), KMessageBox::Dangerous);
            if(ret == KMessageBox::Yes)
            {
                event->accept();
            } else if (ret == KMessageBox::No) {
                //Save backtrace and accept event (dialog will be closed)
                DrKonqi::saveReport(reportInterface()->generateReportFullText(false));
                event->accept();
            } else {
                event->ignore();
            }
        } else {
            if (KMessageBox::questionYesNo(this, i18nc("@info","Do you really want to close the bug "
                                                   "reporting assistant?"),
                                       i18nc("@title:window","Close the Assistant"),
                                           closeItem, keepOpenItem, QString(), KMessageBox::Dangerous)
                                        == KMessageBox::Yes) {
                event->accept();
            } else {
                event->ignore();
            }
        }
    } else {
        event->accept();
    }
}
