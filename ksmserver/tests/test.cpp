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

    KAboutData about(QStringLiteral("kapptest"), i18n("kapptest"), QStringLiteral("version"));
    KAboutData::setApplicationData(about);
    QCommandLineParser options;
    options.addOption(QCommandLineOption(QStringList(QStringLiteral("t")) << QStringLiteral("type"), i18n("The type of shutdown to emulate: Default, None, Reboot, Halt or Logout"), QStringLiteral("name"), QStringLiteral("None")));
    options.addOption(QCommandLineOption(QStringLiteral("choose"), i18n("Sets the mode where the user can choose between the different options. Use with --type.")));

    about.setupCommandLine(&options);
    options.process(app);
    about.processCommandLine(&options);

    KIconLoader::global()->addAppDir(QStringLiteral("ksmserver"));

    QString sdtypeOption = options.value(QStringLiteral("type")).toLower();
    KWorkSpace::ShutdownType sdtype = KWorkSpace::ShutdownTypeDefault;
    if (sdtypeOption == QStringLiteral("reboot")) {
        sdtype = KWorkSpace::ShutdownTypeReboot;
    } else if (sdtypeOption == QStringLiteral("halt")) {
        sdtype = KWorkSpace::ShutdownTypeHalt;
    } else if (sdtypeOption == QStringLiteral("logout")) {
        sdtype = KWorkSpace::ShutdownTypeNone;
    }

    QString bopt;
    (void)KSMShutdownDlg::confirmShutdown( true, options.isSet(QStringLiteral("choose")), sdtype, bopt, QString() );
/*   (void)KSMShutdownDlg::confirmShutdown( false, false, sdtype, bopt ); */
}
