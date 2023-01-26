/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"
#include "../kcms-common_p.h"
#include "config-kcm.h"
#include "config-workspace.h"
#include "krdb.h"

#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KService>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleFactory>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <KLocalizedString>
#include <KPackage/PackageLoader>

#include <array>

#include <X11/Xlib.h>

#include <KNSCore/EntryInternal>
#include <QFileInfo>
#include <updatelaunchenvjob.h>

#ifdef HAVE_XCURSOR
#include "../cursortheme/xcursor/xcursortheme.h"
#include <X11/Xcursor/Xcursor.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

KCMLookandFeel::KCMLookandFeel(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_lnf(new LookAndFeelManager(this))
{
    constexpr char uri[] = "org.kde.private.kcms.lookandfeel";
    qmlRegisterAnonymousType<LookAndFeelSettings>("", 1);
    qmlRegisterAnonymousType<QStandardItemModel>("", 1);
    qmlRegisterUncreatableType<KCMLookandFeel>(uri, 1, 0, "KCMLookandFeel", "Can't create KCMLookandFeel");
    qmlRegisterUncreatableType<LookAndFeelManager>(uri, 1, 0, "LookandFeelManager", "Can't create LookandFeelManager");

    setButtons(Default | Help);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[DescriptionRole] = "description";
    roles[ScreenshotRole] = "screenshot";
    roles[FullScreenPreviewRole] = "fullScreenPreview";
    roles[HasSplashRole] = "hasSplash";
    roles[HasLockScreenRole] = "hasLockScreen";
    roles[HasRunCommandRole] = "hasRunCommand";
    roles[HasLogoutRole] = "hasLogout";
    roles[HasGlobalThemeRole] = "hasGlobalTheme"; // For the Global Theme global checkbox
    roles[HasLayoutSettingsRole] = "hasLayoutSettings"; // For the Desktop Layout checkbox in More Options
    roles[HasDesktopLayoutRole] = "hasDesktopLayout";
    roles[HasTitlebarLayoutRole] = "hasTitlebarLayout";
    roles[HasColorsRole] = "hasColors";
    roles[HasWidgetStyleRole] = "hasWidgetStyle";
    roles[HasIconsRole] = "hasIcons";
    roles[HasPlasmaThemeRole] = "hasPlasmaTheme";
    roles[HasCursorsRole] = "hasCursors";
    roles[HasWindowSwitcherRole] = "hasWindowSwitcher";
    roles[HasDesktopSwitcherRole] = "hasDesktopSwitcher";
    roles[HasWindowDecorationRole] = "hasWindowDecoration";
    roles[HasFontsRole] = "hasFonts";

    m_model->setItemRoleNames(roles);
    loadModel();

    connect(m_lnf, &LookAndFeelManager::appearanceToApplyChanged, this, &KCMLookandFeel::appearanceToApplyChanged);
    connect(m_lnf, &LookAndFeelManager::layoutToApplyChanged, this, &KCMLookandFeel::layoutToApplyChanged);

    connect(m_lnf, &LookAndFeelManager::refreshServices, this, [](const QStringList &toStop, const QList<KService::Ptr> &toStart) {
        for (const auto &serviceName : toStop) {
            // FIXME: quite ugly way to stop things, and what about non KDE things?
            QProcess::startDetached(QStringLiteral("kquitapp5"), {QStringLiteral("--service"), serviceName});
        }
        for (const auto &service : toStart) {
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
            job->start();
        }
    });
    connect(m_lnf, &LookAndFeelManager::styleChanged, this, [] {
        // FIXME: changing style on the fly breaks QQuickWidgets
        notifyKcmChange(GlobalChangeType::StyleChanged);
    });
    connect(m_lnf, &LookAndFeelManager::colorsChanged, this, [] {
        // FIXME: changing style on the fly breaks QQuickWidgets
        notifyKcmChange(GlobalChangeType::PaletteChanged);
    });
    connect(m_lnf, &LookAndFeelManager::iconsChanged, this, [] {
        for (int i = 0; i < KIconLoader::LastGroup; i++) {
            KIconLoader::emitChange(KIconLoader::Group(i));
        }
    });
    connect(m_lnf, &LookAndFeelManager::cursorsChanged, this, &KCMLookandFeel::cursorsChanged);
    connect(m_lnf, &LookAndFeelManager::fontsChanged, this, [] {
        QDBusMessage message = QDBusMessage::createSignal("/KDEPlatformTheme", "org.kde.KDEPlatformTheme", "refreshFonts");
        QDBusConnection::sessionBus().send(message);
    });
}

KCMLookandFeel::~KCMLookandFeel()
{
}

void KCMLookandFeel::knsEntryChanged(KNSCore::EntryWrapper *wrapper)
{
    if (!wrapper) {
        return;
    }
    const KNSCore::EntryInternal entry = wrapper->entry();
    auto removeItemFromModel = [&entry, this]() {
        if (entry.uninstalledFiles().isEmpty()) {
            return;
        }
        const QString guessedPluginId = QFileInfo(entry.uninstalledFiles().constFirst()).fileName();
        const int index = pluginIndex(guessedPluginId);
        if (index != -1) {
            m_model->removeRows(index, 1);
        }
    };
    if (entry.status() == KNS3::Entry::Deleted) {
        removeItemFromModel();
    } else if (entry.status() == KNS3::Entry::Installed && !entry.installedFiles().isEmpty()) {
        if (!entry.uninstalledFiles().isEmpty()) {
            removeItemFromModel(); // In case we updated it we don't want to have it in twice
        }
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        pkg.setPath(entry.installedFiles().constFirst());
        addKPackageToModel(pkg);
    }
}

QStandardItemModel *KCMLookandFeel::lookAndFeelModel() const
{
    return m_model;
}

int KCMLookandFeel::pluginIndex(const QString &pluginName) const
{
    const auto results = m_model->match(m_model->index(0, 0), PluginNameRole, pluginName, 1, Qt::MatchExactly);
    if (results.count() == 1) {
        return results.first().row();
    }

    return -1;
}

QList<KPackage::Package> KCMLookandFeel::availablePackages(const QStringList &components)
{
    QList<KPackage::Package> packages;
    QStringList paths;
    const QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    paths.reserve(dataPaths.count());
    for (const QString &path : dataPaths) {
        QDir dir(path + QStringLiteral("/plasma/look-and-feel"));
        paths << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    }

    for (const QString &path : paths) {
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        pkg.setPath(path);
        pkg.setFallbackPackage(KPackage::Package());
        if (components.isEmpty()) {
            packages << pkg;
        } else {
            for (const auto &component : components) {
                if (!pkg.filePath(component.toUtf8()).isEmpty()) {
                    packages << pkg;
                    break;
                }
            }
        }
    }

    return packages;
}

LookAndFeelSettings *KCMLookandFeel::lookAndFeelSettings() const
{
    return m_lnf->settings();
}

void KCMLookandFeel::loadModel()
{
    m_model->clear();

    QList<KPackage::Package> pkgs = availablePackages({"defaults", "layouts"});

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(pkgs.begin(), pkgs.end(), [&collator](const KPackage::Package &a, const KPackage::Package &b) {
        return collator.compare(a.metadata().name(), b.metadata().name()) < 0;
    });

    for (const KPackage::Package &pkg : pkgs) {
        addKPackageToModel(pkg);
    }

    // Model has been cleared so pretend the selected look and fell changed to force view update
    Q_EMIT lookAndFeelSettings()->lookAndFeelPackageChanged();
}

void KCMLookandFeel::addKPackageToModel(const KPackage::Package &pkg)
{
    if (!pkg.metadata().isValid()) {
        return;
    }
    QStandardItem *row = new QStandardItem(pkg.metadata().name());
    row->setData(pkg.metadata().pluginId(), PluginNameRole);
    row->setData(pkg.metadata().description(), DescriptionRole);
    row->setData(pkg.filePath("preview"), ScreenshotRole);
    row->setData(pkg.filePath("fullscreenpreview"), FullScreenPreviewRole);

    // What the package provides
    row->setData(!pkg.filePath("defaults").isEmpty(), HasGlobalThemeRole);
    row->setData(!pkg.filePath("layoutdefaults").isEmpty(), HasLayoutSettingsRole);
    row->setData(!pkg.filePath("layouts").isEmpty(), HasDesktopLayoutRole);
    row->setData(!pkg.filePath("splashmainscript").isEmpty(), HasSplashRole);
    row->setData(!pkg.filePath("lockscreenmainscript").isEmpty(), HasLockScreenRole);
    row->setData(!pkg.filePath("runcommandmainscript").isEmpty(), HasRunCommandRole);
    row->setData(!pkg.filePath("logoutmainscript").isEmpty(), HasLogoutRole);

    if (!pkg.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("defaults"));
        KConfigGroup cg(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "General");
        bool hasColors = !cg.readEntry("ColorScheme", QString()).isEmpty();
        if (!hasColors) {
            hasColors = !pkg.filePath("colors").isEmpty();
        }
        row->setData(hasColors, HasColorsRole);

        cg = KConfigGroup(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "KDE");
        row->setData(!cg.readEntry("widgetStyle", QString()).isEmpty(), HasWidgetStyleRole);

        cg = KConfigGroup(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "Icons");
        row->setData(!cg.readEntry("Theme", QString()).isEmpty(), HasIconsRole);

        cg = KConfigGroup(conf, "plasmarc");
        cg = KConfigGroup(&cg, "Theme");
        row->setData(!cg.readEntry("name", QString()).isEmpty(), HasPlasmaThemeRole);

        cg = KConfigGroup(conf, "kcminputrc");
        cg = KConfigGroup(&cg, "Mouse");
        row->setData(!cg.readEntry("cursorTheme", QString()).isEmpty(), HasCursorsRole);

        cg = KConfigGroup(conf, "kwinrc");
        cg = KConfigGroup(&cg, "WindowSwitcher");
        row->setData(!cg.readEntry("LayoutName", QString()).isEmpty(), HasWindowSwitcherRole);

        cg = KConfigGroup(conf, "kwinrc");
        cg = KConfigGroup(&cg, "DesktopSwitcher");
        row->setData(!cg.readEntry("LayoutName", QString()).isEmpty(), HasDesktopSwitcherRole);

        cg = KConfigGroup(conf, "kwinrc");
        cg = KConfigGroup(&cg, "org.kde.kdecoration2");
        row->setData(!cg.readEntry("library", QString()).isEmpty() || !cg.readEntry("NoPlugin", QString()).isEmpty(), HasWindowDecorationRole);

        cg = KConfigGroup(conf, "kdeglobals");
        KConfigGroup cg2(&cg, "WM"); // for checking activeFont
        cg = KConfigGroup(&cg, "General");
        row->setData((!cg.readEntry("font", QString()).isEmpty() || !cg.readEntry("fixed", QString()).isEmpty()
                      || !cg.readEntry("smallestReadableFont", QString()).isEmpty() || !cg.readEntry("toolBarFont", QString()).isEmpty()
                      || !cg.readEntry("menuFont", QString()).isEmpty() || !cg2.readEntry("activeFont", QString()).isEmpty()),
                     HasFontsRole);
    } else {
        // This fallback is needed since the sheet 'breaks' without it
        row->setData(false, HasColorsRole);
        row->setData(false, HasWidgetStyleRole);
        row->setData(false, HasIconsRole);
        row->setData(false, HasPlasmaThemeRole);
        row->setData(false, HasCursorsRole);
        row->setData(false, HasWindowSwitcherRole);
        row->setData(false, HasDesktopSwitcherRole);
        row->setData(false, HasWindowDecorationRole);
        row->setData(false, HasFontsRole);
    }
    if (!pkg.filePath("layoutdefaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("layoutdefaults"));
        KConfigGroup cg(conf, "kwinrc");
        cg = KConfigGroup(&cg, "org.kde.kdecoration2");
        row->setData((!cg.readEntry("ButtonsOnLeft", QString()).isEmpty() || !cg.readEntry("ButtonsOnRight", QString()).isEmpty()), HasTitlebarLayoutRole);
    } else {
        // This fallback is needed since the sheet 'breaks' without it
        row->setData(false, HasTitlebarLayoutRole);
    }

    m_model->appendRow(row);
}

bool KCMLookandFeel::isSaveNeeded() const
{
    return lookAndFeelSettings()->isSaveNeeded();
}

void KCMLookandFeel::load()
{
    ManagedConfigModule::load();

    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), lookAndFeelSettings()->lookAndFeelPackage());
}

void KCMLookandFeel::save()
{
    QString newLnfPackage = lookAndFeelSettings()->lookAndFeelPackage();
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    package.setPath(newLnfPackage);

    if (!package.isValid()) {
        return;
    }

    // Disable unavailable flags to prevent unintentional applies
    const int index = pluginIndex(lookAndFeelSettings()->lookAndFeelPackage());
    auto layoutApplyFlags = m_lnf->layoutToApply();
    // Layout Options:
    constexpr std::array layoutPairs{
        std::make_pair(LookAndFeelManager::DesktopLayout, HasDesktopLayoutRole),
        std::make_pair(LookAndFeelManager::TitlebarLayout, HasTitlebarLayoutRole),
        std::make_pair(LookAndFeelManager::WindowPlacement, HasDesktopLayoutRole),
        std::make_pair(LookAndFeelManager::ShellPackage, HasDesktopLayoutRole),
        std::make_pair(LookAndFeelManager::DesktopSwitcher, HasDesktopLayoutRole),
    };
    for (const auto &pair : layoutPairs) {
        if (m_lnf->layoutToApply().testFlag(pair.first)) {
            layoutApplyFlags.setFlag(pair.first, m_model->data(m_model->index(index, 0), pair.second).toBool());
        }
    }
    m_lnf->setLayoutToApply(layoutApplyFlags);
    // Appearance Options:
    auto appearanceApplyFlags = m_lnf->appearanceToApply();
    constexpr std::array appearancePairs{
        std::make_pair(LookAndFeelManager::Colors, HasColorsRole),
        std::make_pair(LookAndFeelManager::WindowDecoration, HasWindowDecorationRole),
        std::make_pair(LookAndFeelManager::Icons, HasIconsRole),
        std::make_pair(LookAndFeelManager::PlasmaTheme, HasPlasmaThemeRole),
        std::make_pair(LookAndFeelManager::Cursors, HasCursorsRole),
        std::make_pair(LookAndFeelManager::Fonts, HasFontsRole),
        std::make_pair(LookAndFeelManager::WindowSwitcher, HasWindowSwitcherRole),
        std::make_pair(LookAndFeelManager::SplashScreen, HasSplashRole),
        std::make_pair(LookAndFeelManager::LockScreen, HasLockScreenRole),
    };
    for (const auto &pair : appearancePairs) {
        if (m_lnf->appearanceToApply().testFlag(pair.first)) {
            appearanceApplyFlags.setFlag(pair.first, m_model->data(m_model->index(index, 0), pair.second).toBool());
        }
    }
    if (m_lnf->appearanceToApply().testFlag(LookAndFeelManager::WidgetStyle)) {
        // Some global themes use styles that may not be installed.
        // Test if style can be installed before updating the config.
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "kdeglobals");
        std::unique_ptr<QStyle> newStyle(QStyleFactory::create(cg.readEntry("widgetStyle", QString())));
        appearanceApplyFlags.setFlag(LookAndFeelManager::WidgetStyle,
                                     (newStyle != nullptr && m_model->data(m_model->index(index, 0), HasWidgetStyleRole).toBool())); // Widget Style isn't in
        // the loop above since it has all of this extra checking too for it
    }
    m_lnf->setAppearanceToApply(appearanceApplyFlags);

    ManagedConfigModule::save();
    m_lnf->save(package, m_package);
    m_package.setPath(newLnfPackage);
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors | KRdbExportQtSettings | KRdbExportXftSettings);
}

void KCMLookandFeel::defaults()
{
    ManagedConfigModule::defaults();
    Q_EMIT showConfirmation();
}

LookAndFeelManager::AppearanceToApply KCMLookandFeel::appearanceToApply() const
{
    return m_lnf->appearanceToApply();
}

void KCMLookandFeel::setAppearanceToApply(LookAndFeelManager::AppearanceToApply items)
{
    m_lnf->setAppearanceToApply(items);
}

void KCMLookandFeel::resetAppearanceToApply()
{
    const int index = pluginIndex(lookAndFeelSettings()->lookAndFeelPackage());
    auto applyFlags = appearanceToApply();

    applyFlags.setFlag(LookAndFeelManager::AppearanceSettings, m_model->data(m_model->index(index, 0), HasGlobalThemeRole).toBool());

    m_lnf->setAppearanceToApply(applyFlags); // emits over in lookandfeelmananager
}

LookAndFeelManager::LayoutToApply KCMLookandFeel::layoutToApply() const
{
    return m_lnf->layoutToApply();
}

void KCMLookandFeel::setLayoutToApply(LookAndFeelManager::LayoutToApply items)
{
    m_lnf->setLayoutToApply(items);
}

void KCMLookandFeel::resetLayoutToApply()
{
    const int index = pluginIndex(lookAndFeelSettings()->lookAndFeelPackage());
    auto applyFlags = layoutToApply();

    if (m_model->data(m_model->index(index, 0), HasGlobalThemeRole).toBool()) {
        m_lnf->setLayoutToApply({}); // Don't enable by default if Global Theme is available
        return;
    }

    applyFlags.setFlag(LookAndFeelManager::LayoutSettings, m_model->data(m_model->index(index, 0), HasLayoutSettingsRole).toBool());

    m_lnf->setLayoutToApply(applyFlags); // emits over in lookandfeelmananager
}

QDir KCMLookandFeel::cursorThemeDir(const QString &theme, const int depth)
{
    // Prevent infinite recursion
    if (depth > 10) {
        return QDir();
    }

    // Search each icon theme directory for 'theme'
    foreach (const QString &baseDir, cursorSearchPaths()) {
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
        KConfigGroup cg(&config, "Icon Theme");

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

QStringList KCMLookandFeel::cursorSearchPaths()
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
    QString path = XcursorLibraryPath();
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
    m_cursorSearchPaths.replaceInStrings(QRegularExpression(QStringLiteral("^~\\/")), QDir::home().path() + QLatin1Char('/'));
#endif
    return m_cursorSearchPaths;
}

void KCMLookandFeel::cursorsChanged(const QString &themeName)
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

    UpdateLaunchEnvJob launchEnvJob(QStringLiteral("XCURSOR_THEME"), themeName);

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

    foreach (const QString &name, names) {
        XFixesChangeCursorByName(QX11Info::display(), theme.loadCursor(name, cursorSize), QFile::encodeName(name));
    }

#else
    KMessageBox::information(this,
                             i18n("You have to restart the Plasma session for these changes to take effect."),
                             i18n("Cursor Settings Changed"),
                             "CursorSettingsChanged");
#endif
#endif
}
