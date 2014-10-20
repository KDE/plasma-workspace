/*******************************************************************
* aboutbugreportingdialog.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    A. L. Spehr <spehr@kde.org>
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

#include "aboutbugreportingdialog.h"

#include <KLocalizedString>
#include <KConfig>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QTextBrowser>
#include <qboxlayout.h>
#include <QDialogButtonBox>
#include <qdesktopservices.h>

#include "drkonqi_globals.h"

AboutBugReportingDialog::AboutBugReportingDialog(QWidget * parent):
        QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    setWindowIcon(QIcon::fromTheme("help-hint"));
    setWindowTitle(i18nc("@title title of the dialog", "About Bug Reporting - Help"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setMinimumSize(QSize(600, 300));
    connect(m_textBrowser, &QTextBrowser::anchorClicked, this, &AboutBugReportingDialog::handleInternalLinks);

    QString text =

        //Introduction
        QString("<a name=\"%1\" /><h1>%2</h1>").arg(QLatin1String(PAGE_HELP_BEGIN_ID),
                                i18nc("@title","Information about bug reporting")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            xi18nc("@info/rich", "You can help us improve this software by filing a bug report."),
            xi18nc("@info/rich","<note>It is safe to close this dialog. If you do not "
                            "want to, you do not have to file a bug report.</note>"),
            xi18nc("@info/rich","In order to generate a useful bug report we need some "
                            "information about both the crash and your system. (You may also "
                            "need to install some debug packages.)")) +

        //Sub-introduction
        QString("<h1>%1</h1>").arg(i18nc("@title","Bug Reporting Assistant Guide")) +
        QString("<p>%1</p>").arg(
            xi18nc("@info/rich","This assistant will guide you through the crash "
                            "reporting process for the KDE bug tracking system. All the "
                            "information you enter on the bug report <strong>must be written "
                            "in English</strong>, if possible, as KDE is formed internationally.")) +

        //Bug Awareness Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_AWARENESS_ID),
                                i18nc("@title","What do you know about the crash?")) +
        QString("<p>%1</p><p>%2<ul><li>%3</li><li>%4</li><li>%5</li><li>%6</li><li>%7</li><li>%8</li>"
        "</ul>%9</p>").arg(
            xi18nc("@info/rich","In this page you need to describe how much do you know about "
                                "the desktop and the application state before it crashed."),
            xi18nc("@info/rich","If you can, describe in as much detail as possible the crash "
                               "circumstances, and what you were doing when the application crashed "
                               "(this information is going to be requested later.) You can mention: "),
            xi18nc("@info/rich crash situation example","actions you were taking inside or outside "
                            "the application"),
            xi18nc("@info/rich crash situation example","documents or images that you were using "
                            "and their type/format (later if you go to look at the report in the "
                            "bug tracking system, you can attach a file to the report)"),
            xi18nc("@info/rich crash situation example","widgets that you were running"),
            xi18nc("@info/rich crash situation example","the URL of a web site you were browsing"),
            xi18nc("@info/rich crash situation example","configuration details of the application"),
            xi18nc("@info/rich crash situation example","or other strange things you notice before "
                            "or after the crash. "),
            xi18nc("@info/rich","Screenshots can be very helpful sometimes. You can attach them to "
                            "the bug report after it is posted to the bug tracking system.")) +

        //Crash Information Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_CRASHINFORMATION_ID),
                                i18nc("@title","Crash Information (backtrace)")) +
        QString("<p>%1</p><p>%2</p><p>%3</p><p>%4</p>").arg(
            xi18nc("@info/rich","This page will generate a \"backtrace\" of the crash. This "
                            "is information that tells the developers where the application "
                            "crashed."),
            xi18nc("@info/rich", "If the crash information is not detailed enough, you may "
                            "need to install some debug packages and reload it (if the "
                            "<interface>Install Debug Symbols</interface> button is available you "
                            "can use it to automatically install the missing information.)"),
            xi18nc("@info/rich", "You can find more information about backtraces, what they mean, "
                            "and how they are useful at <link>%1</link>",QString(TECHBASE_HOWTO_DOC) ),
            xi18nc("@info/rich","Once you get a useful backtrace (or if you do not want to "
                            "install the missing debugging packages) you can continue.")) +

        //Conclusions Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_CONCLUSIONS_ID),
                                i18nc("@title","Conclusions")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            xi18nc("@info/rich","Using the quality of the crash information gathered, "
                            "and your answers on the previous page, the assistant will "
                            "tell you if the crash is worth reporting or not."),
            xi18nc("@info/rich","If the crash is worth reporting but the application "
                            "is not supported in the KDE bug tracking system, you "
                            "will need to directly contact the maintainer of the application."),
            xi18nc("@info/rich","If the crash is listed as being not worth reporting, "
                            "and you think the assistant has made a mistake, "
                            "you can still manually report the bug by logging into the "
                            "bug tracking system. You can also go back and change information "
                            "and download debug packages.")) +

        //Bugzilla Login Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZLOGIN_ID),
                                    i18nc("@title","Login into the bug tracking system")) +
        QString("<p>%1</p><p>%2</p><p>%3</p>").arg(
            xi18nc("@info/rich","We may need to contact you in the future to ask for "
                            "further information. As we need to keep track of the bug reports, "
                            "you "
                            "need to have an account on the KDE bug tracking system. If you do "
                            "not have one, you can create one here: <link>%1</link>",
                            QString(KDE_BUGZILLA_CREATE_ACCOUNT_URL)),
            xi18nc("@info/rich","Then, enter your username and password and "
                            "press the Login button. You can use this login to directly access the "
                            "KDE bug tracking system later."),
            xi18nc("@info/rich","The KWallet dialog may appear when pressing Login to "
                            "save your password in the KWallet password system. Also, it will "
                            "prompt you for the KWallet password upon loading to autocomplete "
                            "the login fields if you use this assistant again.")) +

        //Bugzilla Duplicates Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZDUPLICATES_ID),
                                i18nc("@title","List of possible duplicate reports")) +
        QString("<p>%1</p><p>%2</p><p>%3</p><p>%4</p><p><strong>%5</strong></p>").arg(
        //needs some more string cleanup below
            xi18nc("@info/rich","This page will search the bug report system for "
                            "similar crashes which are possible duplicates of your bug. If "
                            "there are similar bug reports found, you can double click on them "
                            "to see details. Then, read the current bug report information so "
                            "you can check to see if they are similar. "),
            xi18nc("@info/rich","If you are very sure your bug is the same as another that is "
                            "previously reported, you can set your information to be attached to "
                            "the existing report."),
            xi18nc("@info/rich","If you are unsure whether your report is the same, follow the main "
                            "options to tentatively mark your crash as a duplicate of that "
                            "report. This is usually the safest thing to do. We cannot "
                            "uncombine bug reports, but we can easily merge them."),
            xi18nc("@info/rich","If not enough possible duplicates are found, or you "
                            "did not find a similar report, then you can force it to search "
                            "for more bug reports (only if the date range limit is not reached.)"),
            xi18nc("@info/rich","If you do not find any related reports, your crash information "
                                "is not useful enough, and you really cannot give additional "
                                "information about the crash context, then it is better to "
                                "not file the bug report, thereby closing the assistant.")) +

        //Bugzilla Crash Information - Details Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZDETAILS_ID),
                                        i18nc("@title","Details of the bug report and your system")) +
        QString("<p>%1<a href=\"#%2\">%3</a></p><p>%4</p><p>%5</p>").arg(
            xi18nc("@info/rich","In this case you need to write a title and description "
                            "of the crash. Explain as best you can. "),
            QLatin1String(PAGE_AWARENESS_ID),
            i18nc("@title","What do you know about the crash?"),
            xi18nc("@info/rich", "You can also specify your distribution method (GNU/Linux "
                                "distribution or packaging system) or if you compiled the KDE "
                                "Platform from sources."),
            xi18nc("@info/rich", "<note>You should <strong>write those information in English</strong>.</note>")) +

        //Bugzilla Send Page
        QString("<a name=\"%1\" /><h2>%2</h2>").arg(QLatin1String(PAGE_BZSEND_ID),
                                       i18nc("@title","Sending the Crash Report")) +
        QString("<p>%1</p><p>%2</p>").arg(
            xi18nc("@info/rich","The last page will send the bug report to the bug tracking "
                            "system and will notify you when it is done. It will then show "
                            "the web address of the bug report in the KDE bug tracking system, "
                            "so that you can look at the report later."),
            xi18nc("@info/rich","If the process fails, you can click "
                            "<interface>Retry</interface> to try sending the bug report again. "
                            "If the report cannot be sent because the bug tracking system has a "
                            "problem, you can save it to a file to manually report later.")) +

        QString("<h1>%1</h1><p>%2</p>").arg(
            xi18nc("@info/rich", "Thank you for being part of KDE!"),
            xi18nc("@info/rich", "If you are interested in helping us to keep the KDE bug tracker system "
                                 "clean and useful, which allows the developers to be more focused on "
                                 "fixing the real issues,  you are welcome to "
                                 "<link url='http://techbase.kde.org/Contribute/Bugsquad'>join the BugSquad team</link>."));

    m_textBrowser->setText(text);

    layout->addWidget(m_textBrowser);

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::accepted, this, &AboutBugReportingDialog::accepted);
    connect(box, &QDialogButtonBox::rejected, this, &AboutBugReportingDialog::rejected);
    layout->addWidget(box);

    KConfigGroup config(KSharedConfig::openConfig(), "AboutBugReportingDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), config);
}

AboutBugReportingDialog::~AboutBugReportingDialog( )
{
    KConfigGroup config(KSharedConfig::openConfig(), "AboutBugReportingDialog");
    KWindowConfig::saveWindowSize(windowHandle(), config);
}

void AboutBugReportingDialog::handleInternalLinks(const QUrl& url)
{
    if (!url.isEmpty()) {
        if (url.scheme().isEmpty() && url.hasFragment()) {
            showSection(url.fragment());
        } else {
            QDesktopServices::openUrl(url);
        }
    }
}

void AboutBugReportingDialog::showSection(const QString& anchor)
{
    m_textBrowser->scrollToAnchor(anchor);
}

