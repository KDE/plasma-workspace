/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "lookandfeelmanager.h"
#include "../../startkde/plasmaautostart/plasmaautostart.h"
#include "../colors/colorsapplicator.h"
#include "config-kcm.h"
#include "lookandfeeldata.h"
#include "lookandfeelsettings.h"
#include <KService>
#include <KSharedConfig>
#include <QDBusConnection>
#include <QDBusMessage>

#ifdef HAVE_XCURSOR
#include "../cursortheme/xcursor/xcursortheme.h"
#include <X11/Xcursor/Xcursor.h>
#endif

LookAndFeelManager::LookAndFeelManager(QObject *parent)
    : QObject(parent)
    , m_data(new LookAndFeelData(this))
{
}

LookAndFeelSettings *LookAndFeelManager::settings() const
{
    return m_data->settings();
}

void LookAndFeelManager::setSplashScreen(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("ksplashrc"));
    KConfigGroup cg(config, QStringLiteral("KSplash"));

    KConfig configDefault(configDefaults(QStringLiteral("ksplashrc")));
    KConfigGroup cgd(&configDefault, QStringLiteral("KSplash"));
    writeNewDefaults(cg, cgd, QStringLiteral("Theme"), theme);
    // TODO: a way to set none as spash in the l&f
    writeNewDefaults(cg, cgd, QStringLiteral("Engine"), QStringLiteral("KSplashQML"));
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

void LookAndFeelManager::setDesktopSwitcher(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup cg(config, QStringLiteral("TabBox"));

    KConfig configDefault(configDefaults(QStringLiteral("kwinrc")));
    KConfigGroup cgd(&configDefault, QStringLiteral("TabBox"));
    writeNewDefaults(cg, cgd, QStringLiteral("DesktopLayout"), theme);
    writeNewDefaults(cg, cgd, QStringLiteral("DesktopListLayout"), theme);
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

void LookAndFeelManager::setWindowDecoration(const QString &library, const QString &theme)
{
    if (library.isEmpty()) {
        return;
    }

    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup cg(config, QStringLiteral("org.kde.kdecoration2"));

    KConfig configDefault(configDefaults(QStringLiteral("kwinrc")));
    KConfigGroup cgd(&configDefault, QStringLiteral("org.kde.kdecoration2"));
    writeNewDefaults(cg, cgd, QStringLiteral("library"), library);
    writeNewDefaults(cg, cgd, QStringLiteral("theme"), theme, KConfig::Notify);
}

void LookAndFeelManager::setWidgetStyle(const QString &style)
{
    if (style.isEmpty()) {
        return;
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
    writeNewDefaults(*kdeGlobalsCfg, configDefault, QStringLiteral("General"), QStringLiteral("ColorScheme"), scheme, KConfig::Notify);

    if (m_mode == Mode::Apply) {
        applyScheme(colorFile, kdeGlobalsCfg.data(), KConfig::Notify);
    }

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

void LookAndFeelManager::setPlasmaTheme(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    writeNewDefaults(QStringLiteral("plasmarc"), QStringLiteral("Theme"), QStringLiteral("name"), theme);
}

void LookAndFeelManager::setResetDefaultLayout(bool reset)
{
    if (m_resetDefaultLayout == reset) {
        return;
    }
    m_resetDefaultLayout = reset;
    emit resetDefaultLayoutChanged();
}

bool LookAndFeelManager::resetDefaultLayout() const
{
    return m_resetDefaultLayout;
}

void LookAndFeelManager::writeNewDefaults(const QString &filename,
                                          const QString &group,
                                          const QString &key,
                                          const QString &value,
                                          KConfig::WriteConfigFlags writeFlags)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(filename);
    KConfigGroup cg(config, group);

    KConfig configDefault(configDefaults(filename));
    KConfigGroup cgd(&configDefault, group);

    writeNewDefaults(cg, cgd, key, value, writeFlags);
}

void LookAndFeelManager::writeNewDefaults(KConfig &config,
                                          KConfig &configDefault,
                                          const QString &group,
                                          const QString &key,
                                          const QString &value,
                                          KConfig::WriteConfigFlags writeFlags)
{
    KConfigGroup cg(&config, group);
    KConfigGroup cgd(&configDefault, group);

    writeNewDefaults(cg, cgd, key, value, writeFlags);
}

void LookAndFeelManager::writeNewDefaults(KConfigGroup &cg, KConfigGroup &cgd, const QString &key, const QString &value, KConfig::WriteConfigFlags writeFlags)
{
    cgd.writeEntry(key, value, writeFlags);
    cgd.sync();

    if (m_mode == Mode::Apply) {
        cg.revertToDefault(key, writeFlags);
        cg.sync();
    }
}

KConfig LookAndFeelManager::configDefaults(const QString &filename)
{
    return KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults/") + filename, KConfig::SimpleConfig);
}

void LookAndFeelManager::save(const KPackage::Package &package, const KPackage::Package &previousPackage)
{
    if (m_resetDefaultLayout && m_mode == Mode::Apply) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                              QStringLiteral("/PlasmaShell"),
                                                              QStringLiteral("org.kde.PlasmaShell"),
                                                              QStringLiteral("loadLookAndFeelDefaultLayout"));

        QList<QVariant> args;
        args << m_data->settings()->lookAndFeelPackage();
        message.setArguments(args);

        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
    }

    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "KDE");
        if (m_applyWidgetStyle) {
            QString widgetStyle = cg.readEntry("widgetStyle", QString());
            // Some global themes refer to breeze's widgetStyle with a lowercase b.
            if (widgetStyle == QStringLiteral("breeze")) {
                widgetStyle = QStringLiteral("Breeze");
            }

            setWidgetStyle(widgetStyle);
        }

        if (m_applyColors) {
            QString colorsFile = package.filePath("colors");
            KConfigGroup cg(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "General");
            QString colorScheme = cg.readEntry("ColorScheme", QString());

            if (!colorsFile.isEmpty()) {
                if (!colorScheme.isEmpty()) {
                    setColors(colorScheme, colorsFile);
                } else {
                    setColors(package.metadata().name(), colorsFile);
                }
            } else if (!colorScheme.isEmpty()) {
                colorScheme.remove(QLatin1Char('\'')); // So Foo's does not become FooS
                QRegExp fixer(QStringLiteral("[\\W,.-]+(.?)"));
                int offset;
                while ((offset = fixer.indexIn(colorScheme)) >= 0) {
                    colorScheme.replace(offset, fixer.matchedLength(), fixer.cap(1).toUpper());
                }
                colorScheme.replace(0, 1, colorScheme.at(0).toUpper());

                // NOTE: why this loop trough all the scheme files?
                // the scheme theme name is an heuristic, there is no plugin metadata whatsoever.
                // is based on the file name stripped from weird characters or the
                // eventual id- prefix store.kde.org puts, so we can just find a
                // theme that ends as the specified name
                bool schemeFound = false;
                const QStringList schemeDirs =
                    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
                for (const QString &dir : schemeDirs) {
                    const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.colors"));
                    for (const QString &file : fileNames) {
                        if (file.endsWith(colorScheme + QStringLiteral(".colors"))) {
                            setColors(colorScheme, dir + QLatin1Char('/') + file);
                            schemeFound = true;
                            break;
                        }
                    }
                    if (schemeFound) {
                        break;
                    }
                }
            }
        }

        if (m_applyIcons) {
            cg = KConfigGroup(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "Icons");
            setIcons(cg.readEntry("Theme", QString()));
        }

        if (m_applyPlasmaTheme) {
            cg = KConfigGroup(conf, "plasmarc");
            cg = KConfigGroup(&cg, "Theme");
            setPlasmaTheme(cg.readEntry("name", QString()));
        }

        if (m_applyCursors) {
            cg = KConfigGroup(conf, "kcminputrc");
            cg = KConfigGroup(&cg, "Mouse");
            setCursorTheme(cg.readEntry("cursorTheme", QString()));
        }

        if (m_applyWindowSwitcher) {
            cg = KConfigGroup(conf, "kwinrc");
            cg = KConfigGroup(&cg, "WindowSwitcher");
            setWindowSwitcher(cg.readEntry("LayoutName", QString()));
        }

        if (m_applyDesktopSwitcher) {
            cg = KConfigGroup(conf, "kwinrc");
            cg = KConfigGroup(&cg, "DesktopSwitcher");
            setDesktopSwitcher(cg.readEntry("LayoutName", QString()));
        }

        if (m_applyWindowPlacement) {
            cg = KConfigGroup(conf, "kwinrc");
            cg = KConfigGroup(&cg, "Windows");
            setWindowPlacement(cg.readEntry("Placement", QStringLiteral("Centered")));
        }

        if (m_applyShellPackage) {
            cg = KConfigGroup(conf, "plasmashellrc");
            cg = KConfigGroup(&cg, "Shell");
            setShellPackage(cg.readEntry("ShellPackage", QString()));
        }

        if (m_applyWindowDecoration) {
            cg = KConfigGroup(conf, "kwinrc");
            cg = KConfigGroup(&cg, "org.kde.kdecoration2");
#ifdef HAVE_BREEZE_DECO
            setWindowDecoration(cg.readEntry("library", QStringLiteral(BREEZE_KDECORATION_PLUGIN_ID)), cg.readEntry("theme", QStringLiteral("Breeze")));
#else
            setWindowDecoration(cg.readEntry("library", QStringLiteral("org.kde.kwin.aurorae")),
                                cg.readEntry("theme", QStringLiteral("kwin4_decoration_qml_plastik")));
#endif
        }

        setSplashScreen(m_data->settings()->lookAndFeelPackage());
        setLockScreen(m_data->settings()->lookAndFeelPackage());

        QFile packageFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kdedefaults/package"));
        packageFile.open(QIODevice::WriteOnly);
        packageFile.write(m_data->settings()->lookAndFeelPackage().toUtf8());

        if (m_mode == Mode::Defaults) {
            return;
        }

        // Reload KWin if something changed, but only once.
        if (m_applyWindowSwitcher || m_applyDesktopSwitcher || m_applyWindowDecoration || m_applyWindowPlacement) {
            QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"));
            QDBusConnection::sessionBus().send(message);
        }

        if (m_plasmashellChanged) {
            QDBusMessage message =
                QDBusMessage::createSignal(QStringLiteral("/PlasmaShell"), QStringLiteral("org.kde.PlasmaShell"), QStringLiteral("refreshCurrentShell"));
            QDBusConnection::sessionBus().send(message);
        }

        // autostart
        if (m_resetDefaultLayout) {
            QStringList toStop;
            KService::List toStart;
            // remove all the old package to autostart
            {
                KSharedConfigPtr oldConf = KSharedConfig::openConfig(previousPackage.filePath("defaults"));
                cg = KConfigGroup(oldConf, QStringLiteral("Autostart"));
                const QStringList autostartServices = cg.readEntry("Services", QStringList());

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
                cg = KConfigGroup(conf, QStringLiteral("Autostart"));
                const QStringList autostartServices = cg.readEntry("Services", QStringList());

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

void LookAndFeelManager::setMode(LookAndFeelManager::Mode mode)
{
    m_mode = mode;
}
