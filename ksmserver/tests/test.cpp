#include "shutdowndlg.h"
#include <kcmdlineargs.h>
#include <k4aboutdata.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qdebug.h>
#include <kworkspace.h>
//#include <Plasma/Theme>
int
main(int argc, char *argv[])
{
    K4AboutData about("kapptest", 0, ki18n("kapptest"), "version");
    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("t");
    options.add("type <name>", ki18n("The type of shutdown to emulate: Default, None, Reboot, Halt or Logout"), "None");
    options.add("theme <name>", ki18n("Shutdown dialog theme."));
    options.add("list-themes", ki18n("Lists available shutdown dialog themes"));
    options.add("choose", ki18n("Sets the mode where the user can choose between the different options. Use with --type."));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication a;
    KIconLoader::global()->addAppDir(QStringLiteral("ksmserver"));


    if (args->isSet("list-themes")) {
        QDir dir = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ksmserver/themes"), QStandardPaths::LocateDirectory);
        QStringList entries = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        if (entries.isEmpty()) {
            QTextStream(stdout) << i18n("No themes found");
        } else {
            QTextStream(stdout) << i18n("Found themes: ") << entries.join(", ") << '\n';
        }
        return 0;
    }

    QString sdtypeOption = args->getOption("type").toLower();
    KWorkSpace::ShutdownType sdtype = KWorkSpace::ShutdownTypeDefault;
    if (sdtypeOption == QStringLiteral("reboot")) {
        sdtype = KWorkSpace::ShutdownTypeReboot;
    } else if (sdtypeOption == QStringLiteral("halt")) {
        sdtype = KWorkSpace::ShutdownTypeHalt;
    } else if (sdtypeOption == QStringLiteral("logout")) {
        sdtype = KWorkSpace::ShutdownTypeNone;
    }

    QString bopt;
    (void)KSMShutdownDlg::confirmShutdown( true, args->isSet("choose"), sdtype, bopt, QStringLiteral("default") );
/*   (void)KSMShutdownDlg::confirmShutdown( false, false, sdtype, bopt ); */
}
