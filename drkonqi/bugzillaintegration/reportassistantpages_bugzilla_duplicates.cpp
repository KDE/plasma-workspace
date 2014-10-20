/*******************************************************************
* reportassistantpages_bugzilla_duplicates.cpp
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

#include "reportassistantpages_bugzilla_duplicates.h"

#include <QtCore/QDate>
#include <QtCore/QTimer>

#include <QTreeWidgetItem>
#include <QHeaderView>

#include <KColorScheme>
#include <KMessageBox>
#include <QInputDialog>
#include <KLocalizedString>
#include <KWindowConfig>

#include "drkonqi_globals.h"
#include "reportinterface.h"
#include "statuswidget.h"

//BEGIN BugzillaDuplicatesPage

BugzillaDuplicatesPage::BugzillaDuplicatesPage(ReportAssistantDialog * parent):
        ReportAssistantPage(parent),
        m_searching(false),
        m_foundDuplicate(false)
{
    resetDates();

    connect(bugzillaManager(), SIGNAL(searchFinished(BugMapList)),
             this, SLOT(searchFinished(BugMapList)));
    connect(bugzillaManager(), SIGNAL(searchError(QString)),
             this, SLOT(searchError(QString)));

    ui.setupUi(this);
    ui.information->hide();

    connect(ui.m_bugListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             this, SLOT(itemClicked(QTreeWidgetItem*,int)));
    connect(ui.m_bugListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    QHeaderView * header = ui.m_bugListWidget->header();
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(1, QHeaderView::Interactive);

    //Create manual bug report entry (first one)
    QTreeWidgetItem * customBugItem = new QTreeWidgetItem(
        QStringList() << i18nc("@item:intable custom/manaul bug report number", "Manual")
                      << i18nc("@item:intable custom bug report number description",
                                                            "Manually enter a bug report ID"));
    customBugItem->setData(0, Qt::UserRole, QLatin1String("custom"));
    customBugItem->setIcon(1, QIcon::fromTheme("edit-rename"));

    QString helpMessage = i18nc("@info:tooltip / whatsthis",
                                "Select this option to manually load a specific bug report");
    customBugItem->setToolTip(0, helpMessage);
    customBugItem->setToolTip(1, helpMessage);
    customBugItem->setWhatsThis(0, helpMessage);
    customBugItem->setWhatsThis(1, helpMessage);

    ui.m_bugListWidget->addTopLevelItem(customBugItem);

    m_searchMoreGuiItem = KGuiItem2(i18nc("@action:button", "Search for more reports"),
                                                   QIcon::fromTheme("edit-find"),
                                                   i18nc("@info:tooltip", "Use this button to "
                                                        "search for more similar bug reports on an "
                                                        "earlier date."));
    KGuiItem::assign(ui.m_searchMoreButton, m_searchMoreGuiItem);
    connect(ui.m_searchMoreButton, SIGNAL(clicked()), this, SLOT(searchMore()));

    m_retrySearchGuiItem = KGuiItem2(i18nc("@action:button", "Retry search"),
                                                   QIcon::fromTheme("edit-find"),
                                                   i18nc("@info:tooltip", "Use this button to "
                                                        "retry the search that previously "
                                                        "failed."));

    KGuiItem::assign(ui.m_openReportButton, KGuiItem2(i18nc("@action:button", "Open selected report"),
                                                   QIcon::fromTheme("document-preview"),
                                                   i18nc("@info:tooltip", "Use this button to view "
                                                   "the information of the selected bug report.")));
    connect(ui.m_openReportButton, SIGNAL(clicked()), this, SLOT(openSelectedReport()));

    KGuiItem::assign(ui.m_stopSearchButton, KGuiItem2(i18nc("@action:button", "Stop searching"),
                                                   QIcon::fromTheme("process-stop"),
                                                   i18nc("@info:tooltip", "Use this button to stop "
                                                   "the current search.")));
    ui.m_stopSearchButton->setText(QString()); //FIXME
    connect(ui.m_stopSearchButton, SIGNAL(clicked()), this, SLOT(stopCurrentSearch()));

    //Possible duplicates list and buttons
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(itemClicked(QListWidgetItem*)));
    connect(ui.m_selectedDuplicatesList, SIGNAL(itemSelectionChanged()),
             this, SLOT(possibleDuplicateSelectionChanged()));

    KGuiItem::assign(ui.m_removeSelectedDuplicateButton,
        KGuiItem2(i18nc("@action:button remove the selected item from a list", "Remove"),
               QIcon::fromTheme("list-remove"),
               i18nc("@info:tooltip", "Use this button to remove a selected possible duplicate")));
    ui.m_removeSelectedDuplicateButton->setEnabled(false);
    connect(ui.m_removeSelectedDuplicateButton, SIGNAL(clicked()), this,
                                                                SLOT(removeSelectedDuplicate()));

    ui.m_attachToReportIcon->setPixmap(QIcon::fromTheme("mail-attachment").pixmap(16,16));
    ui.m_attachToReportIcon->setFixedSize(16,16);
    ui.m_attachToReportIcon->setVisible(false);
    ui.m_attachToReportLabel->setVisible(false);
    connect(ui.m_attachToReportLabel, SIGNAL(linkActivated(QString)), this,
                                                                SLOT(cancelAttachToBugReport()));
    connect(ui.information, SIGNAL(linkActivated(QString)), this, SLOT(informationClicked(QString)));
    showDuplicatesPanel(false);
}

BugzillaDuplicatesPage::~BugzillaDuplicatesPage()
{
}

void BugzillaDuplicatesPage::aboutToShow()
{
    //Perform initial search if we are not currently searching and if there are no results yet
    if (!m_searching && ui.m_bugListWidget->topLevelItemCount() == 1 && canSearchMore()) {
        searchMore();
    }
}

void BugzillaDuplicatesPage::aboutToHide()
{
    stopCurrentSearch();

    //Save selected possible duplicates by user
    QStringList possibleDuplicates;
    int count = ui.m_selectedDuplicatesList->count();
    for(int i = 0; i<count; i++) {
        possibleDuplicates << ui.m_selectedDuplicatesList->item(i)->text();
    }
    reportInterface()->setPossibleDuplicates(possibleDuplicates);

    //Save possible duplicates by query
    QStringList duplicatesByQuery;
    count = ui.m_bugListWidget->topLevelItemCount();
    for(int i = 1; i<count; i++) {
        duplicatesByQuery << ui.m_bugListWidget->topLevelItem(i)->text(0);
    }
    reportInterface()->setPossibleDuplicatesByQuery(duplicatesByQuery);
}

bool BugzillaDuplicatesPage::isComplete()
{
    return !m_searching;
}

bool BugzillaDuplicatesPage::showNextPage()
{
    //Ask the user to check all the possible duplicates...
    if (ui.m_bugListWidget->topLevelItemCount() != 1 && ui.m_selectedDuplicatesList->count() == 0
        && reportInterface()->attachToBugNumber() == 0 && !m_foundDuplicate) {
        //The user didn't selected any possible duplicate nor a report to attach the new info.
        //Double check this, we need to reduce the duplicate count.
        KGuiItem noDuplicatesButton;
        noDuplicatesButton.setText(i18n("There are no real duplicates"));
        noDuplicatesButton.setWhatsThis(i18n("Press this button to declare that, in your opinion "
                                             "and according to your experience, the reports found "
                                             "as similar do not match the crash you have "
                                             "experienced, and you believe it is unlikely that a "
                                             "better match would be found after further review."));
        noDuplicatesButton.setIcon(QIcon::fromTheme("dialog-cancel"));

        KGuiItem letMeCheckMoreReportsButton;
        letMeCheckMoreReportsButton.setText(i18n("Let me check more reports"));
        letMeCheckMoreReportsButton.setWhatsThis(i18n("Press this button if you would rather "
                                                      "review more reports in order to find a "
                                                      "match for the crash you have experienced."));
        letMeCheckMoreReportsButton.setIcon(QIcon::fromTheme("document-preview"));

        if (KMessageBox::questionYesNo(this,
           i18nc("@info","You have not selected any possible duplicates, or a report to which to attach your "
           "crash information. Have you read all the reports, and can you confirm that there are no "
           "real duplicates?"),
           i18nc("@title:window","No selected possible duplicates"), letMeCheckMoreReportsButton,
                               noDuplicatesButton)
                                        == KMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

//BEGIN Search related methods
void BugzillaDuplicatesPage::searchMore()
{
    //1 year back
    m_searchingEndDate = m_startDate;
    m_searchingStartDate = m_searchingEndDate.addYears(-1);

    performSearch();
}

void BugzillaDuplicatesPage::performSearch()
{
    markAsSearching(true);

    QString startDateStr = m_searchingStartDate.toString("yyyy-MM-dd");
    QString endDateStr = m_searchingEndDate.toString("yyyy-MM-dd");

    ui.m_statusWidget->setBusy(i18nc("@info:status","Searching for duplicates (from %1 to %2)...",
                                   startDateStr, endDateStr));

    //Bugzilla will not search on Today bugs if we send the date.
    //we need to send "Now"
    if (m_searchingEndDate == QDate::currentDate()) {
        endDateStr = QLatin1String("Now");
    }

#if 1
    BugReport report = reportInterface()->newBugReportTemplate();
    bugzillaManager()->searchBugs(reportInterface()->relatedBugzillaProducts(),
                                  report.bugSeverity(), startDateStr, endDateStr,
                                  reportInterface()->firstBacktraceFunctions().join(" "));
#else //Test search
    bugzillaManager()->searchBugs(QStringList() << "plasma", "crash", startDateStr, endDateStr,
       "QGraphicsScenePrivate::processDirtyItemsRecursive");
#endif
}

void BugzillaDuplicatesPage::stopCurrentSearch()
{
    if (m_searching) {
        bugzillaManager()->stopCurrentSearch();

        markAsSearching(false);

        if (m_startDate==m_endDate) { //Never searched
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search stopped."));
        } else {
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search stopped. Showing results from "
                                        "%1 to %2", m_startDate.toString("yyyy-MM-dd"),
                                        m_endDate.toString("yyyy-MM-dd")));
        }
    }
}

void BugzillaDuplicatesPage::markAsSearching(bool searching)
{
    m_searching = searching;
    emitCompleteChanged();

    ui.m_bugListWidget->setEnabled(!searching);
    ui.m_searchMoreButton->setEnabled(!searching);
    ui.m_searchMoreButton->setVisible(!searching);
    ui.m_stopSearchButton->setEnabled(searching);
    ui.m_stopSearchButton->setVisible(searching);

    ui.m_selectedDuplicatesList->setEnabled(!searching);
    ui.m_selectedPossibleDuplicatesLabel->setEnabled(!searching);
    ui.m_removeSelectedDuplicateButton->setEnabled(!searching &&
                                        !ui.m_selectedDuplicatesList->selectedItems().isEmpty());

    ui.m_attachToReportLabel->setEnabled(!searching);

    if (!searching) {
        itemSelectionChanged();
    } else {
        ui.m_openReportButton->setEnabled(false);
    }
}

bool BugzillaDuplicatesPage::canSearchMore()
{
    return (m_startDate.year() >= 2009);
}

void BugzillaDuplicatesPage::searchFinished(const BugMapList & list)
{
    KGuiItem::assign(ui.m_searchMoreButton, m_searchMoreGuiItem);
    m_startDate = m_searchingStartDate;

    int results = list.count();
    if (results > 0) {
        markAsSearching(false);

        ui.m_statusWidget->setIdle(i18nc("@info:status","Showing results from %1 to %2",
                                     m_startDate.toString("yyyy-MM-dd"),
                                     m_endDate.toString("yyyy-MM-dd")));

        QList<int> bugIds;
        for (int i = 0; i < results; i++) {
            BugMap bug = list.at(i);

            bool ok;
            int bugId = bug.value("bug_id").toInt(&ok);
            if (ok) {
                bugIds << bugId;
            }

            QString title;

            //Generate a non-geek readable status
            QString customStatusString;
            BugReport::Status status = BugReport::parseStatus(bug.value("bug_status"));
            BugReport::Resolution resolution = BugReport::parseResolution(bug.value("resolution"));
            if (BugReport::isOpen(status)) {
                customStatusString = i18nc("@info bug status", "[Open]");
            } else if (BugReport::isClosed(status) && status != BugReport::NeedsInfo) {
                if (resolution == BugReport::Fixed) {
                    customStatusString = i18nc("@info bug resolution", "[Fixed]");
                } else if (resolution == BugReport::WorksForMe) {
                    customStatusString = i18nc("@info bug resolution", "[Non-reproducible]");
                } else if (resolution == BugReport::Duplicate) {
                    customStatusString = i18nc("@info bug resolution", "[Duplicate report]");
                } else if (resolution == BugReport::Invalid) {
                    customStatusString = i18nc("@info bug resolution", "[Invalid]");
                } else if (resolution == BugReport::Downstream
                    || resolution == BugReport::Upstream) {
                    customStatusString = i18nc("@info bug resolution", "[External problem]");
                }
            } else if (status == BugReport::NeedsInfo) {
                customStatusString = i18nc("@info bug status", "[Incomplete]");
            }

            title = customStatusString + ' ' + bug["short_desc"];

            QStringList fields = QStringList() << bug["bug_id"] << title;

            QTreeWidgetItem * item = new QTreeWidgetItem(fields);
            item->setToolTip(0, bug["short_desc"]);
            item->setToolTip(1, bug["short_desc"]);

            ui.m_bugListWidget->addTopLevelItem(item);
        }

        if (!m_foundDuplicate) {
            markAsSearching(true);
            DuplicateFinderJob *job = new DuplicateFinderJob(bugIds, bugzillaManager(), this);
            connect(job, SIGNAL(result(KJob*)), this, SLOT(analyzedDuplicates(KJob*)));
            job->start();
        }

        ui.m_bugListWidget->sortItems(0 , Qt::DescendingOrder);
        ui.m_bugListWidget->resizeColumnToContents(1);

        if (!canSearchMore()) {
            ui.m_searchMoreButton->setEnabled(false);
        }

    } else {

        if (canSearchMore()) {
            //We don't call markAsSearching(false) to avoid flicker
            //Delayed call to searchMore to avoid unexpected behaviour (signal/slot)
            //because we are in a slot, and searchMore() will be ending calling this slot again
            QTimer::singleShot(0, this, SLOT(searchMore()));
        } else {
            markAsSearching(false);
            ui.m_statusWidget->setIdle(i18nc("@info:status","Search Finished. "
                                                         "No reports found."));
            ui.m_searchMoreButton->setEnabled(false);
            if (ui.m_bugListWidget->topLevelItemCount() == 0) {
                //No reports to mark as possible duplicate
                ui.m_selectedDuplicatesList->setEnabled(false);
            }
        }
    }
}

void BugzillaDuplicatesPage::analyzedDuplicates(KJob *j)
{
    markAsSearching(false);

    DuplicateFinderJob *job = static_cast<DuplicateFinderJob*>(j);
    m_result = job->result();
    m_foundDuplicate = m_result.parentDuplicate;
    reportInterface()->setDuplicateId(m_result.parentDuplicate);
    ui.m_searchMoreButton->setEnabled(!m_foundDuplicate);
    ui.information->setVisible(m_foundDuplicate);
    BugReport::Status status = m_result.status;
    const int duplicate = m_result.duplicate;
    const int parentDuplicate = m_result.parentDuplicate;

    if (m_foundDuplicate) {
        const QList<QTreeWidgetItem*> items = ui.m_bugListWidget->findItems(QString::number(parentDuplicate), Qt::MatchExactly, 0);
        const QBrush brush = KColorScheme(QPalette::Active, KColorScheme::View).background(KColorScheme::NeutralBackground);
        Q_FOREACH (QTreeWidgetItem* item, items) {
            for (int i = 0; i < item->columnCount(); ++i) {
                item->setBackground(i, brush);
            }
        }

        QString text;
        if (BugReport::isOpen(status) || (BugReport::isClosed(status) && status == BugReport::NeedsInfo)) {
            text = (parentDuplicate == duplicate ? i18nc("@label", "Your crash is a <strong>duplicate</strong> and has already been reported as <a href=\"%1\">Bug %1</a>.", QString::number(duplicate)) :
                i18nc("@label", "Your crash has already been reported as <a href=\"%1\">Bug %1</a>, which is a <strong>duplicate</strong> of <a href=\"%2\">Bug %2</a>", QString::number(duplicate), QString::number(parentDuplicate))) +
                '\n' + i18nc("@label", "Only <strong><a href=\"%1\">attach</a></strong> if you can add needed information to the bug report.", QString("attach"));
        } else if (BugReport::isClosed(status)) {
            text = (parentDuplicate == duplicate ? i18nc("@label", "Your crash has already been reported as <a href=\"%1\">Bug %1</a> which has been <strong>closed</strong>.", QString::number(duplicate)) :
                i18nc("@label", "Your crash has already been reported as <a href=\"%1\">Bug %1</a>, which is a duplicate of the <strong>closed</strong> <a href=\"%2\">Bug %2</a>.", QString::number(duplicate), QString::number(parentDuplicate)));
        }
        ui.information->setText(text);
    }
}

void BugzillaDuplicatesPage::informationClicked(const QString &activatedLink)
{
    if (activatedLink == QLatin1String("attach")) {
        attachToBugReport(m_result.parentDuplicate);
    } else {
        int number = activatedLink.toInt();
        if (number) {
            showReportInformationDialog(number, false);
        }
    }
}

void BugzillaDuplicatesPage::searchError(QString err)
{
    KGuiItem::assign(ui.m_searchMoreButton, m_retrySearchGuiItem);
    markAsSearching(false);

    ui.m_statusWidget->setIdle(i18nc("@info:status","Error fetching the bug report list"));

    KMessageBox::error(this , xi18nc("@info/rich","Error fetching the bug report list<nl/>"
                                                 "<message>%1.</message><nl/>"
                                                 "Please wait some time and try again.", err));
}

void BugzillaDuplicatesPage::resetDates()
{
    m_endDate = QDate::currentDate();
    m_startDate = m_endDate;
}
//END Search related methods

//BEGIN Duplicates list related methods
void BugzillaDuplicatesPage::openSelectedReport()
{
    QList<QTreeWidgetItem*> selected = ui.m_bugListWidget->selectedItems();
    if (selected.count() == 1) {
        itemClicked(selected.at(0), 0);
    }
}

void BugzillaDuplicatesPage::itemClicked(QTreeWidgetItem * item, int col)
{
    Q_UNUSED(col);

    int bugNumber = 0;
    if (item->data(0, Qt::UserRole) == QLatin1String("custom")) {
        bool ok = false;
        bugNumber = QInputDialog::getInteger(this,
                    i18nc("@title:window", "Enter a custom bug report number"),
                    i18nc("@label", "Enter the number of the bug report you want to check"),
                    0, 0, 1000000, 1, &ok);
    } else {
        bugNumber = item->text(0).toInt();
    }
    showReportInformationDialog(bugNumber);
}

void BugzillaDuplicatesPage::itemClicked(QListWidgetItem * item)
{
    showReportInformationDialog(item->text().toInt());
}

void BugzillaDuplicatesPage::showReportInformationDialog(int bugNumber, bool relatedButtonEnabled)
{
    if (bugNumber <= 0) {
        return;
    }

    BugzillaReportInformationDialog * infoDialog = new BugzillaReportInformationDialog(this);
    connect(infoDialog, SIGNAL(possibleDuplicateSelected(int)), this,
                                                            SLOT(addPossibleDuplicateNumber(int)));
    connect(infoDialog, SIGNAL(attachToBugReportSelected(int)), this, SLOT(attachToBugReport(int)));

    infoDialog->showBugReport(bugNumber, relatedButtonEnabled);
}

void BugzillaDuplicatesPage::itemSelectionChanged()
{
    ui.m_openReportButton->setEnabled(ui.m_bugListWidget->selectedItems().count() == 1);
}
//END Duplicates list related methods

//BEGIN Selected duplicates list related methods
void BugzillaDuplicatesPage::addPossibleDuplicateNumber(int bugNumber)
{
    QString stringNumber = QString::number(bugNumber);
    if (ui.m_selectedDuplicatesList->findItems(stringNumber, Qt::MatchExactly).isEmpty()) {
        ui.m_selectedDuplicatesList->addItem(stringNumber);
    }

    showDuplicatesPanel(true);
}

void BugzillaDuplicatesPage::removeSelectedDuplicate()
{
    QList<QListWidgetItem*> items = ui.m_selectedDuplicatesList->selectedItems();
    if (items.length() > 0) {
        delete ui.m_selectedDuplicatesList->takeItem(ui.m_selectedDuplicatesList->row(items.at(0)));
    }

    if (ui.m_selectedDuplicatesList->count() == 0) {
        showDuplicatesPanel(false);
    }
}

void BugzillaDuplicatesPage::showDuplicatesPanel(bool show)
{
    ui.m_removeSelectedDuplicateButton->setVisible(show);
    ui.m_selectedDuplicatesList->setVisible(show);
    ui.m_selectedPossibleDuplicatesLabel->setVisible(show);
}

void BugzillaDuplicatesPage::possibleDuplicateSelectionChanged()
{
    ui.m_removeSelectedDuplicateButton->setEnabled(
                                        !ui.m_selectedDuplicatesList->selectedItems().isEmpty());
}
//END Selected duplicates list related methods

//BEGIN Attach to bug related methods
void BugzillaDuplicatesPage::attachToBugReport(int bugNumber)
{
    ui.m_attachToReportLabel->setText(xi18nc("@label", "The report is going to be "
                            "<strong>attached</strong> to bug %1. "
                            "<a href=\"#\">Cancel</a>", QString::number(bugNumber)));
    ui.m_attachToReportLabel->setVisible(true);
    ui.m_attachToReportIcon->setVisible(true);
    reportInterface()->setAttachToBugNumber(bugNumber);
}

void BugzillaDuplicatesPage::cancelAttachToBugReport()
{
    ui.m_attachToReportLabel->setVisible(false);
    ui.m_attachToReportIcon->setVisible(false);
    reportInterface()->setAttachToBugNumber(0);
}
//END Attach to bug related methods

//END BugzillaDuplicatesPage

//BEGIN BugzillaReportInformationDialog

BugzillaReportInformationDialog::BugzillaReportInformationDialog(BugzillaDuplicatesPage * parent) :
        QDialog(parent),
        m_relatedButtonEnabled(true),
        m_parent(parent),
        m_bugNumber(0),
        m_duplicatesCount(0)
{
    setWindowTitle(i18nc("@title:window","Bug Description"));

    ui.setupUi(this);

    KGuiItem::assign(ui.m_retryButton, KGuiItem2(i18nc("@action:button", "Retry..."),
                                  QIcon::fromTheme("view-refresh"),
                                  i18nc("@info:tooltip", "Use this button to retry "
                                                  "loading the bug report.")));
    connect(ui.m_retryButton, SIGNAL(clicked()), this, SLOT(reloadReport()));

    m_suggestButton = new QPushButton(this);
    ui.buttonBox->addButton(m_suggestButton, QDialogButtonBox::ActionRole);
    KGuiItem::assign(m_suggestButton,
                KGuiItem2(i18nc("@action:button", "Suggest this crash is related"),
                    QIcon::fromTheme("list-add"), i18nc("@info:tooltip", "Use this button to suggest that "
                                             "the crash you experienced is related to this bug "
                                             "report")));
    connect(this, SIGNAL(user1Clicked()) , this, SLOT(relatedReportClicked()));

    connect(ui.m_showOwnBacktraceCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleShowOwnBacktrace(bool)));

    //Connect bugzillalib signals
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportFetched(BugReport,QObject*)),
             this, SLOT(bugFetchFinished(BugReport,QObject*)));
    connect(m_parent->bugzillaManager(), SIGNAL(bugReportError(QString,QObject*)),
             this, SLOT(bugFetchError(QString,QObject*)));

    resize(QSize(800, 600));
    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), config);
}

BugzillaReportInformationDialog::~BugzillaReportInformationDialog()
{
    disconnect(m_parent->bugzillaManager(), SIGNAL(bugReportFetched(BugReport,QObject*)),
             this, SLOT(bugFetchFinished(BugReport,QObject*)));
    disconnect(m_parent->bugzillaManager(), SIGNAL(bugReportError(QString,QObject*)),
             this, SLOT(bugFetchError(QString,QObject*)));

    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void BugzillaReportInformationDialog::reloadReport()
{
    showBugReport(m_bugNumber);
}

void BugzillaReportInformationDialog::showBugReport(int bugNumber, bool relatedButtonEnabled)
{
    m_relatedButtonEnabled = relatedButtonEnabled;
    ui.m_retryButton->setVisible(false);

    m_closedStateString.clear();
    m_bugNumber = bugNumber;
    m_parent->bugzillaManager()->fetchBugReport(m_bugNumber, this);

    m_suggestButton->setEnabled(false);
    m_suggestButton->setVisible(m_relatedButtonEnabled);

    ui.m_infoBrowser->setText(i18nc("@info:status","Loading..."));
    ui.m_infoBrowser->setEnabled(false);

    ui.m_linkLabel->setText(xi18nc("@info","<link url='%1'>Report's webpage</link>",
                                    m_parent->bugzillaManager()->urlForBug(m_bugNumber)));

    ui.m_statusWidget->setBusy(xi18nc("@info:status","Loading information about bug "
                                                           "%1 from %2....",
                                            QString::number(m_bugNumber),
                                            QLatin1String(KDE_BUGZILLA_SHORT_URL)));

    ui.m_backtraceBrowser->setPlainText(
                    i18nc("@info","Backtrace of the crash I experienced:\n\n") +
                    m_parent->reportInterface()->backtrace());

    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    bool showOwnBacktrace = config.readEntry("ShowOwnBacktrace", false);
    ui.m_showOwnBacktraceCheckBox->setChecked(showOwnBacktrace);
    if (!showOwnBacktrace) { //setChecked(false) will not emit toggled(false)
        toggleShowOwnBacktrace(false);
    }

    show();
}

void BugzillaReportInformationDialog::bugFetchFinished(BugReport report, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        if (report.isValid()) {

            //Handle duplicate state
            QString duplicate = report.markedAsDuplicateOf();
            if (!duplicate.isEmpty()) {
                bool ok = false;
                int dupId = duplicate.toInt(&ok);
                if (ok && dupId > 0) {
                    ui.m_statusWidget->setIdle(QString());

                    KGuiItem yesItem = KStandardGuiItem::yes();
                    yesItem.setText(i18nc("@action:button let the user to choose to read the "
                    "main report", "Yes, read the main report"));

                    KGuiItem noItem = KStandardGuiItem::no();
                    noItem.setText(i18nc("@action:button let the user choose to read the original "
                    "report", "No, let me read the report I selected"));

                    if (KMessageBox::questionYesNo(this,
                       xi18nc("@info","The report you selected (bug %1) is already "
                       "marked as duplicate of bug %2. "
                       "Do you want to read that report instead? (recommended)",
                       report.bugNumber(), QString::number(dupId)),
                       i18nc("@title:window","Nested duplicate detected"), yesItem, noItem)
                                                    == KMessageBox::Yes) {
                        showBugReport(dupId);
                        return;
                    }
                }
            }

            //Generate html for comments (with proper numbering)
            QLatin1String duplicatesMark = QLatin1String("has been marked as a duplicate of this bug.");

            QString comments;
            QStringList commentList = report.comments();
            for (int i = 0; i < commentList.count(); i++) {
                QString comment = commentList.at(i);
                //Don't add duplicates mark comments
                if (!comment.contains(duplicatesMark)) {
                    comment.replace('\n', "<br />");
                    comments += i18nc("comment $number to use as subtitle", "<h4>Comment %1:</h4>", (i+1))
                                    + "<p>" + comment + "</p><hr />";
                    //Count the inline attached crashes (DrKonqi feature)
                    QLatin1String attachedCrashMark =
                        QLatin1String("New crash information added by DrKonqi");
                    if (comment.contains(attachedCrashMark)) {
                        m_duplicatesCount++;
                    }
                } else {
                    //Count duplicate
                    m_duplicatesCount++;
                }
            }

            //Generate a non-geek readable status
            QString customStatusString;
            BugReport::Status status = report.statusValue();
            BugReport::Resolution resolution = report.resolutionValue();
            if (status == BugReport::Unconfirmed) {
                customStatusString = i18nc("@info bug status", "Opened (Unconfirmed)");
            } else if (report.isOpen()) {
                customStatusString = i18nc("@info bug status", "Opened (Unfixed)");
            } else if (report.isClosed() && status != BugReport::NeedsInfo) {
                QString customResolutionString;
                if (resolution == BugReport::Fixed) {
                    if (!report.versionFixedIn().isEmpty()) {
                        customResolutionString = i18nc("@info bug resolution, fixed in version",
                                                       "Fixed in version \"%1\"",
                                                       report.versionFixedIn());
                        m_closedStateString = i18nc("@info bug resolution, fixed by kde devs in version",
                                                    "the bug was fixed by KDE developers in version \"%1\"",
                                                    report.versionFixedIn());
                    } else {
                        customResolutionString = i18nc("@info bug resolution", "Fixed");
                        m_closedStateString = i18nc("@info bug resolution", "the bug was fixed by KDE developers");
                    }
                } else if (resolution == BugReport::WorksForMe) {
                    customResolutionString = i18nc("@info bug resolution", "Non-reproducible");
                } else if (resolution == BugReport::Duplicate) {
                    customResolutionString = i18nc("@info bug resolution", "Duplicate report "
                                                   "(Already reported before)");
                } else if (resolution == BugReport::Invalid) {
                    customResolutionString = i18nc("@info bug resolution", "Not a valid report/crash");
                } else if (resolution == BugReport::Downstream || resolution == BugReport::Upstream) {
                    customResolutionString = i18nc("@info bug resolution", "Not caused by a problem "
                                                "in the KDE's Applications or libraries");
                    m_closedStateString = i18nc("@info bug resolution", "the bug is caused by a "
                                                "problem in an external application or library, or "
                                                "by a distribution or packaging issue");
                } else {
                    customResolutionString = report.resolution();
                }

                customStatusString = i18nc("@info bug status, %1 is the resolution", "Closed (%1)",
                                           customResolutionString);
            } else if (status == BugReport::NeedsInfo) {
                customStatusString = i18nc("@info bug status", "Temporarily closed, because of a lack "
                                            "of information");
            } else { //Fallback to other raw values
                customStatusString = QString("%1 (%2)").arg(report.bugStatus(), report.resolution());
            }

            //Generate notes
            QString notes = xi18n("<p><note>The bug report's title is often written by its reporter "
                                 "and may not reflect the bug's nature, root cause or other visible "
                                 "symptoms you could use to compare to your crash. Please read the "
                                 "complete report and all the comments below.</note></p>");

            if (m_duplicatesCount >= 10) { //Consider a possible mass duplicate crash
                notes += xi18np("<p><note>This bug report has %1 duplicate report. That means this "
                               "is probably a <strong>common crash</strong>. <i>Please consider only "
                               "adding a comment or a note if you can provide new valuable "
                               "information which was not already mentioned.</i></note></p>",
                               "<p><note>This bug report has %1 duplicate reports. That means this "
                               "is probably a <strong>common crash</strong>. <i>Please consider only "
                               "adding a comment or a note if you can provide new valuable "
                               "information which was not already mentioned.</i></note></p>",
                              m_duplicatesCount);
            }

            //A manually entered bug ID could represent a normal bug
            if (report.bugSeverity() != QLatin1String("crash")
                && report.bugSeverity() != QLatin1String("major")
                && report.bugSeverity() != QLatin1String("grave")
                && report.bugSeverity() != QLatin1String("critical"))
            {
                notes += xi18n("<p><note>This bug report is not about a crash or about any other "
                              "critical bug.</note></p>");
            }

            //Generate HTML text
            QString text =
                i18nc("@info bug report title (quoted)",
                                "<h3>\"%1\"</h3>", report.shortDescription()) +
                notes +
                i18nc("@info bug report status",
                                "<h4>Bug Report Status: %1</h4>", customStatusString) +
                i18nc("@info bug report product and component",
                                "<h4>Affected Component: %1 (%2)</h4>",
                                report.product(), report.component()) +
                i18nc("@info bug report description",
                                "<h3>Description of the bug</h3><p>%1</p>",
                                report.description().replace('\n', "<br />"));

            if (!comments.isEmpty()) {
                text += i18nc("@label:textbox bug report comments (already formatted)",
                                "<h2>Additional Comments</h2>%1", comments);
            }

            ui.m_infoBrowser->setText(text);
            ui.m_infoBrowser->setEnabled(true);

            m_suggestButton->setEnabled(m_relatedButtonEnabled);
            m_suggestButton->setVisible(m_relatedButtonEnabled);

            ui.m_statusWidget->setIdle(xi18nc("@info:status", "Showing bug %1",
                                                              QString::number(report.bugNumberAsInt())));
        } else {
            bugFetchError(i18nc("@info", "Invalid report information (malformed data). This could "
                                "mean that the bug report does not exist, or the bug tracking site "
                                "is experiencing a problem."), this);
        }
    }
}

void BugzillaReportInformationDialog::markAsDuplicate()
{
    emit possibleDuplicateSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::attachToBugReport()
{
    emit attachToBugReportSelected(m_bugNumber);
    hide();
}

void BugzillaReportInformationDialog::cancelAssistant()
{
    m_parent->assistant()->close();
    hide();
}

void BugzillaReportInformationDialog::relatedReportClicked()
{
    BugzillaReportConfirmationDialog * confirmation =
        new BugzillaReportConfirmationDialog(m_bugNumber, (m_duplicatesCount >= 10),
                                             m_closedStateString, this);
    confirmation->show();
}

void BugzillaReportInformationDialog::bugFetchError(QString err, QObject * jobOwner)
{
    if (jobOwner == this && isVisible()) {
        KMessageBox::error(this , xi18nc("@info/rich","Error fetching the bug report<nl/>"
                                         "<message>%1.</message><nl/>"
                                         "Please wait some time and try again.", err));
        m_suggestButton->setEnabled(false);
        ui.m_infoBrowser->setText(i18nc("@info","Error fetching the bug report"));
        ui.m_statusWidget->setIdle(i18nc("@info:status","Error fetching the bug report"));
        ui.m_retryButton->setVisible(true);
    }
}

void BugzillaReportInformationDialog::toggleShowOwnBacktrace(bool show)
{
    QList<int> sizes;
    if (show) {
        int size = (ui.m_reportSplitter->sizeHint().width()-ui.m_reportSplitter->handleWidth())/2;
        sizes << size << size;
    } else {
        sizes << ui.m_reportSplitter->sizeHint().width() << 0; //Hide backtrace
    }
    ui.m_reportSplitter->setSizes(sizes);

    //Save the current show value
    KConfigGroup config(KSharedConfig::openConfig(), "BugzillaReportInformationDialog");
    config.writeEntry("ShowOwnBacktrace", show);
}

//END BugzillaReportInformationDialog

//BEGIN BugzillaReportConfirmationDialog

BugzillaReportConfirmationDialog::BugzillaReportConfirmationDialog(int bugNumber, bool commonCrash,
    QString closedState, BugzillaReportInformationDialog * parent)
    : QDialog(parent),
    m_parent(parent),
    m_showProceedQuestion(false),
    m_bugNumber(bugNumber)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setModal(true);

    ui.setupUi(this);

    //Setup dialog
    setWindowTitle(i18nc("@title:window", "Related Bug Report"));

    //Setup buttons
    ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(i18nc("@action:button", "Cancel (Go back to the report)"));
    ui.buttonBox->button(QDialogButtonBox::Ok)->setText(i18nc("@action:button continue with the selected option "
                                       "and close the dialog", "Continue"));
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(this, SIGNAL(okClicked()) , this, SLOT(proceedClicked()));
    connect(this, SIGNAL(cancelClicked()) , this, SLOT(hide()));

    //Set introduction text
    ui.introLabel->setText(i18n("You are going to mark your crash as related to bug %1",
                             QString::number(m_bugNumber)));

    if (commonCrash) { //Common ("massive") crash
        m_showProceedQuestion = true;
        ui.commonCrashIcon->setPixmap(QIcon::fromTheme("edit-bomb").pixmap(22,22));
    } else {
        ui.commonCrashLabel->setVisible(false);
        ui.commonCrashIcon->setVisible(false);
    }

    if (!closedState.isEmpty()) { //Bug report closed
        ui.closedReportLabel->setText(
                     i18nc("@info", "The report is closed because %1. "
                           "<i>If the crash is the same, adding further information will be useless "
                           "and will consume developers' time.</i>",
                           closedState));
        ui.closedReportIcon->setPixmap(QIcon::fromTheme("document-close").pixmap(22,22));
        m_showProceedQuestion = true;
    } else {
        ui.closedReportLabel->setVisible(false);
        ui.closedReportIcon->setVisible(false);
    }

    //Disable all the radio buttons
    ui.proceedRadioYes->setChecked(false);
    ui.proceedRadioNo->setChecked(false);
    ui.markAsDuplicateCheck->setChecked(false);
    ui.attachToBugReportCheck->setChecked(false);

    connect(ui.buttonGroupProceed, SIGNAL(buttonClicked(int)), this, SLOT(checkProceed()));
    connect(ui.buttonGroupProceedQuestion, SIGNAL(buttonClicked(int)), this, SLOT(checkProceed()));

    if (!m_showProceedQuestion) {
        ui.proceedLabel->setEnabled(false);
        ui.proceedRadioYes->setEnabled(false);
        ui.proceedRadioNo->setEnabled(false);

        ui.proceedLabel->setVisible(false);
        ui.proceedRadioYes->setVisible(false);
        ui.proceedRadioNo->setVisible(false);

        ui.proceedRadioYes->setChecked(true);
    }

    checkProceed();

    setMinimumSize(QSize(600, 350));
    resize(QSize(600, 350));
}

BugzillaReportConfirmationDialog::~BugzillaReportConfirmationDialog()
{
}

void BugzillaReportConfirmationDialog::checkProceed()
{
    bool yes = ui.proceedRadioYes->isChecked();
    bool no = ui.proceedRadioNo->isChecked();

    //Enable/disable labels and controls
    ui.areYouSureLabel->setEnabled(yes);
    ui.markAsDuplicateCheck->setEnabled(yes);
    ui.attachToBugReportCheck->setEnabled(yes);

    //Enable Continue button if valid options are selected
    bool possibleDupe = ui.markAsDuplicateCheck->isChecked();
    bool attach = ui.attachToBugReportCheck->isChecked();
    bool enableContinueButton = yes ? (possibleDupe || attach) : no;
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enableContinueButton);
}

void BugzillaReportConfirmationDialog::proceedClicked()
{
    if (ui.proceedRadioYes->isChecked()) {
        if (ui.markAsDuplicateCheck->isChecked()) {
            m_parent->markAsDuplicate();
            hide();
        } else {
            m_parent->attachToBugReport();
            hide();
        }
    } else {
        hide();
        m_parent->cancelAssistant();
    }
}

//END BugzillaReportConfirmationDialog
