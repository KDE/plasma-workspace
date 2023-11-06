/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <array>

#include <private/qtx11extras_p.h>

#include <KActivities/Consumer>
#include <KWindowInfo>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "common.h"
#include "xwindowtasksmodel.h"

using namespace TaskManager;
using MimeDataMap = QMap<QString, QByteArray>;

class XWindowTasksModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void test_winIdFromMimeData_data();
    void test_winIdFromMimeData();
    void test_winIdsFromMimeData_data();
    void test_winIdsFromMimeData();

    void test_openCloseWindow();
    void test_modelData();
    void test_isMinimized();
    void test_fullscreen();
    void test_geometry();
    void test_stackingOrder();
    void test_lastActivated();
    void test_modelDataFromDesktopFile();
    void test_windowState();

    void test_request();

private:
    WId m_WId = 12345;
    QByteArray m_singleWId;
    QByteArray m_threeWIds;
    QByteArray m_negativeCountWIds;
    QByteArray m_insufficientWIds;

    XWindowTasksModel m_model;
};

void XWindowTasksModelTest::initTestCase()
{
    TestUtils::initTestCase();
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

    QStandardPaths::setTestModeEnabled(true);

    const QString applicationDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("applications");
    QDir dir;
    if (!dir.exists(applicationDir)) {
        dir.mkpath(applicationDir);
    }
}

void XWindowTasksModelTest::cleanupTestCase()
{
    TestUtils::cleanupTestCase();
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
    TestUtils::testOpenCloseWindow(m_model);
}

void XWindowTasksModelTest::test_modelData()
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = TestUtils::createSingleWindow(m_model, title, index);

    // See XWindowTasksModel::data for available roles
    { // BEGIN Icon
        QSignalSpy dataChangedSpy(&m_model, &XWindowTasksModel::dataChanged);
        qDebug() << "Start testing Qt::DecorationRole";
        const QIcon oldWindowIcon = index.data(Qt::DecorationRole).value<QIcon>(); // X11 icon
        QVERIFY(!oldWindowIcon.isNull());
        window->setIcon(QIcon(QFINDTESTDATA("data/windows/samplewidgetwindow.png")));
        QVERIFY(dataChangedSpy.wait());
        QVERIFY(dataChangedSpy.takeLast().at(2).value<QList<int>>().contains(Qt::DecorationRole));

        const QIcon newWindowIcon = index.data(Qt::DecorationRole).value<QIcon>();
        QVERIFY(!newWindowIcon.isNull());
        QVERIFY(oldWindowIcon.pixmap(KIconLoader::SizeLarge).toImage().pixelColor(KIconLoader::SizeLarge / 2, KIconLoader::SizeLarge / 2)
                != newWindowIcon.pixmap(KIconLoader::SizeLarge).toImage().pixelColor(KIconLoader::SizeLarge / 2, KIconLoader::SizeLarge / 2));

        window->setIcon(QIcon());
        QVERIFY(dataChangedSpy.wait());
        QVERIFY(dataChangedSpy.takeLast().at(2).value<QList<int>>().contains(Qt::DecorationRole));
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

void XWindowTasksModelTest::test_isMinimized()
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = TestUtils::createSingleWindow(m_model, title, index);

    QTRY_VERIFY(!index.data(AbstractTasksModel::IsMinimized).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsHidden).toBool());

    // Minimize the window
    QSignalSpy dataChangedSpy(&m_model, &XWindowTasksModel::dataChanged);
    window->showMinimized();
    dataChangedSpy.wait();
    // There can be more than one dataChanged signal being emitted due to caching
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsMinimized);
    }));
    // The model doesn't notify data change stored under IsHidden role
    QTRY_VERIFY(std::none_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsHidden);
    }));
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsActive);
    }));
    QTRY_VERIFY(index.data(AbstractTasksModel::IsMinimized).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsHidden).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsActive).toBool());

    // Restore the window
    dataChangedSpy.clear();
    window->showNormal();
    window->raise();
    window->requestActivate();
    dataChangedSpy.wait();
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsMinimized);
    }));
    QVERIFY(std::none_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsHidden);
    }));
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsActive);
    }));
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsMinimized).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsHidden).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsActive).toBool());
}

void XWindowTasksModelTest::test_fullscreen()
{
    TestUtils::testFullscreen(m_model);
}

void XWindowTasksModelTest::test_geometry()
{
    TestUtils::testGeometry(m_model);
}

void XWindowTasksModelTest::test_stackingOrder()
{
    TestUtils::testStackingOrder(m_model);
}

void XWindowTasksModelTest::test_lastActivated()
// Re-activate the window to update the last activated time
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = TestUtils::createSingleWindow(m_model, title, index);

    QSignalSpy dataChangedSpy(&m_model, &XWindowTasksModel::dataChanged);
    window->showMinimized();
    dataChangedSpy.wait();
    QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::IsMinimized);
    }));
    QTRY_VERIFY(index.data(AbstractTasksModel::IsMinimized).toBool());

    window->showNormal();
    window->raise();
    window->requestActivate();
    const QTime lastActivatedTime = QTime::currentTime();
    dataChangedSpy.wait();
    // There can be more than one dataChanged signal being emitted due to caching
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsMinimized).toBool());
    // The model doesn't notify data change stored under LastActivated role
    QTRY_VERIFY(std::none_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [](const QVariantList &list) {
        return list.at(2).value<QList<int>>().contains(AbstractTasksModel::LastActivated);
    }));
    qDebug() << lastActivatedTime.msecsSinceStartOfDay() << index.data(AbstractTasksModel::LastActivated).toTime().msecsSinceStartOfDay();
    QVERIFY(std::abs(lastActivatedTime.msecsSinceStartOfDay() - index.data(AbstractTasksModel::LastActivated).toTime().msecsSinceStartOfDay()) < 1000);
}

void XWindowTasksModelTest::test_modelDataFromDesktopFile()
{
    TestUtils::testModelDataFromDesktopFile(m_model);
}

void XWindowTasksModelTest::test_windowState()
{
    QSignalSpy rowsInsertedSpy(&m_model, &XWindowTasksModel::rowsInserted);

    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = TestUtils::createSingleWindow(m_model, title, index);

    QSignalSpy dataChangedSpy(&m_model, &XWindowTasksModel::dataChanged);

    // NETWinInfo only allows a window manager set window states
    std::array<std::string, 11> actions{
        "_NET_WM_ACTION_MOVE",
        "_NET_WM_ACTION_RESIZE",
        "_NET_WM_ACTION_MINIMIZE",
        "_NET_WM_ACTION_SHADE",
        "_NET_WM_ACTION_STICK",
        "_NET_WM_ACTION_MAXIMIZE_VERT",
        "_NET_WM_ACTION_MAXIMIZE_HORZ",
        "_NET_WM_ACTION_FULLSCREEN",
        "_NET_WM_ACTION_CHANGE_DESKTOP",
        "_NET_WM_ACTION_CLOSE",
        "_NET_WM_ALLOWED_ACTIONS",
    };
    xcb_atom_t atoms[11];
    {
        xcb_intern_atom_cookie_t cookies[11];
        for (std::size_t i = 0; i < actions.size(); ++i) {
            cookies[i] = xcb_intern_atom(QX11Info::connection(), false, actions[i].size(), actions[i].c_str());
        }
        // Get the replies
        for (std::size_t i = 0; i < actions.size(); ++i) {
            xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(QX11Info::connection(), cookies[i], nullptr);
            if (!reply) {
                continue;
            }
            atoms[i] = reply->atom;
            free(reply);
        }
        qDebug() << "XCB atom cached";
    }

    auto setAllowedActionsAndVerify = [&](AbstractTasksModel::AdditionalRoles role, NET::Actions allowedActions, bool &success) {
        qDebug() << "Start testing" << role;
        success = false;
        dataChangedSpy.clear();
        QVERIFY(index.data(role).toBool());

        uint32_t data[50];
        int count = 0;
        if (allowedActions & NET::ActionMove) {
            data[count++] = atoms[0];
        }
        if (allowedActions & NET::ActionResize) {
            data[count++] = atoms[1];
        }
        if (allowedActions & NET::ActionMinimize) {
            data[count++] = atoms[2];
        }
        if (allowedActions & NET::ActionShade) {
            data[count++] = atoms[3];
        }
        if (allowedActions & NET::ActionStick) {
            data[count++] = atoms[4];
        }
        if (allowedActions & NET::ActionMaxVert) {
            data[count++] = atoms[5];
        }
        if (allowedActions & NET::ActionMaxHoriz) {
            data[count++] = atoms[6];
        }
        if (allowedActions & NET::ActionFullScreen) {
            data[count++] = atoms[7];
        }
        if (allowedActions & NET::ActionChangeDesktop) {
            data[count++] = atoms[8];
        }
        if (allowedActions & NET::ActionClose) {
            data[count++] = atoms[9];
        }

        xcb_change_property(QX11Info::connection(), XCB_PROP_MODE_REPLACE, window->winId(), atoms[10], XCB_ATOM_ATOM, 32, count, (const void *)data);
        xcb_flush(QX11Info::connection());
        QCoreApplication::processEvents();

        QTRY_VERIFY(std::any_of(dataChangedSpy.cbegin(), dataChangedSpy.cend(), [role](const QVariantList &list) {
            return list.at(2).value<QList<int>>().contains(role);
        }));
        QVERIFY(!index.data(role).toBool());
        success = true;
    };

    bool success = false;
    const auto fullFlags = NET::ActionMove | NET::ActionResize | NET::ActionMinimize | NET::ActionShade | NET::ActionStick | NET::ActionMaxVert
        | NET::ActionMaxHoriz | NET::ActionFullScreen | NET::ActionChangeDesktop | NET::ActionClose;

    // Make the window not movable
    setAllowedActionsAndVerify(AbstractTasksModel::IsMovable, fullFlags & (~NET::ActionMove), success);
    QVERIFY(success);

    // Make the window not resizable
    setAllowedActionsAndVerify(AbstractTasksModel::IsResizable, fullFlags & (~NET::ActionResize), success);
    QVERIFY(success);

    // Make the window not maximizable
    setAllowedActionsAndVerify(AbstractTasksModel::IsMaximizable, fullFlags & (~NET::ActionMax), success);
    QVERIFY(success);

    // Make the window not minimizable
    setAllowedActionsAndVerify(AbstractTasksModel::IsMinimizable, fullFlags & (~NET::ActionMinimize), success);
    QVERIFY(success);

    // Make the window not fullscreenable
    setAllowedActionsAndVerify(AbstractTasksModel::IsFullScreenable, fullFlags & (~NET::ActionFullScreen), success);
    QVERIFY(success);

    // Make the window not shadeable
    setAllowedActionsAndVerify(AbstractTasksModel::IsShadeable, fullFlags & (~NET::ActionShade), success);
    QVERIFY(success);

    // Make the window not able to change virtual desktop
    setAllowedActionsAndVerify(AbstractTasksModel::IsVirtualDesktopsChangeable, fullFlags & (~NET::ActionChangeDesktop), success);
    QVERIFY(success);

    // Make the window not closable
    setAllowedActionsAndVerify(AbstractTasksModel::IsClosable, fullFlags & (~NET::ActionClose), success);
    QVERIFY(success);
}

void XWindowTasksModelTest::test_request()
{
    TestUtils::testRequest(m_model);
}

QTEST_MAIN(XWindowTasksModelTest)

#include "xwindowtasksmodeltest.moc"
