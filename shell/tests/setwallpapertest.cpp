/*
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QDebug>
#include <QObject>
#include <QtDBus>

#define SERVICE_NAME "org.kde.plasmashell"

class SetWallPaperTest : public QObject
{
    Q_OBJECT
public:
    SetWallPaperTest(QObject *parent = nullptr);
    void setWallpaper(const QString &wallpaperPath);

public Q_SLOTS:
    void onWallpaperChanged(uint screenNum);

private:
    void printCurrentWallpaper();
};

void SetWallPaperTest::printCurrentWallpaper()
{
    QDBusMessage request =
        QDBusMessage::createMethodCall(SERVICE_NAME, QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("wallpaper"));
    request.setArguments({uint(0)});

    const QDBusReply<QVariantMap> response = QDBusConnection::sessionBus().call(request);
    qDebug() << "Current wallpaper:" << response.value();
}

SetWallPaperTest::SetWallPaperTest(QObject *parent)
    : QObject(parent)
{
}

void SetWallPaperTest::onWallpaperChanged(uint screenNum)
{
    qDebug() << "WallpaperChanged on screen:" << screenNum;
    QCoreApplication::quit();
}

void SetWallPaperTest::setWallpaper(const QString &wallpaperPath)
{
    printCurrentWallpaper();

    // find our remote
    auto iface = new QDBusInterface(SERVICE_NAME, "/PlasmaShell", "org.kde.PlasmaShell", QDBusConnection::sessionBus(), this);
    if (!iface->isValid()) {
        fprintf(stderr, "%s\n", qPrintable(QDBusConnection::sessionBus().lastError().message()));
        QCoreApplication::instance()->quit();
    }

    const bool connected = QDBusConnection::sessionBus().connect(QStringLiteral(SERVICE_NAME),
                                                                 QStringLiteral("/PlasmaShell"),
                                                                 QStringLiteral("org.kde.PlasmaShell"),
                                                                 QStringLiteral("wallpaperChanged"),
                                                                 this,
                                                                 SLOT(onWallpaperChanged(uint)));
    if (!connected) {
        qDebug() << "Could not connect to interface";
        return;
    }

    QVariantMap params;
    params.insert(QStringLiteral("Image"), wallpaperPath);
    const QDBusReply<void> response = iface->call(QStringLiteral("setWallpaper"), QStringLiteral("org.kde.image"), params, uint(0));
    if (!response.isValid()) {
        qDebug() << "failed to set wallpaper:" << response.error();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString wallpaperPath;
    if (argc == 1) {
        wallpaperPath = QStringLiteral("/usr/share/wallpapers/Altai/");
    } else {
        wallpaperPath = QString(argv[1]);
    }

    qDebug() << "Setting wallpaper to" << wallpaperPath;

    SetWallPaperTest setWallpaperTest;
    setWallpaperTest.setWallpaper(wallpaperPath);

    return app.exec();
}

#include "setwallpapertest.moc"
