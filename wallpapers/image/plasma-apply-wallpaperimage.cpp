/*
   Copyright (c) 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "config-X11.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma-apply-wallpaperimage"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    KLocalizedString::setApplicationDomain("plasma-apply-wallpaperimage");

    QCommandLineParser *parser = new QCommandLineParser;
    parser->addHelpOption();
    parser->setApplicationDescription(i18n("This tool allows you to set an image as the wallpaper for the Plasma session."));
    parser->addPositionalArgument(QStringLiteral("imagefile"), i18n("An image file or an installed wallpaper kpackage that you wish to set as the wallpaper for your Plasma session"));
    parser->process(app);

    int errorCode{0};
    QTextStream ts(stdout);
    if (!parser->positionalArguments().isEmpty()) {
        QString wallpaperFile{parser->positionalArguments().first()};
        QFileInfo wallpaperInfo{wallpaperFile};
        bool isWallpaper{false};
        bool isKPackage{false};
        if (wallpaperFile.contains(QStringLiteral("\'"))) {
            // If this happens, we might very well assume that there is some kind of funny business going on
            // even if technically it could just be a possessive. But, security first, so...
            ts << i18n(
                "There is a stray single quote in the filename of this wallpaper (') - please contact the author of the wallpaper to fix this, or rename the "
                "file yourself: %1",
                wallpaperFile)
               << Qt::endl;
            errorCode = -1;
        } else {
            if (wallpaperInfo.exists()) {
                if (wallpaperInfo.isFile()) {
                    // then we assume it's an image file... we could check with QImage, but
                    // that makes the operation much heavier than it needs to be
                    isWallpaper = true;
                } else {
                    if (QFileInfo(QStringLiteral("%1/metadata.desktop").arg(wallpaperFile)).exists()
                    || QFileInfo(QStringLiteral("%1/metadata.json").arg(wallpaperFile)).exists()
                    ) {
                        isWallpaper = true;
                        isKPackage = true;
                        // Similarly to above, we could read all the information out of the kpackage, but
                        // that also is not hugely important, so we just deduce that this reasonably should
                        // be an installed kpackage
                    }
                }
            }
        }
        if (isWallpaper) {
            QString script;
            QTextStream out(&script);
            out << "for (var key in desktops()) {"
                << "var d = desktops()[key];"
                << "d.wallpaperPlugin = 'org.kde.image';"
                << "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];"
                << "d.writeConfig('Image', 'file://" + wallpaperFile + "');"
                << "}";
                auto message = QDBusMessage::createMethodCall("org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell", "evaluateScript");
                message.setArguments(QVariantList() << QVariant(script));
                auto reply = QDBusConnection::sessionBus().call(message);

                if (reply.type() == QDBusMessage::ErrorMessage) {
                    ts << i18n("An error occurred while attempting to set the Plasma wallpaper:\n") << reply.errorMessage() << Qt::endl;
                    errorCode = -1;
                } else {
                    if (isKPackage) {
                        ts << i18n("Successfully set the wallpaper for all desktops to the KPackage based %1", wallpaperFile) << Qt::endl;
                    } else {
                        ts << i18n("Successfully set the wallpaper for all desktops to the image %1", wallpaperFile) << Qt::endl;
                    }
                }

        } else if (errorCode == 0) {
            // Just to avoid spitting out multiple errors
            ts << i18n("The file passed to be set as wallpaper does not exist, or we cannot identify it as a wallpaper: %1", wallpaperFile) << Qt::endl;
            errorCode = -1;
        }
    } else {
        parser->showHelp();
    }
    QTimer::singleShot(0, &app, [&app,&errorCode](){ app.exit(errorCode); });

    return app.exec();
}
