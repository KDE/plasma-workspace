/*******************************************************************
* reportassistantpages_bugzilla.cpp
* Copyright 2009, 2010, 2011    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "reportassistantpages_bugzilla.h"

#include <QtCore/QTimer>

#include <QLabel>
#include <QCheckBox>
#include <QToolTip>
#include <QCursor>
#include <QFileDialog>
#include <QTemporaryFile>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <QDebug>
#include <KMessageBox>
#include <KToolInvocation>
#include <KLocalizedString>
#include <KWallet/kwallet.h>
#include <KCapacityBar>

/* Unhandled error dialog includes */
#include <KWebView>
#include <KIO/Job>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KJobWidgets>

#include "reportinterface.h"
#include "systeminformation.h"
#include "crashedapplication.h"
#include "bugzillalib.h"
#include "statuswidget.h"
#include "drkonqi.h"
#include "drkonqi_globals.h"
#include "applicationdetailsexamples.h"

static const char kWalletEntryName[] = "drkonqi_bugzilla";
static const char kWalletEntryUsername[] = "username";
static const char kWalletEntryPassword[] = "password";

static const char konquerorKWalletEntryName[] = KDE_BUGZILLA_URL "index.cgi#login";
static const char konquerorKWalletEntryUsername[] = "Bugzilla_login";
static const char konquerorKWalletEntryPassword[] = "Bugzilla_password";

//BEGIN BugzillaLoginPage

BugzillaLoginPage::BugzillaLoginPage(ReportAssistantDialog * parent) :
        ReportAssistantPage(parent),
        m_wallet(0), m_walletWasOpenedBefore(false)
{
    connect(bugzillaManager(), SIGNAL(loginFinished(bool)), this, SLOT(loginFinished(bool)));
    connect(bugzillaManager(), SIGNAL(loginError(QString,QString)), this, SLOT(loginError(QString,QString)));

    ui.setupUi(this);
    ui.m_statusWidget->setIdle(i18nc("@info:status '1' is replaced with the short URL of the bugzilla ",
                                   "You need to login with your %1 account in order to proceed.",
                                   QLatin1String(KDE_BUGZILLA_SHORT_URL)));

    KGuiItem::assign(ui.m_loginButton, KGuiItem2(i18nc("@action:button", "Login"),
                                              QIcon::fromTheme("network-connect"),
                                              i18nc("@info:tooltip", "Use this button to login "
                                              "to the KDE bug tracking system using the provided "
                                              "username and password.")));
    ui.m_loginButton->setEnabled(false);

    connect(ui.m_loginButton, &QPushButton::clicked, this, &BugzillaLoginPage::loginClicked);

    connect(ui.m_userEdit, &KLineEdit::returnPressed, this, &BugzillaLoginPage::loginClicked);
    connect(ui.m_passwordEdit, &KLineEdit::returnPressed, this, &BugzillaLoginPage::loginClicked);

    connect(ui.m_userEdit, &KLineEdit::textChanged, this, &BugzillaLoginPage::updateLoginButtonStatus);
    connect(ui.m_passwordEdit, &KLineEdit::textChanged, this, &BugzillaLoginPage::updateLoginButtonStatus);

    ui.m_noticeLabel->setText(
                        xi18nc("@info/rich","<note>You need a user account on the "
                            "<link url='%1'>KDE bug tracking system</link> in order to "
                            "file a bug report, because we may need to contact you later "
                            "for requesting further information. If you do not have "
                            "one, you can freely <link url='%2'>create one here</link>. "
                            "Please do not use disposable email accounts.</note>",
                            DrKonqi::crashedApplication()->bugReportAddress(),
                            QLatin1String(KDE_BUGZILLA_CREATE_ACCOUNT_URL)));
}

bool BugzillaLoginPage::isComplete()
{
    return bugzillaManager()->getLogged();
}

void BugzillaLoginPage::updateLoginButtonStatus()
{
    ui.m_loginButton->setEnabled( !ui.m_userEdit->text().isEmpty() &&
                                  !ui.m_passwordEdit->text().isEmpty() );
}

void BugzillaLoginPage::loginError(const QString & err, const QString & extendedMessage)
{
    loginFinished(false);
    ui.m_statusWidget->setIdle(xi18nc("@info:status","Error when trying to login: "
                                                 "<message>%1.</message>", err));
    if (!extendedMessage.isEmpty()) {
        new UnhandledErrorDialog(this, err, extendedMessage);
    }
}

void BugzillaLoginPage::aboutToShow()
{
    if (bugzillaManager()->getLogged()) {
        ui.m_loginButton->setEnabled(false);

        ui.m_userEdit->setEnabled(false);
        ui.m_userEdit->clear();
        ui.m_passwordEdit->setEnabled(false);
        ui.m_passwordEdit->clear();

        ui.m_loginButton->setVisible(false);
        ui.m_userEdit->setVisible(false);
        ui.m_passwordEdit->setVisible(false);
        ui.m_userLabel->setVisible(false);
        ui.m_passwordLabel->setVisible(false);

        ui.m_savePasswordCheckBox->setVisible(false);

        ui.m_noticeLabel->setVisible(false);

        ui.m_statusWidget->setIdle(i18nc("@info:status the user is logged at the bugtracker site "
                                      "as USERNAME",
                                      "Logged in at the KDE bug tracking system (%1) as: %2.",
                                      QLatin1String(KDE_BUGZILLA_SHORT_URL),
                                      bugzillaManager()->getUsername()));
    } else {
        //Try to show wallet dialog once this dialog is shown
        QTimer::singleShot(100, this, SLOT(walletLogin()));
    }
}

bool BugzillaLoginPage::kWalletEntryExists(const QString& entryName)
{
    return !KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
                                               KWallet::Wallet::FormDataFolder(),
                                               entryName);
}

void BugzillaLoginPage::openWallet()
{
    //Store if the wallet was previously opened so we can know if we should close it later
    m_walletWasOpenedBefore = KWallet::Wallet::isOpen(KWallet::Wallet::NetworkWallet());
    //Request open the wallet
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                    static_cast<QWidget*>(this->parent())->winId());
}

void BugzillaLoginPage::walletLogin()
{
    if (!m_wallet) {
        if (kWalletEntryExists(QLatin1String(kWalletEntryName))) {  //Key exists!
            openWallet();
            ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);
            //Was the wallet opened?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                //Use wallet data to try login
                QMap<QString, QString> values;
                m_wallet->readMap(QLatin1String(kWalletEntryName), values);
                QString username = values.value(QLatin1String(kWalletEntryUsername));
                QString password = values.value(QLatin1String(kWalletEntryPassword));

                if (!username.isEmpty() && !password.isEmpty()) {
                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setText(password);
                }
            }
        } else if (kWalletEntryExists(QLatin1String(konquerorKWalletEntryName))) {
            //If the DrKonqi entry is empty, but a Konqueror entry exists, use and copy it.
            openWallet();
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                //Fetch Konqueror data
                QMap<QString, QString> values;
                m_wallet->readMap(QLatin1String(konquerorKWalletEntryName), values);
                QString username = values.value(QLatin1String(konquerorKWalletEntryUsername));
                QString password = values.value(QLatin1String(konquerorKWalletEntryPassword));

                if (!username.isEmpty() && !password.isEmpty()) {
                    //Copy to DrKonqi own entries
                    values.clear();
                    values.insert(QLatin1String(kWalletEntryUsername), username);
                    values.insert(QLatin1String(kWalletEntryPassword), password);
                    m_wallet->writeMap(QLatin1String(kWalletEntryName), values);

                    ui.m_savePasswordCheckBox->setCheckState(Qt::Checked);

                    ui.m_userEdit->setText(username);
                    ui.m_passwordEdit->setText(password);
                }
            }

        }
    }
}

bool BugzillaLoginPage::canSetCookies()
{
    QDBusInterface kded(QLatin1String("org.kde.kded5"),
                        QLatin1String("/kded"),
                        QLatin1String("org.kde.kded5"));
    QDBusReply<bool> kcookiejarLoaded = kded.call(QLatin1String("loadModule"),
                                                  QLatin1String("kcookiejar"));
    if (!kcookiejarLoaded.isValid()) {
        KMessageBox::error(this, i18n("Failed to communicate with kded. Make sure it is running."));
        return false;
    } else if (!kcookiejarLoaded.value()) {
        KMessageBox::error(this, i18n("Failed to load KCookieServer. Check your KDE installation."));
        return false;
    }


    QDBusInterface kcookiejar(QLatin1String("org.kde.kded5"),
                              QLatin1String("/modules/kcookiejar"),
                              QLatin1String("org.kde.KCookieServer"));
    QDBusReply<QString> advice = kcookiejar.call(QLatin1String("getDomainAdvice"),
                                                 QLatin1String(KDE_BUGZILLA_URL));

    if (!advice.isValid()) {
        KMessageBox::error(this, i18n("Failed to communicate with KCookieServer."));
        return false;
    }

    qDebug() << "Got reply from KCookieServer:" << advice.value();

    if (advice.value() == QLatin1String("Reject")) {
        QString msg = i18nc("@info 1 is the bugzilla website url",
                            "Cookies are not allowed in your KDE network settings. In order to "
                            "proceed, you need to allow %1 to set cookies.", KDE_BUGZILLA_URL);

        KGuiItem yesItem = KStandardGuiItem::yes();
        yesItem.setText(i18nc("@action:button 1 is the bugzilla website url",
                              "Allow %1 to set cookies", KDE_BUGZILLA_URL));

        KGuiItem noItem = KStandardGuiItem::no();
        noItem.setText(i18nc("@action:button do not allow the bugzilla website "
                             "to set cookies", "No, do not allow"));

        if (KMessageBox::warningYesNo(this, msg, QString(), yesItem, noItem) == KMessageBox::Yes) {
            QDBusReply<bool> success = kcookiejar.call(QLatin1String("setDomainAdvice"),
                                                       QLatin1String(KDE_BUGZILLA_URL),
                                                       QLatin1String("Accept"));
            if (!success.isValid() || !success.value()) {
                qWarning() << "Failed to set domain advice in KCookieServer";
                return false;
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

    return true;
}

void BugzillaLoginPage::loginClicked()
{
    if (!(ui.m_userEdit->text().isEmpty() || ui.m_passwordEdit->text().isEmpty())) {

        if (!canSetCookies()) {
            return;
        }

        ui.m_loginButton->setEnabled(false);

        ui.m_userLabel->setEnabled(false);
        ui.m_passwordLabel->setEnabled(false);

        ui.m_userEdit->setEnabled(false);
        ui.m_passwordEdit->setEnabled(false);
        ui.m_savePasswordCheckBox->setEnabled(false);

        if (ui.m_savePasswordCheckBox->checkState()==Qt::Checked) { //Wants to save data
            if (!m_wallet) {
                openWallet();
            }
            //Got wallet open ?
            if (m_wallet) {
                m_wallet->setFolder(KWallet::Wallet::FormDataFolder());

                QMap<QString, QString> values;
                values.insert(QLatin1String(kWalletEntryUsername), ui.m_userEdit->text());
                values.insert(QLatin1String(kWalletEntryPassword), ui.m_passwordEdit->text());
                m_wallet->writeMap(QLatin1String(kWalletEntryName), values);
            }

        } else { //User doesn't want to save or wants to remove.
            if (kWalletEntryExists(QLatin1String(kWalletEntryName))) {
                if (!m_wallet) {
                    openWallet();
                }
                //Got wallet open ?
                if (m_wallet) {
                    m_wallet->setFolder(KWallet::Wallet::FormDataFolder());
                    m_wallet->removeEntry(QLatin1String(kWalletEntryName));
                }
            }
        }

        ui.m_statusWidget->setBusy(i18nc("@info:status '1' is a url, '2' the username",
                                      "Performing login at %1 as %2...",
                                      QLatin1String(KDE_BUGZILLA_SHORT_URL), ui.m_userEdit->text()));

        bugzillaManager()->tryLogin(ui.m_userEdit->text(), ui.m_passwordEdit->text());
    } else {
        loginFinished(false);
    }
}

void BugzillaLoginPage::loginFinished(bool logged)
{
    if (logged) {
        emitCompleteChanged();

        aboutToShow();
        if (m_wallet) {
            if (m_wallet->isOpen() && !m_walletWasOpenedBefore) {
                m_wallet->lockWallet();
            }
        }

        emit loggedTurnToNextPage();
    } else {
        ui.m_statusWidget->setIdle(i18nc("@info:status","<b>Error: Invalid username or "
                                                                                  "password</b>"));

        ui.m_loginButton->setEnabled(true);

        ui.m_userEdit->setEnabled(true);
        ui.m_passwordEdit->setEnabled(true);
        ui.m_savePasswordCheckBox->setEnabled(true);

        ui.m_userEdit->setFocus(Qt::OtherFocusReason);
    }
}

BugzillaLoginPage::~BugzillaLoginPage()
{
    //Close wallet if we close the assistant in this step
    if (m_wallet) {
        if (m_wallet->isOpen() && !m_walletWasOpenedBefore) {
            m_wallet->lockWallet();
        }
        delete m_wallet;
    }
}

//END BugzillaLoginPage

//BEGIN BugzillaInformationPage

BugzillaInformationPage::BugzillaInformationPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        m_textsOK(false), m_distributionComboSetup(false), m_distroComboVisible(false),
        m_requiredCharacters(1)
{
    ui.setupUi(this);
    m_textCompleteBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    ui.horizontalLayout_2->addWidget(m_textCompleteBar);

    connect(ui.m_titleEdit, &KLineEdit::textChanged, this, &BugzillaInformationPage::checkTexts);
    connect(ui.m_detailsEdit, &QTextEdit::textChanged, this, &BugzillaInformationPage::checkTexts);

    connect(ui.m_titleLabel, &QLabel::linkActivated, this, &BugzillaInformationPage::showTitleExamples);
    connect(ui.m_detailsLabel, &QLabel::linkActivated, this, &BugzillaInformationPage::showDescriptionHelpExamples);

    ui.m_compiledSourcesCheckBox->setChecked(
                                    DrKonqi::systemInformation()->compiledSources());

}

void BugzillaInformationPage::aboutToShow()
{
    if (!m_distributionComboSetup) {
        //Autodetecting distro failed ?
        if (DrKonqi::systemInformation()->bugzillaPlatform() == QLatin1String("unspecified")) {
            m_distroComboVisible = true;
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Unspecified"),"unspecified");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Archlinux"), "Archlinux Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Chakra"), "Chakra");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian stable"), "Debian stable");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian testing"), "Debian testing");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Debian unstable"), "Debian unstable");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Exherbo"), "Exherbo Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Fedora"), "Fedora RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Gentoo"), "Gentoo Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Mageia"), "Mageia RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Mandriva"), "Mandriva RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "OpenSUSE"), "openSUSE RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Pardus"), "Pardus Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "RedHat"), "RedHat RPMs");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Slackware"), "Slackware Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Ubuntu (and derivatives)"),
                                                   "Ubuntu Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "FreeBSD (Ports)"), "FreeBSD Ports");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "NetBSD (pkgsrc)"), "NetBSD pkgsrc");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "OpenBSD"), "OpenBSD Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Mac OS X"), "MacPorts Packages");
            ui.m_distroChooserCombo->addItem(i18nc("@label:listbox KDE distribution method",
                                                   "Solaris"), "Solaris Packages");

            //Restore previously selected bugzilla platform (distribution)
            KConfigGroup config(KSharedConfig::openConfig(), "BugzillaInformationPage");
            QString entry = config.readEntry("BugzillaPlatform","unspecified");
            int index = ui.m_distroChooserCombo->findData(entry);
            if ( index == -1 ) index = 0;
            ui.m_distroChooserCombo->setCurrentIndex(index);
        } else {
            ui.m_distroChooserCombo->setVisible(false);
        }
        m_distributionComboSetup = true;
    }

    //Calculate the minimum number of characters required for a description
    //If creating a new report: minimum 40, maximum 80
    //If attaching to an existent report: minimum 30, maximum 50
    int multiplier = (reportInterface()->attachToBugNumber() == 0) ? 10 : 5;
    m_requiredCharacters = 20 + (reportInterface()->selectedOptionsRating() * multiplier);

    //Fill the description textedit with some headings:
    QString descriptionTemplate;
    if (ui.m_detailsEdit->toPlainText().isEmpty()) {
        if (reportInterface()->userCanProvideActionsAppDesktop()) {
            descriptionTemplate += "- What I was doing when the application crashed:\n\n";
        }
        if (reportInterface()->userCanProvideUnusualBehavior()) {
            descriptionTemplate += "- Unusual behavior I noticed:\n\n";
        }
        if (reportInterface()->userCanProvideApplicationConfigDetails()) {
            descriptionTemplate += "- Custom settings of the application:\n\n";
        }
        ui.m_detailsEdit->setText(descriptionTemplate);
    }

    checkTexts(); //May be the options (canDetail) changed and we need to recheck
}

int BugzillaInformationPage::currentDescriptionCharactersCount()
{
    QString description = ui.m_detailsEdit->toPlainText();

    //Do not count template messages, and other misc chars
    description.remove("What I was doing when the application crashed");
    description.remove("Unusual behavior I noticed");
    description.remove("Custom settings of the application");
    description.remove('\n');
    description.remove('-');
    description.remove(':');
    description.remove(' ');

    return description.size();
}

void BugzillaInformationPage::checkTexts()
{
    //If attaching this report to an existing one then the title is not needed
    bool showTitle = (reportInterface()->attachToBugNumber() == 0);
    ui.m_titleEdit->setVisible(showTitle);
    ui.m_titleLabel->setVisible(showTitle);

    bool ok = !((ui.m_titleEdit->isVisible() && ui.m_titleEdit->text().isEmpty())
        || ui.m_detailsEdit->toPlainText().isEmpty());

    QString message;
    int percent = currentDescriptionCharactersCount() * 100 / m_requiredCharacters;
    if (percent >= 100) {
        percent = 100;
        message = i18nc("the minimum required length of a text was reached",
                        "Minimum length reached");
    } else {
        message = i18nc("the minimum required length of a text wasn't reached yet",
                        "Provide more information");
    }
    m_textCompleteBar->setValue(percent);
    m_textCompleteBar->setText(message);

    if (ok != m_textsOK) {
        m_textsOK = ok;
        emitCompleteChanged();
    }
}

bool BugzillaInformationPage::showNextPage()
{
    checkTexts();

    if (m_textsOK) {
        bool detailsShort = currentDescriptionCharactersCount() < m_requiredCharacters;

        if (detailsShort) {
            //The user input is less than we want.... encourage to write more
            QString message = i18nc("@info","The description about the crash details does not provide "
                                        "enough information yet.<br /><br />");

            message += ' ' + i18nc("@info","The amount of required information is proportional to "
                                        "the quality of the other information like the backtrace "
                                        "or the reproducibility rate."
                                        "<br /><br />");

            if (reportInterface()->userCanProvideActionsAppDesktop()
                || reportInterface()->userCanProvideUnusualBehavior()
                || reportInterface()->userCanProvideApplicationConfigDetails()) {
                message += ' ' + i18nc("@info","Previously, you told DrKonqi that you could provide some "
                                        "contextual information. Try writing more details about your situation. "
                                        "(even little ones could help us.)<br /><br />");
            }

            message += ' ' + i18nc("@info","If you cannot provide more information, your report "
                                    "will probably waste developers' time. Can you tell us more?");

            KGuiItem yesItem = KStandardGuiItem::yes();
            yesItem.setText(i18n("Yes, let me add more information"));

            KGuiItem noItem = KStandardGuiItem::no();
            noItem.setText(i18n("No, I cannot add any other information"));

            if (KMessageBox::warningYesNo(this, message,
                                           i18nc("@title:window","We need more information"),
                                           yesItem, noItem)
                                            == KMessageBox::No) {
                //Request the assistant to close itself (it will prompt for confirmation anyways)
                assistant()->close();
                return false;
            }
        } else {
            return true;
        }
    }

    return false;
}

bool BugzillaInformationPage::isComplete()
{
    return m_textsOK;
}

void BugzillaInformationPage::aboutToHide()
{
    //Save fields data
    reportInterface()->setTitle(ui.m_titleEdit->text());
    reportInterface()->setDetailText(ui.m_detailsEdit->toPlainText());

    if (m_distroComboVisible) {
        //Save bugzilla platform (distribution)
        QString bugzillaPlatform = ui.m_distroChooserCombo->itemData(
                                        ui.m_distroChooserCombo->currentIndex()).toString();
        KConfigGroup config(KSharedConfig::openConfig(), "BugzillaInformationPage");
        config.writeEntry("BugzillaPlatform", bugzillaPlatform);
        DrKonqi::systemInformation()->setBugzillaPlatform(bugzillaPlatform);
    }
    bool compiledFromSources = ui.m_compiledSourcesCheckBox->checkState() == Qt::Checked;
    DrKonqi::systemInformation()->setCompiledSources(compiledFromSources);

}

void BugzillaInformationPage::showTitleExamples()
{
    QString titleExamples = xi18nc("@info:tooltip examples of good bug report titles",
          "<strong>Examples of good titles:</strong><nl />\"Plasma crashed after adding the Notes "
          "widget and writing on it\"<nl />\"Konqueror crashed when accessing the Facebook "
          "application 'X'\"<nl />\"Kopete suddenly closed after resuming the computer and "
          "talking to a MSN buddy\"<nl />\"Kate closed while editing a log file and pressing the "
          "Delete key a couple of times\"");
    QToolTip::showText(QCursor::pos(), titleExamples);
}

void BugzillaInformationPage::showDescriptionHelpExamples()
{
    QString descriptionHelp = i18nc("@info:tooltip help and examples of good bug descriptions",
                                  "Describe in as much detail as possible the crash circumstances:");
    if (reportInterface()->userCanProvideActionsAppDesktop()) {
        descriptionHelp += "<br />" +
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Detail which actions were you taking inside and outside the "
                                 "application an instant before the crash.");
    }
    if (reportInterface()->userCanProvideUnusualBehavior()) {
        descriptionHelp += "<br />" +
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Note if you noticed any unusual behavior in the application "
                                 "or in the whole environment.");
    }
    if (reportInterface()->userCanProvideApplicationConfigDetails()) {
        descriptionHelp += "<br />" +
                           i18nc("@info:tooltip help and examples of good bug descriptions",
                                 "- Note any non-default configuration in the application.");
        if (reportInterface()->appDetailsExamples()->hasExamples()) {
            descriptionHelp += ' ' +
                               i18nc("@info:tooltip examples of configuration details. "
                                     "the examples are already translated",
                                     "Examples: %1",
                                     reportInterface()->appDetailsExamples()->examples());
        }
    }
    QToolTip::showText(QCursor::pos(), descriptionHelp);
}

//END BugzillaInformationPage

//BEGIN BugzillaPreviewPage

BugzillaPreviewPage::BugzillaPreviewPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent)
{
    ui.setupUi(this);
}

void BugzillaPreviewPage::aboutToShow()
{
    ui.m_previewEdit->setText(reportInterface()->generateReportFullText(true));
}

//END BugzillaPreviewPage

//BEGIN BugzillaSendPage

BugzillaSendPage::BugzillaSendPage(ReportAssistantDialog * parent)
        : ReportAssistantPage(parent),
        m_contentsDialog(0)
{
    connect(reportInterface(), SIGNAL(reportSent(int)), this, SLOT(sent(int)));
    connect(reportInterface(), SIGNAL(sendReportError(QString,QString)), this, SLOT(sendError(QString,QString)));

    ui.setupUi(this);

    KGuiItem::assign(ui.m_retryButton, KGuiItem2(i18nc("@action:button", "Retry..."),
                                              QIcon::fromTheme("view-refresh"),
                                              i18nc("@info:tooltip", "Use this button to retry "
                                                  "sending the crash report if it failed before.")));

    KGuiItem::assign(ui.m_showReportContentsButton,
                    KGuiItem2(i18nc("@action:button", "Sho&w Contents of the Report"),
                            QIcon::fromTheme("document-preview"),
                            i18nc("@info:tooltip", "Use this button to show the generated "
                            "report information about this crash.")));
    connect(ui.m_showReportContentsButton, &QPushButton::clicked, this, &BugzillaSendPage::openReportContents);

    ui.m_retryButton->setVisible(false);
    connect(ui.m_retryButton, SIGNAL(clicked()), this , SLOT(retryClicked()));

    ui.m_launchPageOnFinish->setVisible(false);
    ui.m_restartAppOnFinish->setVisible(false);

    connect(assistant(), SIGNAL(user1Clicked()), this, SLOT(finishClicked()));
}

void BugzillaSendPage::retryClicked()
{
    ui.m_retryButton->setEnabled(false);
    aboutToShow();
}

void BugzillaSendPage::aboutToShow()
{
    ui.m_statusWidget->setBusy(i18nc("@info:status","Sending crash report... (please wait)"));
    reportInterface()->sendBugReport();
}

void BugzillaSendPage::sent(int bug_id)
{
    ui.m_statusWidget->setVisible(false);
    ui.m_retryButton->setEnabled(false);
    ui.m_retryButton->setVisible(false);

    ui.m_showReportContentsButton->setVisible(false);

    ui.m_launchPageOnFinish->setVisible(true);
    ui.m_restartAppOnFinish->setVisible(!DrKonqi::crashedApplication()->hasBeenRestarted());
    ui.m_restartAppOnFinish->setChecked(false);

    reportUrl = bugzillaManager()->urlForBug(bug_id);
    ui.m_finishedLabel->setText(xi18nc("@info/rich","Crash report sent.<nl/>"
                                             "URL: <link>%1</link><nl/>"
                                             "Thank you for being part of KDE. "
                                             "You can now close this window.", reportUrl));

    emit finished(false);
}

void BugzillaSendPage::sendError(const QString & errorString, const QString & extendedMessage)
{
    ui.m_statusWidget->setIdle(xi18nc("@info:status","Error sending the crash report:  "
                                  "<message>%1.</message>", errorString));

    ui.m_retryButton->setEnabled(true);
    ui.m_retryButton->setVisible(true);

    if (!extendedMessage.isEmpty()) {
        new UnhandledErrorDialog(this,errorString, extendedMessage);
    }
}

void BugzillaSendPage::finishClicked()
{
    if (ui.m_launchPageOnFinish->isChecked() && !reportUrl.isEmpty()) {
        KToolInvocation::invokeBrowser(reportUrl);
    }
    if (ui.m_restartAppOnFinish->isChecked()) {
        DrKonqi::crashedApplication()->restart();
    }
}

void BugzillaSendPage::openReportContents()
{
    if (!m_contentsDialog)
    {
        QString report = reportInterface()->generateReportFullText(false) + QLatin1Char('\n') +
                            i18nc("@info report to KDE bugtracker address","Report to %1",
                                  DrKonqi::crashedApplication()->bugReportAddress());
        m_contentsDialog = new ReportInformationDialog(report);
    }
    m_contentsDialog->show();
    m_contentsDialog->raise();
    m_contentsDialog->activateWindow();
}

//END BugzillaSendPage

/* Dialog for Unhandled Bugzilla Errors */
/* The user can save the bugzilla html output to check the error and/or to report this as a DrKonqi bug */

//BEGIN UnhandledErrorDialog

UnhandledErrorDialog::UnhandledErrorDialog(QWidget * parent, const QString & error, const QString & extendedMessage)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Unhandled Bugzilla Error"));
    setWindowModality(Qt::ApplicationModal);

    QPushButton* saveButton = new QPushButton(this);
    saveButton->setText(i18nc("@action:button save html to a file","Save to a file"));
    saveButton->setIcon(QIcon::fromTheme("document-save"));
    connect(saveButton, &QPushButton::clicked, this, &UnhandledErrorDialog::saveErrorMessage);

    setAttribute(Qt::WA_DeleteOnClose);

    KWebView * htmlView = new KWebView(this);

    QLabel * iconLabel = new QLabel(this);
    iconLabel->setFixedSize(32, 32);
    iconLabel->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(32, 32));

    QLabel * mainLabel = new QLabel(this);
    mainLabel->setWordWrap(true);
    mainLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    QHBoxLayout * titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(5,2,5,2);
    titleLayout->setSpacing(5);
    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(mainLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout * layout = new QVBoxLayout();
    layout->addLayout(titleLayout);
    layout->addWidget(htmlView);
    layout->addWidget(buttonBox);
    setLayout(layout);

    m_extendedHTMLError = extendedMessage;
    mainLabel->setText(i18nc("@label", "There was an unhandled Bugzilla error: %1.<br />"
                              "Below is the HTML that DrKonqi received. "
                              "Try to perform the action again or save this error page "
                              "to submit a bug against DrKonqi.").arg(error));
    htmlView->setHtml(extendedMessage);

    setMinimumSize(QSize(550, 350));
    resize(minimumSize());

    show();
}

void UnhandledErrorDialog::saveErrorMessage()
{
    QString defaultName = QLatin1String("drkonqi-unhandled-bugzilla-error.html");
    QWeakPointer<QFileDialog> dlg = new QFileDialog(this);
    dlg.data()->selectFile(defaultName);
    dlg.data()->setWindowTitle(i18nc("@title:window","Select Filename"));
    dlg.data()->setAcceptMode(QFileDialog::AcceptSave);
    dlg.data()->setFileMode(QFileDialog::AnyFile);
    dlg.data()->setConfirmOverwrite(true);
    if ( dlg.data()->exec() )
    {
        if (dlg.isNull()) {
            //Dialog closed externally (ex. via DBus)
            return;
        }

        QUrl fileUrl;
        if(!dlg.data()->selectedUrls().isEmpty())
            fileUrl = dlg.data()->selectedUrls().first();
        delete dlg.data();

        if (fileUrl.isValid()) {
            QTemporaryFile tf;
            if (tf.open()) {
                QTextStream ts(&tf);
                ts << m_extendedHTMLError;
                ts.flush();
            } else {
                KMessageBox::sorry(this, xi18nc("@info","Cannot open file <filename>%1</filename> "
                                                "for writing.", tf.fileName()));
                return;
            }

            KIO::FileCopyJob* job = KIO::file_copy(tf.fileName(), fileUrl);
            KJobWidgets::setWindow(job, this);
            if (!job->exec()) {
                KMessageBox::sorry(this, job->errorString());
            }
        }
    }
    else
        delete dlg.data();

}

//END UnhandledErrorDialog
