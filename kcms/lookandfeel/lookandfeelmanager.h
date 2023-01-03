/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KConfig>
#include <KConfigGroup>
#include <KPackage/Package>
#include <KService>
#include <QDir>
#include <QObject>

class LookAndFeelData;
class LookAndFeelSettings;

class LookAndFeelManager : public QObject
{
    Q_OBJECT
public:
    enum class Mode {
        Apply, // Apply the look and feel theme, i.e. change the active settings and set them up as new defaults. This is the default.
        Defaults // Only set up the options of the look and feel theme as new defaults without changing any active setting
    };

    enum ContentFlags {
        Empty = 0,
        // Appearance
        Colors = 1 << 0,
        WidgetStyle = 1 << 1,
        WindowDecoration = 1 << 2,
        Icons = 1 << 3,
        PlasmaTheme = 1 << 4,
        Cursors = 1 << 5,
        Fonts = 1 << 6,
        WindowSwitcher = 1 << 7,
        SplashScreen = 1 << 8,
        LockScreen = 1 << 9,
        AppearanceSettings = (1 << 10) - 1, // All the contents within Appearance
        // Layout
        DesktopLayout = 1 << 16,
        TitlebarLayout = 1 << 17,
        WindowPlacement = 1 << 18,
        ShellPackage = 1 << 19,
        DesktopSwitcher = 1 << 20,
        LayoutSettings = (1 << 21) - (1 << 16), // All the contents within Layout
        // Maybe unused or deprecated?
        RunCommand = 1 << 24,
        LogOutScript = 1 << 25,
        // General Flag combinations
        KWinSettings = WindowSwitcher | WindowDecoration | DesktopSwitcher | WindowPlacement | TitlebarLayout,
        AllSettings = (1 << 26) - 1,
    };
    Q_DECLARE_FLAGS(Contents, ContentFlags)
    Q_FLAG(Contents)

    LookAndFeelManager(QObject *parent = nullptr);

    void setMode(Mode mode);
    /**
     * Apply the theme represented by @p package, with @p previousPackage being the currently active package.
     * Effects depend upon the Mode of this object. If Mode is Defaults, @p previousPackage is ignored.
     * @p applyMask filters the package contents that will be applied.
     */
    void save(const KPackage::Package &package, const KPackage::Package &previousPackage, Contents applyMask = AllSettings);

    Contents packageContents(const KPackage::Package &package) const;

    QString colorSchemeFile(const QString &schemeName) const;

    // Setters of the various theme pieces
    void setWidgetStyle(const QString &style);
    void setColors(const QString &scheme, const QString &colorFile);
    void setIcons(const QString &theme);
    void setPlasmaTheme(const QString &theme);
    void setCursorTheme(const QString theme);
    void setSplashScreen(const QString &theme);
    void setLatteLayout(const QString &filepath, const QString &name);
    void setLockScreen(const QString &theme);
    void setWindowSwitcher(const QString &theme);
    void setDesktopSwitcher(const QString &theme);
    void setWindowDecoration(const QString &library, const QString &theme, bool noPlugin);
    void setTitlebarLayout(const QString &leftbtns, const QString &rightbtns);
    void setBorderlessMaximized(const QString &value);
    void setWindowPlacement(const QString &value);
    void setShellPackage(const QString &name);

    void setGeneralFont(const QString &font);
    void setFixedFont(const QString &font);
    void setSmallFont(const QString &font);
    void setSmallestReadableFont(const QString &font);
    void setToolbarFont(const QString &font);
    void setMenuFont(const QString &font);
    void setWindowTitleFont(const QString &font);

    LookAndFeelSettings *settings() const;

Q_SIGNALS:
    void message();
    void iconsChanged();
    void colorsChanged();
    void styleChanged(const QString &newStyle);
    void cursorsChanged(const QString &newStyle);
    void fontsChanged();
    void refreshServices(const QStringList &toStop, const KService::List &toStart);

private:
    void writeNewDefaults(const QString &filename,
                          const QString &group,
                          const QString &key,
                          const QString &value,
                          KConfig::WriteConfigFlags writeFlags = KConfig::Normal);
    void writeNewDefaults(KConfig &config,
                          KConfig &configDefault,
                          const QString &group,
                          const QString &key,
                          const QString &value,
                          KConfig::WriteConfigFlags writeFlags = KConfig::Normal);
    void
    writeNewDefaults(KConfigGroup &cg, KConfigGroup &cgd, const QString &key, const QString &value, KConfig::WriteConfigFlags writeFlags = KConfig::Normal);
    static KConfig configDefaults(const QString &filename);

    QStringList m_cursorSearchPaths;
    LookAndFeelData *const m_data;
    Mode m_mode = Mode::Apply;
    bool m_applyLatteLayout : 1;
    bool m_plasmashellChanged : 1;
    bool m_fontsChanged : 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LookAndFeelManager::Contents)
