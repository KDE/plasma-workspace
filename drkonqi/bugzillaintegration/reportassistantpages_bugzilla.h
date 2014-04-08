/*******************************************************************
* reportassistantpages_bugzilla.h
* Copyright 2009, 2011    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#ifndef REPORTASSISTANTPAGES__BUGZILLA__H
#define REPORTASSISTANTPAGES__BUGZILLA__H

#include "reportassistantpage.h"

#include "reportassistantpages_base.h"

#include "ui_assistantpage_bugzilla_login.h"
#include "ui_assistantpage_bugzilla_information.h"
#include "ui_assistantpage_bugzilla_preview.h"
#include "ui_assistantpage_bugzilla_send.h"

namespace KWallet { class Wallet; }
class KCapacityBar;

/** Bugzilla login **/
class BugzillaLoginPage: public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaLoginPage(ReportAssistantDialog *);
    ~BugzillaLoginPage();

    void aboutToShow();
    bool isComplete();

private Q_SLOTS:
    void loginClicked();
    void loginFinished(bool);
    void loginError(const QString &, const QString &);

    void walletLogin();

    void updateLoginButtonStatus();

Q_SIGNALS:
    void loggedTurnToNextPage();

private:
    bool kWalletEntryExists(const QString&);
    void openWallet();
    bool canSetCookies();

    Ui::AssistantPageBugzillaLogin      ui;

    KWallet::Wallet *                   m_wallet;
    bool                                m_walletWasOpenedBefore;
};

/** Title and details page **/
class BugzillaInformationPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaInformationPage(ReportAssistantDialog *);

    void aboutToShow();
    void aboutToHide();

    bool isComplete();
    bool showNextPage();

private Q_SLOTS:
    void showTitleExamples();
    void showDescriptionHelpExamples();

    void checkTexts();

private:
    int currentDescriptionCharactersCount();

    Ui::AssistantPageBugzillaInformation    ui;
    KCapacityBar *                          m_textCompleteBar;

    bool                                    m_textsOK;
    bool                                    m_distributionComboSetup;
    bool                                    m_distroComboVisible;

    int                                     m_requiredCharacters;
};

/** Preview report page **/
class BugzillaPreviewPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaPreviewPage(ReportAssistantDialog *);

    void aboutToShow();

private:
    Ui::AssistantPageBugzillaPreview    ui;
};

/** Send crash report page **/
class BugzillaSendPage : public ReportAssistantPage
{
    Q_OBJECT

public:
    explicit BugzillaSendPage(ReportAssistantDialog *);

    void aboutToShow();

private Q_SLOTS:
    void sent(int);
    void sendError(const QString &, const QString &);

    void retryClicked();
    void finishClicked();

    void openReportContents();

private:
    Ui::AssistantPageBugzillaSend           ui;
    QString                                 reportUrl;

    QPointer<QDialog>                       m_contentsDialog;

Q_SIGNALS:
    void finished(bool);

};

class UnhandledErrorDialog: public QDialog
{
    Q_OBJECT

public:
    UnhandledErrorDialog(QWidget * parent, const QString &, const QString &);

private Q_SLOTS:
    void saveErrorMessage();

private:
    QString    m_extendedHTMLError;
};

#endif
