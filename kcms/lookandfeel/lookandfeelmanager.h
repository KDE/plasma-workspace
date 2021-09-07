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
    LookAndFeelManager(QObject *parent = nullptr);

    void save(const KPackage::Package &package, const KPackage::Package &oldPackage);

    bool resetDefaultLayout() const;
    void setResetDefaultLayout(bool reset);

    // Setters of the various theme pieces
    void setWidgetStyle(const QString &style);
    void setColors(const QString &scheme, const QString &colorFile);
    void setIcons(const QString &theme);
    void setPlasmaTheme(const QString &theme);
    void setCursorTheme(const QString theme);
    void setSplashScreen(const QString &theme);
    void setLockScreen(const QString &theme);
    void setWindowSwitcher(const QString &theme);
    void setDesktopSwitcher(const QString &theme);
    void setWindowDecoration(const QString &library, const QString &theme);
    void setWindowPlacement(const QString &value);
    void setShellPackage(const QString &name);

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
    bool m_applyColors : 1 = true;
    bool m_applyWidgetStyle : 1 = true;
    bool m_applyIcons : 1 = true;
    bool m_applyPlasmaTheme : 1 = true;
    bool m_applyCursors : 1 = true;
    bool m_applyWindowSwitcher : 1 = true;
    bool m_applyDesktopSwitcher : 1 = true;
    bool m_applyWindowPlacement : 1 = true;
    bool m_applyShellPackage : 1 = true;
    bool m_resetDefaultLayout : 1;
    bool m_applyWindowDecoration : 1 = true;

    bool m_plasmashellChanged = false;
};

#endif // LOOKANDFEELMANAGER_H
