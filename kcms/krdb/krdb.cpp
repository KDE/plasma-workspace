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
#endif

#include <filesystem>

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
    gtkrc += 2 == version ? "/gtkrc-2.0" : "/gtkrc";
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

    list.removeAll(QLatin1String(""));

    const QString gtkkde = writableGtkrc(version);
    list.removeAll(gtkkde);
    list.append(gtkkde);

    // Pass env. var to kdeinit.
    const QString value = list.join(QLatin1Char(':'));
    QProcessEnvironment newEnv;
    newEnv.insert(varName, value);
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
    KConfigGroup wmCfgGroup(kglobalcfg, "WM");

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

    KConfigGroup kdeCfgGroup(kglobalcfg, "KDE");
    settings.setValue(QStringLiteral("/qt/KDE/contrast"), kdeCfgGroup.readEntry("contrast", 7));
}

// -----------------------------------------------------------------------------

static void applyQtSettings(KSharedConfigPtr kglobalcfg, QSettings &settings)
{
    /* export font settings */

    // NOTE keep this in sync with kfontsettingsdata in plasma-integration (cf. also Bug 378262)
    QFont defaultFont(QStringLiteral("Noto Sans"), 10, -1);
    defaultFont.setStyleHint(QFont::SansSerif);

    const KConfigGroup configGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
    const QString fontInfo = configGroup.readEntry(QStringLiteral("font"), QString());
    if (!fontInfo.isEmpty()) {
        defaultFont.fromString(fontInfo);
    }

    settings.setValue(QStringLiteral("/qt/font"), defaultFont.toString());

    /* export effects settings */
    KConfigGroup kdeCfgGroup(kglobalcfg, "General");
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

static void copyFile(QFile &tmp, QString const &filename, bool)
{
    QFile f(filename);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray buf(8192, ' ');
        while (!f.atEnd()) {
            int read = f.read(buf.data(), buf.size());
            if (read > 0)
                tmp.write(buf.data(), read);
        }
    }
}

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
        if (gtkTheme.toLower() == QLatin1String("oxygen"))
            gtkStyle = QStringLiteral("oxygen-gtk");
        else
            gtkStyle = gtkTheme;

        bool exist_gtkrc = false;
        QByteArray gtkrc = getenv(gtkEnvVar(version));
        QStringList listGtkrc = QFile::decodeName(gtkrc).split(QLatin1Char(':'));
        if (listGtkrc.contains(saveFile.fileName()))
            listGtkrc.removeAll(saveFile.fileName());
        listGtkrc.append(QDir::homePath() + userGtkrc(version));
        listGtkrc.append(QDir::homePath() + "/.gtkrc-2.0-kde");
        listGtkrc.append(QDir::homePath() + "/.gtkrc-2.0-kde4");
        listGtkrc.removeAll(QLatin1String(""));
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
                gtk2ThemePath.append(QDir::homePath() + "/.local");
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

int xftDpi()
{
    KConfig cfg(QStringLiteral("kcmfonts"));
    int dpi = 0;

    if (KWindowSystem::isPlatformWayland()) {
        KConfig cfg(QStringLiteral("kwinrc"));
        KConfigGroup xwaylandGroup = cfg.group(QStringLiteral("Xwayland"));
        qreal scale = xwaylandGroup.readEntry("Scale", 1.0);
        dpi = scale * 96;
    } else {
        KConfigGroup fontsCfg(&cfg, "General");
        dpi = fontsCfg.readEntry(QStringLiteral("forceFontDPI"), 96);
    }

    return dpi;
}

void runRdb(unsigned int flags)
{
    // Obtain the application palette that is about to be set.
    bool exportQtColors = flags & KRdbExportQtColors;
    bool exportQtSettings = flags & KRdbExportQtSettings;
    bool exportXftSettings = flags & KRdbExportXftSettings;
    bool exportGtkTheme = flags & KRdbExportGtkTheme;

    KSharedConfigPtr kglobalcfg = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup kglobals(kglobalcfg, "KDE");
    QPalette newPal = KColorScheme::createApplicationPalette(kglobalcfg);

    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        qDebug() << "Couldn't open temp file";
        exit(0);
    }

    KConfigGroup generalCfgGroup(kglobalcfg, "General");

    QString gtkTheme;
    if (kglobals.hasKey("widgetStyle"))
        gtkTheme = kglobals.readEntry("widgetStyle");
    else
        gtkTheme = QStringLiteral("oxygen");

    createGtkrc(newPal, exportGtkTheme, gtkTheme, 1);
    createGtkrc(newPal, exportGtkTheme, gtkTheme, 2);

    // Merge ~/.Xresources or fallback to ~/.Xdefaults
    QString homeDir = QDir::homePath();
    QString xResources = homeDir + "/.Xresources";

    // very primitive support for ~/.Xresources by appending it
    if (QFile::exists(xResources))
        copyFile(tmpFile, xResources, true);
    else
        copyFile(tmpFile, homeDir + "/.Xdefaults", true);

    // Export the Xcursor theme & size settings
    KConfigGroup mousecfg(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), "Mouse");
    QString theme = mousecfg.readEntry("cursorTheme", QStringLiteral("breeze_cursors"));
    int cursorSize = mousecfg.readEntry("cursorSize", 24);

    if (KWindowSystem::isPlatformWayland()) {
        KConfig kwinConfig(QStringLiteral("kwinrc"));
        KConfigGroup xwaylandGroup(&kwinConfig, "Xwayland");
        cursorSize *= xwaylandGroup.readEntry("Scale", 1.0);
    }

    QString contents;
    contents += "Xcursor.theme: " + theme + '\n';
    contents += "Xcursor.size: " + QString::number(cursorSize) + '\n';

    if (exportXftSettings) {
        contents += QLatin1String("Xft.antialias: ");
        if (generalCfgGroup.readEntry("XftAntialias", true))
            contents += QLatin1String("1\n");
        else
            contents += QLatin1String("0\n");

        QString hintStyle = generalCfgGroup.readEntry("XftHintStyle", "hintslight");
        contents += QLatin1String("Xft.hinting: ");
        if (hintStyle.isEmpty())
            contents += QLatin1String("-1\n");
        else {
            if (hintStyle != QLatin1String("hintnone"))
                contents += QLatin1String("1\n");
            else
                contents += QLatin1String("0\n");
            contents += "Xft.hintstyle: " + hintStyle + '\n';
        }

        QString subPixel = generalCfgGroup.readEntry("XftSubPixel", "rgb");
        if (!subPixel.isEmpty())
            contents += "Xft.rgba: " + subPixel + '\n';

        int dpi = xftDpi();
        if (dpi > 0)
            contents += "Xft.dpi: " + QString::number(dpi) + '\n';
        else {
            KProcess queryProc;
            queryProc << QStringLiteral("xrdb") << QStringLiteral("-query");
            queryProc.setOutputChannelMode(KProcess::OnlyStdoutChannel);
            queryProc.start();
            if (queryProc.waitForFinished()) {
                QByteArray db = queryProc.readAllStandardOutput();
                int idx1 = 0;
                while (idx1 < db.size()) {
                    int idx2 = db.indexOf('\n', idx1);
                    if (idx2 == -1) {
                        idx2 = db.size() - 1;
                    }
                    const auto entry = QByteArray::fromRawData(db.constData() + idx1, idx2 - idx1 + 1);
                    if (entry.startsWith("Xft.dpi:")) {
                        db.remove(idx1, entry.size());
                    } else {
                        idx1 = idx2 + 1;
                    }
                }

                KProcess loadProc;
                loadProc << QStringLiteral("xrdb") << QStringLiteral("-quiet") << QStringLiteral("-load") << QStringLiteral("-nocpp");
                loadProc.start();
                if (loadProc.waitForStarted()) {
                    loadProc.write(db);
                    loadProc.closeWriteChannel();
                    loadProc.waitForFinished();
                }
            }
        }
    }

    if (contents.length() > 0)
        tmpFile.write(contents.toLatin1(), contents.length());

    tmpFile.flush();

    KProcess proc;
#ifndef NDEBUG
    proc << QStringLiteral("xrdb") << QStringLiteral("-merge") << tmpFile.fileName();
#else
    proc << "xrdb"
         << "-quiet"
         << "-merge" << tmpFile.fileName();
#endif
    proc.execute();

    // Needed for applications that don't set their own cursor.
    QProcess::execute(QStringLiteral("xsetroot"), {QStringLiteral("-cursor_name"), QStringLiteral("left_ptr")});

    applyGtkStyles(1);
    applyGtkStyles(2);

    /* Qt exports */
    if (exportQtColors || exportQtSettings) {
        QSettings *settings = new QSettings(QStringLiteral("Trolltech"));

        if (exportQtColors)
            applyQtColors(kglobalcfg, *settings, newPal); // For kcmcolors

        if (exportQtSettings)
            applyQtSettings(kglobalcfg, *settings); // For kcmstyle

        delete settings;
        QCoreApplication::processEvents();
#if HAVE_X11
        if (qApp->platformName() == QLatin1String("xcb")) {
            // We let KIPC take care of ourselves, as we are in a KDE app with
            // QApp::setDesktopSettingsAware(false);
            // Instead of calling QApp::x11_apply_settings() directly, we instead
            // modify the timestamp which propagates the settings changes onto
            // Qt-only apps without adversely affecting ourselves.

            // Cheat and use the current timestamp, since we just saved to qtrc.
            QDateTime settingsstamp = QDateTime::currentDateTime();

            static Atom qt_settings_timestamp = 0;
            if (!qt_settings_timestamp) {
                QString atomname(QStringLiteral("_QT_SETTINGS_TIMESTAMP_"));
                atomname += XDisplayName(nullptr); // Use the $DISPLAY envvar.
                qt_settings_timestamp = XInternAtom(QX11Info::display(), atomname.toLatin1(), False);
            }

            QBuffer stamp;
            QDataStream s(&stamp.buffer(), QIODevice::WriteOnly);
            s << settingsstamp;
            XChangeProperty(QX11Info::display(),
                            QX11Info::appRootWindow(),
                            qt_settings_timestamp,
                            qt_settings_timestamp,
                            8,
                            PropModeReplace,
                            (unsigned char *)stamp.buffer().data(),
                            stamp.buffer().size());
            qApp->processEvents();
        }
#endif
    }
}
