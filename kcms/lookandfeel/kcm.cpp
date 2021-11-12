/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"
#include "../kcms-common_p.h"
#include "config-kcm.h"
#include "config-workspace.h"
#include "krdb.h"

#include <KAboutData>
#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KService>
#include <KSharedConfig>

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
#include <QX11Info>

#include <KLocalizedString>
#include <KPackage/PackageLoader>

#include <X11/Xlib.h>

#include <KNSCore/EntryInternal>
#include <QFileInfo>
#include <updatelaunchenvjob.h>

#include "lookandfeelmanager.h"
#include "lookandfeelsettings.h"
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
    qmlRegisterType<LookAndFeelSettings>();
    qmlRegisterType<QStandardItemModel>();
    qmlRegisterType<KCMLookandFeel>();

    KAboutData *about = new KAboutData(QStringLiteral("kcm_lookandfeel"), i18n("Global Theme"), QStringLiteral("0.1"), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"));
    setAboutData(about);
    setButtons(Apply | Default);

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

    roles[HasColorsRole] = "hasColors";
    roles[HasWidgetStyleRole] = "hasWidgetStyle";
    roles[HasIconsRole] = "hasIcons";
    roles[HasPlasmaThemeRole] = "hasPlasmaTheme";
    roles[HasCursorsRole] = "hasCursors";
    roles[HasWindowSwitcherRole] = "hasWindowSwitcher";
    roles[HasDesktopSwitcherRole] = "hasDesktopSwitcher";
    m_model->setItemRoleNames(roles);
    loadModel();
    connect(m_lnf, &LookAndFeelManager::resetDefaultLayoutChanged, this, [this] {
        Q_EMIT resetDefaultLayoutChanged();
        settingsChanged();
    });

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
        const QString guessedPluginId = QFileInfo(entry.uninstalledFiles().constFirst()).fileName();
        const int index = pluginIndex(guessedPluginId);
        if (index != -1) {
            m_model->removeRows(index, 1);
        }
    };
    if (entry.status() == KNS3::Entry::Deleted && !entry.uninstalledFiles().isEmpty()) {
        removeItemFromModel();
    } else if (entry.status() == KNS3::Entry::Installed && !entry.installedFiles().isEmpty()) {
        removeItemFromModel(); // In case we updated it we don't want to have it in twice
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
    emit lookAndFeelSettings()->lookAndFeelPackageChanged();
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
        cg = KConfigGroup(&cg, "KDE");
        row->setData(!cg.readEntry("widgetStyle", QString()).isEmpty(), HasWidgetStyleRole);
        cg = KConfigGroup(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "Icons");
        row->setData(!cg.readEntry("Theme", QString()).isEmpty(), HasIconsRole);

        cg = KConfigGroup(conf, "kdeglobals");
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
    }

    m_model->appendRow(row);
}

bool KCMLookandFeel::isSaveNeeded() const
{
    return m_lnf->resetDefaultLayout() || lookAndFeelSettings()->isSaveNeeded();
}

void KCMLookandFeel::load()
{
    ManagedConfigModule::load();

    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), lookAndFeelSettings()->lookAndFeelPackage());
    m_lnf->setResetDefaultLayout(false);
}

void KCMLookandFeel::save()
{
    QString newLnfPackage = lookAndFeelSettings()->lookAndFeelPackage();
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    package.setPath(newLnfPackage);

    if (!package.isValid()) {
        return;
    }

    {
        // Some global themes use styles that may not be installed.
        // Test if style can be installed before updating the config.
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "kdeglobals");
        QScopedPointer<QStyle> newStyle(QStyleFactory::create(cg.readEntry("widgetStyle", QString())));
        m_lnf->setApplyWidgetStyle(newStyle);
    }

    ManagedConfigModule::save();
    m_lnf->save(package, m_package);
    m_package.setPath(newLnfPackage);
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors | KRdbExportQtSettings | KRdbExportXftSettings);
    setResetDefaultLayout(false);
}

void KCMLookandFeel::defaults()
{
    m_lnf->setResetDefaultLayout(false);
    ManagedConfigModule::defaults();
}

void KCMLookandFeel::setResetDefaultLayout(bool reset)
{
    m_lnf->setResetDefaultLayout(reset);
}

bool KCMLookandFeel::resetDefaultLayout() const
{
    return m_lnf->resetDefaultLayout();
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
    m_cursorSearchPaths.replaceInStrings(QRegExp(QStringLiteral("^~\\/")), QDir::home().path() + QLatin1Char('/'));
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
