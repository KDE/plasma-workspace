/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDateTime>
#include <QProcess>
#include <QRasterWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <KIconLoader>
#include <KSycoca>
#include <KWindowSystem>

#include "abstractwindowtasksmodel.h"
#include "samplewidgetwindow.h"

namespace TestUtils
{
using namespace TaskManager;
constexpr const char *dummyDesktopFileName = "org.kde.plasma.test.dummy.desktop";

void cleanupTestCase()
{
    QFile dummyFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("applications")
                    + QDir::separator() + QString::fromLatin1(dummyDesktopFileName));
    dummyFile.remove();
}

std::unique_ptr<QRasterWindow> createSingleWindow(const AbstractWindowTasksModel &model, const QString &title, QModelIndex &index)
{
    auto window = std::make_unique<QRasterWindow>();
    window->setTitle(title);
    window->setBaseSize(QSize(320, 240));

    QSignalSpy rowInsertedSpy(&model, &AbstractWindowTasksModel::rowsInserted);
    window->show();
    if (rowInsertedSpy.empty()) {
        Q_ASSERT(rowInsertedSpy.wait());
    }

    // Find the window index
    const auto results = model.match(model.index(0, 0), Qt::DisplayRole, title);
    Q_ASSERT(results.size() == 1);
    index = results.at(0);
    Q_ASSERT(index.isValid());
    qDebug() << "Window title:" << index.data(Qt::DisplayRole).toString();

    return window;
}

void testOpenCloseWindow(const AbstractWindowTasksModel &model)
{
    auto findWindow = [&model](const QString &windowTitle) {
        for (int i = 0; i < model.rowCount(); ++i) {
            const QString title = model.index(i, 0).data(Qt::DisplayRole).toString();
            if (title == windowTitle) {
                return true;
            }
        }
        return false;
    };

    // Create a window to test if the model can receive it
    QSignalSpy rowInsertedSpy(&model, &AbstractWindowTasksModel::rowsInserted);

    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = createSingleWindow(model, title, index);

    // A new window appears
    // Find the window in the model
    QVERIFY(findWindow(title));

    // Change the title of the window
    {
        QSignalSpy dataChangedSpy(&model, &AbstractWindowTasksModel::dataChanged);
        const QString newTitle = title + QStringLiteral("__newtitle__");
        window->setTitle(newTitle);
        QVERIFY(dataChangedSpy.wait());
        QTRY_VERIFY(dataChangedSpy.constLast().at(2).value<QVector<int>>().contains(Qt::DisplayRole));
        // Make sure the title is updated
        QTRY_VERIFY(!findWindow(title));
        QTRY_VERIFY(findWindow(newTitle));
    }

    // Now close the window
    {
        int modelCount = model.rowCount();
        QSignalSpy rowsRemovedSpy(&model, &AbstractWindowTasksModel::rowsRemoved);
        window->close();
        QVERIFY(rowsRemovedSpy.wait());
        QCOMPARE(modelCount - 1, model.rowCount());
    }
}

void testFullscreen(const AbstractWindowTasksModel &model)
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = createSingleWindow(model, title, index);

    QVERIFY(!index.data(AbstractTasksModel::IsFullScreen).toBool());

    QSignalSpy dataChangedSpy(&model, &AbstractWindowTasksModel::dataChanged);
    window->showFullScreen();
    dataChangedSpy.wait();

    // There can be more than one dataChanged signal being emitted due to caching
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QVector<int>>().contains(AbstractTasksModel::IsFullScreen);
    }));
    QTRY_VERIFY(index.data(AbstractTasksModel::IsFullScreen).toBool());
    dataChangedSpy.clear();
    window->showNormal();
    QVERIFY(dataChangedSpy.wait());
    // There can be more than one dataChanged signal being emitted due to caching
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QVector<int>>().contains(AbstractTasksModel::IsFullScreen);
    }));
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsFullScreen).toBool());
}

void testGeometry(const AbstractWindowTasksModel &model)
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = createSingleWindow(model, title, index);

    const QSize oldSize = index.data(AbstractTasksModel::Geometry).toRect().size();
    QCoreApplication::processEvents();
    QSignalSpy dataChangedSpy(&model, &AbstractWindowTasksModel::dataChanged);
    window->resize(QSize(240, 320));
    QVERIFY(dataChangedSpy.wait());

    // There can be more than one dataChanged signal being emitted due to caching
    // When using openbox the test is flaky
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QVector<int>>().contains(AbstractTasksModel::Geometry);
    }));
    QTRY_VERIFY(index.data(AbstractTasksModel::Geometry).toRect().size() != oldSize);
}

void testStackingOrder(const AbstractWindowTasksModel &model)
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto firstWindow = createSingleWindow(model, title, index);
    QTRY_VERIFY(index.data(AbstractTasksModel::StackingOrder).toInt() >= 0);
    const int stackingOrder1 = index.data(AbstractTasksModel::StackingOrder).toInt();

    // Create another window to make stacking order change
    QModelIndex index2;
    auto secondWindow = createSingleWindow(model, QStringLiteral("second test window"), index2);
    QTRY_VERIFY(index2.data(AbstractTasksModel::StackingOrder).toInt() >= 0);
    const int stackingOrder2 = index2.data(AbstractTasksModel::StackingOrder).toInt();
    QVERIFY2(
        stackingOrder1 < stackingOrder2,
        QLatin1String("stackingOrder1: %1 stackingOrder2: %2").arg(QString::number(stackingOrder1), QString::number(stackingOrder2)).toLatin1().constData());

    firstWindow->close();
    QCoreApplication::processEvents();
    QTRY_VERIFY(index2.data(AbstractTasksModel::StackingOrder).toInt() < stackingOrder2);
}

void createDesktopFile(const char *fileName, const std::vector<std::string> &lines, QString &path)
{
    path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("applications") + QDir::separator()
        + QString::fromUtf8(fileName);

    QSignalSpy databaseChangedSpy(KSycoca::self(), &KSycoca::databaseChanged);

    QFile out(path);
    if (out.exists()) {
        qDebug() << "Removing the old desktop file in" << path;
        out.remove();
    }

    qDebug() << "Creating a desktop file in" << path;
    QVERIFY(out.open(QIODevice::WriteOnly));
    out.write("[Desktop Entry]\n");
    for (const std::string &l : lines) {
        out.write((l + "\n").c_str());
    }
    out.close();

    KSycoca::self()->ensureCacheValid();
    databaseChangedSpy.wait(2500);
}

void testModelDataFromDesktopFile(const AbstractWindowTasksModel &model)
{
    // Case 1: A normal window
    std::vector<std::string> lines;
    lines.emplace_back("Name=DummyWindow");
    lines.emplace_back("GenericName=DummyGenericName");
    lines.emplace_back(std::string("Exec=") + TaskManagerTest::samplewidgetwindowExecutablePath);
    lines.emplace_back("Terminal=false");
    lines.emplace_back("Type=Application");
    lines.emplace_back(std::string("Icon=") + QFINDTESTDATA("data/windows/none.png").toStdString());

    // Test generic name, icon and launcher url
    QString desktopFilePath;
    TestUtils::createDesktopFile(dummyDesktopFileName, lines, desktopFilePath);

    QSignalSpy rowsInsertedSpy(&model, &AbstractWindowTasksModel::rowsInserted);
    QProcess sampleWindowProcess;
    sampleWindowProcess.setProgram(QString::fromUtf8(TaskManagerTest::samplewidgetwindowExecutablePath));
    sampleWindowProcess.setArguments(QStringList{
        QStringLiteral("__testwindow__%1").arg(QString::number(QDateTime::currentDateTime().offsetFromUtc())),
        QFINDTESTDATA("data/windows/samplewidgetwindow.png"),
    });
    sampleWindowProcess.start();
    rowsInsertedSpy.wait();

    // Find the window index
    auto findWindowIndex = [&model, &sampleWindowProcess](QModelIndex &index) {
        const auto results = model.match(model.index(0, 0), Qt::DisplayRole, sampleWindowProcess.arguments().at(0));
        QVERIFY(results.size() == 1);
        index = results.at(0);
        QVERIFY(index.isValid());
        qDebug() << "Window title:" << index.data(Qt::DisplayRole).toString();
    };

    QModelIndex index;
    findWindowIndex(index);

    QCOMPARE(index.data(AbstractTasksModel::AppName).toString(), QStringLiteral("DummyWindow"));
    QCOMPARE(index.data(AbstractTasksModel::GenericName).toString(), QStringLiteral("DummyGenericName"));
    QCOMPARE(index.data(AbstractTasksModel::LauncherUrl).toUrl(), QUrl(QStringLiteral("applications:%1").arg(QString::fromLatin1(dummyDesktopFileName))));
    QCOMPARE(index.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl(),
             QUrl(QStringLiteral("applications:%1").arg(QString::fromLatin1(dummyDesktopFileName))));

    // Test icon should use the icon from the desktop file (Not the png file filled with red color)
    const QIcon windowIcon = index.data(Qt::DecorationRole).value<QIcon>();
    QVERIFY(!windowIcon.isNull());
    QVERIFY(windowIcon.pixmap(KIconLoader::SizeLarge).toImage().pixelColor(KIconLoader::SizeLarge / 2, KIconLoader::SizeLarge / 2).red() < 200);

    // SingleMainWindow is not set, which implies it can launch a new instance.
    QVERIFY(index.data(AbstractTasksModel::CanLaunchNewInstance).toBool());

    QSignalSpy rowsRemovedSpy(&model, &AbstractWindowTasksModel::rowsRemoved);
    sampleWindowProcess.terminate();
    QVERIFY(rowsRemovedSpy.wait());

    auto testCanLaunchNewInstance = [&](bool canLaunchNewInstance) {
        TestUtils::createDesktopFile(dummyDesktopFileName, lines, desktopFilePath);
        sampleWindowProcess.start();
        rowsInsertedSpy.wait();

        findWindowIndex(index);
        QCOMPARE(index.data(AbstractTasksModel::CanLaunchNewInstance).toBool(), canLaunchNewInstance);

        sampleWindowProcess.terminate();
        QVERIFY(rowsRemovedSpy.wait());
    };

    // Case 2: Set SingleMainWindow or X-GNOME-SingleWindow or both
    lines.emplace_back("SingleMainWindow=true");
    testCanLaunchNewInstance(false);

    lines.pop_back();
    lines.emplace_back("X-GNOME-SingleWindow=true");
    testCanLaunchNewInstance(false);

    lines.pop_back();
    lines.emplace_back("SingleMainWindow=false");
    lines.emplace_back("X-GNOME-SingleWindow=true");
    testCanLaunchNewInstance(false);

    lines.pop_back();
    lines.pop_back();
    lines.emplace_back("SingleMainWindow=true");
    lines.emplace_back("X-GNOME-SingleWindow=false");
    testCanLaunchNewInstance(false);

    lines.pop_back();
    lines.pop_back();
    lines.emplace_back("SingleMainWindow=false");
    lines.emplace_back("X-GNOME-SingleWindow=false");
    testCanLaunchNewInstance(true);
}

void testRequest(AbstractWindowTasksModel &model)
{
    QSignalSpy rowsInsertedSpy(&model, &AbstractWindowTasksModel::rowsInserted);

    QProcess sampleWindowProcess;
    sampleWindowProcess.setProgram(QString::fromUtf8(TaskManagerTest::samplewidgetwindowExecutablePath));
    sampleWindowProcess.setArguments(QStringList{
        QStringLiteral("__testwindow__%1").arg(QString::number(QDateTime::currentDateTime().offsetFromUtc())),
    });
    sampleWindowProcess.start();
    QVERIFY(rowsInsertedSpy.wait());

    // Find the window index
    auto findWindowIndex = [&model](QModelIndex &index, const QString &title) {
        const auto results = model.match(model.index(0, 0), Qt::DisplayRole, title);
        qCritical() << "results for" << title << results << "total:" << model.rowCount();
        QVERIFY(results.size() == 1);
        index = results.at(0);
        QVERIFY(index.isValid());
        qDebug() << "Window title:" << index.data(Qt::DisplayRole).toString();
    };

    QModelIndex index;
    findWindowIndex(index, sampleWindowProcess.arguments().at(0));
    QVERIFY(index.isValid());

    QSignalSpy dataChangedSpy(&model, &AbstractWindowTasksModel::dataChanged);

    {
        qDebug("requestActivate");
        model.requestActivate(index);
        QVERIFY(dataChangedSpy.wait());
        QVERIFY(!index.data(AbstractTasksModel::IsMinimized).toBool());
    }

    {
        qDebug("requestNewInstance");
        model.requestNewInstance(index);
        QVERIFY(rowsInsertedSpy.wait());
    }

    {
        qDebug("requestToggleMinimized");
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleMinimized(index);
        if (dataChangedSpy.empty()) {
            QVERIFY(dataChangedSpy.wait());
        }
        QTRY_VERIFY(index.data(AbstractTasksModel::IsMinimized).toBool());
    }

    {
        qDebug("requestToggleMaximized");
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleMaximized(index);
        if (dataChangedSpy.empty()) {
            QVERIFY(dataChangedSpy.wait());
        }
        QTRY_VERIFY(!index.data(AbstractTasksModel::IsMinimized).toBool());
        QTRY_VERIFY(index.data(AbstractTasksModel::IsMaximized).toBool());
    }

    {
        qDebug("requestToggleKeepAbove");
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleKeepAbove(index);
        if (dataChangedSpy.empty()) {
            QVERIFY(dataChangedSpy.wait());
        }
        QTRY_VERIFY(index.data(AbstractTasksModel::IsKeepAbove).toBool());
        QTRY_VERIFY(!index.data(AbstractTasksModel::IsKeepBelow).toBool());
    }

    {
        qDebug("requestToggleKeepBelow");
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleKeepBelow(index);
        if (dataChangedSpy.empty()) {
            QVERIFY(dataChangedSpy.wait());
        }
        QTRY_VERIFY(!index.data(AbstractTasksModel::IsKeepAbove).toBool());
        QTRY_VERIFY(index.data(AbstractTasksModel::IsKeepBelow).toBool());
    }

    {
        qDebug("requestToggleFullScreen");
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleFullScreen(index);
        if (dataChangedSpy.empty()) {
            QVERIFY(dataChangedSpy.wait());
        }
        QTRY_VERIFY(index.data(AbstractTasksModel::IsFullScreen).toBool());
    }

#ifndef Q_OS_FREEBSD
    if (KWindowSystem::isPlatformX11()) {
        QCoreApplication::processEvents();
        dataChangedSpy.clear();
        model.requestToggleShaded(index);
        dataChangedSpy.wait();
        QTRY_VERIFY(index.data(AbstractTasksModel::IsShaded).toBool());
    }
#endif

    QSignalSpy rowsRemovedSpy(&model, &AbstractWindowTasksModel::rowsRemoved);
    {
        qDebug("requestClose");
        model.requestClose(index);
        QVERIFY(rowsRemovedSpy.wait());
    }

    {
        qDebug("Close the new instance");
        findWindowIndex(index, QStringLiteral("__test_window_no_title__"));
        model.requestClose(index);
        QVERIFY(rowsRemovedSpy.wait());
    }
}
}