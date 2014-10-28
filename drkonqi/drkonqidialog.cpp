/*******************************************************************
* drkonqidialog.cpp
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

#include "drkonqidialog.h"

#include <KToolInvocation>
#include <KWindowConfig>
#include <KLocalizedString>

#include <QStandardPaths>
#include <QMenu>
#include <QDialogButtonBox>
#include <QDebug>
#include <QDesktopServices>

#include "drkonqi.h"
#include "backtracewidget.h"
#include "aboutbugreportingdialog.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "debuggerlaunchers.h"
#include "drkonqi_globals.h"
#include "config-drkonqi.h"
#if HAVE_XMLRPCCLIENT
    #include "bugzillaintegration/reportassistantdialog.h"
#endif

static const char ABOUT_BUG_REPORTING_URL[] = "#aboutbugreporting";
static const char DRKONQI_REPORT_BUG_URL[] =
    KDE_BUGZILLA_URL "enter_bug.cgi?product=drkonqi&format=guided";

DrKonqiDialog::DrKonqiDialog(QWidget * parent) :
        QDialog(parent),
        m_aboutBugReportingDialog(0),
        m_backtraceWidget(0)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    //Setting dialog title and icon
    setWindowTitle(DrKonqi::crashedApplication()->name());
    setWindowIcon(QIcon::fromTheme("tools-report-bug"));

    QVBoxLayout* l = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);
    l->addWidget(m_tabWidget);
    m_buttonBox = new QDialogButtonBox(this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::rejected);
    l->addWidget(m_buttonBox);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &DrKonqiDialog::tabIndexChanged);

    buildIntroWidget();
    m_tabWidget->addTab(m_introWidget, i18nc("@title:tab general information", "&General"));

    m_backtraceWidget = new BacktraceWidget(DrKonqi::debuggerManager()->backtraceGenerator(), this);
    m_backtraceWidget->setMinimumSize(QSize(575, 240));
    m_tabWidget->addTab(m_backtraceWidget, i18nc("@title:tab", "&Developer Information"));

    buildDialogButtons();

    setMinimumSize(QSize(640,320));
    resize(minimumSize());
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    KWindowConfig::restoreWindowSize(windowHandle(), config);
    setLayout(l);
}

DrKonqiDialog::~DrKonqiDialog()
{
    KConfigGroup config(KSharedConfig::openConfig(), "General");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void DrKonqiDialog::tabIndexChanged(int index)
{
    if (index == m_tabWidget->indexOf(m_backtraceWidget)) {
        m_backtraceWidget->generateBacktrace();
    }
}

void DrKonqiDialog::buildIntroWidget()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    m_introWidget = new QWidget(this);
    ui.setupUi(m_introWidget);

    ui.titleLabel->setText(xi18nc("@info", "<para>We are sorry, <application>%1</application> "
                                               "closed unexpectedly.</para>", crashedApp->name()));

    QString reportMessage;
    if (!crashedApp->bugReportAddress().isEmpty()) {
        if (crashedApp->fakeExecutableBaseName() == QLatin1String("drkonqi")) { //Handle own crashes
            reportMessage = xi18nc("@info", "<para>As the Crash Handler itself has failed, the "
                                            "automatic reporting process is disabled to reduce the "
                                            "risks of failing again.<nl /><nl />"
                                            "Please, <link url='%1'>manually report</link> this error "
                                            "to the KDE bug tracking system. Do not forget to include "
                                            "the backtrace from the <interface>Developer Information</interface> "
                                            "tab.</para>",
                                            QLatin1String(DRKONQI_REPORT_BUG_URL));
        } else if (DrKonqi::isSafer()) {
            reportMessage = xi18nc("@info", "<para>The reporting assistant is disabled because "
                                            "the crash handler dialog was started in safe mode."
                                            "<nl />You can manually report this bug to %1 "
                                            "(including the backtrace from the "
                                            "<interface>Developer Information</interface> "
                                            "tab.)</para>", crashedApp->bugReportAddress());
        } else {
            reportMessage = xi18nc("@info", "<para>You can help us improve KDE Software by reporting "
                                            "this error.<nl /><link url='%1'>Learn "
                                            "more about bug reporting.</link></para><para><note>It is "
                                            "safe to close this dialog if you do not want to report "
                                            "this bug.</note></para>",
                                            QLatin1String(ABOUT_BUG_REPORTING_URL));
        }
    } else {
        reportMessage = xi18nc("@info", "<para>You cannot report this error, because "
                                        "<application>%1</application> does not provide a bug reporting "
                                        "address.</para>",
                                        crashedApp->name()
                                        );
    }
    ui.infoLabel->setText(reportMessage);
    connect(ui.infoLabel, &QLabel::linkActivated, this, &DrKonqiDialog::linkActivated);

    ui.iconLabel->setPixmap(
                        QPixmap(QStandardPaths::locate(QStandardPaths::DataLocation, QLatin1String("pics/crash.png"))));

    ui.detailsTitleLabel->setText(QString("<strong>%1</strong>").arg(i18nc("@label","Details:")));

    ui.detailsLabel->setText(xi18nc("@info Note the time information is divided into date and time parts",
                                            "<para>Executable: <application>%1"
                                            "</application> PID: %2 Signal: %3 (%4) "
                                            "Time: %5 %6</para>",
                                             crashedApp->fakeExecutableBaseName(),
                                             crashedApp->pid(),
                                             crashedApp->signalName(),
                                    #if defined(Q_OS_UNIX)
                                             crashedApp->signalNumber(),
                                    #else
                                             //windows uses weird big numbers for exception codes,
                                             //so it doesn't make sense to display them in decimal
                                             QString().sprintf("0x%8x", crashedApp->signalNumber()),
                                    #endif
                                             crashedApp->datetime().date().toString(Qt::DefaultLocaleShortDate),

                                             crashedApp->datetime().time().toString()
                                             ));
}

void DrKonqiDialog::buildDialogButtons()
{
    const CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    //Set dialog buttons
    m_buttonBox->setStandardButtons(QDialogButtonBox::Close);

    //Report bug button: User1
    QPushButton* reportButton = new QPushButton(m_buttonBox);
    KGuiItem2 reportItem(i18nc("@action:button", "Report &Bug"),
                         QIcon::fromTheme("tools-report-bug"),
                         i18nc("@info:tooltip", "Starts the bug report assistant."));
    KGuiItem::assign(reportButton, reportItem);
    m_buttonBox->addButton(reportButton, QDialogButtonBox::ActionRole);

    bool enableReportAssistant = !crashedApp->bugReportAddress().isEmpty() &&
                                 crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi") &&
                                 !DrKonqi::isSafer() &&
                                 HAVE_XMLRPCCLIENT;
    reportButton->setEnabled(enableReportAssistant);
    connect(reportButton, &QPushButton::clicked, this, &DrKonqiDialog::startBugReportAssistant);

    //Default debugger button and menu (only for developer mode): User2
    DebuggerManager *debuggerManager = DrKonqi::debuggerManager();
    m_debugButton = new QPushButton(m_buttonBox);
    KGuiItem2 debugItem(i18nc("@action:button this is the debug menu button label which contains the debugging applications",
                                               "&Debug"), QIcon::fromTheme("applications-development"),
                                               i18nc("@info:tooltip", "Starts a program to debug "
                                                     "the crashed application."));
    KGuiItem::assign(m_debugButton, debugItem);
    m_debugButton->setVisible(debuggerManager->showExternalDebuggers());
    m_buttonBox->addButton(m_debugButton, QDialogButtonBox::ActionRole);

    m_debugMenu = new QMenu(this);
    m_debugButton->setMenu(m_debugMenu);

    QList<AbstractDebuggerLauncher*> debuggers = debuggerManager->availableExternalDebuggers();
    foreach(AbstractDebuggerLauncher *launcher, debuggers) {
        addDebugger(launcher);
    }

    connect(debuggerManager, &DebuggerManager::externalDebuggerAdded, this, &DrKonqiDialog::addDebugger);
    connect(debuggerManager, &DebuggerManager::externalDebuggerRemoved, this, &DrKonqiDialog::removeDebugger);
    connect(debuggerManager, &DebuggerManager::debuggerRunning, this, &DrKonqiDialog::enableDebugMenu);

    //Restart application button
    KGuiItem2 restartItem(i18nc("@action:button", "&Restart Application"),
              QIcon::fromTheme("system-reboot"),
              i18nc("@info:tooltip", "Use this button to restart "
              "the crashed application."));
    m_restartButton = new QPushButton(m_buttonBox);
    KGuiItem::assign(m_restartButton, restartItem);
    m_restartButton->setEnabled(!crashedApp->hasBeenRestarted() &&
                                 crashedApp->fakeExecutableBaseName() != QLatin1String("drkonqi"));
    m_buttonBox->addButton(m_restartButton, QDialogButtonBox::ActionRole);
    connect(m_restartButton, SIGNAL(clicked(bool)), crashedApp, SLOT(restart()));
    connect(crashedApp, SIGNAL(restarted(bool)), this, SLOT(applicationRestarted(bool)));

    //Close button
    QString tooltipText = i18nc("@info:tooltip",
                                "Close this dialog (you will lose the crash information.)");
    m_buttonBox->button(QDialogButtonBox::Close)->setToolTip(tooltipText);
    m_buttonBox->button(QDialogButtonBox::Close)->setWhatsThis(tooltipText);
    m_buttonBox->button(QDialogButtonBox::Close)->setFocus();
}

void DrKonqiDialog::addDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = new QAction(QIcon::fromTheme("applications-development"),
                                  i18nc("@action:inmenu 1 is the debugger name",
                                         "Debug in %1",
                                         launcher->name()), m_debugMenu);
    m_debugMenu->addAction(action);
    connect(action, SIGNAL(triggered()), launcher, SLOT(start()));
    m_debugMenuActions.insert(launcher, action);
}

void DrKonqiDialog::removeDebugger(AbstractDebuggerLauncher *launcher)
{
    QAction *action = m_debugMenuActions.take(launcher);
    if ( action ) {
        m_debugMenu->removeAction(action);
        action->deleteLater();
    } else {
        qWarning() << "Invalid launcher";
    }
}

void DrKonqiDialog::enableDebugMenu(bool debuggerRunning)
{
    m_debugButton->setEnabled(!debuggerRunning);
}

void DrKonqiDialog::startBugReportAssistant()
{
#if HAVE_XMLRPCCLIENT
    ReportAssistantDialog * bugReportAssistant = new ReportAssistantDialog();
    bugReportAssistant->show();
    connect(bugReportAssistant, SIGNAL(finished()), SLOT([] () { close(); }));
    hide();
#endif
}

void DrKonqiDialog::linkActivated(const QString& link)
{
    if (link == QLatin1String(ABOUT_BUG_REPORTING_URL)) {
        showAboutBugReporting();
    } else if (link == QLatin1String(DRKONQI_REPORT_BUG_URL)) {
        QDesktopServices::openUrl(QUrl(link));
    } else if (link.startsWith(QLatin1String("http"))) {
        qWarning() << "unexpected link";
        QDesktopServices::openUrl(QUrl(link));
    }
}

void DrKonqiDialog::showAboutBugReporting()
{
    if (!m_aboutBugReportingDialog) {
        m_aboutBugReportingDialog = new AboutBugReportingDialog();
        connect(this, &DrKonqiDialog::destroyed, m_aboutBugReportingDialog.data(), &AboutBugReportingDialog::close);
    }
    m_aboutBugReportingDialog->show();
    m_aboutBugReportingDialog->raise();
    m_aboutBugReportingDialog->activateWindow();
}

void DrKonqiDialog::applicationRestarted(bool success)
{
    m_restartButton->setEnabled(!success);
}
