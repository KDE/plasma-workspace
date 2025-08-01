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
#include "../colors/colorsapplicator.h"
#include "../kcms-common_p.h"
#include "config-kcm.h"
#include "krdb.h"
#include <KIO/CommandLauncherJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KPackage/PackageLoader>
#include <KSharedConfig>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleFactory>

#include "config-X11.h"
#if HAVE_X11
#include <X11/Xlib.h>
#include <private/qtx11extras_p.h>

#ifdef HAVE_XCURSOR
#include "../cursortheme/xcursor/xcursortheme.h"
#include <X11/Xcursor/Xcursor.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#endif

using namespace Qt::StringLiterals;

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
    , m_plasmashellChanged(false)
    , m_fontsChanged(false)
    , m_plasmaLocked(false)
{
    m_applyLatteLayout = (KService::serviceByDesktopName(u"org.kde.latte-dock"_s) != nullptr);

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                          QStringLiteral("/PlasmaShell"),
                                                          QStringLiteral("org.kde.PlasmaShell"),
                                                          QStringLiteral("immutable"));
    QDBusPendingCall async = QDBusConnection::sessionBus().asyncCall(message);

    // Create watcher for the pending call
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    // Connect watcher finished signal to our slot
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<bool> reply = *call;

        if (reply.isError()) {
            qWarning() << "Error:" << reply.error().message();
        } else {
            const bool locked = reply.value();
            if (locked != m_plasmaLocked) {
                m_plasmaLocked = locked;
                Q_EMIT plasmaLockedChanged(locked);
            }
        }
        call->deleteLater();
    });
}

bool LookAndFeelManager::isPlasmaLocked() const
{
    return m_plasmaLocked;
}

LookAndFeelManager::Contents LookAndFeelManager::packageContents(const KPackage::Package &pkg) const
{
    Contents contents = Empty;

    contents.setFlag(SplashScreen, !pkg.filePath("splashmainscript").isEmpty());

    contents.setFlag(DesktopLayout, !pkg.filePath("layouts").isEmpty());

    // TODO: Those seem unused... are deprecated?
    contents.setFlag(RunCommand, !pkg.filePath("runcommandmainscript").isEmpty());
    contents.setFlag(LogOutScript, !pkg.filePath("logoutmainscript").isEmpty());

    if (!pkg.filePath("layoutdefaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("layoutdefaults"));
        contents.setFlag(TitlebarLayout, configProvides(conf, u"kwinrc/org.kde.kdecoration2"_s, {u"ButtonsOnLeft"_s, u"ButtonsOnRight"_s}));
    }

    if (!pkg.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("defaults"));

        contents.setFlag(Colors, configProvides(conf, u"kdeglobals/General"_s, u"ColorScheme"_s) || !pkg.filePath("colors"_ba).isEmpty());
        contents.setFlag(WidgetStyle, configProvides(conf, u"kdeglobals/KDE"_s, u"widgetStyle"_s));
        contents.setFlag(Icons, configProvides(conf, u"kdeglobals/Icons"_s, u"Theme"_s));

        contents.setFlag(PlasmaTheme, configProvides(conf, u"plasmarc/Theme"_s, u"name"_s));
        contents.setFlag(Wallpaper, configProvides(conf, u"Wallpaper"_s, u"Image"_s));
        contents.setFlag(Cursors, configProvides(conf, u"kcminputrc/Mouse"_s, u"cursorTheme"_s));

        contents.setFlag(WindowSwitcher, configProvides(conf, u"kwinrc/WindowSwitcher"_s, u"LayoutName"_s));
        contents.setFlag(WindowDecoration, configProvides(conf, u"kwinrc/org.kde.kdecoration2"_s, {u"library"_s, u"NoPlugin"_s}));
        contents.setFlag(BorderSize, configProvides(conf, u"kwinrc/org.kde.kdecoration2"_s, u"BorderSize"_s));

        contents.setFlag(Fonts,
                         configProvides(conf, u"kdeglobals/WM"_s, {u"font"_s, u"fixed"_s, u"smallestReadableFont"_s, u"toolBarFont"_s, u"menuFont"_s})
                             || configProvides(conf, u"kdeglobals/General"_s, u"activeFont"_s));

        contents.setFlag(WindowPlacement, configProvides(conf, u"kwinrc/Windows"_s, u"Placement"_s));
        contents.setFlag(ShellPackage, configProvides(conf, u"plasmashellrc/Shell"_s, u"ShellPackage"_s));

        // TODO: Currently managed together within the "DesktopLayout" content
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
    writeNewDefaults(group, defaultGroup, QStringLiteral("NoPlugin"), noPlugin ? u"true"_s : u"false"_s, KConfig::Notify);
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
        writeNewDefaults(QStringLiteral("kwinrc"), QStringLiteral("Windows"), QStringLiteral("BorderlessMaximizedWindows"), u"false"_s, KConfig::Notify);
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
    if (m_mode == Mode::Apply) {
        // FIXME: changing style on the fly breaks QQuickWidgets
        notifyKcmChange(GlobalChangeType::StyleChanged);
    }

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
    if (m_mode == Mode::Apply) {
        // FIXME: changing style on the fly breaks QQuickWidgets
        notifyKcmChange(GlobalChangeType::PaletteChanged);
    }

    Q_EMIT colorsChanged();
}

void LookAndFeelManager::setIcons(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kdeglobals"), QStringLiteral("Icons"), QStringLiteral("Theme"), theme, KConfig::Notify);
    if (m_mode == Mode::Apply) {
        for (int i = 0; i < KIconLoader::LastGroup; i++) {
            KIconLoader::emitChange(KIconLoader::Group(i));
        }
    }

    Q_EMIT iconsChanged();
}

void LookAndFeelManager::setLatteLayout(const QString &filepath, const QString &name)
{
    if (filepath.isEmpty()) {
        // there is no latte layout
        KIO::CommandLauncherJob latteapp(QStringLiteral("latte-dock"), {QStringLiteral("--disable-autostart")});
        latteapp.setDesktopName(u"org.kde.latte-dock"_s);
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
        latteapp.setDesktopName(u"org.kde.latte-dock"_s);
        latteapp.start();
    }
}

void LookAndFeelManager::setPlasmaTheme(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("plasmarc"), QStringLiteral("Theme"), QStringLiteral("name"), theme, KConfig::Notify);
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

void LookAndFeelManager::save(const KPackage::Package &package, Contents applyMask)
{
    // The items to apply are the package contents filtered with the user selection mask
    const QString packageId = package.metadata().pluginId();
    const Contents itemsToApply = packageContents(package) & applyMask;

    if (itemsToApply.testFlag(DesktopLayout) && m_mode == Mode::Apply) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                              QStringLiteral("/PlasmaShell"),
                                                              QStringLiteral("org.kde.PlasmaShell"),
                                                              QStringLiteral("loadLookAndFeelDefaultLayout"));

        QList<QVariant> args;
        args << packageId;
        message.setArguments(args);

        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);

        if (m_applyLatteLayout) {
            //! latte exists in system and user has chosen to update desktop layout
            setLatteLayout(package.filePath("layouts", u"looknfeel.layout.latte"_s), package.metadata().name());
        }
    }

    if (!package.filePath("layoutdefaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("layoutdefaults"));
        KConfigGroup group(conf, u"kwinrc"_s);
        if (itemsToApply.testFlag(TitlebarLayout)) {
            group = KConfigGroup(&group, u"org.kde.kdecoration2"_s);
            setTitlebarLayout(group.readEntry("ButtonsOnLeft", QString()), group.readEntry("ButtonsOnRight", QString()));
        }
        if (itemsToApply.testFlag(DesktopLayout) && m_mode == Mode::Apply) {
            group = KConfigGroup(conf, u"kwinrc"_s);
            group = KConfigGroup(&group, u"Windows"_s);
            setBorderlessMaximized(group.readEntry("BorderlessMaximizedWindows", QString()));
        }
    }
    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup group(conf, u"kdeglobals"_s);
        group = KConfigGroup(&group, u"KDE"_s);
        if (itemsToApply.testFlag(WidgetStyle)) {
            QString widgetStyle = group.readEntry("widgetStyle", QString());
            // Some global themes refer to breeze's widgetStyle with a lowercase b.
            if (widgetStyle == u"breeze") {
                widgetStyle = QStringLiteral("Breeze");
            }

            setWidgetStyle(widgetStyle);
        }

        if (itemsToApply.testFlag(Colors)) {
            QString colorsFile = package.filePath("colors");
            KConfigGroup group(conf, u"kdeglobals"_s);
            group = KConfigGroup(&group, u"General"_s);
            QString colorScheme = group.readEntry("ColorScheme", QString());

            // TODO: Port to KLookAndFeel::colorSchemeFilePath().
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
            group = KConfigGroup(conf, u"kdeglobals"_s);
            group = KConfigGroup(&group, u"Icons"_s);
            setIcons(group.readEntry("Theme", QString()));
        }

        if (itemsToApply.testFlag(PlasmaTheme)) {
            group = KConfigGroup(conf, u"plasmarc"_s);
            group = KConfigGroup(&group, u"Theme"_s);
            setPlasmaTheme(group.readEntry("name", QString()));
        }

        if (itemsToApply.testFlag(Cursors)) {
            group = KConfigGroup(conf, u"kcminputrc"_s);
            group = KConfigGroup(&group, u"Mouse"_s);
            setCursorTheme(group.readEntry("cursorTheme", QString()));
        }

        if (itemsToApply.testFlag(WindowSwitcher)) {
            group = KConfigGroup(conf, u"kwinrc"_s);
            group = KConfigGroup(&group, u"WindowSwitcher"_s);
            setWindowSwitcher(group.readEntry("LayoutName", QString()));
        }

        if (itemsToApply.testFlag(WindowPlacement)) {
            group = KConfigGroup(conf, u"kwinrc"_s);
            group = KConfigGroup(&group, u"Windows"_s);
            setWindowPlacement(group.readEntry("Placement", QStringLiteral("Centered")));
        }

        if (itemsToApply.testFlag(ShellPackage)) {
            group = KConfigGroup(conf, u"plasmashellrc"_s);
            group = KConfigGroup(&group, u"Shell"_s);
            setShellPackage(group.readEntry("ShellPackage", QString()));
        }

        if (itemsToApply.testFlag(WindowDecoration)) {
            group = KConfigGroup(conf, u"kwinrc"_s);
            group = KConfigGroup(&group, u"org.kde.kdecoration2"_s);

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
            group = KConfigGroup(conf, u"kdeglobals"_s);
            group = KConfigGroup(&group, u"General"_s);
            setGeneralFont(group.readEntry("font", QString()));
            setFixedFont(group.readEntry("fixed", QString()));
            setSmallestReadableFont(group.readEntry("smallestReadableFont", QString()));
            setToolbarFont(group.readEntry("toolBarFont", QString()));
            setMenuFont(group.readEntry("menuFont", QString()));
            group = KConfigGroup(conf, u"kdeglobals"_s);
            group = KConfigGroup(&group, u"WM"_s);
            setWindowTitleFont(group.readEntry("activeFont"));
            if (m_fontsChanged) {
                if (m_mode == Mode::Apply) {
                    QDBusMessage message = QDBusMessage::createSignal(u"/KDEPlatformTheme"_s, u"org.kde.KDEPlatformTheme"_s, u"refreshFonts"_s);
                    QDBusConnection::sessionBus().send(message);
                }
                Q_EMIT fontsChanged();
                m_fontsChanged = false;
            }
        }

        if (itemsToApply.testFlag(BorderSize)) {
            group = KConfigGroup(conf, u"kwinrc"_s);
            group = KConfigGroup(&group, u"org.kde.kdecoration2"_s);
            setBorderSize(group.readEntry("BorderSize", QString()));
        }

        if (itemsToApply.testFlag(SplashScreen)) {
            group = KConfigGroup(conf, u"ksplashrc"_s);
            group = KConfigGroup(&group, u"KSplash"_s);
            QString splashScreen = (group.readEntry("Theme", QString()));
            if (!splashScreen.isEmpty()) {
                setSplashScreen(splashScreen);
            } else {
                setSplashScreen(packageId);
            }
        }

        QFile packageFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults/package"));
        packageFile.open(QIODevice::WriteOnly);
        packageFile.write(packageId.toUtf8());

        if (m_mode == Mode::Defaults) {
            return;
        }

        if (m_plasmashellChanged) {
            QDBusMessage message =
                QDBusMessage::createSignal(QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("refreshCurrentShell"));
            QDBusConnection::sessionBus().send(message);
        }

        runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors | KRdbExportQtSettings | KRdbExportXftSettings);
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
        const QString widgetStyle = configValue(config, u"kdeglobals/KDE"_s, u"widgetStyle"_s);

        const QDir widgetStyleDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + QStringLiteral("kstyle")
                                  + QDir::separator() + QStringLiteral("themes"));
        if (widgetStyleDir.exists()) {
            // Read config to get style name
            const QStringList styleFileList = widgetStyleDir.entryList({QStringLiteral("*.themerc")}, QDir::Files | QDir::NoDotAndDotDot);
            for (const QString &path : styleFileList) {
                auto widgetStyleConfig = KSharedConfig::openConfig(widgetStyleDir.absoluteFilePath(path), KConfig::SimpleConfig);
                KConfigGroup widgetStyleKDEGroup(widgetStyleConfig, u"KDE"_s);
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
        const QString colorScheme = configValue(config, u"kdeglobals/General"_s, u"ColorScheme"_s);
        QFile schemeFile(colorSchemeFile(colorScheme));
        if (schemeFile.exists()) {
            schemeFile.remove();
        }
    }

    // Desktop Theme and Icon (Icons are in the theme folder). Only remove if both selected
    // TODO: Remove separately
    if (itemsToRemove.testFlag(PlasmaTheme) && itemsToRemove.testFlag(Icons)) {
        const QString themeName = configValue(config, u"plasmarc/Theme"_s, u"name"_s);
        QDir themeDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + u"plasma" + QDir::separator()
                      + u"desktoptheme" + QDir::separator() + themeName);
        if (themeDir.exists()) {
            themeDir.removeRecursively();
        }
    }

    // Wallpaper
    if (itemsToRemove.testFlag(Wallpaper)) {
        const QString image = configValue(config, u"Wallpaper"_s, u"Image"_s);
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
        const QString layoutName = configValue(config, u"kwinrc/WindowSwitcher"_s, u"LayoutName"_s);
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

QDir LookAndFeelManager::cursorThemeDir(const QString &theme, const int depth)
{
    // Prevent infinite recursion
    if (depth > 10) {
        return QDir();
    }

    // Search each icon theme directory for 'theme'
    for (const QString &baseDir : cursorSearchPaths()) {
        QDir dir(baseDir);
        if (!dir.exists() || !dir.cd(theme)) {
            continue;
        }

        // If there's a cursors subdir, we'll assume this is a cursor theme
        if (dir.exists(QStringLiteral("cursors"))) {
            return dir;
        }

        // If the theme doesn't have an index.theme file, it can't inherit any themes.
        if (!dir.exists(QStringLiteral("index.theme"))) {
            continue;
        }

        // Open the index.theme file, so we can get the list of inherited themes
        KConfig config(dir.path() + QStringLiteral("/index.theme"), KConfig::NoGlobals);
        KConfigGroup cg(&config, u"Icon Theme"_s);

        // Recurse through the list of inherited themes, to check if one of them
        // is a cursor theme.
        const QStringList inherits = cg.readEntry("Inherits", QStringList());
        for (const QString &inherit : inherits) {
            // Avoid possible DoS
            if (inherit == theme) {
                continue;
            }

            if (cursorThemeDir(inherit, depth + 1).exists()) {
                return dir;
            }
        }
    }

    return QDir();
}

QStringList LookAndFeelManager::cursorSearchPaths()
{
#ifdef HAVE_XCURSOR
#if XCURSOR_LIB_MAJOR == 1 && XCURSOR_LIB_MINOR < 1

    if (!m_cursorSearchPaths.isEmpty())
        return m_cursorSearchPaths;
    // These are the default paths Xcursor will scan for cursor themes
    QString path("~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons");

    // If XCURSOR_PATH is set, use that instead of the default path
    char *xcursorPath = std::getenv("XCURSOR_PATH");
    if (xcursorPath)
        path = xcursorPath;
#else
    // Get the search path from Xcursor
    QString path = QString::fromLocal8Bit(XcursorLibraryPath());
#endif

    // Separate the paths
    m_cursorSearchPaths = path.split(QLatin1Char(':'), Qt::SkipEmptyParts);

    // Remove duplicates
    QMutableStringListIterator i(m_cursorSearchPaths);
    while (i.hasNext()) {
        const QString path = i.next();
        QMutableStringListIterator j(i);
        while (j.hasNext())
            if (j.next() == path)
                j.remove();
    }

    // Expand all occurrences of ~/ to the home dir
    m_cursorSearchPaths.replaceInStrings(QRegularExpression(QStringLiteral("^~\\/")), QString(QDir::home().path() + QDir::separator()));
#endif
    return m_cursorSearchPaths;
}

void LookAndFeelManager::applyCursorTheme(const QString &themeName)
{
#ifdef HAVE_XCURSOR
    // Require the Xcursor version that shipped with X11R6.9 or greater, since
    // in previous versions the Xfixes code wasn't enabled due to a bug in the
    // build system (freedesktop bug #975).
#if defined(HAVE_XFIXES) && XFIXES_MAJOR >= 2 && XCURSOR_LIB_VERSION >= 10105
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kcminputrc"));
    KConfigGroup cg(config, QStringLiteral("Mouse"));
    const int cursorSize = cg.readEntry("cursorSize", 24);

    QDir themeDir = cursorThemeDir(themeName, 0);
    if (!themeDir.exists()) {
        return;
    }

    XCursorTheme theme(themeDir);

    if (!CursorTheme::haveXfixes()) {
        return;
    }

    // Update the Xcursor X resources
    runRdb(0);

    // Notify all applications that the cursor theme has changed
    notifyKcmChange(GlobalChangeType::CursorChanged);

    // Reload the standard cursors
    QStringList names;

    // Qt cursors
    names << QStringLiteral("left_ptr") << QStringLiteral("up_arrow") << QStringLiteral("cross") << QStringLiteral("wait") << QStringLiteral("left_ptr_watch")
          << QStringLiteral("ibeam") << QStringLiteral("size_ver") << QStringLiteral("size_hor") << QStringLiteral("size_bdiag") << QStringLiteral("size_fdiag")
          << QStringLiteral("size_all") << QStringLiteral("split_v") << QStringLiteral("split_h") << QStringLiteral("pointing_hand")
          << QStringLiteral("openhand") << QStringLiteral("closedhand") << QStringLiteral("forbidden") << QStringLiteral("whats_this") << QStringLiteral("copy")
          << QStringLiteral("move") << QStringLiteral("link");

    // X core cursors
    names << QStringLiteral("X_cursor") << QStringLiteral("right_ptr") << QStringLiteral("hand1") << QStringLiteral("hand2") << QStringLiteral("watch")
          << QStringLiteral("xterm") << QStringLiteral("crosshair") << QStringLiteral("left_ptr_watch") << QStringLiteral("center_ptr")
          << QStringLiteral("sb_h_double_arrow") << QStringLiteral("sb_v_double_arrow") << QStringLiteral("fleur") << QStringLiteral("top_left_corner")
          << QStringLiteral("top_side") << QStringLiteral("top_right_corner") << QStringLiteral("right_side") << QStringLiteral("bottom_right_corner")
          << QStringLiteral("bottom_side") << QStringLiteral("bottom_left_corner") << QStringLiteral("left_side") << QStringLiteral("question_arrow")
          << QStringLiteral("pirate");

    for (const QString &name : std::as_const(names)) {
        XFixesChangeCursorByName(QX11Info::display(), theme.loadCursor(name, cursorSize), QFile::encodeName(name).constData());
    }

#else
    KMessageBox::information(this,
                             i18n("You have to restart the Plasma session for these changes to take effect."),
                             i18n("Cursor Settings Changed"),
                             "CursorSettingsChanged");
#endif
#endif
}

void LookAndFeelManager::setCursorTheme(const QString themeName)
{
    // TODO: use pieces of cursor kcm when moved to plasma-desktop
    if (themeName.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("kcminputrc"), QStringLiteral("Mouse"), QStringLiteral("cursorTheme"), themeName, KConfig::Notify);
    if (m_mode == Mode::Apply) {
        applyCursorTheme(themeName);
    }

    Q_EMIT cursorsChanged(themeName);
}

void LookAndFeelManager::setMode(Mode mode)
{
    m_mode = mode;
}
