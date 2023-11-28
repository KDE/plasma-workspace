/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../kcm.h"
#include "../lookandfeelmanager.h"
// Qt
#include <KJob>
#include <KPackage/Package>
#include <KPackage/PackageJob>
#include <KSycoca>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class KcmTest : public QObject
{
    /**
     *  Following tests ensure lookandfeel configuration are saved to kdedefaults,
     *  and configuration (kdeglobals, kwinrc...) from XDG_CONFIG_HOME return default value.
     *
     *  Because test don't have a modified XDG_CONFIG_DIRS with kdedefault dir added,
     *  we can easily check XDG_CONFIG_HOME values have default value and still using cascade/global behavior
     */
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testWidgetStyle();
    void testColors();
    void testIcons();
    void testPlasmaTheme();
    void testCursorTheme();
    void testSplashScreen();
    void testLockScreen();
    void testWindowSwitcher();
    void testKCMSave();

private:
    QDir m_configDir;
    QDir m_dataDir;
    KCMLookandFeel *m_KCMLookandFeel;
};

void KcmTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qunsetenv("XDG_CONFIG_DIRS");

    m_configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    m_configDir.removeRecursively();

    m_dataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    m_dataDir.removeRecursively();

    QVERIFY(m_configDir.mkpath("."));

    // we need an existing colorscheme file, even if empty
    QVERIFY(m_dataDir.mkpath(QStringLiteral("color-schemes")));
    QFile f(m_dataDir.path() + QStringLiteral("/color-schemes/TestValue.colors"));
    f.open(QIODevice::WriteOnly);
    f.close();

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

    KConfig config(u"kdeglobals"_s);
    KConfigGroup cg(&config, u"KDE"_s);
    cg.writeEntry("LookAndFeelPackage", "org.kde.test");
    cg.sync();
    m_KCMLookandFeel = new KCMLookandFeel(nullptr, KPluginMetaData());
    m_KCMLookandFeel->load();
}

void KcmTest::cleanupTestCase()
{
    m_configDir.removeRecursively();
    m_dataDir.removeRecursively();
}

void KcmTest::testWidgetStyle()
{
    // We have to use an actual theme name here because setWidgetStyle checks
    // if the theme can actually be used before it changes the config
    m_KCMLookandFeel->lookAndFeel()->setWidgetStyle(QStringLiteral("Fusion"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"KDE"_s);
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"KDE"_s);
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));

    // Test that a fake style is not actually written to config
    m_KCMLookandFeel->lookAndFeel()->setWidgetStyle(QStringLiteral("customStyle"));
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));
}

void KcmTest::testColors()
{
    // TODO: test colorFile as well
    m_KCMLookandFeel->lookAndFeel()->setColors(QStringLiteral("customTestValue"), QString());

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"General"_s);
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"General"_s);
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testIcons()
{
    m_KCMLookandFeel->lookAndFeel()->setIcons(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, u"Icons"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Icons"_s);
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testPlasmaTheme()
{
    m_KCMLookandFeel->lookAndFeel()->setPlasmaTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("plasmarc"));
    KConfigGroup cg(&config, u"Theme"_s);
    QCOMPARE(cg.readEntry("name", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/plasmarc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Theme"_s);
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testCursorTheme()
{
    m_KCMLookandFeel->lookAndFeel()->setCursorTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kcminputrc"));
    KConfigGroup cg(&config, u"Mouse"_s);
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Mouse"_s);
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testSplashScreen()
{
    m_KCMLookandFeel->lookAndFeel()->setSplashScreen(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("ksplashrc"));
    KConfigGroup cg(&config, u"KSplash"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"KSplash"_s);
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));
}

void KcmTest::testLockScreen()
{
    m_KCMLookandFeel->lookAndFeel()->setLockScreen(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kscreenlockerrc"));
    KConfigGroup cg(&config, u"Greeter"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kscreenlockerrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"Greeter"_s);
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testWindowSwitcher()
{
    m_KCMLookandFeel->lookAndFeel()->setWindowSwitcher(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, u"TabBox"_s);
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kwinrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, u"TabBox"_s);
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testKCMSave()
{
    // TODO: Previous bussiness code was effectively filtering every item from
    // getting save, so the values would remain those set in the previous tests.
    // This is the simpler fix to emulate the old behavior.
    // Ideally we would test for the real items in the default theme or better,
    // in a custom-made test package.
    m_KCMLookandFeel->setSelectedContents(LookAndFeelManager::Empty);

    m_KCMLookandFeel->save();

    // On real setup we read entries from kdedefaults directory (XDG_CONFIG_DIRS is modified but not in test scenario)

    KConfig config(QStringLiteral("kdeglobals"));
    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cg(&config, u"KDE"_s);
    KConfigGroup cgd(&configDefault, u"KDE"_s);
    // See comment in testWidgetStyle
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));

    cg = KConfigGroup(&config, u"General"_s);
    cgd = KConfigGroup(&configDefault, u"General"_s);
    // save() capitalizes the ColorScheme
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));

    cg = KConfigGroup(&config, u"Icons"_s);
    cgd = KConfigGroup(&configDefault, u"Icons"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));

    KConfig plasmaConfig(QStringLiteral("plasmarc"));
    KConfig plasmaConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/plasmarc", KConfig::SimpleConfig);
    cg = KConfigGroup(&plasmaConfig, u"Theme"_s);
    cgd = KConfigGroup(&plasmaConfigDefault, u"Theme"_s);
    QCOMPARE(cg.readEntry("name", QString()), QString());
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));

    KConfig inputConfig(QStringLiteral("kcminputrc"));
    KConfig inputConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&inputConfig, u"Mouse"_s);
    cgd = KConfigGroup(&inputConfigDefault, u"Mouse"_s);
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));

    KConfig splashConfig(QStringLiteral("ksplashrc"));
    KConfig splashConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&splashConfig, u"KSplash"_s);
    cgd = KConfigGroup(&splashConfigDefault, u"KSplash"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));

    KConfig lockerConfig(QStringLiteral("kscreenlockerrc"));
    KConfig lockerConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kscreenlockerrc",
                                KConfig::SimpleConfig);
    cg = KConfigGroup(&lockerConfig, u"Greeter"_s);
    cgd = KConfigGroup(&lockerConfigDefault, u"Greeter"_s);
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));

    KConfig kwinConfig(QStringLiteral("kwinrc"));
    KConfig kwinConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kwinrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&kwinConfig, u"TabBox"_s);
    cgd = KConfigGroup(&kwinConfigDefault, u"TabBox"_s);
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
}

QTEST_MAIN(KcmTest)
#include "kcmtest.moc"
