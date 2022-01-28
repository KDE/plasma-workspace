/*
    SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <QApplication>

#include "../screenpool.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    } else {
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }

    QGuiApplication::setApplicationDisplayName(QStringLiteral("ScreenPool test"));

    QApplication app(argc, argv);

    ScreenPool *screenPool = new ScreenPool(KSharedConfig::openConfig("plasmashellrc"));

    return app.exec();
}
