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
#include <KPackage/PackageLoader>
#include <KSycoca>
#include <QtTest>

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
    void testDesktopSwitcher();
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

    KPackage::Package p = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    p.setPath(packagePath);
    QVERIFY(p.isValid());

    const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/plasma/look-and-feel/";
    auto installJob = p.install(packagePath, packageRoot);
    installJob->exec();

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, "KDE");
    cg.writeEntry("LookAndFeelPackage", "org.kde.test");
    cg.sync();
    m_KCMLookandFeel = new KCMLookandFeel(nullptr, KPluginMetaData(), QVariantList());
    m_KCMLookandFeel->load();
}

void KcmTest::cleanupTestCase()
{
    m_configDir.removeRecursively();
    m_dataDir.removeRecursively();
}

void KcmTest::testWidgetStyle()
{
    m_KCMLookandFeel->lookAndFeel()->setWidgetStyle(QStringLiteral("Fusion"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, "KDE");
    // We have to use an actual theme name here because setWidgetStyle checks
    // if the theme can actually be used before it changes the config
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "KDE");
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));
}

void KcmTest::testColors()
{
    // TODO: test colorFile as well
    m_KCMLookandFeel->lookAndFeel()->setColors(QStringLiteral("customTestValue"), QString());

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, "General");
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "General");
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testIcons()
{
    m_KCMLookandFeel->lookAndFeel()->setIcons(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kdeglobals"));
    KConfigGroup cg(&config, "Icons");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "Icons");
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testPlasmaTheme()
{
    m_KCMLookandFeel->lookAndFeel()->setPlasmaTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("plasmarc"));
    KConfigGroup cg(&config, "Theme");
    QCOMPARE(cg.readEntry("name", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/plasmarc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "Theme");
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testCursorTheme()
{
    m_KCMLookandFeel->lookAndFeel()->setCursorTheme(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kcminputrc"));
    KConfigGroup cg(&config, "Mouse");
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "Mouse");
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testSplashScreen()
{
    m_KCMLookandFeel->lookAndFeel()->setSplashScreen(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("ksplashrc"));
    KConfigGroup cg(&config, "KSplash");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "KSplash");
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));
}

void KcmTest::testLockScreen()
{
    m_KCMLookandFeel->lookAndFeel()->setLockScreen(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kscreenlockerrc"));
    KConfigGroup cg(&config, "Greeter");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kscreenlockerrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "Greeter");
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testWindowSwitcher()
{
    m_KCMLookandFeel->lookAndFeel()->setWindowSwitcher(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, "TabBox");
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kwinrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "TabBox");
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testDesktopSwitcher()
{
    m_KCMLookandFeel->lookAndFeel()->setDesktopSwitcher(QStringLiteral("customTestValue"));

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, "TabBox");
    QCOMPARE(cg.readEntry("DesktopLayout", QString()), QString());
    QCOMPARE(cg.readEntry("DesktopListLayout", QString()), QString());

    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kwinrc", KConfig::SimpleConfig);
    KConfigGroup cgd(&configDefault, "TabBox");
    QCOMPARE(cgd.readEntry("DesktopLayout", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("DesktopListLayout", QString()), QStringLiteral("customTestValue"));
}

void KcmTest::testKCMSave()
{
    m_KCMLookandFeel->save();

    // On real setup we read entries from kdedefaults directory (XDG_CONFIG_DIRS is modified but not in test scenario)

    KConfig config(QStringLiteral("kdeglobals"));
    KConfig configDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kdeglobals", KConfig::SimpleConfig);
    KConfigGroup cg(&config, "KDE");
    KConfigGroup cgd(&configDefault, "KDE");
    // See comment in testWidgetStyle
    QCOMPARE(cg.readEntry("widgetStyle", QString()), QString());
    QCOMPARE(cgd.readEntry("widgetStyle", QString()), QStringLiteral("Fusion"));

    cg = KConfigGroup(&config, "General");
    cgd = KConfigGroup(&configDefault, "General");
    // save() capitalizes the ColorScheme
    QCOMPARE(cg.readEntry("ColorScheme", QString()), QString());
    QCOMPARE(cgd.readEntry("ColorScheme", QString()), QStringLiteral("customTestValue"));

    cg = KConfigGroup(&config, "Icons");
    cgd = KConfigGroup(&configDefault, "Icons");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));

    KConfig plasmaConfig(QStringLiteral("plasmarc"));
    KConfig plasmaConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/plasmarc", KConfig::SimpleConfig);
    cg = KConfigGroup(&plasmaConfig, "Theme");
    cgd = KConfigGroup(&plasmaConfigDefault, "Theme");
    QCOMPARE(cg.readEntry("name", QString()), QString());
    QCOMPARE(cgd.readEntry("name", QString()), QStringLiteral("customTestValue"));

    KConfig inputConfig(QStringLiteral("kcminputrc"));
    KConfig inputConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kcminputrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&inputConfig, "Mouse");
    cgd = KConfigGroup(&inputConfigDefault, "Mouse");
    QCOMPARE(cg.readEntry("cursorTheme", QString()), QString());
    QCOMPARE(cgd.readEntry("cursorTheme", QString()), QStringLiteral("customTestValue"));

    KConfig splashConfig(QStringLiteral("ksplashrc"));
    KConfig splashConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/ksplashrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&splashConfig, "KSplash");
    cgd = KConfigGroup(&splashConfigDefault, "KSplash");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cg.readEntry("Engine", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("Engine", QString()), QStringLiteral("KSplashQML"));

    KConfig lockerConfig(QStringLiteral("kscreenlockerrc"));
    KConfig lockerConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kscreenlockerrc",
                                KConfig::SimpleConfig);
    cg = KConfigGroup(&lockerConfig, "Greeter");
    cgd = KConfigGroup(&lockerConfigDefault, "Greeter");
    QCOMPARE(cg.readEntry("Theme", QString()), QString());
    QCOMPARE(cgd.readEntry("Theme", QString()), QStringLiteral("customTestValue"));

    KConfig kwinConfig(QStringLiteral("kwinrc"));
    KConfig kwinConfigDefault(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/kwinrc", KConfig::SimpleConfig);
    cg = KConfigGroup(&kwinConfig, "TabBox");
    cgd = KConfigGroup(&kwinConfigDefault, "TabBox");
    QCOMPARE(cg.readEntry("LayoutName", QString()), QString());
    QCOMPARE(cg.readEntry("DesktopLayout", QString()), QString());
    QCOMPARE(cg.readEntry("DesktopListLayout", QString()), QString());
    QCOMPARE(cgd.readEntry("LayoutName", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("DesktopLayout", QString()), QStringLiteral("customTestValue"));
    QCOMPARE(cgd.readEntry("DesktopListLayout", QString()), QStringLiteral("customTestValue"));
}

QTEST_MAIN(KcmTest)
#include "kcmtest.moc"
