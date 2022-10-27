/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>

#include <QDir>
#include <QScreen>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <kactivities/controller.h>

#include "../desktopview.h"
#include "../panelview.h"
#include "../screenpool.h"
#include "../shellcorona.h"
#include "mockcompositor.h"
#include "xdgoutputv1.h"

using namespace MockCompositor;

class ShellTest : public QObject, DefaultCompositor
{
    Q_OBJECT

    QPair<int, QScreen *> insertScreen(const QRect &geometry, const QString &name);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testScreenInsertion();
    void testPanelInsertion();
    void testSecondScreenInsertion();
    void testScreenRemoval();

private:
    ShellCorona *m_corona;
};

QPair<int, QScreen *> ShellTest::insertScreen(const QRect &geometry, const QString &name)
{
    QPair<int, QScreen *> result = {-1, nullptr};

    auto doTest = [=](QPair<int, QScreen *> &result) {
        int oldNumScreens = qApp->screens().size();
        // Fake screen?
        if (qApp->screens().size() == 1 && qApp->screens()[0]->name().isEmpty()) {
            QCOMPARE(m_corona->numScreens(), 0);
            oldNumScreens = 0;
        }

        QCOMPARE(oldNumScreens, m_corona->numScreens());
        QSignalSpy coronaAddedSpy(m_corona, SIGNAL(screenAdded(int)));
        QSignalSpy appAddedSpy(qGuiApp, SIGNAL(screenAdded(QScreen *)));

        // Add a new output
        exec([=] {
            OutputData data;
            data.mode.resolution = {geometry.width(), geometry.height()};
            data.position = {geometry.x(), geometry.y()};
            data.physicalSize = data.mode.physicalSizeForDpi(96);
            data.connector = name;
            // NOTE: assumes that when a screen is added it will already have the final geometry
            add<Output>(data);
        });

        coronaAddedSpy.wait();
        QTRY_COMPARE(appAddedSpy.size(), 1);

        QCOMPARE(m_corona->numScreens(), oldNumScreens + 1);
        QCOMPARE(qApp->screens().size(), oldNumScreens + 1);
        QCOMPARE(coronaAddedSpy.size(), 1);
        result.first = coronaAddedSpy.takeFirst().at(0).value<int>();
        result.second = appAddedSpy.takeFirst().at(0).value<QScreen *>();
        QCOMPARE(result.second->name(), name);
        QCOMPARE(result.second->geometry(), geometry);
        QCOMPARE(m_corona->m_desktopViewForScreen.count(), oldNumScreens + 1);
        auto *cont = m_corona->m_desktopViewForScreen[result.second]->containment();
        QCOMPARE(cont->screen(), result.first);
        QCOMPARE(m_corona->containmentForScreen(result.first, m_corona->m_activityController->currentActivity(), QString()), cont);
    };
    doTest(result);
    return result;
}

void ShellTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<QScreen *>();

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();

    qApp->setProperty("org.kde.KActivities.core.disableAutostart", true);
    m_corona = new ShellCorona();
    m_corona->setShell("org.kde.plasma.desktop");
    m_corona->init();

    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_corona->screenPool()->screens().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
    QCOMPARE(QGuiApplication::primaryScreen(), m_corona->screenPool()->primaryScreen());
    QCOMPARE(m_corona->screenPool()->id(m_corona->screenPool()->primaryScreen()->name()), 0);
    QCOMPARE(m_corona->screenPool()->connector(0), QStringLiteral("WL-1"));
}

void ShellTest::cleanupTestCase()
{
    QCOMPOSITOR_COMPARE(getAll<Output>().size(), 1); // Only the default output should be left
    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    // m_corona->deleteLater();
    m_corona->unload();
    QCOMPARE(m_corona->m_desktopViewForScreen.count(), 0);

    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
}

void ShellTest::cleanup()
{
    const int oldNumScreens = qApp->screens().size();
    QSignalSpy removedSpy(m_corona, SIGNAL(screenRemoved(int)));

    exec([=] {
        for (int i = oldNumScreens - 1; i >= 0; --i) {
            remove(output(i));
        }
    });

    removedSpy.wait();
    QCOMPARE(removedSpy.size(), oldNumScreens);

    // Cleanup all the containments that were created
    for (auto *c : m_corona->containments()) {
        if (c->containmentType() == Plasma::Types::PanelContainment || c->screen() != 0) {
            c->destroy();
        }
    }
    m_corona->m_waitingPanels.clear();
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    QCOMPARE(m_corona->numScreens(), 0);
    QCOMPARE(qApp->screens().size(), 1);
    QCOMPARE(qApp->screens()[0]->name(), QString());
    insertScreen(QRect(0, 0, 1920, 1080), QStringLiteral("WL-1"));
    QCOMPARE(qApp->screens().size(), 1);
}

void ShellTest::testScreenInsertion()
{
    auto geom = QRect(1920, 0, 1920, 1080);
    auto name = QStringLiteral("DP-1");
    auto result = insertScreen(geom, name);
    QCOMPARE(result.first, 1);
    QCOMPARE(result.second->geometry(), geom);
    QCOMPARE(qApp->screens().size(), 2);
    QCOMPARE(qApp->screens()[0]->geometry(), m_corona->m_desktopViewForScreen[qApp->screens()[0]]->geometry());
    QCOMPARE(qApp->screens()[1]->geometry(), m_corona->m_desktopViewForScreen[qApp->screens()[1]]->geometry());
    QCOMPARE(qApp->screens()[1]->name(), name);
}

void ShellTest::testPanelInsertion()
{
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
    // If the panel fails to load (on ci plasma-desktop isn't here) we want the "failed" containment to be of panel type anyways
    panelCont->setContainmentType(Plasma::Types::PanelContainment);
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    QVERIFY(m_corona->m_panelViews.contains(panelCont));
    QCOMPARE(panelCont->screen(), 0);
    QCOMPARE(m_corona->m_panelViews[panelCont]->screen(), qApp->primaryScreen());
}

void ShellTest::testSecondScreenInsertion()
{
    auto geom1 = QRect(1920, 0, 1920, 1080);
    auto name1 = QStringLiteral("DP-1");
    auto result1 = insertScreen(geom1, name1);

    auto geom2 = QRect(3840, 0, 1920, 1080);
    auto name2 = QStringLiteral("DP-2");
    auto result2 = insertScreen(geom2, name2);

    QCOMPARE(qApp->screens().size(), 3);
    QCOMPARE(result1.first, 1);
    QCOMPARE(result1.second->geometry(), geom1);
    QCOMPARE(result1.second->name(), name1);

    QCOMPARE(result2.first, 2);
    QCOMPARE(result2.second->geometry(), geom2);
    QCOMPARE(result2.second->name(), name2);
}

void ShellTest::testScreenRemoval()
{
    // Create 2 new screens
    testSecondScreenInsertion();

    Plasma::Containment *cont0 = m_corona->containmentForScreen(0, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont0->screen(), 0);
    Plasma::Containment *cont1 = m_corona->containmentForScreen(1, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont1->screen(), 1);
    Plasma::Containment *cont2 = m_corona->containmentForScreen(2, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont2->screen(), 2);

    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
    panelCont->setContainmentType(Plasma::Types::PanelContainment);
    m_corona->m_panelViews[panelCont]->setScreenToFollow(m_corona->m_screenPool->screenForId(2));
    QCOMPARE(panelCont->screen(), 2);
    QCOMPARE(m_corona->m_panelViews[panelCont]->screen(), m_corona->m_screenPool->screenForId(2));
    QCOMPARE(m_corona->m_panelViews.size(), 1);

    QSignalSpy removedSpy(m_corona, SIGNAL(screenRemoved(int)));

    // Remove outputs
    exec([=] {
        remove(output(2));
        remove(output(1));
    });

    QTRY_COMPARE(removedSpy.count(), 2);

    QCOMPARE(m_corona->numScreens(), 1);
    QCOMPARE(qApp->screens().size(), 1);
    QCOMPARE(removedSpy.size(), 2);
    // the removed screens are what corona calls 1 and 2, 0 remains
    QCOMPARE(removedSpy.takeFirst().at(0).value<int>(), 2);
    QCOMPARE(removedSpy.takeFirst().at(0).value<int>(), 1);
    QCOMPARE(m_corona->m_desktopViewForScreen.count(), 1);
    // The only view remained is the one which was primary already
    QCOMPARE(cont0, m_corona->m_desktopViewForScreen[qApp->primaryScreen()]->containment());
    QCOMPARE(qApp->screens()[0]->geometry(), m_corona->m_desktopViewForScreen[qApp->screens()[0]]->geometry());

    // Has the panelview been removed?
    QCOMPARE(m_corona->m_panelViews.size(), 0);

    bool cont1Found = false;
    bool cont2Found = false;
    // Search for the other two containments, should have screen() -1 and lastScreen 1 and 2
    for (auto *cont : m_corona->containments()) {
        if (cont == cont1) {
            cont1Found = true;
            QCOMPARE(cont->screen(), -1);
            QCOMPARE(cont->lastScreen(), 1);
        } else if (cont == cont2) {
            cont2Found = true;
            QCOMPARE(cont->screen(), -1);
            QCOMPARE(cont->lastScreen(), 2);
        }
    }
    QVERIFY(cont1Found);
    QVERIFY(cont2Found);
}

QCOMPOSITOR_TEST_MAIN(ShellTest)

#include "shelltest.moc"
