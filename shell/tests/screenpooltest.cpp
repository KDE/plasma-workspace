/*
    SPDX-FileCopyrightText: 2022 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <QApplication>
#include <QDebug>
#include <QScreen>

#include "../screenpool.h"

class ScreenPoolTester : public QObject
{
    Q_OBJECT
public:
    ScreenPoolTester(QObject *parent = nullptr);

private:
    void handleScreenRemoved(QScreen *screen);
    void handleScreenOrderChanged(QList<QScreen *> screens);

    ScreenPool *m_screenPool = nullptr;
};

ScreenPoolTester::ScreenPoolTester(QObject *parent)
    : QObject(parent)
    , m_screenPool(new ScreenPool)
{
    connect(m_screenPool, &ScreenPool::screenRemoved, this, &ScreenPoolTester::handleScreenRemoved);
    connect(m_screenPool, &ScreenPool::screenOrderChanged, this, &ScreenPoolTester::handleScreenOrderChanged);

    qWarning() << "Screenpool started";
    qWarning() << m_screenPool;
}

void ScreenPoolTester::handleScreenRemoved(QScreen *screen)
{
    qWarning() << "SCREEN REMOVED, not reacting yet" << screen;
    qWarning() << m_screenPool;
}

void ScreenPoolTester::handleScreenOrderChanged(QList<QScreen *> screens)
{
    qWarning() << "SCREEN ORDER CHANGED:" << screens;
    qWarning() << m_screenPool;
}

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

    ScreenPoolTester screenPoolTester;

    return app.exec();
}

#include "screenpooltest.moc"
