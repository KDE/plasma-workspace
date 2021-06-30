/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QObject>
#include <QSignalSpy>
#include <QTest>

#include "launchertasksmodel.h"
#include "tasktools.h"

using namespace TaskManager;

class LauncherTasksModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void shouldRoundTripLauncherUrlList();
    void shouldIgnoreInvalidUrls();
    void shouldAcceptSpaces();
    void shouldRejectDuplicates();
    void shouldAddRemoveLauncher();
    void shouldReturnValidLauncherPositions();
    void shouldReturnValidData();

private:
    QStringList m_urlStrings;
};

void LauncherTasksModelTest::initTestCase()
{
    qApp->setProperty("org.kde.KActivities.core.disableAutostart", true);
    m_urlStrings = QStringList{QUrl::fromLocalFile(QFINDTESTDATA("data/applications/org.kde.dolphin.desktop")).toString(),
                               QUrl::fromLocalFile(QFINDTESTDATA("data/applications/org.kde.konversation.desktop")).toString()};
    // We don't want the globally installed apps to interfere and want the test to use our data files
    qputenv("XDG_DATA_DIRS", QFINDTESTDATA("data").toLocal8Bit());
}

void LauncherTasksModelTest::shouldRoundTripLauncherUrlList()
{
    LauncherTasksModel m;

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    m.setLauncherList(m_urlStrings);

    QCOMPARE(launcherListChangedSpy.count(), 1);

    QCOMPARE(m.launcherList(), m_urlStrings);

    QVERIFY(launcherUrlsMatch(m.data(m.index(0, 0), AbstractTasksModel::LauncherUrl).toUrl(), QUrl(m_urlStrings.at(0))));
    QVERIFY(launcherUrlsMatch(m.data(m.index(1, 0), AbstractTasksModel::LauncherUrl).toUrl(), QUrl(m_urlStrings.at(1))));
}

void LauncherTasksModelTest::shouldIgnoreInvalidUrls()
{
    LauncherTasksModel m;

    QStringList urlStrings;
    urlStrings << QLatin1String("GARBAGE URL");

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    m.setLauncherList(urlStrings);

    QCOMPARE(launcherListChangedSpy.count(), 0);

    bool added = m.requestAddLauncher(QUrl(urlStrings.at(0)));

    QVERIFY(!added);
    QCOMPARE(launcherListChangedSpy.count(), 0);

    QCOMPARE(m.launcherList(), QStringList());
}

void LauncherTasksModelTest::shouldAcceptSpaces()
{
    LauncherTasksModel m;

    const QStringList urlStrings{QLatin1String("applications:App with spaces.desktop")};

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    const bool added = m.requestAddLauncher(QUrl(urlStrings.at(0)));

    QVERIFY(added);
    QCOMPARE(launcherListChangedSpy.count(), 1);

    QCOMPARE(m.launcherList(), QStringList() << urlStrings.at(0));
}

void LauncherTasksModelTest::shouldRejectDuplicates()
{
    LauncherTasksModel m;

    const QStringList urlStrings = {QUrl::fromLocalFile(QFINDTESTDATA("data/applications/org.kde.dolphin.desktop")).toString(),
                                    QUrl::fromLocalFile(QFINDTESTDATA("data/applications/org.kde.dolphin.desktop")).toString()};

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    m.setLauncherList(urlStrings);

    QCOMPARE(launcherListChangedSpy.count(), 1);

    bool added = m.requestAddLauncher(QUrl(urlStrings.at(0)));

    QVERIFY(!added);
    QCOMPARE(launcherListChangedSpy.count(), 1);

    QCOMPARE(m.launcherList(), QStringList() << urlStrings.at(0));
}

void LauncherTasksModelTest::shouldAddRemoveLauncher()
{
    LauncherTasksModel m;

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    bool added = m.requestAddLauncher(QUrl(m_urlStrings.at(0)));

    QVERIFY(added);
    QCOMPARE(launcherListChangedSpy.count(), 1);

    QVERIFY(launcherUrlsMatch(QUrl(m.launcherList().at(0)), QUrl(m_urlStrings.at(0))));

    bool removed = m.requestRemoveLauncher(QUrl(m_urlStrings.at(0)));

    QVERIFY(removed);
    QCOMPARE(launcherListChangedSpy.count(), 2);

    removed = m.requestRemoveLauncher(QUrl(m_urlStrings.at(0)));

    QVERIFY(!removed);

    QCOMPARE(m.launcherList(), QStringList());
}

void LauncherTasksModelTest::shouldReturnValidLauncherPositions()
{
    LauncherTasksModel m;

    QSignalSpy launcherListChangedSpy(&m, &LauncherTasksModel::launcherListChanged);
    QVERIFY(launcherListChangedSpy.isValid());

    m.setLauncherList(m_urlStrings);

    QCOMPARE(launcherListChangedSpy.count(), 1);

    QCOMPARE(m.launcherPosition(QUrl(m_urlStrings.at(0))), 0);
    QCOMPARE(m.launcherPosition(QUrl(m_urlStrings.at(1))), 1);
}

void LauncherTasksModelTest::shouldReturnValidData()
{
    // FIXME Reuse TaskToolsTest app link setup, then run URLs through model.
}

QTEST_MAIN(LauncherTasksModelTest)

#include "launchertasksmodeltest.moc"
