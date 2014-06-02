#include "shutdowndlg.h"
#include <kiconloader.h>
#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qcommandlineparser.h>
#include <qstandardpaths.h>
#include <qapplication.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KAboutData about("kapptest", i18n("kapptest"), "version");
    KAboutData::setApplicationData(about);
    QCommandLineParser options;
    options.addOption(QCommandLineOption(QStringList("t") << "type", i18n("The type of shutdown to emulate: Default, None, Reboot, Halt or Logout"), "name", "None"));
    options.addOption(QCommandLineOption("theme", i18n("Shutdown dialog theme."), "name"));
    options.addOption(QCommandLineOption("list-themes", i18n("Lists available shutdown dialog themes")));
    options.addOption(QCommandLineOption("choose", i18n("Sets the mode where the user can choose between the different options. Use with --type.")));

    about.setupCommandLine(&options);
    options.process(app);
    about.processCommandLine(&options);

    KIconLoader::global()->addAppDir(QStringLiteral("ksmserver"));

    if (options.isSet("list-themes")) {
        QDir dir = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ksmserver/themes"), QStandardPaths::LocateDirectory);
        QStringList entries = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        if (entries.isEmpty()) {
            QTextStream(stdout) << i18n("No themes found");
        } else {
            QTextStream(stdout) << i18n("Found themes: ") << entries.join(", ") << '\n';
        }
        return 0;
    }

    QString sdtypeOption = options.value("type").toLower();
    KWorkSpace::ShutdownType sdtype = KWorkSpace::ShutdownTypeDefault;
    if (sdtypeOption == QStringLiteral("reboot")) {
        sdtype = KWorkSpace::ShutdownTypeReboot;
    } else if (sdtypeOption == QStringLiteral("halt")) {
        sdtype = KWorkSpace::ShutdownTypeHalt;
    } else if (sdtypeOption == QStringLiteral("logout")) {
        sdtype = KWorkSpace::ShutdownTypeNone;
    }

    QString bopt;
    (void)KSMShutdownDlg::confirmShutdown( true, options.isSet("choose"), sdtype, bopt, QStringLiteral("default") );
/*   (void)KSMShutdownDlg::confirmShutdown( false, false, sdtype, bopt ); */
}
