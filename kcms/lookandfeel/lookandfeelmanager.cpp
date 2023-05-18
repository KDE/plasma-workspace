/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "lookandfeelmanager.h"
#include "../../startkde/plasmaautostart/plasmaautostart.h"
#include "../colors/colorsapplicator.h"
#include "config-kcm.h"
#include "lookandfeeldata.h"
#include "lookandfeelsettings.h"
#include <KIO/CommandLauncherJob>
#include <KPackage/PackageLoader>
#include <KSharedConfig>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleFactory>

#ifdef HAVE_XCURSOR
#include "../cursortheme/xcursor/xcursortheme.h"
#include <X11/Xcursor/Xcursor.h>
#endif

namespace
{
// Helper methods to read or check entries from a nested group within a config

QString configValue(KSharedConfigPtr config, const QString &groupPath, const QString &entry)
{
    // Navigate through the group hierarchy
    QStringList groups = groupPath.split(QLatin1Char('/'));
    KConfigGroup cg(config, groups.takeAt(0));
    for (const QString &group : groups) {
        cg = KConfigGroup(&cg, group);
    }

    return cg.readEntry(entry, QString());
}

bool configProvides(KSharedConfigPtr config, const QString &groupPath, const QStringList &entries)
{
    return std::any_of(entries.cbegin(), entries.cend(), [config, groupPath](const QString &entry) {
        return !configValue(config, groupPath, entry).isEmpty();
    });
}

bool configProvides(KSharedConfigPtr config, const QString &groupPath, const QString &entry)
{
    return !configValue(config, groupPath, entry).isEmpty();
}

} // Anonymouse namespace

LookAndFeelManager::LookAndFeelManager(QObject *parent)
    : QObject(parent)
    , m_data(new LookAndFeelData(this))
    , m_plasmashellChanged(false)
    , m_fontsChanged(false)
{
    m_applyLatteLayout = (KService::serviceByDesktopName("org.kde.latte-dock") != nullptr);
}

LookAndFeelSettings *LookAndFeelManager::settings() const
{
    return m_data->settings();
}

LookAndFeelManager::Contents LookAndFeelManager::packageContents(const KPackage::Package &pkg) const
{
    Contents contents = Empty;

    contents.setFlag(SplashScreen, !pkg.filePath("splashmainscript").isEmpty());
    contents.setFlag(LockScreen, !pkg.filePath("lockscreenmainscript").isEmpty());

    contents.setFlag(DesktopLayout, !pkg.filePath("layouts").isEmpty());

    // TODO: Those seem unused... are deprecated?
    contents.setFlag(RunCommand, !pkg.filePath("runcommandmainscript").isEmpty());
    contents.setFlag(LogOutScript, !pkg.filePath("logoutmainscript").isEmpty());

    if (!pkg.filePath("layoutdefaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("layoutdefaults"));
        contents.setFlag(TitlebarLayout, configProvides(conf, "kwinrc/org.kde.kdecoration2", {"ButtonsOnLeft", "ButtonsOnRight"}));
    }

    if (!pkg.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("defaults"));

        contents.setFlag(Colors, configProvides(conf, "kdeglobals/General", "ColorScheme") || !pkg.filePath("colors").isEmpty());
        contents.setFlag(WidgetStyle, configProvides(conf, "kdeglobals/KDE", "widgetStyle"));
        contents.setFlag(Icons, configProvides(conf, "kdeglobals/Icons", "Theme"));

        contents.setFlag(PlasmaTheme, configProvides(conf, "plasmarc/Theme", "name"));
        contents.setFlag(Wallpaper, configProvides(conf, "Wallpaper", "Image"));
        contents.setFlag(Cursors, configProvides(conf, "kcminputrc/Mouse", "cursorTheme"));

        contents.setFlag(WindowSwitcher, configProvides(conf, "kwinrc/WindowSwitcher", "LayoutName"));
        contents.setFlag(WindowDecoration, configProvides(conf, "kwinrc/org.kde.kdecoration2", {"library", "NoPlugin"}));
        contents.setFlag(BorderSize, configProvides(conf, "kwinrc/org.kde.kdecoration2", "BorderSize"));

        contents.setFlag(Fonts,
                         configProvides(conf, "kdeglobals/WM", {"font", "fixed", "smallestReadableFont", "toolBarFont", "menuFont"})
                             || configProvides(conf, "kdeglobals/General", "activeFont"));

        contents.setFlag(WindowPlacement, configProvides(conf, "kwinrc/Windows", "Placement"));
        contents.setFlag(ShellPackage, configProvides(conf, "plasmashellrc/Shell", "ShellPackage"));

        // TODO: Currently managed together within the "DesktopLayout" content
        // contents.setFlag(Autostart, configProvides(conf, "Autostart", "Services"));
    }

    return contents;
}

void LookAndFeelManager::setSplashScreen(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("ksplashrc"));
    KConfigGroup group(config, QStringLiteral("KSplash"));

    KConfig configDefault(configDefaults(QStringLiteral("ksplashrc")));
    KConfigGroup defaultGroup(&configDefault, QStringLiteral("KSplash"));
    writeNewDefaults(group, defaultGroup, QStringLiteral("Theme"), theme);
    // TODO: a way to set none as spash in the l&f
    writeNewDefaults(group, defaultGroup, QStringLiteral("Engine"), QStringLiteral("KSplashQML"));
}

void LookAndFeelManager::setLockScreen(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kscreenlockerrc"), QStringLiteral("Greeter"), QStringLiteral("Theme"), theme);
}

void LookAndFeelManager::setWindowSwitcher(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("TabBox"), QStringLiteral("LayoutName"), theme);
}

void LookAndFeelManager::setWindowPlacement(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("Windows"), QStringLiteral("Placement"), value);
}

void LookAndFeelManager::setShellPackage(const QString &value)
{
    if (value.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("plasmashellrc"), QStringLiteral("Shell"), QStringLiteral("ShellPackage"), value);
    m_plasmashellChanged = true;
}

void LookAndFeelManager::setWindowDecoration(const QString &library, const QString &theme, bool noPlugin)
{
    if (library.isEmpty()) {
        return;
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup group(config, QStringLiteral("org.kde.kdecoration2"));

    KConfig configDefault(configDefaults(QStringLiteral("kwinrc")));
    KConfigGroup defaultGroup(&configDefault, QStringLiteral("org.kde.kdecoration2"));
    writeNewDefaults(group, defaultGroup, QStringLiteral("library"), library);
    writeNewDefaults(group, defaultGroup, QStringLiteral("theme"), theme, KConfig::Notify);
    writeNewDefaults(group, defaultGroup, QStringLiteral("NoPlugin"), noPlugin ? "true" : "false", KConfig::Notify);
}

void LookAndFeelManager::setTitlebarLayout(const QString &leftbtns, const QString &rightbtns)
{
    if (leftbtns.isEmpty() && rightbtns.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("org.kde.kdecoration2"), QStringLiteral("ButtonsOnLeft"), leftbtns, KConfig::Notify);
    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("org.kde.kdecoration2"), QStringLiteral("ButtonsOnRight"), rightbtns, KConfig::Notify);
}

void LookAndFeelManager::setBorderSize(const QString &size)
{
    if (size.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("org.kde.kdecoration2"), QStringLiteral("BorderSize"), size, KConfig::Notify);
}

void LookAndFeelManager::setBorderlessMaximized(const QString &value)
{
    if (value.isEmpty()) { // Turn borderless off for unsupported LNFs to prevent issues
        writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("Windows"), QStringLiteral("BorderlessMaximizedWindows"), "false", KConfig::Notify);
        return;
    }

    writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("Windows"), QStringLiteral("BorderlessMaximizedWindows"), value, KConfig::Notify);
}

void LookAndFeelManager::setWidgetStyle(const QString &style)
{
    if (style.isEmpty()) {
        return;
    }

    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        // Some global themes use styles that may not be installed.
        // Test if style can be installed before updating the config.
        std::unique_ptr<QStyle> testStyle(QStyleFactory::create(style));
        if (!testStyle) {
            return;
        }
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("KDE"), QStringLiteral("widgetStyle"), style, KConfig::Notify);
    Q_EMIT styleChanged(style);
}

void LookAndFeelManager::setColors(const QString &scheme, const QString &colorFile)
{
    if (scheme.isEmpty() && colorFile.isEmpty()) {
        return;
    }

    KConfig configDefault(configDefaults(QStringLiteral("kdeglobals")));
    auto kdeGlobalsCfg = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::FullConfig);

    if (m_mode == Mode::Apply) {
        applyScheme(colorFile, kdeGlobalsCfg.data(), KConfig::Notify);
    }

    writeNewDefaults(*kdeGlobalsCfg, configDefault, QStringLiteral("General"), QStringLiteral("ColorScheme"), scheme, KConfig::Notify);

    Q_EMIT colorsChanged();
}

void LookAndFeelManager::setIcons(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("Icons"), QStringLiteral("Theme"), theme, KConfig::Notify);

    Q_EMIT iconsChanged();
}

void LookAndFeelManager::setLatteLayout(const QString &filepath, const QString &name)
{
    if (filepath.isEmpty()) {
        // there is no latte layout
        KIO::CommandLauncherJob latteapp(QStringLiteral("latte-dock"), {QStringLiteral("--disable-autostart")});
        latteapp.setDesktopName("org.kde.latte-dock");
        latteapp.start();

        QDBusMessage quitmessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.lattedock"),
                                                                  QStringLiteral("/MainApplication"),
                                                                  QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                                  QStringLiteral("quit"));
        QDBusConnection::sessionBus().call(quitmessage, QDBus::NoBlock);
    } else {
        KIO::CommandLauncherJob latteapp(
            QStringLiteral("latte-dock"),
            {QStringLiteral("--enable-autostart"), QStringLiteral("--import-layout"), filepath, QStringLiteral("--suggested-layout-name"), name});
        latteapp.setDesktopName("org.kde.latte-dock");
        latteapp.start();
    }
}

void LookAndFeelManager::setPlasmaTheme(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("plasmarc"), QStringLiteral("Theme"), QStringLiteral("name"), theme);
}

void LookAndFeelManager::setGeneralFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("General"), QStringLiteral("font"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::setFixedFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("General"), QStringLiteral("fixed"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::setSmallestReadableFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("General"), QStringLiteral("smallestReadableFont"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::setToolbarFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("General"), QStringLiteral("toolBarFont"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::setMenuFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("General"), QStringLiteral("menuFont"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::setWindowTitleFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("WM"), QStringLiteral("activeFont"), font, KConfig::Notify);
    m_fontsChanged = true;
}

void LookAndFeelManager::writeNewDefaults(const QString &filename,
                                          const QString &group,
                                          const QString &key,
                                          const QString &value,
                                          KConfig::WriteConfigFlags writeFlags)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(filename);
    KConfigGroup configGroup(config, group);

    KConfig configDefault(configDefaults(filename));
    KConfigGroup defaultGroup(&configDefault, group);

    writeNewDefaults(configGroup, defaultGroup, key, value, writeFlags);
}

void LookAndFeelManager::writeNewDefaults(KConfig &config,
                                          KConfig &configDefault,
                                          const QString &group,
                                          const QString &key,
                                          const QString &value,
                                          KConfig::WriteConfigFlags writeFlags)
{
    KConfigGroup configGroup(&config, group);
    KConfigGroup defaultGroup(&configDefault, group);

    writeNewDefaults(configGroup, defaultGroup, key, value, writeFlags);
}

void LookAndFeelManager::writeNewDefaults(KConfigGroup &group,
                                          KConfigGroup &defaultGroup,
                                          const QString &key,
                                          const QString &value,
                                          KConfig::WriteConfigFlags writeFlags)
{
    defaultGroup.writeEntry(key, value, writeFlags);
    defaultGroup.sync();

    if (m_mode == Mode::Apply) {
        group.revertToDefault(key, writeFlags);
        group.sync();
    }
}

KConfig LookAndFeelManager::configDefaults(const QString &filename)
{
    return KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults/") + filename, KConfig::SimpleConfig);
}

QString LookAndFeelManager::colorSchemeFile(const QString &schemeName) const
{
    QString colorScheme(schemeName);
    colorScheme.remove(QLatin1Char('\'')); // So Foo's does not become FooS
    QRegularExpression fixer(QStringLiteral("[\\W,.-]+(.?)"));
    for (auto match = fixer.match(colorScheme); match.hasMatch(); match = fixer.match(colorScheme)) {
        colorScheme.replace(match.capturedStart(), match.capturedLength(), match.captured(1).toUpper());
    }
    colorScheme.replace(0, 1, colorScheme.at(0).toUpper());

    // NOTE: why this loop trough all the scheme files?
    // the scheme theme name is an heuristic, there is no plugin metadata whatsoever.
    // is based on the file name stripped from weird characters or the
    // eventual id- prefix store.kde.org puts, so we can just find a
    // theme that ends as the specified name
    const QStringList schemeDirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
    for (const QString &dir : schemeDirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.colors"));
        for (const QString &file : fileNames) {
            if (file.endsWith(colorScheme + QStringLiteral(".colors"))) {
                return dir + QLatin1Char('/') + file;
            }
        }
    }
    return QString();
}

void LookAndFeelManager::save(const KPackage::Package &package, const KPackage::Package &previousPackage, Contents applyMask)
{
    // The items to apply are the package contents filtered with the user selection mask
    const Contents itemsToApply = packageContents(package) & applyMask;

    if (itemsToApply.testFlag(DesktopLayout) && m_mode == Mode::Apply) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                              QStringLiteral("/PlasmaShell"),
                                                              QStringLiteral("org.kde.PlasmaShell"),
                                                              QStringLiteral("loadLookAndFeelDefaultLayout"));

        QList<QVariant> args;
        args << m_data->settings()->lookAndFeelPackage();
        message.setArguments(args);

        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);

        if (m_applyLatteLayout) {
            //! latte exists in system and user has chosen to update desktop layout
            setLatteLayout(package.filePath("layouts", "looknfeel.layout.latte"), package.metadata().name());
        }
    }

    if (!package.filePath("layoutdefaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("layoutdefaults"));
        KConfigGroup group(conf, "kwinrc");
        if (itemsToApply.testFlag(TitlebarLayout)) {
            group = KConfigGroup(&group, "org.kde.kdecoration2");
            setTitlebarLayout(group.readEntry("ButtonsOnLeft", QString()), group.readEntry("ButtonsOnRight", QString()));
        }
        if (itemsToApply.testFlag(DesktopLayout) && m_mode == Mode::Apply) {
            group = KConfigGroup(conf, "kwinrc");
            group = KConfigGroup(&group, "Windows");
            setBorderlessMaximized(group.readEntry("BorderlessMaximizedWindows", QString()));
        }
    }
    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup group(conf, "kdeglobals");
        group = KConfigGroup(&group, "KDE");
        if (itemsToApply.testFlag(WidgetStyle)) {
            QString widgetStyle = group.readEntry("widgetStyle", QString());
            // Some global themes refer to breeze's widgetStyle with a lowercase b.
            if (widgetStyle == QStringLiteral("breeze")) {
                widgetStyle = QStringLiteral("Breeze");
            }

            setWidgetStyle(widgetStyle);
        }

        if (itemsToApply.testFlag(Colors)) {
            QString colorsFile = package.filePath("colors");
            KConfigGroup group(conf, "kdeglobals");
            group = KConfigGroup(&group, "General");
            QString colorScheme = group.readEntry("ColorScheme", QString());

            if (!colorsFile.isEmpty()) {
                if (!colorScheme.isEmpty()) {
                    setColors(colorScheme, colorsFile);
                } else {
                    setColors(package.metadata().name(), colorsFile);
                }
            } else if (!colorScheme.isEmpty()) {
                QString path = colorSchemeFile(colorScheme);
                if (!path.isEmpty()) {
                    setColors(colorScheme, path);
                }
            }
        }

        if (itemsToApply.testFlag(Icons)) {
            group = KConfigGroup(conf, "kdeglobals");
            group = KConfigGroup(&group, "Icons");
            setIcons(group.readEntry("Theme", QString()));
        }

        if (itemsToApply.testFlag(PlasmaTheme)) {
            group = KConfigGroup(conf, "plasmarc");
            group = KConfigGroup(&group, "Theme");
            setPlasmaTheme(group.readEntry("name", QString()));
        }

        if (itemsToApply.testFlag(Cursors)) {
            group = KConfigGroup(conf, "kcminputrc");
            group = KConfigGroup(&group, "Mouse");
            setCursorTheme(group.readEntry("cursorTheme", QString()));
        }

        if (itemsToApply.testFlag(WindowSwitcher)) {
            group = KConfigGroup(conf, "kwinrc");
            group = KConfigGroup(&group, "WindowSwitcher");
            setWindowSwitcher(group.readEntry("LayoutName", QString()));
        }

        if (itemsToApply.testFlag(WindowPlacement)) {
            group = KConfigGroup(conf, "kwinrc");
            group = KConfigGroup(&group, "Windows");
            setWindowPlacement(group.readEntry("Placement", QStringLiteral("Centered")));
        }

        if (itemsToApply.testFlag(ShellPackage)) {
            group = KConfigGroup(conf, "plasmashellrc");
            group = KConfigGroup(&group, "Shell");
            setShellPackage(group.readEntry("ShellPackage", QString()));
        }

        if (itemsToApply.testFlag(WindowDecoration)) {
            group = KConfigGroup(conf, "kwinrc");
            group = KConfigGroup(&group, "org.kde.kdecoration2");

#ifdef HAVE_BREEZE_DECO
            setWindowDecoration(group.readEntry("library", QStringLiteral(BREEZE_KDECORATION_PLUGIN_ID)),
                                group.readEntry("theme", QStringLiteral("Breeze")),
                                group.readEntry("NoPlugin", false));
#else
            setWindowDecoration(group.readEntry("library", QStringLiteral("org.kde.kwin.aurorae")),
                                group.readEntry("theme", QStringLiteral("kwin4_decoration_qml_plastik")),
                                group.readEntry("NoPlugin", false));
#endif
        }

        if (itemsToApply.testFlag(Fonts)) {
            group = KConfigGroup(conf, "kdeglobals");
            group = KConfigGroup(&group, "General");
            setGeneralFont(group.readEntry("font", QString()));
            setFixedFont(group.readEntry("fixed", QString()));
            setSmallestReadableFont(group.readEntry("smallestReadableFont", QString()));
            setToolbarFont(group.readEntry("toolBarFont", QString()));
            setMenuFont(group.readEntry("menuFont", QString()));
            group = KConfigGroup(conf, "kdeglobals");
            group = KConfigGroup(&group, "WM");
            setWindowTitleFont(group.readEntry("activeFont"));
            if (m_fontsChanged) {
                Q_EMIT fontsChanged();
                m_fontsChanged = false;
            }
        }

        if (itemsToApply.testFlag(BorderSize)) {
            group = KConfigGroup(conf, "kwinrc");
            group = KConfigGroup(&group, "org.kde.kdecoration2");
            setBorderSize(group.readEntry("BorderSize", QString()));
        }

        if (itemsToApply.testFlag(SplashScreen)) {
            group = KConfigGroup(conf, "ksplashrc");
            group = KConfigGroup(&group, "KSplash");
            QString splashScreen = (group.readEntry("Theme", QString()));
            if (!splashScreen.isEmpty()) {
                setSplashScreen(splashScreen);
            } else {
                setSplashScreen(m_data->settings()->lookAndFeelPackage());
            }
        }
        if (itemsToApply.testFlag(LockScreen)) {
            setLockScreen(m_data->settings()->lookAndFeelPackage());
        }

        QFile packageFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults/package"));
        packageFile.open(QIODevice::WriteOnly);
        packageFile.write(m_data->settings()->lookAndFeelPackage().toUtf8());

        if (m_mode == Mode::Defaults) {
            return;
        }

        if (m_plasmashellChanged) {
            QDBusMessage message =
                QDBusMessage::createSignal(QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("refreshCurrentShell"));
            QDBusConnection::sessionBus().send(message);
        }

        // autostart
        if (itemsToApply.testFlag(DesktopLayout)) {
            QStringList toStop;
            KService::List toStart;
            // remove all the old package to autostart
            {
                KSharedConfigPtr oldConf = KSharedConfig::openConfig(previousPackage.filePath("defaults"));
                group = KConfigGroup(oldConf, QStringLiteral("Autostart"));
                const QStringList autostartServices = group.readEntry("Services", QStringList());

                if (qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
                    for (const QString &serviceFile : autostartServices) {
                        KService service(serviceFile + QStringLiteral(".desktop"));
                        PlasmaAutostart as(serviceFile);
                        as.setAutostarts(false);
                        QString serviceName = service.property(QStringLiteral("X-DBUS-ServiceName")).toString();
                        toStop.append(serviceName);
                    }
                }
            }
            // Set all the stuff in the new lnf to autostart
            {
                group = KConfigGroup(conf, QStringLiteral("Autostart"));
                const QStringList autostartServices = group.readEntry("Services", QStringList());

                for (const QString &serviceFile : autostartServices) {
                    KService::Ptr service(new KService(serviceFile + QStringLiteral(".desktop")));
                    PlasmaAutostart as(serviceFile);
                    as.setCommand(service->exec());
                    as.setAutostarts(true);
                    const QString serviceName = service->property(QStringLiteral("X-DBUS-ServiceName")).toString();
                    toStop.removeAll(serviceName);
                    if (qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
                        toStart += service;
                    }
                }
            }
            Q_EMIT refreshServices(toStop, toStart);
        }
    }
    // Reload KWin if something changed, but only once.
    if (itemsToApply & KWinSettings) {
        QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"));
        QDBusConnection::sessionBus().send(message);
    }
}

bool LookAndFeelManager::remove(const KPackage::Package &package, LookAndFeelManager::Contents contentsMask)
{
    QDir packageRootDir(package.path());
    if (!packageRootDir.isReadable()) {
        // Permission denied
        return false;
    }

    const Contents itemsToRemove = packageContents(package) & contentsMask;

    // Remove package dependencies
    auto config = KSharedConfig::openConfig(package.filePath("defaults"), KConfig::NoGlobals);

    // WidgetStyle
    if (itemsToRemove.testFlag(WidgetStyle)) {
        const QString widgetStyle = configValue(config, "kdeglobals/KDE", "widgetStyle");

        const QDir widgetStyleDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("kstyle")
                                  + QDir::separator() + QStringLiteral("themes"));
        if (widgetStyleDir.exists()) {
            // Read config to get style name
            const QStringList styleFileList = widgetStyleDir.entryList({QStringLiteral("*.themerc")}, QDir::Files | QDir::NoDotAndDotDot);
            for (const QString &path : styleFileList) {
                auto widgetStyleConfig = KSharedConfig::openConfig(widgetStyleDir.absoluteFilePath(path), KConfig::SimpleConfig);
                KConfigGroup widgetStyleKDEGroup(widgetStyleConfig, "KDE");
                if (widgetStyleKDEGroup.exists() && widgetStyleKDEGroup.hasKey("WidgetStyle")) {
                    if (widgetStyleKDEGroup.readEntry("WidgetStyle", "") == widgetStyle) {
                        QFile(widgetStyleDir.absoluteFilePath(path)).remove();
                        break;
                    }
                }
            }
        }
    }

    // Color scheme
    if (itemsToRemove.testFlag(Colors)) {
        const QString colorScheme = configValue(config, "kdeglobals/General", "ColorScheme");
        QFile schemeFile(colorSchemeFile(colorScheme));
        if (schemeFile.exists()) {
            schemeFile.remove();
        }
    }

    // Desktop Theme and Icon (Icons are in the theme folder). Only remove if both selected
    // TODO: Remove separately
    if (itemsToRemove.testFlag(PlasmaTheme) && itemsToRemove.testFlag(Icons)) {
        const QString themeName = configValue(config, "plasmarc/Theme", "name");
        QDir themeDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("plasma") + QDir::separator()
                      + QStringLiteral("desktoptheme") + QDir::separator() + themeName);
        if (themeDir.exists()) {
            themeDir.removeRecursively();
        }
    }

    // Wallpaper
    if (itemsToRemove.testFlag(Wallpaper)) {
        const QString image = configValue(config, "Wallpaper", "Image");
        const QDir wallpaperDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("wallpapers"));
        if (wallpaperDir.exists()) {
            QFileInfo wallpaperFile(wallpaperDir.absoluteFilePath(image));
            if (wallpaperFile.isFile()) {
                // Single file, use QFile to remove
                QFile(wallpaperFile.absoluteFilePath()).remove();
            } else if (wallpaperFile.isDir()) {
                // A package, use QDir to remove the folder
                QDir(wallpaperFile.absoluteFilePath()).removeRecursively();
            }
        }
    }

    if (itemsToRemove.testFlag(WindowSwitcher)) {
        const QString layoutName = configValue(config, "kwinrc/WindowSwitcher", "LayoutName");
        const auto windowSwitcherPackages =
            KPackage::PackageLoader::self()->findPackages(QStringLiteral("KWin/WindowSwitcher"), QString(), [&layoutName](const KPluginMetaData &meta) {
                return meta.pluginId() == layoutName;
            });
        for (const auto &meta : windowSwitcherPackages) {
            QFileInfo(meta.fileName()).absoluteDir().removeRecursively();
        }
    }

    return packageRootDir.removeRecursively();
}

void LookAndFeelManager::setCursorTheme(const QString themeName)
{
    // TODO: use pieces of cursor kcm when moved to plasma-desktop
    if (themeName.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kcminputrc"), QStringLiteral("Mouse"), QStringLiteral("cursorTheme"), themeName, KConfig::Notify);
    Q_EMIT cursorsChanged(themeName);
}

void LookAndFeelManager::setMode(Mode mode)
{
    m_mode = mode;
}
