/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>
#include <QtTest>

#include "commontestdata.h"

class TestSetup : public QObject
{
    Q_OBJECT

public:
    TestSetup();

public Q_SLOTS:
    void qmlEngineAvailable(QQmlEngine *engine);

private:
    QDir m_dataDir;
    QString m_wallpaperPath;
    QString m_packagePath;
    QString m_bizzarePath;
};

TestSetup::TestSetup()
{
    // For target size test
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    m_dataDir = QDir(QFINDTESTDATA("testdata/default"));

    m_wallpaperPath = m_dataDir.absoluteFilePath(QStringLiteral("wallpaper.jpg.jpg"));
    m_packagePath = m_dataDir.absoluteFilePath(QStringLiteral("package/"));
    m_bizzarePath = m_dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5);
}

void TestSetup::qmlEngineAvailable(QQmlEngine *engine)
{
    engine->rootContext()->setContextProperty(QStringLiteral("testDirs"), QStringList{m_dataDir.absolutePath()});
    engine->rootContext()->setContextProperty(QStringLiteral("testImage"), QUrl::fromLocalFile(m_wallpaperPath));
    engine->rootContext()->setContextProperty(QStringLiteral("testPackage"), QUrl::fromLocalFile(m_packagePath));
    engine->rootContext()->setContextProperty(QStringLiteral("testBizzareFileName"), m_bizzarePath);
    engine->rootContext()->setContextProperty(QStringLiteral("testBizzareFileName_modelImage"), QUrl::fromUserInput(m_bizzarePath).toString());
}

QUICK_TEST_MAIN_WITH_SETUP(ImageBackend, TestSetup)

#include "tst_imagebackend.moc"
