/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utils/windowvisibilitymonitor.h"

#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

class WindowVisibilityMonitorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testNoWindow();
    void testWindowAlreadyExposed();
    void testWindowShownLater();
    void testWindowReplaced();
};

void WindowVisibilityMonitorTest::testNoWindow()
{
    WindowVisibilityMonitor monitor;
    QCOMPARE(monitor.exposed(), false);
    QCOMPARE(monitor.window(), nullptr);
}

void WindowVisibilityMonitorTest::testWindowAlreadyExposed()
{
    QQuickWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    WindowVisibilityMonitor monitor;
    QSignalSpy windowSpy(&monitor, &WindowVisibilityMonitor::windowChanged);
    QSignalSpy exposedSpy(&monitor, &WindowVisibilityMonitor::exposedChanged);

    monitor.setWindow(&window);

    QCOMPARE(monitor.window(), &window);
    QCOMPARE(monitor.exposed(), true);
    QCOMPARE(windowSpy.count(), 1);
    // The initial state is picked up without an expose event
    QCOMPARE(exposedSpy.count(), 1);

    window.hide();
    QTRY_COMPARE(monitor.exposed(), false);
}

void WindowVisibilityMonitorTest::testWindowShownLater()
{
    QQuickWindow window;

    WindowVisibilityMonitor monitor;
    monitor.setWindow(&window);
    QCOMPARE(monitor.exposed(), false);

    window.show();
    QTRY_COMPARE(monitor.exposed(), true);

    window.hide();
    QTRY_COMPARE(monitor.exposed(), false);
}

void WindowVisibilityMonitorTest::testWindowReplaced()
{
    QQuickWindow firstWindow;
    QQuickWindow secondWindow;

    WindowVisibilityMonitor monitor;
    monitor.setWindow(&firstWindow);
    firstWindow.show();
    QTRY_COMPARE(monitor.exposed(), true);

    monitor.setWindow(&secondWindow);
    QCOMPARE(monitor.exposed(), false);

    // Changing the visibility of the old window must not affect the monitor anymore.
    firstWindow.hide();
    QTest::qWait(50);
    QCOMPARE(monitor.exposed(), false);

    secondWindow.show();
    QTRY_COMPARE(monitor.exposed(), true);
}

QTEST_MAIN(WindowVisibilityMonitorTest)

#include "test_windowvisibilitymonitor.moc"
