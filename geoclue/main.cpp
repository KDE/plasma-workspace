#include <QApplication>

#include <KAboutData>
#include <KLocalizedString>

#include "agent.h"
#include "notifier.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setQuitLockEnabled(false);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("plasma-geoclue-agent"));

    KAboutData aboutData(QStringLiteral("plasma-geoclue-agent"),
                         i18n("KDE GeoClue Agent"),
                         QStringLiteral(PROJECT_VERSION),
                         i18n("KDE GeoClue Agent"),
                         KAboutLicense::GPL,
                         i18n("© 2025 Plasma Development Team"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.plasma.geoclue.agent"));
    aboutData.addAuthor(i18n("Vlad Zahorodnii"), i18n("Development"), QStringLiteral("vlad.zahorodnii@kde.org"));
    KAboutData::setApplicationData(aboutData);

    Agent agent;
    Notifier notifier(&agent);

    return app.exec();
}
