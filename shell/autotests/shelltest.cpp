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
#include <qtestcase.h>

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

    QScreen *insertScreen(const QRect &geometry, const QString &name);
    void setScreenOrder(const QStringList &order, bool expectOrderChanged);

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

private:
    ShellCorona *m_corona;
};

QScreen *ShellTest::insertScreen(const QRect &geometry, const QString &name)
{
    QScreen *result = nullptr;

    auto doTest = [=](QScreen *&res) {
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
        exec([=] {
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

    exec([=] {
        outputOrder()->setList(order);
    });

    if (expectOrderChanged) {
        coronaScreenOrderSpy.wait();
        QCOMPARE(coronaScreenOrderSpy.count(), 1);
        auto order = coronaScreenOrderSpy.takeFirst().at(0).value<QList<QScreen *>>();
        QCOMPARE(m_corona->m_desktopViewForScreen.size(), order.size());
    } else {
        coronaScreenOrderSpy.wait(250);
        QCOMPARE(coronaScreenOrderSpy.count(), 0);
    }

    QCOMPARE(m_corona->m_desktopViewForScreen.size(), m_corona->m_screenPool->screenOrder().size());

    for (int i = 0; i < m_corona->m_screenPool->screenOrder().size(); ++i) {
        QVERIFY(m_corona->m_desktopViewForScreen.contains(i));
        QCOMPARE(m_corona->m_desktopViewForScreen[i]->containment()->screen(), i);
        QCOMPARE(m_corona->m_desktopViewForScreen[i]->screenToFollow(), m_corona->m_screenPool->screenOrder()[i]);
    }
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
    QCOMPARE(m_corona->screenPool()->screenOrder().size(), 1);
    QCOMPARE(QGuiApplication::screens().first()->name(), QStringLiteral("WL-1"));
    QCOMPARE(QGuiApplication::primaryScreen(), QGuiApplication::screens().first());
    QCOMPARE(QGuiApplication::primaryScreen(), m_corona->screenPool()->primaryScreen());
    QCOMPARE(m_corona->screenPool()->idForScreen(m_corona->screenPool()->primaryScreen()), 0);
    QCOMPARE(m_corona->screenPool()->screenForId(0)->name(), QStringLiteral("WL-1"));
}

void ShellTest::cleanupTestCase()
{
    exec([=] {
        outputOrder()->setList({"WL-1"});
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
}

void ShellTest::cleanup()
{
    const int oldNumScreens = qApp->screens().size();
    const int oldCoronaNumScreens = m_corona->numScreens();
    QVERIFY(oldCoronaNumScreens <= oldNumScreens);
    QSignalSpy coronaRemovedSpy(m_corona, SIGNAL(screenRemoved(int)));

    exec([=] {
        for (int i = oldNumScreens - 1; i >= 0; --i) {
            remove(output(i));
        }
    });
    setScreenOrder({""}, true);

    QTRY_COMPARE(coronaRemovedSpy.size(), oldCoronaNumScreens);

    // Cleanup all the containments that were created
    for (auto *c : m_corona->containments()) {
        if (c->containmentType() == Plasma::Types::ContainmentType::Panel || c->screen() != 0) {
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
    setScreenOrder({"WL-1"}, true);
}

void ShellTest::testScreenInsertion()
{
    const auto geom = QRect(1920, 0, 1920, 1080);
    const auto name = QStringLiteral("DP-1");
    auto result = insertScreen(geom, name);
    setScreenOrder({"WL-1", "DP-1"}, true);
    QCOMPARE(result->geometry(), geom);
    QCOMPARE(qApp->screens().size(), 2);
    QCOMPARE(qApp->screens()[0]->geometry(), m_corona->m_desktopViewForScreen[0]->geometry());
    QCOMPARE(qApp->screens()[1]->geometry(), m_corona->m_desktopViewForScreen[1]->geometry());
    QCOMPARE(qApp->screens()[1]->name(), name);
}

void ShellTest::testPanelInsertion()
{
    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
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

    setScreenOrder({"WL-1", "DP-1", "DP-2"}, true);

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
    setScreenOrder({"WL-1", "DP-1"}, false);
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

    exec([=] {
        auto *out = output(1);
        auto *xdgOut = xdgOutput(out);
        out->m_data.mode.resolution = {1280, 2048};
        xdgOut->sendLogicalSize(QSize(1280, 2048));
        out->sendDone();
        outputOrder()->setList({"WL-1", "DP-1"});
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
    QCOMPARE(cont0->screen(), 0);
    Plasma::Containment *cont1 = m_corona->containmentForScreen(1, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont1->screen(), 1);
    Plasma::Containment *cont2 = m_corona->containmentForScreen(2, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont2->screen(), 2);

    QCOMPARE(m_corona->m_panelViews.size(), 0);
    auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
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

    QTRY_COMPARE(removedSpy.size(), 2);

    setScreenOrder({"WL-1"}, true);

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
    QCOMPARE(cont0->screen(), 0);
    Plasma::Containment *cont1 = m_corona->containmentForScreen(1, m_corona->m_activityController->currentActivity(), QString());
    QCOMPARE(cont1->screen(), 1);
    Plasma::Containment *cont2 = m_corona->containmentForScreen(2, m_corona->m_activityController->currentActivity(), QString());
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
    auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    auto panelView = m_corona->m_panelViews[panelCont];
    panelView->setScreenToFollow(m_corona->m_screenPool->screenForId(1));
    QCOMPARE(panelCont->screen(), 1);
    QCOMPARE(panelView->screen(), m_corona->m_screenPool->screenForId(1));
    QCOMPARE(m_corona->m_panelViews.size(), 1);

    QSignalSpy removedSpy(m_corona, SIGNAL(screenRemoved(int)));

    // Remove output
    exec([=] {
        remove(output(1));
    });

    QTRY_COMPARE(removedSpy.size(), 1);

    setScreenOrder({"WL-1", "DP-2"}, true);

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

    setScreenOrder({"WL-1", "DP-2", "DP-1"}, true);

    auto *newView2 = m_corona->m_desktopViewForScreen[2];
    QCOMPARE(m_corona->m_desktopViewForScreen.size(), 3);
    QCOMPARE(newView2->containment(), cont2);
}

void ShellTest::testReorderScreens_data()
{
    QTest::addColumn<QVector<QRect>>("geometries");
    QTest::addColumn<QStringList>("orderBefore");
    QTest::addColumn<QStringList>("orderAfter");

    QTest::newRow("twoScreens") << QVector<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}}) << QStringList({"WL-1", "DP-1"})
                                << QStringList({"DP-1", "WL-1"});
    QTest::newRow("3screensReorder0_1") << QVector<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                        << QStringList({"WL-1", "DP-1", "DP-2"}) << QStringList({"DP-1", "WL-1", "DP-2"});
    QTest::newRow("3screensReorder2_3") << QVector<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                        << QStringList({"WL-1", "DP-1", "DP-2"}) << QStringList({"DP-1", "WL-1", "DP-2"});
    QTest::newRow("3screensShuffled") << QVector<QRect>({{0, 0, 1920, 1080}, {1920, 0, 1920, 1080}, {3840, 0, 1920, 1080}})
                                      << QStringList({"WL-1", "DP-1", "DP-2"}) << QStringList({"DP-2", "DP-1", "WL-1"});
}

void ShellTest::testReorderScreens()
{
    QFETCH(QVector<QRect>, geometries);
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

    QVector<DesktopView *> desktopViews;
    QVector<PanelView *> panelViews;
    QVector<Plasma::Containment *> desktopContainments;
    QVector<Plasma::Containment *> panelContainments;
    QVector<QScreen *> screens;

    for (int i = 0; i < orderBefore.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViews.append(view);
        desktopContainments.append(view->containment());
        screens.append(view->screenToFollow());

        QCOMPARE(view->screen()->name(), orderBefore[i]);
        QCOMPARE(view->containment()->screen(), i);
    }

    // Add a panel for each screen
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *s = screens[i];
        auto panelCont = m_corona->addPanel(QStringLiteral("org.kde.plasma.panel"));
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

    QVector<DesktopView *> desktopViewsAfter;
    QVector<PanelView *> panelViewsAfter;
    panelViewsAfter.resize(orderAfter.size());
    QVector<Plasma::Containment *> desktopContainmentsAfter;
    QVector<Plasma::Containment *> panelContainmentsAfter;
    panelContainmentsAfter.resize(orderAfter.size());
    QVector<QScreen *> screensAfter;

    for (int i = 0; i < orderAfter.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViewsAfter.append(view);
        desktopContainmentsAfter.append(view->containment());
        screensAfter.append(view->screenToFollow());

        QCOMPARE(view->screenToFollow(), qScreenOrderFormSignal[i]);
        QCOMPARE(view->screen()->name(), orderAfter[i]);
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
    testSecondScreenInsertion();
    testPanelInsertion();

    QVector<DesktopView *> desktopViews;
    QVector<Plasma::Containment *> desktopContainments;
    QVector<QScreen *> screens = m_corona->m_screenPool->screenOrder().toVector();
    QCOMPARE(screens.size(), 3);

    for (int i = 0; i < screens.size(); ++i) {
        auto *view = m_corona->m_desktopViewForScreen[i];
        desktopViews.append(view);
        desktopContainments.append(view->containment());

        QCOMPARE(view->screen(), screens[i]);
        QCOMPARE(view->containment()->screen(), i);
    }
    QCOMPARE(desktopViews.size(), 3);
    QCOMPARE(m_corona->m_panelViews.size(), 1);
    auto *panelView = m_corona->m_panelViews.values().first();
    auto panelContainment = panelView->containment();
    QCOMPARE(panelView->screen(), screens[0]);
    QCOMPARE(panelContainment->screen(), 0);

    // Move panel to screen 1
    m_corona->setScreenForContainment(panelContainment, 1);
    QCOMPARE(panelView->screen(), screens[1]);
    QCOMPARE(panelContainment->screen(), 1);

    // Move containment at screen 0 to 1
    QCOMPARE(desktopContainments[0]->screen(), 0);
    QCOMPARE(desktopContainments[1]->screen(), 1);
    m_corona->setScreenForContainment(desktopContainments[0], 1);

    QCOMPARE(desktopContainments[0]->screen(), 1);
    QCOMPARE(desktopContainments[1]->screen(), 0);
    QCOMPARE(desktopViews[0]->screenToFollow(), screens[1]);
    QCOMPARE(desktopViews[1]->screenToFollow(), screens[0]);
}

QCOMPOSITOR_TEST_MAIN(ShellTest)

#include "shelltest.moc"
