/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef LOOKANDFEELMANAGER_H
#define LOOKANDFEELMANAGER_H

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

    LookAndFeelManager(QObject *parent = nullptr);

    void setMode(Mode mode);
    /**
     * Apply the theme represented by package, with oldPackage being the currently active package.
     * Effects depend upon the Mode of this object. If Mode is Defaults, oldPackage is ignored.
     */
    void save(const KPackage::Package &package, const KPackage::Package &oldPackage);

    QString colorSchemeFile(const QString &schemeName) const;

    bool resetDefaultLayout() const;
    void setResetDefaultLayout(bool reset);

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
    void setWindowDecoration(const QString &library, const QString &theme);
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

    void setApplyWidgetStyle(bool apply)
    {
        m_applyWidgetStyle = apply;
    }

Q_SIGNALS:
    void message();
    void resetDefaultLayoutChanged();
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
    bool m_applyColors : 1;
    bool m_applyWidgetStyle : 1;
    bool m_applyIcons : 1;
    bool m_applyPlasmaTheme : 1;
    bool m_applyCursors : 1;
    bool m_applyWindowSwitcher : 1;
    bool m_applyDesktopSwitcher : 1;
    bool m_applyWindowPlacement : 1;
    bool m_applyShellPackage : 1;
    bool m_resetDefaultLayout : 1;
    bool m_applyLatteLayout : 1;
    bool m_applyWindowDecoration : 1;

    bool m_plasmashellChanged : 1;
    bool m_fontsChanged : 1;
};

#endif // LOOKANDFEELMANAGER_H
