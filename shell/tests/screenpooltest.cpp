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
    void handleScreenAdded(QScreen *screen);
    void handleScreenRemoved(QScreen *screen);
    void handlePrimaryScreenChanged(QScreen *oldPrimary, QScreen *newPrimary);

    ScreenPool *m_screenPool = nullptr;
};

ScreenPoolTester::ScreenPoolTester(QObject *parent)
    : QObject(parent)
    , m_screenPool(new ScreenPool(KSharedConfig::openConfig("plasmashellrc")))
{
    connect(m_screenPool, &ScreenPool::screenAdded, this, &ScreenPoolTester::handleScreenAdded);
    connect(m_screenPool, &ScreenPool::screenRemoved, this, &ScreenPoolTester::handleScreenRemoved);
    connect(m_screenPool, &ScreenPool::primaryScreenChanged, this, &ScreenPoolTester::handlePrimaryScreenChanged);
    m_screenPool->load();
    qWarning() << "Load completed";
    qWarning() << m_screenPool;
}

void ScreenPoolTester::handleScreenAdded(QScreen *screen)
{
    qWarning() << "SCREEN ADDED" << screen;
    qWarning() << m_screenPool;
}

void ScreenPoolTester::handleScreenRemoved(QScreen *screen)
{
    qWarning() << "SCREEN REMOVED" << screen;
    qWarning() << m_screenPool;
}

void ScreenPoolTester::handlePrimaryScreenChanged(QScreen *oldPrimary, QScreen *newPrimary)
{
    qWarning() << "PRIMARY SCREEN CHANGED:" << oldPrimary << "-->" << newPrimary;
    qWarning() << m_screenPool;
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsSet("PLASMA_DISABLE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    } else {
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    }

    QGuiApplication::setApplicationDisplayName(QStringLiteral("ScreenPool test"));

    QApplication app(argc, argv);

    ScreenPoolTester screenPoolTester;

    return app.exec();
}

#include "screenpooltest.moc"
