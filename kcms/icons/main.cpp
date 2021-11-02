/*
    main.cpp

    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2000 Antonio Larrosa <larrosa@kde.org>
    SPDX-FileCopyrightText: 2000 Geert Jansen <jansen@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    KDE Frameworks 5 port:
    SPDX-FileCopyrightText: 2013 Jonathan Riddell <jr@jriddell.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "main.h"
#include "../kcms-common_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmapCache>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStringList>
#include <QSvgRenderer>

#include <KAboutData>
#include <KConfigGroup>
#include <KIconLoader>
#include <KIconTheme>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTar>
#include <Kdelibs4Migration>

#include <KIO/DeleteJob>
#include <KIO/FileCopyJob>

#include <algorithm>
#include <unistd.h> // for unlink

#include "iconsdata.h"
#include "iconsizecategorymodel.h"
#include "iconsmodel.h"
#include "iconssettings.h"

#include "config.h" // for CMAKE_INSTALL_FULL_LIBEXECDIR

K_PLUGIN_FACTORY_WITH_JSON(IconsFactory, "kcm_icons.json", registerPlugin<IconModule>(); registerPlugin<IconsData>();)

IconModule::IconModule(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_data(new IconsData(this))
    , m_model(new IconsModel(m_data->settings(), this))
    , m_iconSizeCategoryModel(new IconSizeCategoryModel(this))
{
    qmlRegisterType<IconsSettings>();
    qmlRegisterType<IconsModel>();
    qmlRegisterType<IconSizeCategoryModel>();

    // to be able to access its enums
    qmlRegisterUncreatableType<KIconLoader>("org.kde.private.kcms.icons", 1, 0, "KIconLoader", QString());

    KAboutData *about = new KAboutData(QStringLiteral("kcm5_icons"),
                                       i18n("Icons"),
                                       QStringLiteral("1.0"),
                                       i18n("Icons Control Panel Module"),
                                       KAboutLicense::GPL,
                                       i18n("(c) 2000-2003 Geert Jansen"));
    about->addAuthor(i18n("Geert Jansen"), QString(), QStringLiteral("jansen@kde.org"));
    about->addAuthor(i18n("Antonio Larrosa Jimenez"), QString(), QStringLiteral("larrosa@kde.org"));
    about->addCredit(i18n("Torsten Rahn"), QString(), QStringLiteral("torsten@kde.org"));
    about->addAuthor(i18n("Jonathan Riddell"), QString(), QStringLiteral("jr@jriddell.org"));
    about->addAuthor(i18n("Kai Uwe Broulik"), QString(), QStringLiteral("kde@privat.broulik.de>"));
    setAboutData(about);

    setButtons(Apply | Default);

    connect(m_model, &IconsModel::pendingDeletionsChanged, this, &IconModule::settingsChanged);

    // When user has a lot of themes installed, preview pixmaps might get evicted prematurely
    QPixmapCache::setCacheLimit(50 * 1024); // 50 MiB
}

IconModule::~IconModule()
{
}

IconsSettings *IconModule::iconsSettings() const
{
    return m_data->settings();
}

IconsModel *IconModule::iconsModel() const
{
    return m_model;
}

IconSizeCategoryModel *IconModule::iconSizeCategoryModel() const
{
    return m_iconSizeCategoryModel;
}

bool IconModule::downloadingFile() const
{
    return m_tempCopyJob;
}

QList<int> IconModule::availableIconSizes(int group) const
{
    const auto themeName = iconsSettings()->theme();
    if (!m_kiconThemeCache.contains(iconsSettings()->theme())) {
        m_kiconThemeCache.insert(themeName, new KIconTheme(themeName));
    }
    return m_kiconThemeCache[themeName]->querySizes(static_cast<KIconLoader::Group>(group));
}

void IconModule::load()
{
    ManagedConfigModule::load();
    m_model->load();
    // Model has been cleared so pretend the theme name changed to force view update
    emit iconsSettings()->ThemeChanged();
}

void IconModule::save()
{
    bool needToExportToKDE4 = iconsSettings()->isSaveNeeded();

    // keep track of Group of icons size that has changed
    QList<int> notifyList;
    for (int i = 0; i < m_iconSizeCategoryModel->rowCount(); ++i) {
        const QModelIndex index = m_iconSizeCategoryModel->index(i, 0);
        const QString key = index.data(IconSizeCategoryModel::ConfigKeyRole).toString();
        if (iconsSettings()->findItem(key)->isSaveNeeded()) {
            notifyList << index.data(IconSizeCategoryModel::KIconLoaderGroupRole).toInt();
        }
    }

    ManagedConfigModule::save();

    if (needToExportToKDE4) {
        // Is this still needed?
        exportToKDE4();
    }

    processPendingDeletions();

    // Notify the group(s) where icon sizes have changed
    for (auto group : qAsConst(notifyList)) {
        KIconLoader::emitChange(KIconLoader::Group(group));
    }
}

bool IconModule::isSaveNeeded() const
{
    return !m_model->pendingDeletions().isEmpty();
}

void IconModule::processPendingDeletions()
{
    const QStringList pendingDeletions = m_model->pendingDeletions();

    for (const QString &themeName : pendingDeletions) {
        Q_ASSERT(themeName != iconsSettings()->theme());

        KIconTheme theme(themeName);
        auto *job = KIO::del(QUrl::fromLocalFile(theme.dir()), KIO::HideProgressInfo);
        // needs to block for it to work on "OK" where the dialog (kcmshell) closes
        job->exec();
    }

    m_model->removeItemsPendingDeletion();
}

void IconModule::ghnsEntriesChanged()
{
    // reload the display icontheme items
    KIconTheme::reconfigure();
    KIconLoader::global()->newIconLoader();
    load();
    QPixmapCache::clear();
}

void IconModule::installThemeFromFile(const QUrl &url)
{
    if (url.isLocalFile()) {
        installThemeFile(url.toLocalFile());
        return;
    }

    if (m_tempCopyJob) {
        return;
    }

    m_tempInstallFile.reset(new QTemporaryFile());
    if (!m_tempInstallFile->open()) {
        emit showErrorMessage(i18n("Unable to create a temporary file."));
        m_tempInstallFile.reset();
        return;
    }

    m_tempCopyJob = KIO::file_copy(url, QUrl::fromLocalFile(m_tempInstallFile->fileName()), -1, KIO::Overwrite);
    m_tempCopyJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    emit downloadingFileChanged();

    connect(m_tempCopyJob, &KIO::FileCopyJob::result, this, [this, url](KJob *job) {
        if (job->error() != KJob::NoError) {
            emit showErrorMessage(i18n("Unable to download the icon theme archive: %1", job->errorText()));
            return;
        }

        installThemeFile(m_tempInstallFile->fileName());
        m_tempInstallFile.reset();
    });
    connect(m_tempCopyJob, &QObject::destroyed, this, &IconModule::downloadingFileChanged);
}

void IconModule::installThemeFile(const QString &path)
{
    const QStringList themesNames = findThemeDirs(path);
    if (themesNames.isEmpty()) {
        emit showErrorMessage(i18n("The file is not a valid icon theme archive."));
        return;
    }

    if (!installThemes(themesNames, path)) {
        emit showErrorMessage(i18n("A problem occurred during the installation process; however, most of the themes in the archive have been installed"));
        return;
    }

    emit showSuccessMessage(i18n("Theme installed successfully."));

    KIconLoader::global()->newIconLoader();
    m_model->load();
}

void IconModule::exportToKDE4()
{
    // TODO: killing the kde4 icon cache: possible? (kde4migration doesn't let access the cache folder)
    Kdelibs4Migration migration;
    QString configFilePath = migration.saveLocation("config");
    if (configFilePath.isEmpty()) {
        return;
    }

    configFilePath += QLatin1String("kdeglobals");

    KSharedConfigPtr kglobalcfg = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfig kde4config(configFilePath, KConfig::SimpleConfig);

    KConfigGroup kde4IconGroup(&kde4config, "Icons");
    kde4IconGroup.writeEntry("Theme", iconsSettings()->theme());

    // Synchronize icon effects
    for (int row = 0; row < m_iconSizeCategoryModel->rowCount(); row++) {
        QModelIndex idx(m_iconSizeCategoryModel->index(row, 0));
        QString group = m_iconSizeCategoryModel->data(idx, IconSizeCategoryModel::ConfigSectionRole).toString();
        const QString groupName = group + QLatin1String("Icons");
        KConfigGroup cg(kglobalcfg, groupName);
        KConfigGroup kde4Cg(&kde4config, groupName);

        // HACK copyTo only copies keys, it doesn't replace the entire group
        // which means if we removed the effects in our config it won't remove
        // them from the kde4 config, hence revert all of them prior to copying
        const QStringList keys = cg.keyList() + kde4Cg.keyList();
        for (const QString &key : keys) {
            kde4Cg.revertToDefault(key);
        }
        // now copy over the new values
        cg.copyTo(&kde4Cg);
    }

    kde4config.sync();

    QProcess *cachePathProcess = new QProcess(this);
    connect(cachePathProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [cachePathProcess](int exitCode, QProcess::ExitStatus status) {
                if (status == QProcess::NormalExit && exitCode == 0) {
                    QString path = cachePathProcess->readAllStandardOutput().trimmed();
                    path.append(QLatin1String("icon-cache.kcache"));
                    QFile::remove(path);
                }

                // message kde4 apps that icon theme has changed
                for (int i = 0; i < KIconLoader::LastGroup; ++i) {
                    notifyKcmChange(GlobalChangeType::IconChanged, KIconLoader::Group(i));
                }

                cachePathProcess->deleteLater();
            });
    cachePathProcess->start(QStringLiteral("kde4-config"), {QStringLiteral("--path"), QStringLiteral("cache")});
}

QStringList IconModule::findThemeDirs(const QString &archiveName)
{
    QStringList foundThemes;

    KTar archive(archiveName);
    archive.open(QIODevice::ReadOnly);
    const KArchiveDirectory *themeDir = archive.directory();

    KArchiveEntry *possibleDir = nullptr;
    KArchiveDirectory *subDir = nullptr;

    // iterate all the dirs looking for an index.theme or index.desktop file
    const QStringList entries = themeDir->entries();
    for (const QString &entry : entries) {
        possibleDir = const_cast<KArchiveEntry *>(themeDir->entry(entry));
        if (!possibleDir->isDirectory()) {
            continue;
        }

        subDir = dynamic_cast<KArchiveDirectory *>(possibleDir);
        if (!subDir) {
            continue;
        }

        if (subDir->entry(QStringLiteral("index.theme")) || subDir->entry(QStringLiteral("index.desktop"))) {
            foundThemes.append(subDir->name());
        }
    }

    archive.close();
    return foundThemes;
}

bool IconModule::installThemes(const QStringList &themes, const QString &archiveName)
{
    bool everythingOk = true;
    const QString localThemesDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/icons/./"));

    emit showProgress(i18n("Installing icon themes…"));

    KTar archive(archiveName);
    archive.open(QIODevice::ReadOnly);
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    const KArchiveDirectory *rootDir = archive.directory();

    KArchiveDirectory *currentTheme = nullptr;
    for (const QString &theme : themes) {
        emit showProgress(i18n("Installing %1 theme…", theme));

        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

        currentTheme = dynamic_cast<KArchiveDirectory *>(const_cast<KArchiveEntry *>(rootDir->entry(theme)));
        if (!currentTheme) {
            // we tell back that something went wrong, but try to install as much
            // as possible
            everythingOk = false;
            continue;
        }

        currentTheme->copyTo(localThemesDir + theme);
    }

    archive.close();

    emit hideProgress();
    return everythingOk;
}

QVariantList IconModule::previewIcons(const QString &themeName, int size, qreal dpr, int limit)
{
    static QVector<QStringList> s_previewIcons{
        {QStringLiteral("system-run"), QStringLiteral("exec")},
        {QStringLiteral("folder")},
        {QStringLiteral("document"), QStringLiteral("text-x-generic")},
        {QStringLiteral("user-trash"), QStringLiteral("user-trash-empty")},
        {QStringLiteral("help-browser"), QStringLiteral("system-help"), QStringLiteral("help-about"), QStringLiteral("help-contents")},
        {QStringLiteral("preferences-system"), QStringLiteral("systemsettings"), QStringLiteral("configure")},

        {QStringLiteral("text-html")},
        {QStringLiteral("image-x-generic"), QStringLiteral("image-png"), QStringLiteral("image-jpeg")},
        {QStringLiteral("video-x-generic"), QStringLiteral("video-x-theora+ogg"), QStringLiteral("video-mp4")},
        {QStringLiteral("x-office-document")},
        {QStringLiteral("x-office-spreadsheet")},
        {QStringLiteral("x-office-presentation"), QStringLiteral("application-presentation")},

        {QStringLiteral("user-home")},
        {QStringLiteral("user-desktop"), QStringLiteral("desktop")},
        {QStringLiteral("folder-image"), QStringLiteral("folder-images"), QStringLiteral("folder-pictures"), QStringLiteral("folder-picture")},
        {QStringLiteral("folder-documents")},
        {QStringLiteral("folder-download"), QStringLiteral("folder-downloads")},
        {QStringLiteral("folder-video"), QStringLiteral("folder-videos")}};

    // created on-demand as it is quite expensive to do and we don't want to do it every loop iteration either
    QScopedPointer<KIconTheme> theme;

    QVariantList pixmaps;

    for (const QStringList &iconNames : s_previewIcons) {
        const QString cacheKey = themeName + QLatin1Char('@') + QString::number(size) + QLatin1Char('@') + QString::number(dpr, 'f', 1) + QLatin1Char('@')
            + iconNames.join(QLatin1Char(','));

        QPixmap pix;
        if (!QPixmapCache::find(cacheKey, &pix)) {
            if (!theme) {
                theme.reset(new KIconTheme(themeName));
            }

            pix = getBestIcon(*theme.data(), iconNames, size, dpr);

            // Inserting a pixmap even if null so we know whether we searched for it already
            QPixmapCache::insert(cacheKey, pix);
        }

        if (pix.isNull()) {
            continue;
        }

        pixmaps.append(pix);

        if (limit > -1 && pixmaps.count() >= limit) {
            break;
        }
    }

    return pixmaps;
}

QPixmap IconModule::getBestIcon(KIconTheme &theme, const QStringList &iconNames, int size, qreal dpr)
{
    QSvgRenderer renderer;

    const int iconSize = size * dpr;

    // not using initializer list as we want to unwrap inherits()
    const QStringList themes = QStringList() << theme.internalName() << theme.inherits();
    for (const QString &themeName : themes) {
        KIconTheme theme(themeName);

        for (const QString &iconName : iconNames) {
            QString path = theme.iconPath(QStringLiteral("%1.png").arg(iconName), iconSize, KIconLoader::MatchBest);
            if (!path.isEmpty()) {
                QPixmap pixmap(path);
                pixmap.setDevicePixelRatio(dpr);
                return pixmap;
            }

            // could not find the .png, try loading the .svg or .svgz
            path = theme.iconPath(QStringLiteral("%1.svg").arg(iconName), iconSize, KIconLoader::MatchBest);
            if (path.isEmpty()) {
                path = theme.iconPath(QStringLiteral("%1.svgz").arg(iconName), iconSize, KIconLoader::MatchBest);
            }

            if (path.isEmpty()) {
                continue;
            }

            if (!renderer.load(path)) {
                continue;
            }

            QPixmap pixmap(iconSize, iconSize);
            pixmap.setDevicePixelRatio(dpr);
            pixmap.fill(QColor(Qt::transparent));
            QPainter p(&pixmap);
            p.setViewport(0, 0, size, size);
            renderer.render(&p);
            return pixmap;
        }
    }

    return QPixmap();
}

int IconModule::pluginIndex(const QString &themeName) const
{
    const auto results = m_model->match(m_model->index(0, 0), ThemeNameRole, themeName, 1, Qt::MatchExactly);
    if (results.count() == 1) {
        return results.first().row();
    }
    return -1;
}

void IconModule::defaults()
{
    for (int i = 0, count = m_model->rowCount(QModelIndex()); i < count; ++i) {
        m_model->setData(m_model->index(i), false, IconsModel::Roles::PendingDeletionRole);
    }
    ManagedConfigModule::defaults();
}

#include "main.moc"
