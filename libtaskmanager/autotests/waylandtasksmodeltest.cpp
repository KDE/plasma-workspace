/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#define QT_FORCE_ASSERTS 1

#include <qpa/qplatformnativeinterface.h>

#include <wayland-client-core.h>

#include "common.h"
#include "waylandtasksmodel.h"

using namespace TaskManager;

class WaylandTasksModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void test_openCloseWindow();
    void test_modelData();
    void test_fullscreen();
    void test_geometry();
    void test_stackingOrder();
    void test_modelDataFromDesktopFile();

    void test_request();

private:
    WaylandTasksModel m_model;
};

void WaylandTasksModelTest::initTestCase()
{
    TestUtils::initTestCase();

    if (!KWindowSystem::isPlatformWayland()) {
        QSKIP("Test is not running on Wayland.");
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);

    QStandardPaths::setTestModeEnabled(true);

    const QString applicationDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("applications");
    QDir dir;
    if (!dir.exists(applicationDir)) {
        dir.mkpath(applicationDir);
    }

    QPlatformNativeInterface *const native = qGuiApp->platformNativeInterface();
    wl_display_roundtrip(static_cast<struct wl_display *>(native->nativeResourceForIntegration("wl_display")));
}

void WaylandTasksModelTest::cleanupTestCase()
{
    TestUtils::cleanupTestCase();
}

void WaylandTasksModelTest::test_openCloseWindow()
{
    TestUtils::testOpenCloseWindow(m_model);
}

void WaylandTasksModelTest::test_modelData()
{
    const QString title = QStringLiteral("__testwindow__%1").arg(QDateTime::currentDateTime().toString());
    QModelIndex index;
    auto window = TestUtils::createSingleWindow(m_model, title, index);

    // See WaylandTasksModel::data for available roles
    // Icon is unavailable on Wayland

    QTRY_COMPARE(index.data(AbstractTasksModel::WinIdList).toList().size(), 1);
    QVERIFY(index.data(AbstractTasksModel::MimeType).toString().startsWith(QLatin1String("windowsystem/winid")));

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
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsShadeable).toBool()); // It's intentionally not implemented on wayland
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsShaded).toBool());
    QTRY_VERIFY(index.data(AbstractTasksModel::IsVirtualDesktopsChangeable).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::IsOnAllVirtualDesktops).toBool());

    // Due to window decoration, the size of a window can't be determined accurately
    const QRect screenGeometry = index.data(AbstractTasksModel::ScreenGeometry).toRect();
    QVERIFY(screenGeometry.width() > 0 && screenGeometry.height() > 0);

    QTRY_VERIFY(!index.data(AbstractTasksModel::IsDemandingAttention).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::SkipTaskbar).toBool());
    QTRY_VERIFY(!index.data(AbstractTasksModel::SkipPager).toBool());
    // AbstractTasksModel::AppPid

    QVERIFY(index.data(AbstractTasksModel::CanLaunchNewInstance).toBool());
}

void WaylandTasksModelTest::test_fullscreen()
{
    TestUtils::testFullscreen(m_model);
}

void WaylandTasksModelTest::test_geometry()
{
    TestUtils::testGeometry(m_model);
}

void WaylandTasksModelTest::test_stackingOrder()
{
    TestUtils::testStackingOrder(m_model);
}

void WaylandTasksModelTest::test_modelDataFromDesktopFile()
{
    TestUtils::testModelDataFromDesktopFile(m_model);
}

void WaylandTasksModelTest::test_request()
{
    TestUtils::testRequest(m_model);
}

QTEST_MAIN(WaylandTasksModelTest)

#include "waylandtasksmodeltest.moc"
