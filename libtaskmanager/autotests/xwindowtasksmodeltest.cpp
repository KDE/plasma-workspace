/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QDateTime>
#include <QQmlApplicationEngine>
#include <QRasterWindow>
#include <QtTest>

#include <KActivities/Consumer>
#include <KIconLoader>
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
    void test_modelData();

private:
    std::unique_ptr<QRasterWindow> createSingleWindow(const QString &title, QModelIndex &index);

    WId m_WId = 12345;
    QByteArray m_singleWId;
    QByteArray m_threeWIds;
    QByteArray m_negativeCountWIds;
    QByteArray m_insufficientWIds;

    XWindowTasksModel m_model;
};

void XWindowTasksModelTest::initTestCase()
{
    if (!KWindowSystem::isPlatformX11()) {
        QSKIP("Test is not running on X11.");
    }

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

void XWindowTasksModelTest::test_modelData()
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = createSingleWindow(title, index);

    // See XWindowTasksModel::data for available roles
    { // BEGIN Icon
        QSignalSpy dataChangedSpy(&m_model, &XWindowTasksModel::dataChanged);
        qDebug() << "Start testing Qt::DecorationRole";
        const QIcon oldWindowIcon = index.data(Qt::DecorationRole).value<QIcon>(); // X11 icon
        QVERIFY(!oldWindowIcon.isNull());
        window->setIcon(QIcon(QFINDTESTDATA("data/windows/samplewidgetwindow.png")));
        QVERIFY(dataChangedSpy.wait());
        QVERIFY(dataChangedSpy.takeLast().at(2).value<QVector<int>>().contains(Qt::DecorationRole));

        const QIcon newWindowIcon = index.data(Qt::DecorationRole).value<QIcon>();
        QVERIFY(!newWindowIcon.isNull());
        QVERIFY(oldWindowIcon.pixmap(KIconLoader::SizeLarge).toImage().pixelColor(KIconLoader::SizeLarge / 2, KIconLoader::SizeLarge / 2)
                != newWindowIcon.pixmap(KIconLoader::SizeLarge).toImage().pixelColor(KIconLoader::SizeLarge / 2, KIconLoader::SizeLarge / 2));

        window->setIcon(QIcon());
        QVERIFY(dataChangedSpy.wait());
        QVERIFY(dataChangedSpy.takeLast().at(2).value<QVector<int>>().contains(Qt::DecorationRole));
    } // END Icon

    const NET::Properties windowInfoFlags =
        NET::WMState | NET::XAWMState | NET::WMDesktop | NET::WMVisibleName | NET::WMGeometry | NET::WMFrameExtents | NET::WMWindowType | NET::WMPid;
    const NET::Properties2 windowInfoFlags2 = NET::WM2DesktopFileName | NET::WM2Activities | NET::WM2WindowClass | NET::WM2AllowedActions
        | NET::WM2AppMenuObjectPath | NET::WM2AppMenuServiceName | NET::WM2GTKApplicationId;
    KWindowInfo info(window->winId(), windowInfoFlags, windowInfoFlags2);

    QTRY_COMPARE(index.data(AbstractTasksModel::AppId).toString(), info.windowClassClass());
    QTRY_COMPARE(index.data(AbstractTasksModel::AppName).toString(), info.windowClassClass());
    QTRY_COMPARE(index.data(AbstractTasksModel::GenericName).toString(), QString());
    QTRY_VERIFY(index.data(AbstractTasksModel::LauncherUrl).toUrl().toLocalFile().endsWith(info.windowClassClass()));
    QTRY_VERIFY(index.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl().toLocalFile().endsWith(info.windowClassClass()));
    QTRY_COMPARE(index.data(AbstractTasksModel::WinIdList).toList().size(), 1);
    QTRY_COMPARE(index.data(AbstractTasksModel::MimeType).toString(), QStringLiteral("windowsystem/winid"));

    {
        QMimeData mimeData;
        mimeData.setData(index.data(AbstractTasksModel::MimeType).toString(), index.data(AbstractTasksModel::MimeData).toByteArray());
        bool ok = false;
        XWindowTasksModel::winIdFromMimeData(&mimeData, &ok);
        QVERIFY(ok);
    }

    QTRY_VERIFY(index.data(AbstractTasksModel::IsWindow).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsActive).toBool());

    QTRY_VERIFY(index.data(AbstractTasksModel::IsClosable).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsMovable).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsResizable).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsMaximizable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsMaximized).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsMinimizable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsKeepAbove).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsKeepBelow).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsFullScreenable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsFullScreen).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsShadeable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsShaded).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsVirtualDesktopsChangeable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsOnAllVirtualDesktops).toBool());

    // Due to window decoration, the size of a window can't be determined accurately
    const QRect screenGeometry = index.data(AbstractTasksModel::ScreenGeometry).toRect();
    QVERIFY(screenGeometry.width() > 0 && screenGeometry.height() > 0);

    KActivities::Consumer activityConsumer;
    qDebug() << "Start testing AbstractTasksModel::Activities. Current activity number:" << activityConsumer.runningActivities().size();
    if (activityConsumer.runningActivities().size() > 0) {
        QCOMPARE(index.data(AbstractTasksModel::Activities).toStringList(), info.activities());
    } else {
        // In CI the window manager is openbox, so there could be no running activity.
        QCOMPARE(index.data(AbstractTasksModel::Activities).toStringList().size(), 0);
    }

    QTRY_VERIFY(!index.data(AbstractTasksModel::IsDemandingAttention).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::SkipTaskbar).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::SkipPager).toBool());
    QTRY_COMPARE(index.data(AbstractTasksModel::AppPid).toInt(), info.pid());

    QVERIFY(index.data(AbstractTasksModel::CanLaunchNewInstance).toBool());
}

std::unique_ptr<QRasterWindow> XWindowTasksModelTest::createSingleWindow(const QString &title, QModelIndex &index)
{
    auto window = std::make_unique<QRasterWindow>();
    window->setTitle(title);
    window->setBaseSize(QSize(320, 240));

    QSignalSpy rowInsertedSpy(&m_model, &XWindowTasksModel::rowsInserted);
    window->show();
    Q_ASSERT(rowInsertedSpy.wait());

    // Find the window index
    const auto results = m_model.match(m_model.index(0, 0), Qt::DisplayRole, title);
    Q_ASSERT(results.size() == 1);
    index = results.at(0);
    Q_ASSERT(index.isValid());
    qDebug() << "Window title:" << index.data(Qt::DisplayRole).toString();

    return window;
}

QTEST_MAIN(XWindowTasksModelTest)

#include "xwindowtasksmodeltest.moc"
