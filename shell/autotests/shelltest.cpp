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
#include <plasmaactivities/controller.h>
#include <qtestcase.h>

#include "../desktopview.h"
#include "../panelview.h"
#include "../screenpool.h"
#include "../shellcorona.h"
#include "mockcompositor.h"
#include "xdgoutputv1.h"

using namespace MockCompositor;

void copyDirectory(const QString &srcDir, const QString &dstDir)
{
    QDir targetDir(dstDir);
    QVERIFY(targetDir.mkpath(dstDir));
    QDirIterator it(srcDir, QDir::Filters(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Name), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString path = it.filePath();
        QString relDestPath = path.last(it.filePath().length() - srcDir.length() - 1);
        if (it.fileInfo().isDir()) {
            QVERIFY(targetDir.mkpath(relDestPath));
        } else {
            QVERIFY(QFile::copy(path, dstDir + u'/' + relDestPath));
        }
    }
}

class ShellTest : public QObject, DefaultCompositor
{
    Q_OBJECT

    QScreen *insertScreen(const QRect &geometry, const QString &name);
    void setScreenOrder(const QStringList &order, bool expectOrderChanged);
    void resetScreen();
    Plasma::Containment *addTestPanel(const QString &plugin);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testScreenInsertion();
    void testPanelInsertion();
    void testSecondScreenInsertion();
    void testRedundantScreenInsertion();
    void testScreenRemovalRecyclingViews();
    void testMoveOutOfRedundant();
    void testScreenRemoval();
    void testReorderScreens_data();
    void testReorderScreens();
    void testReorderContainments();
    void testPanelSizeModes();

private:
    ShellCorona *m_corona;
    QDir m_plasmaDir;
};

QScreen *ShellTest::insertScreen(const QRect &geometry, const QString &name)
{
    QScreen *result = nullptr;

    auto doTest = [=, this](QScreen *&res) {
        int oldAppNumScreens = qApp->screens().size();
        int oldCoronaNumScreens = m_corona->numScreens();

        // Fake screen?
        if (qApp->screens().size() == 1 && qApp->screens()[0]->name().isEmpty()) {
            QCOMPARE(m_corona->numScreens(), 0);
            oldAppNumScreens = 0;
        }

        // QSignalSpy coronaAddedSpy(m_corona, SIGNAL(screenAdded(int)));
        QSignalSpy appAddedSpy(qGuiApp, SIGNAL(screenAdded(QScreen *)));
        //  QSignalSpy orderChangeSpy(m_corona->m_screenPool, &ScreenPool::screenOrderChanged);

        // Add a new output
        exec([=, this] {
            OutputData data;
            data.mode.resolution = {geometry.width(), geometry.height()};
            data.position = {geometry.x(), geometry.y()};
            data.physicalSize = data.mode.physicalSizeForDpi(96);
            data.connector = name;
            // NOTE: assumes that when a screen is added it will already have the final geometry
            add<Output>(data);
        });

        //  coronaAddedSpy.wait();
        QTRY_COMPARE(appAddedSpy.size(), 1);
        //   QTRY_COMPARE(orderChangeSpy.size(), 1);
        QCOMPARE(m_corona->numScreens(), oldCoronaNumScreens); // Corona has *not* been notified yer as setScreenOrder has not been called
        QCOMPARE(m_corona->m_desktopViewForScreen.count(), oldCoronaNumScreens);
        QCOMPARE(qApp->screens().size(), oldAppNumScreens + 1);
        //     QCOMPARE(coronaAddedSpy.size(), 1);
        //     result.first = coronaAddedSpy.takeFirst().at(0).value<int>();
        res = appAddedSpy.takeFirst().at(0).value<QScreen *>();
        QCOMPARE(res->name(), name);
        QCOMPARE(res->geometry(), geometry);
        /*    auto *cont = m_corona->m_desktopViewForScreen[result.first]->containment();
            QCOMPARE(cont->screen(), result.first);
            QCOMPARE(m_corona->containmentForScreen(result.first, m_corona->m_activityController->currentActivity(), QString()), cont);*/
    };
    doTest(result);
    return result;
}

void ShellTest::setScreenOrder(const QStringList &order, bool expectOrderChanged)
{
    QSignalSpy coronaScreenOrderSpy(m_corona, &ShellCorona::screenOrderChanged);

    exec([=, this] {
        outputOrder()->setList(order);
    });

    if (expectOrderChanged) {
        coronaScreenOrderSpy.wait();
        Q_ASSERT(coronaScreenOrderSpy.count() == 1);
        QCOMPARE(coronaScreenOrderSpy.count(), 1);
        auto newOrder = coronaScreenOrderSpy.takeFirst().at(0).value<QList<QScreen *>>();
        QCOMPARE(m_corona->m_desktopViewForScreen.size(), newOrder.size());
    } else {
        coronaScreenOrderSpy.wait(250);
        Q_ASSERT(coronaScreenOrderSpy.count() == 0);
        QCOMPARE(coronaScreenOrderSpy.count(), 0);
    }

    QCOMPARE(m_corona->m_desktopViewForScreen.size(), m_corona->m_screenPool->screenOrder().size());

    for (int i = 0; i < m_corona->m_screenPool->screenOrder().size(); ++i) {
        QVERIFY(m_corona->m_desktopViewForScreen.contains(i));
        QCOMPARE(m_corona->m_desktopViewForScreen[i]->containment()->screen(), i);
        QCOMPARE(m_corona->m_desktopViewForScreen[i]->screenToFollow(), m_corona->m_screenPool->screenOrder()[i]);
        QCOMPARE(m_corona->m_screenPool->screenOrder()[i]->name(), order[i]);
    }
}

// Resets the screen order and waits for the ui to be ready
void ShellTest::resetScreen()
{
    QSignalSpy screenOrderChangedSpy(m_corona, &ShellCorona::screenOrderChanged);
    setScreenOrder({u"WL-1"_s}, false);
    screenOrderChangedSpy.wait();
}

// Adds a panel and waits for the waitingPanels to be empty
Plasma::Containment *ShellTest::addTestPanel(const QString &plugin)
{
    QSignalSpy uiReadySpy(m_corona, &ShellCorona::screenUiReadyChanged);
    auto panelCont = m_corona->addPanel(plugin);
    if (!m_corona->isScreenUiReady(panelCont->screen())) {
        QVERIFY(uiReadySpy.wait());
    }

    return panelCont;
}

void ShellTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<QScreen *>();

    m_plasmaDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + u"plasma");
    m_plasmaDir.removeRecursively();

    copyDirectory(QFINDTESTDATA("data/testpanel"), m_plasmaDir.absolutePath() + u"/plasmoids/org.kde.plasma.testpanel");

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();

    qApp->setProperty("org.kde.KActivities.core.disableAutostart", true);
    m_corona = new ShellCorona();
    m_corona->setShell(u"org.kde.plasma.nano"_s);
    m_corona->init();

    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    QCOMPARE(m_corona->screenPool()->screenOrder().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
    QCOMPARE(QGuiApplication::primaryScreen(), m_corona->screenPool()->primaryScreen());
    QCOMPARE(m_corona->screenPool()->idForScreen(m_corona->screenPool()->primaryScreen()), 0);
    QCOMPARE(m_corona->screenPool()->screenForId(0)->name(), QStringLiteral("WL-1"));
}

void ShellTest::cleanupTestCase()
{
    exec([this] {
        outputOrder()->setList({u"WL-1"_s});
    });
    QCOMPOSITOR_COMPARE(getAll<Output>().size(), 1); // Only the default output should be left
    QTRY_COMPARE(QGuiApplication::screens().size(), 1);
    // m_corona->deleteLater();
    m_corona->unload();
    QCOMPARE(m_corona->m_desktopViewForScreen.count(), 0);

    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("ScreenConnectors"));
    cg.deleteGroup();
    cg.sync();
    delete m_corona;

    // m_plasmaDir.removeRecursively();
}

void ShellTest::cleanup()
{
    const int oldNumScreens = qApp->screens().size();
    const int oldCoronaNumScreens = m_corona->numScreens();
    QVERIFY(oldCoronaNumScreens <= oldNumScreens);
    QSignalSpy coronaRemovedSpy(m_corona, SIGNAL(screenRemoved(int)));

    exec([=, this] {
        for (int i = oldNumScreens - 1; i >= 0; --i) {
            remove(output(i));
        }
    });
    setScreenOrder({}, true);

    QTRY_COMPARE(coronaRemovedSpy.size(), oldCoronaNumScreens);

    // Cleanup all the containments that were created
    for (auto *c : m_corona->containments()) {
        if (c->containmentType() == Plasma::Containment::Panel || c->screen() != 0) {
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
    QSignalSpy coronaScreenOrderSpy(m_corona, &ShellCorona::screenOrderChanged);
    setScreenOrder({u"WL-1"_s}, true);
}

void ShellTest::testScreenInsertion()
{
    const auto geom = QRect(1920, 0, 1920, 1080);
    const auto name = QStringLiteral("DP-1");
    auto result = insertScreen(geom, name);
    setScreenOrder({u"WL-1"_s, u"DP-1"_s}, true);
    QCOMPARE(result->geometry(), geom);
    QCOMPARE(qApp->screens().size(), 2);
    QCOMPARE(qApp->screens()[0]->geometry(), m_corona->m_desktopViewForScreen[0]->geometry());
    QCOMPARE(qApp->screens()[1]->geometry(), m_corona->m_desktopViewForScreen[1]->geometry());
    QCOMPARE(qApp->screens()[1]->name(), name);
}

void ShellTest::testPanelInsertion()
{
    resetScreen();
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
    QCOMPARE(panelCont->pluginMetaData().pluginId(), QStringLiteral("org.kde.plasma.testpanel"));
    // If the panel fails to load (on ci plasma-desktop isn't here) we want the "failed" containment to be of panel type anyways
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

    setScreenOrder({u"WL-1"_s, u"DP-1"_s, u"DP-2"_s}, true);

    QCOMPARE(qApp->screens().size(), 3);
    QCOMPARE(result1->geometry(), geom1);
    QCOMPARE(result1->name(), name1);

    QCOMPARE(result2->geometry(), geom2);
    QCOMPARE(result2->name(), name2);
}

void ShellTest::testRedundantScreenInsertion()
{
    QCOMPARE(m_corona->m_desktopViewForScreen.size(), 1);
    auto *view0 = m_corona->m_desktopViewForScreen[0];
    QCOMPARE(view0->screen()->name(), QStringLiteral("WL-1"));

    auto *cont0 = m_corona->m_desktopViewForScreen[0]->containment();
    QCOMPARE(cont0->screen(), 0);
    auto *oldScreen0 = view0->screen();

    const auto geom = QRect(0, 0, 1920, 1080);
    const auto name = QStringLiteral("DP-1");
    auto result = insertScreen(geom, name);
    // false as we do not expect to be notified for screenorderchanged
    setScreenOrder({u"WL-1"_s, u"DP-1"_s}, false);
    QCOMPARE(result->geometry(), geom);
    QCOMPARE(qApp->screens().size(), 2);
    // m_desktopViewForScreen did *not* get a view for the new screen
    QCOMPARE(m_corona->m_desktopViewForScreen.size(), 1);
    // associations of old things didn't change
    QCOMPARE(view0->containment(), cont0);
    QCOMPARE(cont0->screen(), 0);
    QCOMPARE(view0->screen(), oldScreen0);
}

void ShellTest::testMoveOutOfRedundant()
{
    testRedundantScreenInsertion();

    QSignalSpy coronaAddedSpy(m_corona, SIGNAL(screenAdded(int)));

    exec([this] {
        auto *out = output(1);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
        outputOrder()->setList({u"WL-1"_s, u"DP-1"_s});
    });

    coronaAddedSpy.wait();
    QCOMPARE(coronaAddedSpy.size(), 1);
    const int screen = coronaAddedSpy.takeFirst().at(0).value<int>();
    QCOMPARE(screen, 1);
}

void ShellTest::testScreenRemoval()
{
    // Create 2 new screens
    testSecondScreenInsertion();

    Plasma::Containment *cont0 = m_corona->containmentForScreen(0, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont0);
    QCOMPARE(cont0->screen(), 0);
    Plasma::Containment *cont1 = m_corona->containmentForScreen(1, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont1);
    QCOMPARE(cont1->screen(), 1);
    Plasma::Containment *cont2 = m_corona->containmentForScreen(2, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont2);
    QCOMPARE(cont2->screen(), 2);

    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
    m_corona->m_panelViews[panelCont]->setScreenToFollow(m_corona->m_screenPool->screenForId(2));
    QCOMPARE(panelCont->screen(), 2);
    QCOMPARE(m_corona->m_panelViews[panelCont]->screen(), m_corona->m_screenPool->screenForId(2));
    QCOMPARE(m_corona->m_panelViews.size(), 1);

    QSignalSpy removedSpy(m_corona, SIGNAL(screenRemoved(int)));

    // Remove outputs
    exec([this] {
        remove(output(2));
        remove(output(1));
    });

    QTRY_COMPARE(removedSpy.size(), 2);

    setScreenOrder({u"WL-1"_s}, true);

    QCOMPARE(m_corona->numScreens(), 1);
    QCOMPARE(qApp->screens().size(), 1);

    // the removed screens are what corona calls 1 and 2, 0 remains
    QCOMPARE(removedSpy.takeFirst().at(0).value<int>(), 2);
    QCOMPARE(removedSpy.takeFirst().at(0).value<int>(), 1);
    QCOMPARE(m_corona->m_desktopViewForScreen.count(), 1);
    // The only view remained is the one which was primary already
    QCOMPARE(cont0, m_corona->m_desktopViewForScreen[0]->containment());
    QCOMPARE(qApp->screens()[0]->geometry(), m_corona->m_desktopViewForScreen[0]->geometry());

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

void ShellTest::testScreenRemovalRecyclingViews()
{
    // Create 2 new screens
    testSecondScreenInsertion();

    Plasma::Containment *cont0 = m_corona->containmentForScreen(0, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont0);
    QCOMPARE(cont0->screen(), 0);
    Plasma::Containment *cont1 = m_corona->containmentForScreen(1, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont1);
    QCOMPARE(cont1->screen(), 1);
    Plasma::Containment *cont2 = m_corona->containmentForScreen(2, m_corona->m_activityController->currentActivity(), QString());
    QVERIFY(cont2);
    QCOMPARE(cont2->screen(), 2);

    auto *view0 = m_corona->m_desktopViewForScreen[0];
    auto screen0 = view0->screenToFollow();
    auto *view1 = m_corona->m_desktopViewForScreen[1];
    auto screen1 = view1->screenToFollow();
    auto *view2 = m_corona->m_desktopViewForScreen[2];
    auto screen2 = view2->screenToFollow();

    QSignalSpy view2DeletedSpy(view2, &DesktopView::destroyed);
    QSignalSpy screen1DeletedSpy(screen1, &DesktopView::destroyed);

    // Create a panel on screen 1
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    auto panelView = m_corona->m_panelViews[panelCont];
    panelView->setScreenToFollow(m_corona->m_screenPool->screenForId(1));
    QCOMPARE(panelCont->screen(), 1);
    QCOMPARE(panelView->screen(), m_corona->m_screenPool->screenForId(1));
    QCOMPARE(m_corona->m_panelViews.size(), 1);

    QSignalSpy removedSpy(m_corona, SIGNAL(screenRemoved(int)));

    // Remove output
    exec([this] {
        remove(output(1));
    });

    QTRY_COMPARE(removedSpy.size(), 1);

    setScreenOrder({u"WL-1"_s, u"DP-2"_s}, true);

    QTRY_COMPARE(view2DeletedSpy.count(), 1);
    QTRY_COMPARE(screen1DeletedSpy.count(), 1);

    QCOMPARE(m_corona->numScreens(), 2);
    QCOMPARE(qApp->screens().size(), 2);
    // the removed screens are what corona calls 1 and 2, 0 remains
    QCOMPARE(removedSpy.takeFirst().at(0).value<int>(), 2);
    QCOMPARE(m_corona->m_desktopViewForScreen.count(), 2);

    // Views for cont0 and cont1 remained, cont1 reassigned screen
    QCOMPARE(cont0, m_corona->m_desktopViewForScreen[0]->containment());
    QCOMPARE(cont1, m_corona->m_desktopViewForScreen[1]->containment());
    QCOMPARE(view0, m_corona->m_desktopViewForScreen[0]);
    QCOMPARE(view1, m_corona->m_desktopViewForScreen[1]);
    QCOMPARE(view0->screenToFollow(), screen0);
    QCOMPARE(view1->screenToFollow(), screen2);

    // The panel remained, moved to screen2
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    QCOMPARE(panelView->screenToFollow(), screen2);

    // Add DP-1 again, test that a view with cont2 inside is created
    insertScreen(QRect(1920, 0, 1920, 1080), QStringLiteral("DP-1"));

    setScreenOrder({u"WL-1"_s, u"DP-2"_s, u"DP-1"_s}, true);

    auto *newView2 = m_corona->m_desktopViewForScreen[2];
    QCOMPARE(m_corona->m_desktopViewForScreen.size(), 3);
    QCOMPARE(newView2->containment(), cont2);
}

void ShellTest::testReorderScreens_data()
{
    QTest::addColumn<QList<QRect>>("geometries");
    QTest::addColumn<QStringList>("orderBefore");
    QTest::addColumn<QStringList>("orderAfter");

    QTest::newRow("twoScreens") << QList<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}}) << QStringList({u"WL-1"_s, u"DP-1"_s})
                                << QStringList({u"DP-1"_s, u"WL-1"_s});
    QTest::newRow("3screensReorder0_1") << QList<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                        << QStringList({u"WL-1"_s, u"DP-1"_s, u"DP-2"_s}) << QStringList({u"DP-1"_s, u"WL-1"_s, u"DP-2"_s});
    QTest::newRow("3screensReorder2_3") << QList<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                        << QStringList({u"WL-1"_s, u"DP-1"_s, u"DP-2"_s}) << QStringList({u"DP-1"_s, u"WL-1"_s, u"DP-2"_s});
    QTest::newRow("3screensShuffled") << QList<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                      << QStringList({u"WL-1"_s, u"DP-1"_s, u"DP-2"_s}) << QStringList({u"DP-2"_s, u"DP-1"_s, u"WL-1"_s});
}

void ShellTest::testReorderScreens()
{
    QFETCH(QList<QRect>, geometries);
    QFETCH(QStringList, orderBefore);
    QFETCH(QStringList, orderAfter);

    QVERIFY(orderBefore.size() > 0);
    QCOMPARE(orderBefore.size(), geometries.size());
    QCOMPARE(orderBefore.size(), orderAfter.size());
    // At the beginning of the test there is always a default one
    QCOMPARE(orderBefore.first(), QStringLiteral("WL-1"));

    QHash<int, int> remapIndexes;

    for (int i = 0; i < orderBefore.size(); ++i) {
        const QString conn = orderBefore[i];
        remapIndexes[i] = orderAfter.indexOf(conn);
        // Verify that we have same connectors in orderBefore and orderAfter
        QVERIFY(remapIndexes[i] >= 0);
        if (conn != QStringLiteral("WL-1")) {
            insertScreen(geometries[i], conn);
        }
    }
    setScreenOrder(orderBefore, true);

    QList<DesktopView *> desktopViews;
    QList<PanelView *> panelViews;
    QList<Plasma::Containment *> desktopContainments;
    QList<Plasma::Containment *> panelContainments;
    QList<QScreen *> screens;

    for (int i = 0; i < orderBefore.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViews.append(view);

        desktopContainments.append(view->containment());
        screens.append(view->screenToFollow());

        QCOMPARE(view->screen()->name(), orderBefore[i]);
        QCOMPARE(view->containment()->screen(), i);
        QTRY_VERIFY(view->isExposed());

    }

    // Add a panel for each screen
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *s = screens[i];
        auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
        // If the panel fails to load (on ci plasma-desktop isn't here) we want the "failed" containment to be of panel type anyways
        QVERIFY(m_corona->m_panelViews.contains(panelCont));
        QCOMPARE(panelCont->screen(), 0);
        m_corona->m_panelViews[panelCont]->setScreenToFollow(s);
        QCOMPARE(panelCont->screen(), i);
        panelViews.append(m_corona->m_panelViews[panelCont]);
        panelContainments.append(panelCont);
    }

    QSignalSpy coronaScreenOrderSpy(m_corona, &ShellCorona::screenOrderChanged);
    setScreenOrder(orderAfter, true);

    QTRY_COMPARE(coronaScreenOrderSpy.count(), 1);
    QList<QScreen *> qScreenOrderFormSignal = coronaScreenOrderSpy.takeFirst().at(0).value<QList<QScreen *>>();

    QCOMPARE(qScreenOrderFormSignal.size(), orderAfter.size());
    for (int i = 0; i < qScreenOrderFormSignal.size(); ++i) {
        QCOMPARE(qScreenOrderFormSignal[i]->name(), orderAfter[i]);
    }

    for (int i = 0; i < orderAfter.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        view->screenToFollow();
    }

    QList<DesktopView *> desktopViewsAfter;
    QList<PanelView *> panelViewsAfter;
    panelViewsAfter.resize(orderAfter.size());
    QList<Plasma::Containment *> desktopContainmentsAfter;
    QList<Plasma::Containment *> panelContainmentsAfter;
    panelContainmentsAfter.resize(orderAfter.size());
    QList<QScreen *> screensAfter;

    for (int i = 0; i < orderAfter.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViewsAfter.append(view);
        desktopContainmentsAfter.append(view->containment());
        screensAfter.append(view->screenToFollow());

        QCOMPARE(view->screenToFollow(), qScreenOrderFormSignal[i]);
        QCOMPARE(view->screenToFollow()->name(), orderAfter[i]);
        QCOMPARE(view->containment()->screen(), i);
    }

    {
        QSet<int> allScreenIds;
        for (int i = 0; i < orderAfter.size(); ++i) {
            allScreenIds.insert(i);
        }
        for (PanelView *v : m_corona->m_panelViews) {
            Plasma::Containment *cont = v->containment();
            QVERIFY(cont->screen() >= 0);
            QVERIFY(allScreenIds.contains(cont->screen()));
            panelViewsAfter[cont->screen()] = v;
            panelContainmentsAfter[cont->screen()] = cont;
            allScreenIds.remove(cont->screen());
        }
    }

    for (int i = 0; i < orderAfter.size(); ++i) {
        QCOMPARE(desktopViews[i], desktopViewsAfter[i]);
        QCOMPARE(panelViews[i], panelViewsAfter[i]);
        QCOMPARE(desktopContainments[i], desktopContainmentsAfter[i]);
        QCOMPARE(panelContainments[i], panelContainmentsAfter[i]);
        QCOMPARE(screens[i], screensAfter[remapIndexes[i]]);

        QCOMPARE(desktopViewsAfter[i]->screenToFollow(), screensAfter[i]);
        QCOMPARE(desktopViewsAfter[i]->screenToFollow(), screens[remapIndexes[i]]);
        QCOMPARE(panelViewsAfter[i]->screenToFollow(), screensAfter[i]);
        QCOMPARE(panelViewsAfter[i]->screenToFollow(), screens[remapIndexes[i]]);
    }
}

void ShellTest::testReorderContainments()
{
    // this tests assigning different screens to containments without the actual order changing
    // Add screens
    resetScreen();
    auto geom1 = QRect(1920, 0, 1920, 1080);
    auto name1 = QStringLiteral("DP-1");
    auto result1 = insertScreen(geom1, name1);

    auto geom2 = QRect(3840, 0, 1920, 1080);
    auto name2 = QStringLiteral("DP-2");
    auto result2 = insertScreen(geom2, name2);

    setScreenOrder({u"WL-1"_s, u"DP-1"_s, u"DP-2"_s}, true);

    QCOMPARE(qApp->screens().size(), 3);
    QCOMPARE(result1->geometry(), geom1);
    QCOMPARE(result1->name(), name1);

    QCOMPARE(result2->geometry(), geom2);
    QCOMPARE(result2->name(), name2);

    // Add panel
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
    QCOMPARE(panelCont->pluginMetaData().pluginId(), QStringLiteral("org.kde.plasma.testpanel"));
    // If the panel fails to load (on ci plasma-desktop isn't here) we want the "failed" containment to be of panel type anyways
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    QVERIFY(m_corona->m_panelViews.contains(panelCont));
    QCOMPARE(panelCont->screen(), 0);
    QCOMPARE(m_corona->m_panelViews[panelCont]->screen(), qApp->primaryScreen());

    // Run the actual test
    QList<DesktopView *> desktopViews;
    QList<Plasma::Containment *> desktopContainments;
    QList<QScreen *> screens = m_corona->m_screenPool->screenOrder().toVector();
    QCOMPARE(screens.size(), 3);

    for (int i = 0; i < screens.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViews.append(view);
        desktopContainments.append(view->containment());

        QCOMPARE(view->screen(), screens[i]);
        QCOMPARE(view->containment()->screen(), i);
    }
    QCOMPARE(desktopViews.size(), 3);
    /*Disable the test on panels for now, as the fake containment we create here would be og NoContainment type
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    auto *panelView = m_corona->m_panelViews.values().first();
    auto panelContainment = panelView->containment();
    QCOMPARE(panelView->screen(), screens[0]);
    QCOMPARE(panelContainment->screen(), 0);

    // Move panel to screen 1
    m_corona->setScreenForContainment(panelContainment, 1);
    QCOMPARE(panelView->screen(), screens[1]);
    QCOMPARE(panelContainment->screen(), 1);
    */

    // Move containment at screen 0 to 1
    QCOMPARE(desktopContainments[0]->screen(), 0);
    QCOMPARE(desktopContainments[1]->screen(), 1);
    m_corona->setScreenForContainment(desktopContainments[0], 1);

    QCOMPARE(desktopContainments[0]->screen(), 1);
    QCOMPARE(desktopContainments[1]->screen(), 0);
    QCOMPARE(desktopViews[0]->screenToFollow(), screens[1]);
    QCOMPARE(desktopViews[1]->screenToFollow(), screens[0]);
}

void ShellTest::testPanelSizeModes()
{
    // Testing variables
    int thickness = 96;
    int lengthMax = 300;
    int lengthMin = lengthMax / 2;
    resetScreen();

    // Create a panel and prepare it for testing
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = addTestPanel(QStringLiteral("org.kde.plasma.testpanel"));
    // If the panel fails to load (on ci plasma-desktop isn't here) we want the "failed" containment to be of panel type anyways
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    QVERIFY(m_corona->m_panelViews.contains(panelCont));
    QCOMPARE(panelCont->screen(), 0);
    QCOMPARE(m_corona->m_panelViews[panelCont]->screen(), qApp->primaryScreen());

    auto panel = m_corona->m_panelViews[panelCont];

    QTRY_VERIFY(panel->isExposed());

    // Plase panel to bottom edge of screen
    panel->containment()->setLocation(Plasma::Types::BottomEdge);
    QCOMPARE(panel->containment()->location(), Plasma::Types::BottomEdge);

    // Set panel thickness
    panel->setThickness(thickness);
    QCOMPARE(panel->thickness(), thickness);

    // Set panel lengths
    panel->setLengthMode(PanelView::FillAvailable);
    QTRY_COMPARE(panel->size(), QSize(panel->screen()->size().width(), thickness)); // Panel should be screen width

    panel->setLengthMode(PanelView::Custom);
    // QVERIFY(panelResizeSpy.wait());
    panel->setMaximumLength(lengthMax);
    panel->setMinimumLength(lengthMin);
    QCOMPARE(panel->maximumLength(), lengthMax); // Panel should be custom width
    QCOMPARE(panel->minimumLength(), lengthMin);
    QTRY_COMPARE(panel->size(), QSize(std::min(panel->length(), lengthMax), thickness)); // Panel should be screen width

    panel->setLengthMode(PanelView::FitContent);
    QTRY_COMPARE(panel->size(), QSize(800, thickness));

    // Test floating setting
    panel->setFloating(true);
    QVERIFY(panel->floating());
    QCOMPARE(panel->thickness(), thickness);

    panel->setFloating(false);
    QVERIFY(!panel->floating());
    QCOMPARE(panel->thickness(), thickness);

    // Set vertical panel
    panel->containment()->setLocation(Plasma::Types::LeftEdge);
    panel->containment()->setFormFactor(Plasma::Types::Vertical);
    QTRY_COMPARE(panel->size(), QSize(thickness, 800));
}

QCOMPOSITOR_TEST_MAIN(ShellTest)

#include "shelltest.moc"
