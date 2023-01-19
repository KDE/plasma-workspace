/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include <QGuiApplication>
#include <QRasterWindow>
#include <QTimer>

#include <cstdlib>

/**
 * Usage: samplewidgetwindow [title] [icon path/name] [width] [height]
 */
int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QRasterWindow w;
    if (argc >= 2) {
        w.setTitle(QString::fromUtf8(argv[1]));
    } else {
        w.setTitle(QStringLiteral("__test_window_no_title__"));
    }
    if (argc >= 3) {
        const QString iconString = QString::fromLatin1(argv[2]);
        QIcon icon(iconString);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(iconString);
        }
        if (!icon.isNull()) {
            w.setIcon(icon);
        }
    }
    if (argc >= 5) {
        const int width = std::max(1, std::stoi(argv[3]));
        const int height = std::max(1, std::stoi(argv[4]));
        w.setBaseSize(QSize(width, height));
    } else {
        w.setBaseSize(QSize(100, 100));
    }

    w.show();
    w.raise();
    w.requestActivate();

    QTimer::singleShot(5000, [&a] {
        a.quit();
    });

    return a.exec();
}