/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include <cstdlib>
#include <unistd.h>

#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>

#include "drkonqi.h"
#include "drkonqidialog.h"

static const char version[] = "2.1.5";
static const char description[] = I18N_NOOP("The KDE Crash Handler gives the user feedback "
                                            "if a program has crashed.");

int main(int argc, char* argv[])
{
#ifndef Q_OS_WIN //krazy:exclude=cpp
// Drop privs.
    setgid(getgid());
    if (setuid(getuid()) < 0 && geteuid() != getuid()) {
        exit(255);
    }
#endif

    QApplication qa(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("drkonqi"));
    QCoreApplication::setApplicationVersion(version);

    // Prevent KApplication from setting the crash handler. We will set it later...
    setenv("KDE_DEBUG", "true", 1);
    // Session management is not needed, do not even connect in order to survive longer than ksmserver.
    unsetenv("SESSION_MANAGER");

    KAboutData aboutData("drkonqi", "drkonqi", i18n("The KDE Crash Handler"),
                         version, i18n(description),
                         KAboutData::License_GPL,
                         i18n("(C) 2000-2009, The DrKonqi Authors"));
    aboutData.addAuthor(i18nc("@info:credit","Hans Petter Bieker"), QString(),
                         "bieker@kde.org");
    aboutData.addAuthor(i18nc("@info:credit","Dario Andres Rodriguez"), QString(),
                         "andresbajotierra@gmail.com");
    aboutData.addAuthor(i18nc("@info:credit","George Kiagiadakis"), QString(),
                         "gkiagia@users.sourceforge.net");
    aboutData.addAuthor(i18nc("@info:credit","A. L. Spehr"), QString(),
                         "spehr@kde.org");
    aboutData.setProgramIconName("tools-report-bug");

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption signalOption(QStringLiteral("signal"), i18nc("@info:shell","The signal <number> that was caught"), QStringLiteral("number"));
    QCommandLineOption appNameOption(QStringLiteral("appname"), i18nc("@info:shell","<Name> of the program"), QStringLiteral("name"));
    QCommandLineOption appPathOption(QStringLiteral("apppath"), i18nc("@info:shell","<Path> to the executable"), QStringLiteral("path"));
    QCommandLineOption appVersionOption(QStringLiteral("appversion"), i18nc("@info:shell","The <version> of the program"), QStringLiteral("version"));
    QCommandLineOption bugAddressOption(QStringLiteral("bugaddress"), i18nc("@info:shell","The bug <address> to use"), QStringLiteral("address"));
    QCommandLineOption programNameOption(QStringLiteral("programname"), i18nc("@info:shell","Translated <name> of the program"), QStringLiteral("name"));
    QCommandLineOption pidOption(QStringLiteral("pid"), i18nc("@info:shell","The <PID> of the program"), QStringLiteral("pid"));
    QCommandLineOption startupIdOption(QStringLiteral("startupid"), i18nc("@info:shell","Startup <ID> of the program"), QStringLiteral("id"));
    QCommandLineOption kdeinitOption(QStringLiteral("kdeinit"), i18nc("@info:shell","The program was started by kdeinit"));
    QCommandLineOption saferOption(QStringLiteral("safer"), i18nc("@info:shell","Disable arbitrary disk access"));
    QCommandLineOption restartedOption(QStringLiteral("restarted"), i18nc("@info:shell","The program has already been restarted"));
    QCommandLineOption keepRunningOption(QStringLiteral("keeprunning"), i18nc("@info:shell","Keep the program running and generate "
                                                    "the backtrace at startup"));
    QCommandLineOption threadOption(QStringLiteral("thread"), i18nc("@info:shell","The <thread id> of the failing thread"), QStringLiteral("threadid"));

    parser.addOption(signalOption);
    parser.addOption(appNameOption);
    parser.addOption(appPathOption);
    parser.addOption(appVersionOption);
    parser.addOption(bugAddressOption);
    parser.addOption(programNameOption);
    parser.addOption(pidOption);
    parser.addOption(startupIdOption);
    parser.addOption(kdeinitOption);
    parser.addOption(saferOption);
    parser.addOption(restartedOption);
    parser.addOption(keepRunningOption);
    parser.addOption(threadOption);

    aboutData.setupCommandLine(&parser);
    parser.process(qa);
    aboutData.processCommandLine(&parser);

    DrKonqi::setSignal(parser.value(signalOption).toInt());
    DrKonqi::setAppName(parser.value(appNameOption));
    DrKonqi::setAppPath(parser.value(appPathOption));
    DrKonqi::setAppVersion(parser.value(appVersionOption));
    DrKonqi::setBugAddress(parser.value(bugAddressOption));
    DrKonqi::setProgramName(parser.value(programNameOption));
    DrKonqi::setPid(parser.value(pidOption).toInt());
    DrKonqi::setStartupId(parser.value(startupIdOption));
    DrKonqi::setKdeinit(parser.isSet(kdeinitOption));
    DrKonqi::setSafer(parser.isSet(saferOption));
    DrKonqi::setRestarted(parser.isSet(restartedOption));
    DrKonqi::setKeepRunning(parser.isSet(keepRunningOption));
    DrKonqi::setThread(parser.value(threadOption).toInt());

    if (!DrKonqi::init()) {
        return 1;
    }

    qa.setQuitOnLastWindowClosed(false);

    DrKonqiDialog *w = new DrKonqiDialog();
    w->show();
    int ret = qa.exec();

    DrKonqi::cleanup();
    return ret;
}
