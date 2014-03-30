#include "shutdowndlg.h"
#include <kcmdlineargs.h>
#include <k4aboutdata.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kworkspace.h>
#include <Plasma/Theme>
int
main(int argc, char *argv[])
{
    K4AboutData about("kapptest", 0, ki18n("kapptest"), "version");
    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("t");
    options.add("type <name>", ki18n("The type of shutdown to emulate: Default, None, Reboot, Halt or Logout"), "None");
    options.add("theme <name>", ki18n("Theme name. List with 'plasmoidviewer --list-themes'"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication a;
    KIconLoader::global()->addAppDir(QStringLiteral("ksmserver"));

    QString sdtypeOption = args->getOption("type").toLower();

//     if (args->isSet("theme")) {
//         Plasma::Theme::defaultTheme()->setUseGlobalSettings(false); //don't change every plasma theme!
//         Plasma::Theme::defaultTheme()->setThemeName(args->getOption("theme"));
//     }

    KWorkSpace::ShutdownType sdtype = KWorkSpace::ShutdownTypeDefault;
    if (sdtypeOption == QStringLiteral("reboot")) {
        sdtype = KWorkSpace::ShutdownTypeReboot;
    } else if (sdtypeOption == QStringLiteral("halt")) {
        sdtype = KWorkSpace::ShutdownTypeHalt;
    } else if (sdtypeOption == QStringLiteral("logout")) {
        sdtype = KWorkSpace::ShutdownTypeNone;
    }

    QString bopt;
    (void)KSMShutdownDlg::confirmShutdown( true, true, sdtype, bopt, QStringLiteral("default") );
/*   (void)KSMShutdownDlg::confirmShutdown( false, false, sdtype, bopt ); */
}
