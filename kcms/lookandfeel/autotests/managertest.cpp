/*
    SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../kcm.h"
#include "../lookandfeelmanager.h"

#include <QSignalSpy>
#include <QTest>

#include <KJob>
#include <KPackage/Package>
#include <KPackage/PackageJob>
#include <KPackage/PackageLoader>
#include <KSycoca>

class ManagerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSwitch();

private:
    QDir m_configDir;
    QDir m_dataDir;
};

void ManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qunsetenv("XDG_CONFIG_DIRS");
    qunsetenv("XDG_DATA_DIRS");

    m_configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    m_configDir.removeRecursively();

    m_dataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    m_dataDir.removeRecursively();

    QVERIFY(m_configDir.mkpath("."));

    {
        const QString packagePath = QFINDTESTDATA("lookandfeel");
        const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/plasma/look-and-feel/";
        auto installJob = KPackage::PackageJob::install(QStringLiteral("Plasma/LookAndFeel"), packagePath, packageRoot);
        connect(installJob, &KJob::result, this, [installJob]() {
            QCOMPARE(installJob->errorText(), QString());
            QCOMPARE(installJob->error(), KJob::NoError);
            QVERIFY(installJob->package().isValid());
        });
        QSignalSpy spy(installJob, &KPackage::PackageJob::finished);
        spy.wait();
    }

    {
        const QString packagePath = QFINDTESTDATA("lookandfeel2");
        const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/plasma/look-and-feel/";
        auto installJob = KPackage::PackageJob::install(QStringLiteral("Plasma/LookAndFeel"), packagePath, packageRoot);
        connect(installJob, &KJob::result, this, [installJob]() {
            QCOMPARE(installJob->errorText(), QString());
            QCOMPARE(installJob->error(), KJob::NoError);
            QVERIFY(installJob->package().isValid());
        });
        QSignalSpy spy(installJob, &KPackage::PackageJob::finished);
        spy.wait();
    }
}

void ManagerTest::testSwitch()
{
    // Load a LnF
    KPackage::Package lookandfeel = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), "lookandfeel");

    LookAndFeelManager lnfManager;
    lnfManager.setMode(LookAndFeelManager::Mode::Defaults);
    lnfManager.save(lookandfeel, KPackage::Package());

    {
        // Verify that its config is applied
        KConfig plasmarc(m_configDir.path() + "/kdedefaults/plasmarc");
        KConfigGroup themeGroup(&plasmarc, "Theme");
        QCOMPARE(themeGroup.readEntry("name"), "testValue");

        KConfig kwinrc(m_configDir.path() + "/kdedefaults/kwinrc");
        KConfigGroup windowSwitcherGroup(&kwinrc, "TabBox");
        QCOMPARE(windowSwitcherGroup.readEntry("LayoutName"), "testValue");
    }

    // Switch to a different LnF
    KPackage::Package lookandfeel2 = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), "lookandfeel2");
    lnfManager.save(lookandfeel2, lookandfeel);

    {
        // Verify that the new config is applied
        KConfig plasmarc(m_configDir.path() + "/kdedefaults/plasmarc");
        KConfigGroup themeGroup(&plasmarc, "Theme");
        QCOMPARE(themeGroup.readEntry("name"), "myOtherTestValue");

        // Verify that when the new LnF doesn't have the new key the value is unset https://bugs.kde.org/show_bug.cgi?id=474957
        KConfig kwinrc(m_configDir.path() + "/kdedefaults/kwinrc");
        KConfigGroup windowSwitcherGroup(&kwinrc, "TabBox");
        QCOMPARE(windowSwitcherGroup.readEntry("LayoutName"), "");
    }
}

QTEST_MAIN(ManagerTest)
#include "managertest.moc"
