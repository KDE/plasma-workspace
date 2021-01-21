/********************************************************************
Copyright 2017 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "mock_kwin.h"
#include "qtest_dbus.h"

#include "../colorcorrectconstants.h"
#include "../compositorcoloradaptor.h"

#include <QtTest>

using namespace ColorCorrect;

class TestNightColor : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testAdaptorInit();
    void testStageData_data();
    void testStageData();
    void testAutoLocationUpdate();

private:
    void setCompBackToDefault()
    {
        m_comp->nightColorAvailable = true;

        m_comp->activeEnabled = true;
        m_comp->active = true;

        m_comp->modeEnabled = true;
        m_comp->mode = 0;

        m_comp->nightTemperatureEnabled = true;
        m_comp->nightTemperature = DEFAULT_NIGHT_TEMPERATURE;

        m_comp->running = false;
        m_comp->currentColorTemperature = NEUTRAL_TEMPERATURE;

        m_comp->latitudeAuto = 0;
        m_comp->longitudeAuto = 0;

        m_comp->locationEnabled = true;
        m_comp->latitudeFixed = 0;
        m_comp->longitudeFixed = 0;

        m_comp->timingsEnabled = true;
        m_comp->morningBeginFixed = QTime(6, 0, 0);
        m_comp->eveningBeginFixed = QTime(18, 0, 0);
        m_comp->transitionTime = FALLBACK_SLOW_UPDATE_TIME;
    }

    kwin_dbus *m_comp = nullptr;
};

void TestNightColor::initTestCase()
{
    m_comp = new kwin_dbus;
    QVERIFY(m_comp->registerDBus());
}

void TestNightColor::cleanupTestCase()
{
    QVERIFY(m_comp->unregisterDBus());
    delete m_comp;
    m_comp = nullptr;
}

void TestNightColor::testAdaptorInit()
{
    m_comp->nightColorAvailable = false;
    CompositorAdaptor *adaptor = new CompositorAdaptor(this);
    QVERIFY(!adaptor->nightColorAvailable());
    delete adaptor;
    m_comp->nightColorAvailable = true;
    adaptor = new CompositorAdaptor(this);
    QVERIFY(adaptor->nightColorAvailable());
    delete adaptor;
}

void TestNightColor::testStageData_data()
{
    QTest::addColumn<bool>("active");
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("nightTemperature");
    QTest::addColumn<double>("latitudeFixed");
    QTest::addColumn<double>("longitudeFixed");
    QTest::addColumn<QTime>("morningBeginFixed");
    QTest::addColumn<QTime>("eveningBeginFixed");
    QTest::addColumn<int>("transitionTime");
    QTest::addColumn<bool>("isChange");
    QTest::addColumn<bool>("isChangeAll");
    QTest::addColumn<bool>("expectChangeSuccess");

    // clang-format off
    QTest::newRow("noChange") << true << 0 << DEFAULT_NIGHT_TEMPERATURE << 0. << 0. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << false << false << false;
    QTest::newRow("wrongChange") << true << 0 << 9001 << 0. << 0. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << true << true << false;
    QTest::newRow("temperature") << true << 0 << 1000 << 0. << 0. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << true << true << true;
    QTest::newRow("deactivate+temperature") << false << 0 << 1000 << 0. << 0. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << true << true << true;
    QTest::newRow("location+differentMode") << true << 2 << 0 << 0. << 0. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << true << true << true;
    QTest::newRow("time+defaultMode") << true << 0 << DEFAULT_NIGHT_TEMPERATURE << 0. << 0. << QTime(10,0,0) << QTime(20,0,0) << 1 << false << true << true;
    QTest::newRow("location+mode") << true << 1 << DEFAULT_NIGHT_TEMPERATURE << 50. << -20. << QTime(6,0,0) << QTime(18,0,0) << FALLBACK_SLOW_UPDATE_TIME << true << true << true;
    QTest::newRow("time+mode") << false << 0 << DEFAULT_NIGHT_TEMPERATURE << 0. << 0. << QTime(10,0,0) << QTime(20,0,0) << 1 << true << true << true;
    // clang-format on
}

void TestNightColor::testStageData()
{
    QFETCH(bool, active);
    QFETCH(int, mode);
    QFETCH(int, nightTemperature);
    QFETCH(double, latitudeFixed);
    QFETCH(double, longitudeFixed);
    QFETCH(QTime, morningBeginFixed);
    QFETCH(QTime, eveningBeginFixed);
    QFETCH(int, transitionTime);
    QFETCH(bool, isChange);
    QFETCH(bool, isChangeAll);
    QFETCH(bool, expectChangeSuccess);

    setCompBackToDefault();
    m_comp->configChangeExpectSuccess = expectChangeSuccess;

    CompositorAdaptor *aptr = new CompositorAdaptor(this);

    QVERIFY(!aptr->checkStaged());
    QVERIFY(!aptr->checkStagedAll());

    auto setAdaptorStaged =
        [&aptr, &active, &mode, &nightTemperature, &latitudeFixed, &longitudeFixed, &morningBeginFixed, &eveningBeginFixed, &transitionTime]() {
            aptr->setActiveStaged(active);
            aptr->setModeStaged(mode);
            aptr->setNightTemperatureStaged(nightTemperature);
            aptr->setLatitudeFixedStaged(latitudeFixed);
            aptr->setLongitudeFixedStaged(longitudeFixed);
            aptr->setMorningBeginFixedStaged(morningBeginFixed);
            aptr->setEveningBeginFixedStaged(eveningBeginFixed);
            aptr->setTransitionTimeStaged(transitionTime);
        };
    setAdaptorStaged();
    QCOMPARE(aptr->checkStaged(), isChange);
    QCOMPARE(aptr->checkStagedAll(), isChangeAll);

    QSignalSpy *dataUpdateSpy = new QSignalSpy(aptr, &CompositorAdaptor::dataUpdated);
    QVERIFY(dataUpdateSpy->isValid());

    // send config relative to active and mode state
    aptr->sendConfiguration();
    // give dbus communication time
    QTest::qWait(300);
    QCOMPARE(dataUpdateSpy->isEmpty(), !expectChangeSuccess || !isChange);
    QCOMPARE(aptr->checkStaged(), !expectChangeSuccess && isChange);

    // reset compositor and adaptor - now send all data at once
    setCompBackToDefault();
    delete aptr;
    aptr = new CompositorAdaptor(this);
    dataUpdateSpy = new QSignalSpy(aptr, &CompositorAdaptor::dataUpdated);
    QVERIFY(dataUpdateSpy->isValid());

    QVERIFY(!aptr->checkStaged());
    QVERIFY(!aptr->checkStagedAll());

    setAdaptorStaged();
    QCOMPARE(aptr->checkStaged(), isChange);
    QCOMPARE(aptr->checkStagedAll(), isChangeAll);
    // dump all config
    aptr->sendConfigurationAll();
    // give dbus communication time
    QTest::qWait(300);
    QCOMPARE(dataUpdateSpy->isEmpty(), !expectChangeSuccess || !isChangeAll);
    QCOMPARE(aptr->checkStaged(), !expectChangeSuccess && isChange);
    QCOMPARE(aptr->checkStagedAll(), !expectChangeSuccess && isChangeAll);
}

void TestNightColor::testAutoLocationUpdate()
{
    setCompBackToDefault();

    CompositorAdaptor *aptr = new CompositorAdaptor(this);
    QSignalSpy *dataUpdateSpy = new QSignalSpy(aptr, &CompositorAdaptor::dataUpdated);
    QVERIFY(dataUpdateSpy->isValid());

    aptr->sendAutoLocationUpdate(10, 20);
    QTest::qWait(300);
    QCOMPARE(dataUpdateSpy->isEmpty(), false);
}

QTEST_GUILESS_MAIN_SYSTEM_DBUS(TestNightColor)

#include "nightcolortest.moc"
