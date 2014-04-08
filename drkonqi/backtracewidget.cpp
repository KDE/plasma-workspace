/*******************************************************************
* backtracewidget.cpp
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

#include "backtracewidget.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>

#include <KMessageBox>
#include <KLocalizedString>
#include <KToolInvocation>

#include "drkonqi.h"
#include "backtraceratingwidget.h"
#include "crashedapplication.h"
#include "backtracegenerator.h"
#include "parser/backtraceparser.h"
#include "drkonqi_globals.h"
#include "debuggermanager.h"
#include "gdbhighlighter.h"

static const char extraDetailsLabelMargin[] = " margin: 5px; ";

BacktraceWidget::BacktraceWidget(BacktraceGenerator *generator, QWidget *parent,
    bool showToggleBacktrace) :
        QWidget(parent),
        m_btGenerator(generator),
        m_highlighter(0)
{
    ui.setupUi(this);

    //Debug package installer
    m_debugPackageInstaller = new DebugPackageInstaller(this);
    connect(m_debugPackageInstaller, SIGNAL(error(QString)), this, SLOT(debugPackageError(QString)));
    connect(m_debugPackageInstaller, SIGNAL(packagesInstalled()), this, SLOT(regenerateBacktrace()));
    connect(m_debugPackageInstaller, SIGNAL(canceled()), this, SLOT(debugPackageCanceled()));

    connect(m_btGenerator, SIGNAL(done()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(someError()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(failedToStart()) , this, SLOT(loadData()));
    connect(m_btGenerator, SIGNAL(newLine(QString)) , this, SLOT(backtraceNewLine(QString)));

    connect(ui.m_extraDetailsLabel, SIGNAL(linkActivated(QString)), this,
            SLOT(extraDetailsLinkActivated(QString)));
    ui.m_extraDetailsLabel->setVisible(false);
    ui.m_extraDetailsLabel->setStyleSheet(QLatin1String(extraDetailsLabelMargin));

    //Setup the buttons
    KGuiItem::assign(ui.m_reloadBacktraceButton,
                KGuiItem2(i18nc("@action:button", "&Reload"),
                          QIcon::fromTheme("view-refresh"), i18nc("@info:tooltip", "Use this button to "
                          "reload the crash information (backtrace). This is useful when you have "
                          "installed the proper debug symbol packages and you want to obtain "
                          "a better backtrace.")));
    connect(ui.m_reloadBacktraceButton, SIGNAL(clicked()), this, SLOT(regenerateBacktrace()));

    KGuiItem::assign(ui.m_installDebugButton,
                KGuiItem2(i18nc("@action:button", "&Install Debug Symbols"),
                          QIcon::fromTheme("system-software-update"), i18nc("@info:tooltip", "Use this button to "
                          "install the missing debug symbols packages.")));
    ui.m_installDebugButton->setVisible(false);
    connect(ui.m_installDebugButton, SIGNAL(clicked()), this, SLOT(installDebugPackages()));

    KGuiItem::assign(ui.m_copyButton, KGuiItem2(QString(), QIcon::fromTheme("edit-copy"),
                                          i18nc("@info:tooltip", "Use this button to copy the "
                                                "crash information (backtrace) to the clipboard.")));
    connect(ui.m_copyButton, SIGNAL(clicked()) , this, SLOT(copyClicked()));
    ui.m_copyButton->setEnabled(false);

    KGuiItem::assign(ui.m_saveButton, KGuiItem2(QString(),
                                          QIcon::fromTheme("document-save"),
                                          i18nc("@info:tooltip", "Use this button to save the "
                                          "crash information (backtrace) to a file. This is useful "
                                          "if you want to take a look at it or to report the bug "
                                          "later.")));
    connect(ui.m_saveButton, SIGNAL(clicked()) , this, SLOT(saveClicked()));
    ui.m_saveButton->setEnabled(false);

    //Create the rating widget
    m_backtraceRatingWidget = new BacktraceRatingWidget(ui.m_statusWidget);
    ui.m_statusWidget->addCustomStatusWidget(m_backtraceRatingWidget);

    ui.m_statusWidget->setIdle(QString());

    //Do we need the "Show backtrace" toggle action ?
    if (!showToggleBacktrace) {
        ui.mainLayout->removeWidget(ui.m_toggleBacktraceCheckBox);
        ui.m_toggleBacktraceCheckBox->setVisible(false);
        toggleBacktrace(true);
    } else {
        //Generate help widget
        ui.m_backtraceHelpLabel->setText(
            i18n("<h2>What is a \"backtrace\" ?</h2><p>A backtrace basically describes what was "
                 "happening inside the application when it crashed, so the developers may track "
                 "down where the mess started. They may look meaningless to you, but they might "
                 "actually contain a wealth of useful information.<br />Backtraces are commonly "
                 "used during interactive and post-mortem debugging.</p>"));
        ui.m_backtraceHelpIcon->setPixmap(QIcon::fromTheme("help-hint").pixmap(48,48));
        connect(ui.m_toggleBacktraceCheckBox, SIGNAL(toggled(bool)), this,
                SLOT(toggleBacktrace(bool)));
        toggleBacktrace(false);
    }

    ui.m_backtraceEdit->setFont( QFontDatabase::systemFont(QFontDatabase::FixedFont) );
}

void BacktraceWidget::setAsLoading()
{
    //remove the syntax highlighter
    delete m_highlighter;
    m_highlighter = 0;

    //Set the widget as loading and disable all the action buttons
    ui.m_backtraceEdit->setText(i18nc("@info:status", "Loading..."));
    ui.m_backtraceEdit->setEnabled(false);

    ui.m_statusWidget->setBusy(i18nc("@info:status",
                                     "Generating backtrace... (this may take some time)"));
    m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);
    m_backtraceRatingWidget->setState(BacktraceGenerator::Loading);

    ui.m_extraDetailsLabel->setVisible(false);
    ui.m_extraDetailsLabel->clear();

    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(false);

    ui.m_copyButton->setEnabled(false);
    ui.m_saveButton->setEnabled(false);
}

//Force backtrace generation
void BacktraceWidget::regenerateBacktrace()
{
    setAsLoading();

    if (!DrKonqi::debuggerManager()->debuggerIsRunning()) {
        m_btGenerator->start();
    } else {
        anotherDebuggerRunning();
    }

    emit stateChanged();
}

void BacktraceWidget::generateBacktrace()
{
    if (m_btGenerator->state() == BacktraceGenerator::NotLoaded) {
        //First backtrace generation
        regenerateBacktrace();
    } else if (m_btGenerator->state() == BacktraceGenerator::Loading) {
        //Set in loading state, the widget will catch the backtrace events anyway
        setAsLoading();
        emit stateChanged();
    } else {
        //*Finished* states
        setAsLoading();
        emit stateChanged();
        //Load already generated information
        loadData();
    }
}

void BacktraceWidget::anotherDebuggerRunning()
{
    //As another debugger is running, we should disable the actions and notify the user
    ui.m_backtraceEdit->setEnabled(false);
    ui.m_backtraceEdit->setText(i18nc("@info", "Another debugger is currently debugging the "
                                   "same application. The crash information could not be fetched."));
    m_backtraceRatingWidget->setState(BacktraceGenerator::Failed);
    m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);
    ui.m_statusWidget->setIdle(i18nc("@info:status", "The crash information could not be fetched."));
    ui.m_extraDetailsLabel->setVisible(true);
    ui.m_extraDetailsLabel->setText(xi18nc("@info/rich", "Another debugging process is attached to "
                                    "the crashed application. Therefore, the DrKonqi debugger cannot "
                                    "fetch the backtrace. Please close the other debugger and "
                                    "click <interface>Reload</interface>."));
    ui.m_installDebugButton->setVisible(false);
    ui.m_reloadBacktraceButton->setEnabled(true);
}

void BacktraceWidget::loadData()
{
    //Load the backtrace data from the generator
    m_backtraceRatingWidget->setState(m_btGenerator->state());

    if (m_btGenerator->state() == BacktraceGenerator::Loaded) {
        ui.m_backtraceEdit->setEnabled(true);
        ui.m_backtraceEdit->setPlainText(m_btGenerator->backtrace());

        // scroll to crash
        QTextCursor crashCursor = ui.m_backtraceEdit->document()->find("[KCrash Handler]");
        if (!crashCursor.isNull()) {
            crashCursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
            ui.m_backtraceEdit->verticalScrollBar()->setValue(ui.m_backtraceEdit->cursorRect(crashCursor).top());
        }

        // highlight if possible
        if (m_btGenerator->debugger().codeName() == "gdb") {
            m_highlighter = new GdbHighlighter(ui.m_backtraceEdit->document(),
                                               m_btGenerator->parser()->parsedBacktraceLines());
        }

        BacktraceParser * btParser = m_btGenerator->parser();
        m_backtraceRatingWidget->setUsefulness(btParser->backtraceUsefulness());

        //Generate the text to put in the status widget (backtrace usefulness)
        QString usefulnessText;
        switch (btParser->backtraceUsefulness()) {
        case BacktraceParser::ReallyUseful:
            usefulnessText = i18nc("@info", "The generated crash information is useful");
            break;
        case BacktraceParser::MayBeUseful:
            usefulnessText = i18nc("@info", "The generated crash information may be useful");
            break;
        case BacktraceParser::ProbablyUseless:
            usefulnessText = i18nc("@info", "The generated crash information is probably not useful");
            break;
        case BacktraceParser::Useless:
            usefulnessText = i18nc("@info", "The generated crash information is not useful");
            break;
        default:
            //let's hope nobody will ever see this... ;)
            usefulnessText = i18nc("@info", "The rating of this crash information is invalid. "
                                            "This is a bug in DrKonqi itself.");
            break;
        }
        ui.m_statusWidget->setIdle(usefulnessText);

        if (btParser->backtraceUsefulness() != BacktraceParser::ReallyUseful) {
            //Not a perfect bactrace, ask the user to try to improve it
            ui.m_extraDetailsLabel->setVisible(true);
            if (canInstallDebugPackages()) {
                //The script to install the debug packages is available
                ui.m_extraDetailsLabel->setText(xi18nc("@info/rich", "You can click the <interface>"
                                    "Install Debug Symbols</interface> button in order to automatically "
                                    "install the missing debugging information packages. If this method "
                                    "does not work: please read <link url='%1'>How to "
                                    "create useful crash reports</link> to learn how to get a useful "
                                    "backtrace; install the needed packages (<link url='%2'>"
                                    "list of files</link>) and click the "
                                    "<interface>Reload</interface> button.",
                                    QLatin1String(TECHBASE_HOWTO_DOC),
                                    QLatin1String("#missingDebugPackages")));
                ui.m_installDebugButton->setVisible(true);
                //Retrieve the libraries with missing debug info
                QStringList missingLibraries = btParser->librariesWithMissingDebugSymbols().toList();
                m_debugPackageInstaller->setMissingLibraries(missingLibraries);
            } else {
                //No automated method to install the missing debug info
                //Tell the user to read the howto and reload
                ui.m_extraDetailsLabel->setText(xi18nc("@info/rich", "Please read <link url='%1'>How to "
                                    "create useful crash reports</link> to learn how to get a useful "
                                    "backtrace; install the needed packages (<link url='%2'>"
                                    "list of files</link>) and click the "
                                    "<interface>Reload</interface> button.",
                                    QLatin1String(TECHBASE_HOWTO_DOC),
                                    QLatin1String("#missingDebugPackages")));
            }
        }

        ui.m_copyButton->setEnabled(true);
        ui.m_saveButton->setEnabled(true);
    } else if (m_btGenerator->state() == BacktraceGenerator::Failed) {
        //The backtrace could not be generated because the debugger had an error
        m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "The debugger has quit unexpectedly."));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));

        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(xi18nc("@info/rich", "You could try to regenerate the "
                                            "backtrace by clicking the <interface>Reload"
                                            "</interface> button."));
    } else if (m_btGenerator->state() == BacktraceGenerator::FailedToStart) {
        //The backtrace could not be generated because the debugger could not start (missing)
        //Tell the user to install it.
        m_backtraceRatingWidget->setUsefulness(BacktraceParser::Useless);

        ui.m_statusWidget->setIdle(i18nc("@info:status", "<strong>The debugger application is missing or "
                                                          "could not be launched.</strong>"));

        ui.m_backtraceEdit->setPlainText(i18nc("@info:status",
                                               "The crash information could not be generated."));
        ui.m_extraDetailsLabel->setVisible(true);
        ui.m_extraDetailsLabel->setText(xi18nc("@info/rich", "<strong>You need to first install the debugger "
                                               "application (%1) then click the <interface>Reload"
                                               "</interface> button.</strong>",
                                               m_btGenerator->debugger().name()));
    }

    ui.m_reloadBacktraceButton->setEnabled(true);
    emit stateChanged();
}

void BacktraceWidget::backtraceNewLine(const QString & line)
{
    //While loading the backtrace (unparsed) a new line was sent from the debugger, append it
    ui.m_backtraceEdit->append(line.trimmed());
}

void BacktraceWidget::copyClicked()
{
    ui.m_backtraceEdit->selectAll();
    ui.m_backtraceEdit->copy();
}

void BacktraceWidget::saveClicked()
{
    DrKonqi::saveReport(m_btGenerator->backtrace(), this);
}

void BacktraceWidget::hilightExtraDetailsLabel(bool hilight)
{
    QString stylesheet;
    if (hilight) {
        stylesheet = QLatin1String("border-width: 2px; "
                                    "border-style: solid; "
                                    "border-color: red;");
    } else {
        stylesheet = QLatin1String("border-width: 0px;");
    }
    stylesheet += QLatin1String(extraDetailsLabelMargin);
    ui.m_extraDetailsLabel->setStyleSheet(stylesheet);
}

void BacktraceWidget::focusImproveBacktraceButton()
{
    ui.m_installDebugButton->setFocus();
}

void BacktraceWidget::installDebugPackages()
{
    ui.m_installDebugButton->setVisible(false);
    m_debugPackageInstaller->installDebugPackages();
}

void BacktraceWidget::debugPackageError(const QString & errorMessage)
{
    ui.m_installDebugButton->setVisible(true);
    KMessageBox::error(this, errorMessage, i18nc("@title:window", "Error during the installation of"
                                                                                " debug symbols"));
}

void BacktraceWidget::debugPackageCanceled()
{
    ui.m_installDebugButton->setVisible(true);
}

bool BacktraceWidget::canInstallDebugPackages() const
{
    return m_debugPackageInstaller->canInstallDebugPackages();
}

void BacktraceWidget::toggleBacktrace(bool show)
{
    ui.m_backtraceStack->setCurrentWidget(show ? ui.backtracePage : ui.backtraceHelpPage);
}

void BacktraceWidget::extraDetailsLinkActivated(QString link)
{
    if (link.startsWith(QLatin1String("http"))) {
        //Open externally
        KToolInvocation::invokeBrowser(link);
    } else if (link == QLatin1String("#missingDebugPackages")) {
        BacktraceParser * btParser = m_btGenerator->parser();

        QStringList missingDbgForFiles = btParser->librariesWithMissingDebugSymbols().toList();
        missingDbgForFiles.insert(0, DrKonqi::crashedApplication()->executable().absoluteFilePath());

        //HTML message
        QString message;
        message = "<html>";
        message += i18n("The packages containing debug information for the following application and libraries are missing:");
        message += "<br /><ul>";

        Q_FOREACH(const QString & string, missingDbgForFiles) {
            message += "<li>" + string + "</li>";
        }

        message += "</ul></html>";

        KMessageBox::information(this, message, i18nc("messagebox title","Missing debug information packages"));
    }
}

