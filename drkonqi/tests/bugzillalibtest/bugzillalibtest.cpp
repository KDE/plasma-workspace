/*******************************************************************
* bugzillalibtest.cpp
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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
#include <QCommandLineParser>
#include <QApplication>
#include <QDebug>
#include <KAboutData>
#include <KLocalizedString>

#include "../../bugzillaintegration/bugzillalib.h"
#include "../../debugpackageinstaller.h"

class BugzillaLibTest : public QObject
{
    Q_OBJECT
    public:
        BugzillaLibTest(QString user, QString password) : QObject()
        {
            manager = new BugzillaManager(QStringLiteral("http://bugstest.kde.org/"));
            connect(manager, &BugzillaManager::loginFinished, this, &BugzillaLibTest::loginFinished);
            connect(manager, &BugzillaManager::loginError, this, &BugzillaLibTest::loginError);
            connect(manager, &BugzillaManager::reportSent, this, &BugzillaLibTest::reportSent);
            connect(manager, &BugzillaManager::sendReportError, this, &BugzillaLibTest::sendReportError);
            connect(manager, &BugzillaManager::sendReportErrorInvalidValues, this, &BugzillaLibTest::sendBR2);
            manager->tryLogin(user, password);
            qDebug() << "Login ...";
        }

    private Q_SLOTS:
        void loginFinished(bool ok)
        {
            qDebug() << "Login Finished" << ok;
            if (!ok) {
                return;
            }

            //Uncomment to Test
            //FIXME provide a way to select the test from the command line

            //Send a new bug report
            /*
            sendBR();
            */

            //Attach a simple text to a report as a file
            /*
            manager->attachTextToReport("Bugzilla Lib Attachment Content Test", "/tmp/var", 
                                        "Bugzilla Lib Attachment Description Test", 150000);
            */

            /*
            manager->addMeToCC(100005);
            */
        }

        void loginError(const QString & msg)
        {
            qDebug() << "Login Error" << msg;
        }

        void sendBR()
        {
            BugReport br;
            br.setValid(true);
            br.setProduct(QStringLiteral("konqueror"));
            br.setComponent(QStringLiteral("general"));
            br.setVersion(QStringLiteral("undefined"));
            br.setOperatingSystem(QStringLiteral("Linux"));
            br.setPriority(QStringLiteral("NOR"));
            br.setPlatform(QStringLiteral("random test"));
            br.setBugSeverity(QStringLiteral("crash"));
            br.setShortDescription(QStringLiteral("bla bla"));
            br.setDescription(QStringLiteral("bla bla large"));

            manager->sendReport(br);
            qDebug() << "Trying to send bug report";
        }

        void sendBR2()
        {
            BugReport br;
            br.setValid(true);
            br.setProduct(QStringLiteral("konqueror"));
            br.setComponent(QStringLiteral("general"));
            br.setVersion(QStringLiteral("undefined"));
            br.setOperatingSystem(QStringLiteral("Linux"));
            br.setPriority(QStringLiteral("NOR"));
            br.setPlatform(QStringLiteral("unspecified"));
            br.setBugSeverity(QStringLiteral("crash"));
            br.setShortDescription(QStringLiteral("bla bla"));
            br.setDescription(QStringLiteral("bla bla large"));

            manager->sendReport(br);
            qDebug() << "Trying to send bug report";
        }

        void reportSent( int num)
        {
            qDebug() << "BR sent " << num << manager->urlForBug(num);
        }

        void sendReportError(const QString & msg)
        {
            qDebug() << "Error sending bug report" << msg;
        }

    private:
        BugzillaManager * manager;

};

int main (int argc, char ** argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData( QStringLiteral("bzlibtest"), i18n("BugzillaLib Test (DrKonqi2)"),
        QStringLiteral("1.0"), i18n("Test application for bugtracker manager lib"), KAboutLicense::GPL,
        i18n("(c) 2009, DrKonqi2 Developers"));

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("user"), i18nc("@info:shell","bugstest.kde.org username"), QStringLiteral("username")));
    parser.addOption(QCommandLineOption(QStringLiteral("pass"), i18nc("@info:shell","bugstest.kde.org password"), QStringLiteral("password")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!parser.isSet(QStringLiteral("user")) || !parser.isSet(QStringLiteral("pass"))) {
        qDebug() << "Provide bugstest.kde.org username and password. See help";
        return 0;
    }

    new BugzillaLibTest(parser.value(QStringLiteral("user")), parser.value(QStringLiteral("pass")));
    return app.exec();
}

#include "bugzillalibtest.moc"
