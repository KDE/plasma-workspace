/*
   KRDB - puts current KDE color scheme into preprocessor statements
   cats specially written application default files and uses xrdb -merge to
   write to RESOURCE_MANAGER. Thus it gives a  simple way to make non-KDE
   applications fit in with the desktop

   SPDX-FileCopyrightText: 1998 Mark Donohoe
   SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>
   SPDX-FileCopyrightText: 2002 Karol Szwed <gallium@kde.org>
   SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

   reworked for KDE 2.0:
   SPDX-FileCopyrightText: 1999 Dirk A. Mueller

   add support for GTK applications:
   SPDX-FileCopyrightText: 2001 Matthias Ettrich

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-X11.h>
#include <config-workspace.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#undef Unsorted
#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QFontDatabase>
#include <QSettings>

#include <QByteArray>
#include <QDBusConnection>
#include <QDateTime>
#include <QDebug>
#include <QPixmap>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextStream>

#include <KColorScheme>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KProcess>
#include <KWindowSystem>

#include <KUpdateLaunchEnvironmentJob>

#include "krdb.h"
#if HAVE_X11
#include <X11/Xlib.h>
#include <private/qtx11extras_p.h>
#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#endif

#include <filesystem>

using namespace Qt::StringLiterals;

inline const char *gtkEnvVar(int version)
{
    return 2 == version ? "GTK2_RC_FILES" : "GTK_RC_FILES";
}

inline QLatin1String sysGtkrc(int version)
{
    std::error_code error;
    if (version == 2) {
        if (std::filesystem::exists("/etc/opt/gnome/gtk-2.0", error) && !error) {
            return QLatin1String("/etc/opt/gnome/gtk-2.0/gtkrc");
        }
        return QLatin1String("/etc/gtk-2.0/gtkrc");
    }
    if (std::filesystem::exists("/etc/opt/gnome/gtk", error) && !error) {
        return QLatin1String("/etc/opt/gnome/gtk/gtkrc");
    }
    return QLatin1String("/etc/gtk/gtkrc");
}

inline QLatin1String userGtkrc(int version)
{
    return version == 2 ? QLatin1String("/.gtkrc-2.0") : QLatin1String("/.gtkrc");
}

// -----------------------------------------------------------------------------
static QString writableGtkrc(int version)
{
    QString gtkrc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir dir;
    dir.mkpath(gtkrc);
    gtkrc += 2 == version ? u"/gtkrc-2.0"_s : u"/gtkrc"_s;
    return gtkrc;
}

// -----------------------------------------------------------------------------
static void applyGtkStyles(int version)
{
    const char *varName = gtkEnvVar(version);
    const char *envVar = getenv(varName);
    if (envVar) { // Already set by user, don't override it
        return;
    }

    const QByteArray gtkrc(envVar);
    const QString userHomeGtkrc = QDir::homePath() + userGtkrc(version);
    QStringList list = QFile::decodeName(gtkrc).split(QLatin1Char(':'));
    if (!list.contains(userHomeGtkrc)) {
        list.prepend(userHomeGtkrc);
    }

    const QLatin1String systemGtkrc = sysGtkrc(version);
    if (!list.contains(systemGtkrc)) {
        list.prepend(systemGtkrc);
    }

    list.removeAll(QString());

    const QString gtkkde = writableGtkrc(version);
    list.removeAll(gtkkde);
    list.append(gtkkde);

    // Pass env. var to kdeinit.
    const QString value = list.join(QLatin1Char(':'));
    QProcessEnvironment newEnv;
    newEnv.insert(QString::fromLatin1(varName), value);
    new KUpdateLaunchEnvironmentJob(newEnv);
}

// -----------------------------------------------------------------------------

static void applyQtColors(KSharedConfigPtr kglobalcfg, QSettings &settings, QPalette &newPal)
{
    QStringList actcg, inactcg, discg;
    /* export kde color settings */
    int i;
    for (i = 0; i < QPalette::NColorRoles; i++)
        actcg << newPal.color(QPalette::Active, (QPalette::ColorRole)i).name();
    for (i = 0; i < QPalette::NColorRoles; i++)
        inactcg << newPal.color(QPalette::Inactive, (QPalette::ColorRole)i).name();
    for (i = 0; i < QPalette::NColorRoles; i++)
        discg << newPal.color(QPalette::Disabled, (QPalette::ColorRole)i).name();

    settings.setValue(QStringLiteral("/qt/Palette/active"), actcg);
    settings.setValue(QStringLiteral("/qt/Palette/inactive"), inactcg);
    settings.setValue(QStringLiteral("/qt/Palette/disabled"), discg);

    // export kwin's colors to qtrc for kstyle to use
    KConfigGroup wmCfgGroup(kglobalcfg, u"WM"_s);

    // active colors
    QColor clr = newPal.color(QPalette::Active, QPalette::Window);
    clr = wmCfgGroup.readEntry("activeBackground", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/activeBackground"), clr.name());
    if (QPixmap::defaultDepth() > 8)
        clr = clr.darker(110);
    clr = wmCfgGroup.readEntry("activeBlend", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/activeBlend"), clr.name());
    clr = newPal.color(QPalette::Active, QPalette::HighlightedText);
    clr = wmCfgGroup.readEntry("activeForeground", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/activeForeground"), clr.name());
    clr = newPal.color(QPalette::Active, QPalette::Window);
    clr = wmCfgGroup.readEntry("frame", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/frame"), clr.name());
    clr = wmCfgGroup.readEntry("activeTitleBtnBg", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/activeTitleBtnBg"), clr.name());

    // inactive colors
    clr = newPal.color(QPalette::Inactive, QPalette::Window);
    clr = wmCfgGroup.readEntry("inactiveBackground", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/inactiveBackground"), clr.name());
    if (QPixmap::defaultDepth() > 8)
        clr = clr.darker(110);
    clr = wmCfgGroup.readEntry("inactiveBlend", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/inactiveBlend"), clr.name());
    clr = newPal.color(QPalette::Inactive, QPalette::Window).darker();
    clr = wmCfgGroup.readEntry("inactiveForeground", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/inactiveForeground"), clr.name());
    clr = newPal.color(QPalette::Inactive, QPalette::Window);
    clr = wmCfgGroup.readEntry("inactiveFrame", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/inactiveFrame"), clr.name());
    clr = wmCfgGroup.readEntry("inactiveTitleBtnBg", clr);
    settings.setValue(QStringLiteral("/qt/KWinPalette/inactiveTitleBtnBg"), clr.name());

    KConfigGroup kdeCfgGroup(kglobalcfg, u"KDE"_s);
    settings.setValue(QStringLiteral("/qt/KDE/contrast"), kdeCfgGroup.readEntry("contrast", 7));
}

// -----------------------------------------------------------------------------

static void applyQtSettings(KSharedConfigPtr kglobalcfg, QSettings &settings)
{
    /* export font settings */

    // NOTE keep this in sync with kfontsettingsdata in plasma-integration (cf. also Bug 378262)
    QFont defaultFont(QStringLiteral("Noto Sans"), 10, -1);
    defaultFont.setStyleHint(QFont::SansSerif);

    const KConfigGroup configGroup(KSharedConfig::openConfig(), u"General"_s);
    const QString fontInfo = configGroup.readEntry(QStringLiteral("font"), QString());
    if (!fontInfo.isEmpty()) {
        defaultFont.fromString(fontInfo);
    }

    settings.setValue(QStringLiteral("/qt/font"), defaultFont.toString());

    /* export effects settings */
    KConfigGroup kdeCfgGroup(kglobalcfg, u"General"_s);
    bool effectsEnabled = kdeCfgGroup.readEntry("EffectsEnabled", false);
    bool fadeMenus = kdeCfgGroup.readEntry("EffectFadeMenu", false);
    bool fadeTooltips = kdeCfgGroup.readEntry("EffectFadeTooltip", false);
    bool animateCombobox = kdeCfgGroup.readEntry("EffectAnimateCombo", false);

    QStringList guieffects;
    if (effectsEnabled) {
        guieffects << QStringLiteral("general");
        if (fadeMenus)
            guieffects << QStringLiteral("fademenu");
        if (animateCombobox)
            guieffects << QStringLiteral("animatecombo");
        if (fadeTooltips)
            guieffects << QStringLiteral("fadetooltip");
    } else
        guieffects << QStringLiteral("none");

    settings.setValue(QStringLiteral("/qt/GUIEffects"), guieffects);
}

// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------

static void createGtkrc(const QPalette &cg, bool exportGtkTheme, const QString &gtkTheme, int version)
{
    Q_UNUSED(cg);
    // lukas: why does it create in ~/.kde/share/config ???
    // pfeiffer: so that we don't overwrite the user's gtkrc.
    // it is found via the GTK_RC_FILES environment variable.
    QSaveFile saveFile(writableGtkrc(version));
    if (!saveFile.open(QIODevice::WriteOnly))
        return;

    QTextStream t(&saveFile);
    t << i18n(
        "# created by KDE Plasma, %1\n"
        "#\n",
        QDateTime::currentDateTime().toString());

    if (2 == version) { // we should maybe check for MacOS settings here
        using Qt::endl;
        t << endl;
        t << "gtk-alternative-button-order = 1" << endl;
        t << endl;
    }

    if (exportGtkTheme) {
        QString gtkStyle;
        if (QString::compare(gtkTheme, QLatin1String("oxygen"), Qt::CaseInsensitive) == 0)
            gtkStyle = QStringLiteral("oxygen-gtk");
        else
            gtkStyle = gtkTheme;

        bool exist_gtkrc = false;
        QByteArray gtkrc = getenv(gtkEnvVar(version));
        QStringList listGtkrc = QFile::decodeName(gtkrc).split(QLatin1Char(':'));
        if (listGtkrc.contains(saveFile.fileName()))
            listGtkrc.removeAll(saveFile.fileName());
        listGtkrc.append(QDir::homePath() + userGtkrc(version));
        listGtkrc.append(QDir::homePath() + "/.gtkrc-2.0-kde"_L1);
        listGtkrc.append(QDir::homePath() + "/.gtkrc-2.0-kde4"_L1);
        listGtkrc.removeAll(QString());
        listGtkrc.removeDuplicates();
        for (int i = 0; i < listGtkrc.size(); ++i) {
            if ((exist_gtkrc = QFile::exists(listGtkrc.at(i))))
                break;
        }

        if (!exist_gtkrc) {
            QString gtk2ThemeFilename;
            gtk2ThemeFilename = QStringLiteral("%1/.themes/%2/gtk-2.0/gtkrc").arg(QDir::homePath(), gtkStyle);
            if (!QFile::exists(gtk2ThemeFilename)) {
                QStringList gtk2ThemePath;
                gtk2ThemeFilename.clear();
                QByteArray xdgDataDirs = getenv("XDG_DATA_DIRS");
                gtk2ThemePath.append(QDir::homePath() + "/.local"_L1);
                gtk2ThemePath.append(QFile::decodeName(xdgDataDirs).split(QLatin1Char(':')));
                gtk2ThemePath.removeDuplicates();
                for (int i = 0; i < gtk2ThemePath.size(); ++i) {
                    gtk2ThemeFilename = QStringLiteral("%1/themes/%2/gtk-2.0/gtkrc").arg(gtk2ThemePath.at(i), gtkStyle);
                    if (QFile::exists(gtk2ThemeFilename))
                        break;
                    else
                        gtk2ThemeFilename.clear();
                }
            }

            if (!gtk2ThemeFilename.isEmpty()) {
                t << "include \"" << gtk2ThemeFilename << "\"" << Qt::endl;
                t << Qt::endl;
                t << "gtk-theme-name=\"" << gtkStyle << "\"" << Qt::endl;
                t << Qt::endl;
            }
        }
    }

    saveFile.commit();
}

// -----------------------------------------------------------------------------

void runRdb(unsigned int flags)
{
    // Obtain the application palette that is about to be set.
    bool exportQtColors = flags & KRdbExportQtColors;
    bool exportQtSettings = flags & KRdbExportQtSettings;
    bool exportGtkTheme = flags & KRdbExportGtkTheme;

    KSharedConfigPtr kglobalcfg = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup kglobals(kglobalcfg, u"KDE"_s);
    QPalette newPal = KColorScheme::createApplicationPalette(kglobalcfg);

    KConfigGroup generalCfgGroup(kglobalcfg, u"General"_s);

    QString gtkTheme;
    if (kglobals.hasKey("widgetStyle"))
        gtkTheme = kglobals.readEntry("widgetStyle");
    else
        gtkTheme = QStringLiteral("oxygen");

    createGtkrc(newPal, exportGtkTheme, gtkTheme, 1);
    createGtkrc(newPal, exportGtkTheme, gtkTheme, 2);

    applyGtkStyles(1);
    applyGtkStyles(2);

    /* Qt exports */
    if (exportQtColors || exportQtSettings) {
        auto *settings = new QSettings(QStringLiteral("Trolltech"));

        if (exportQtColors)
            applyQtColors(kglobalcfg, *settings, newPal); // For kcmcolors

        if (exportQtSettings)
            applyQtSettings(kglobalcfg, *settings); // For kcmstyle

        delete settings;
    }
}
