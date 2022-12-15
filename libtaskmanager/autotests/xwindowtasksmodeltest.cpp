/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QDateTime>
#include <QQmlApplicationEngine>
#include <QtTest>

#include <KWindowSystem>

#include "xwindowtasksmodel.h"

using namespace TaskManager;
using MimeDataMap = QMap<QString, QByteArray>;

class XWindowTasksModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void test_winIdFromMimeData_data();
    void test_winIdFromMimeData();
    void test_winIdsFromMimeData_data();
    void test_winIdsFromMimeData();

    void test_openCloseWindow();

private:
    WId m_WId = 12345;
    QByteArray m_singleWId;
    QByteArray m_threeWIds;
    QByteArray m_negativeCountWIds;
    QByteArray m_insufficientWIds;
};

void XWindowTasksModelTest::initTestCase()
{
    char *singleWIdData = new char[sizeof(WId)];
    memcpy(singleWIdData, &m_WId, sizeof(WId));
    m_singleWId = QByteArray(singleWIdData, sizeof(WId));
    delete[] singleWIdData;

    constexpr int count = 3;
    char *threeWIdsData = new char[sizeof(int) + sizeof(WId) * count];
    memcpy(threeWIdsData, &count, sizeof(int));
    for (int i = 0; i < count; ++i) {
        memcpy(threeWIdsData + sizeof(int) + sizeof(WId) * i, &m_WId, sizeof(WId));
    }
    m_threeWIds = QByteArray(threeWIdsData, sizeof(int) + sizeof(WId) * count);
    delete[] threeWIdsData;

    constexpr int negativeCount = -count;
    char *negativeWIdsData = new char[sizeof(int) + sizeof(WId) * count];
    memcpy(negativeWIdsData, &negativeCount, sizeof(int));
    for (int i = 0; i < count; ++i) {
        memcpy(negativeWIdsData + sizeof(int) + sizeof(WId) * i, &m_WId, sizeof(WId));
    }
    m_negativeCountWIds = QByteArray(negativeWIdsData, sizeof(int) + sizeof(WId) * count);
    delete[] negativeWIdsData;

    constexpr int insufficientCount = count - 1;
    char *insufficientWIdsData = new char[sizeof(int) + sizeof(WId) * insufficientCount];
    memcpy(insufficientWIdsData, &count, sizeof(int));
    for (int i = 0; i < insufficientCount; ++i) {
        memcpy(insufficientWIdsData + sizeof(int) + sizeof(WId) * i, &m_WId, sizeof(WId));
    }
    m_insufficientWIds = QByteArray(insufficientWIdsData, sizeof(int) + sizeof(WId) * insufficientCount);
    delete[] insufficientWIdsData;

    QGuiApplication::setQuitOnLastWindowClosed(false);
}

void XWindowTasksModelTest::test_winIdFromMimeData_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("isOK");

    QTest::newRow("Valid WId") << QStringLiteral("windowsystem/winid") // XWindowTasksModel::Private::mimeType()
                               << m_singleWId << true;
    QTest::newRow("Invalid WId") << QStringLiteral("windowsystem/winid") << QByteArray("\x06") << false;
    QTest::newRow("Invalid mimeType") << QStringLiteral("text/plain") << m_singleWId << false;
}

void XWindowTasksModelTest::test_winIdFromMimeData()
{
    QFETCH(QString, mimeType);
    QFETCH(QByteArray, data);
    QFETCH(bool, isOK);

    QMimeData mimeData;
    mimeData.setData(mimeType, data);
    qDebug() << data << "data size:" << data.size();

    bool ok = !isOK;
    WId result = XWindowTasksModel::winIdFromMimeData(&mimeData, &ok);
    QCOMPARE(ok, isOK);
    if (isOK) {
        QCOMPARE(result, m_WId);
    }
}

void XWindowTasksModelTest::test_winIdsFromMimeData_data()
{
    QTest::addColumn<MimeDataMap>("mimeDataMap");
    QTest::addColumn<bool>("isOK");
    QTest::addColumn<int>("count");

    // if (!mimeData->hasFormat(Private::groupMimeType()))
    MimeDataMap data1;
    data1.insert(QStringLiteral("windowsystem/winid"), m_singleWId);
    QTest::newRow("Single WId") << data1 << true << data1.size();

    MimeDataMap data2;
    data2.insert(QStringLiteral("windowsystem/winid"), QByteArray("\x06"));
    QTest::newRow("Invalid WId") << data2 << false << 0;

    MimeDataMap data3;
    data3.insert(QStringLiteral("windowsystem/multiple-winids"), m_threeWIds);
    QTest::newRow("Three WIds") << data3 << true << 3;

    // if ((unsigned int)data.size() < sizeof(int) + sizeof(WId))
    MimeDataMap data4;
    data4.insert(QStringLiteral("windowsystem/multiple-winids"), QByteArray("\x06"));
    data4.insert(QStringLiteral("windowsystem/winid"), m_singleWId);
    QTest::newRow("Invalid WIds") << data4 << false << 0;

    // count < 1
    MimeDataMap data5;
    data5.insert(QStringLiteral("windowsystem/multiple-winids"), m_negativeCountWIds);
    QTest::newRow("Negative count WIds") << data5 << false << 0;

    // (unsigned int)data.size() < sizeof(int) + sizeof(WId) * count
    MimeDataMap data6;
    data6.insert(QStringLiteral("windowsystem/multiple-winids"), m_insufficientWIds);
    QTest::newRow("Insufficient count WIds") << data6 << false << 0;
}

void XWindowTasksModelTest::test_winIdsFromMimeData()
{
    QFETCH(MimeDataMap, mimeDataMap);
    QFETCH(bool, isOK);
    QFETCH(int, count);

    QMimeData mimeData;
    for (auto it = mimeDataMap.cbegin(); it != mimeDataMap.cend(); it = std::next(it)) {
        mimeData.setData(it.key(), it.value());
    }

    bool ok = !isOK;
    auto results = XWindowTasksModel::winIdsFromMimeData(&mimeData, &ok);
    QCOMPARE(ok, isOK);
    QCOMPARE(results.size(), count);
    if (count > 0) {
        const bool verified = std::all_of(results.cbegin(), results.cend(), [this](WId id) {
            return id == m_WId;
        });
        QVERIFY(verified);
    }
}

void XWindowTasksModelTest::test_openCloseWindow()
{
    if (!KWindowSystem::isPlatformX11()) {
        QSKIP("Test is not running on X11.");
    }

    XWindowTasksModel model;

    auto findWindow = [&model](const QString &windowTitle) {
        for (int i = 0; i < model.rowCount(); ++i) {
            const QString title = model.index(i, 0).data(Qt::DisplayRole).toString();
            if (title == windowTitle) {
                return true;
            }
        }
        return false;
    };

    int modelCount = model.rowCount();

    // Create a window to test if XWindowTasksModel can receive it
    QSignalSpy rowInsertedSpy(&model, &XWindowTasksModel::rowsInserted);

    const QString qmlFileName = QFINDTESTDATA("data/windows/SampleWindow.qml");
    QQmlApplicationEngine engine;
    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("title"), QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString()));
    engine.setInitialProperties(initialProperties);
    engine.load(qmlFileName);

    // A new window appears
    QVERIFY(rowInsertedSpy.wait());
    QCOMPARE(modelCount + 1, model.rowCount());
    // Find the window in the model
    QVERIFY(findWindow(initialProperties[QStringLiteral("title")].toString()));

    // Change the title of the window
    QSignalSpy dataChangedSpy(&model, &XWindowTasksModel::dataChanged);
    const QString newTitle = initialProperties[QStringLiteral("title")].toString() + QStringLiteral("__newtitle__");
    QVERIFY(engine.rootObjects().at(0)->setProperty("title", newTitle));
    QVERIFY(dataChangedSpy.wait());
    QVERIFY(dataChangedSpy.takeLast().at(2).value<QVector<int>>().contains(Qt::DisplayRole));
    // Make sure the title is updated
    QVERIFY(!findWindow(initialProperties[QStringLiteral("title")].toString()));
    QVERIFY(findWindow(newTitle));

    // Now close the window
    modelCount = model.rowCount();
    QSignalSpy rowsRemovedSpy(&model, &XWindowTasksModel::rowsRemoved);
    QMetaObject::invokeMethod(engine.rootObjects().at(0), "close", Qt::QueuedConnection);
    QVERIFY(rowsRemovedSpy.wait());
    QCOMPARE(modelCount - 1, model.rowCount());
}

QTEST_MAIN(XWindowTasksModelTest)

#include "xwindowtasksmodeltest.moc"
