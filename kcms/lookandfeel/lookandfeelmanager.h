/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

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
    // Flags for storing values
    enum AppearanceToApplyFlags {
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
        AppearanceSettings = (1 << 10) - 1, // Blanket switch - sets everything
    };
    Q_DECLARE_FLAGS(AppearanceToApply, AppearanceToApplyFlags)
    Q_FLAG(AppearanceToApply)
    Q_PROPERTY(AppearanceToApply appearanceToApply READ appearanceToApply WRITE setAppearanceToApply NOTIFY appearanceToApplyChanged)

    enum LayoutToApplyFlags {
        DesktopLayout = 1 << 0,
        TitlebarLayout = 1 << 1,
        WindowPlacement = 1 << 2, // FIXME: Do we still want these three?
        ShellPackage = 1 << 3,
        DesktopSwitcher = 1 << 4,
        LayoutSettings = (1 << 5) - 1,
    };
    Q_DECLARE_FLAGS(LayoutToApply, LayoutToApplyFlags)
    Q_FLAG(LayoutToApply)
    Q_PROPERTY(LayoutToApply layoutToApply READ layoutToApply WRITE setLayoutToApply NOTIFY layoutToApplyChanged)

    LookAndFeelManager(QObject *parent = nullptr);

    void setMode(Mode mode);
    /**
     * Apply the theme represented by package, with oldPackage being the currently active package.
     * Effects depend upon the Mode of this object. If Mode is Defaults, oldPackage is ignored.
     */
    void save(const KPackage::Package &package, const KPackage::Package &oldPackage);
    AppearanceToApply appearanceToApply() const;
    void setAppearanceToApply(AppearanceToApply value);
    LayoutToApply layoutToApply() const;
    void setLayoutToApply(LayoutToApply value);

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
    void appearanceToApplyChanged();
    void layoutToApplyChanged();
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
    AppearanceToApply m_appearanceToApply;
    LayoutToApply m_layoutToApply;
    bool m_plasmashellChanged : 1;
    bool m_fontsChanged : 1;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LookAndFeelManager::AppearanceToApply)
Q_DECLARE_OPERATORS_FOR_FLAGS(LookAndFeelManager::LayoutToApply)
