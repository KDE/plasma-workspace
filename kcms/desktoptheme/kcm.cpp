/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2016 David Rosca <nowrep@gmail.com>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"

#include <KLocalizedString>
#include <KPackage/PackageJob>
#include <KPluginFactory>

#include <KIO/FileCopyJob>
#include <KIO/JobUiDelegate>

#include <KSvg/FrameSvg>
#include <KSvg/ImageSet>
#include <KSvg/Svg>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "desktopthemedata.h"
#include "filterproxymodel.h"

Q_LOGGING_CATEGORY(KCM_DESKTOP_THEME, "kcm_desktoptheme")

K_PLUGIN_FACTORY_WITH_JSON(KCMDesktopThemeFactory, "kcm_desktoptheme.json", registerPlugin<KCMDesktopTheme>(); registerPlugin<DesktopThemeData>();)

KCMDesktopTheme::KCMDesktopTheme(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new DesktopThemeData(this))
    , m_model(new ThemesModel(this))
    , m_filteredModel(new FilterProxyModel(this))
    , m_haveThemeExplorerInstalled(false)
{
    qmlRegisterAnonymousType<DesktopThemeSettings>("org.kde.private.kcms.desktoptheme", 1);
    qmlRegisterUncreatableType<ThemesModel>("org.kde.private.kcms.desktoptheme", 1, 0, "ThemesModel", QStringLiteral("Cannot create ThemesModel"));
    qmlRegisterUncreatableType<FilterProxyModel>("org.kde.private.kcms.desktoptheme",
                                                 1,
                                                 0,
                                                 "FilterProxyModel",
                                                 QStringLiteral("Cannot create FilterProxyModel"));

    setButtons(Apply | Default | Help);

    m_haveThemeExplorerInstalled = !QStandardPaths::findExecutable(QStringLiteral("plasmathemeexplorer")).isEmpty();

    connect(m_model, &ThemesModel::pendingDeletionsChanged, this, &KCMDesktopTheme::settingsChanged);

    connect(m_model, &ThemesModel::selectedThemeChanged, this, [this](const QString &pluginName) {
        desktopThemeSettings()->setName(pluginName);
    });

    connect(desktopThemeSettings(), &DesktopThemeSettings::nameChanged, this, [this] {
        m_model->setSelectedTheme(desktopThemeSettings()->name());
    });

    connect(m_model, &ThemesModel::selectedThemeChanged, m_filteredModel, &FilterProxyModel::setSelectedTheme);

    m_filteredModel->setSourceModel(m_model);
}

KCMDesktopTheme::~KCMDesktopTheme()
{
}

DesktopThemeSettings *KCMDesktopTheme::desktopThemeSettings() const
{
    return m_data->settings();
}

ThemesModel *KCMDesktopTheme::desktopThemeModel() const
{
    return m_model;
}

FilterProxyModel *KCMDesktopTheme::filteredModel() const
{
    return m_filteredModel;
}

bool KCMDesktopTheme::downloadingFile() const
{
    return m_tempCopyJob;
}

void KCMDesktopTheme::installThemeFromFile(const QUrl &url)
{
    if (url.isLocalFile()) {
        installTheme(url.toLocalFile());
        return;
    }

    if (m_tempCopyJob) {
        return;
    }

    m_tempInstallFile.reset(new QTemporaryFile());
    if (!m_tempInstallFile->open()) {
        Q_EMIT showErrorMessage(i18n("Unable to create a temporary file."));
        m_tempInstallFile.reset();
        return;
    }

    m_tempCopyJob = KIO::file_copy(url, QUrl::fromLocalFile(m_tempInstallFile->fileName()), -1, KIO::Overwrite);
    m_tempCopyJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    Q_EMIT downloadingFileChanged();

    connect(m_tempCopyJob, &KIO::FileCopyJob::result, this, [this, url](KJob *job) {
        if (job->error() != KJob::NoError) {
            Q_EMIT showErrorMessage(i18n("Unable to download the theme: %1", job->errorText()));
            return;
        }

        installTheme(m_tempInstallFile->fileName());
        m_tempInstallFile.reset();
    });
    connect(m_tempCopyJob, &QObject::destroyed, this, &KCMDesktopTheme::downloadingFileChanged);
}

void KCMDesktopTheme::installTheme(const QString &path)
{
    qCDebug(KCM_DESKTOP_THEME) << "Installing ... " << path;

    const QString program = QStringLiteral("kpackagetool6");
    const QStringList arguments = {QStringLiteral("--type"), QStringLiteral("Plasma/Theme"), QStringLiteral("--install"), path};

    qCDebug(KCM_DESKTOP_THEME) << program << arguments.join(QLatin1Char(' '));
    QProcess *myProcess = new QProcess(this);
    connect(myProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitStatus)
                if (exitCode == 0) {
                    Q_EMIT showSuccessMessage(i18n("Theme installed successfully."));
                    load();
                } else {
                    Q_EMIT showErrorMessage(i18n("Theme installation failed."));
                }
            });

    connect(myProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        qCWarning(KCM_DESKTOP_THEME) << "Theme installation failed: " << e;
        Q_EMIT showErrorMessage(i18n("Theme installation failed."));
    });

    myProcess->start(program, arguments);
}

void KCMDesktopTheme::applyPlasmaTheme(QQuickItem *item, const QString &themeName)
{
    if (!item) {
        return;
    }

    KSvg::ImageSet *imageSet = m_themes[themeName];
    if (!imageSet) {
        imageSet = new KSvg::ImageSet(themeName, QStringLiteral("plasma/desktoptheme"), this);
        m_themes[themeName] = imageSet;
    }

    Q_FOREACH (KSvg::Svg *svg, item->findChildren<KSvg::Svg *>()) {
        auto *frame = qobject_cast<KSvg::FrameSvg *>(svg);
        svg->setUsingRenderingCache(false);
        if (frame) {
            frame->setCacheAllRenderedFrames(false);
        }
        svg->setImageSet(imageSet);
    }
}

void KCMDesktopTheme::load()
{
    KQuickManagedConfigModule::load();
    m_model->load();
    m_model->setSelectedTheme(desktopThemeSettings()->name());
}

void KCMDesktopTheme::save()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                              QStringLiteral("/org/kde/KWin/BlendChanges"),
                                              QStringLiteral("org.kde.KWin.BlendChanges"),
                                              QStringLiteral("start"));
    // Plasma theme changes are known to be slow, so make the blend take a while
    msg << 1000;
    // This is deliberately blocking so that we ensure Kwin has processed the
    // animation start event before we potentially trigger client side changes
    QDBusConnection::sessionBus().call(msg);

    KQuickManagedConfigModule::save();
    KSvg::ImageSet().setImageSetName(desktopThemeSettings()->name());
    processPendingDeletions();
}

void KCMDesktopTheme::defaults()
{
    KQuickManagedConfigModule::defaults();

    // can this be done more elegantly?
    const auto pendingDeletions = m_model->match(m_model->index(0, 0), ThemesModel::PendingDeletionRole, true);
    for (const QModelIndex &idx : pendingDeletions) {
        m_model->setData(idx, false, ThemesModel::PendingDeletionRole);
    }
}

bool KCMDesktopTheme::canEditThemes() const
{
    return m_haveThemeExplorerInstalled;
}

void KCMDesktopTheme::editTheme(const QString &theme)
{
    QProcess::startDetached(QStringLiteral("plasmathemeexplorer"), {QStringLiteral("-t"), theme});
}

bool KCMDesktopTheme::isSaveNeeded() const
{
    return !m_model->match(m_model->index(0, 0), ThemesModel::PendingDeletionRole, true).isEmpty();
}

void KCMDesktopTheme::processPendingDeletions()
{
    const auto pendingDeletions = m_model->match(m_model->index(0, 0), ThemesModel::PendingDeletionRole, true, -1 /*all*/);
    QList<QPersistentModelIndex> persistentPendingDeletions;
    // turn into persistent model index so we can delete as we go
    std::transform(pendingDeletions.begin(), pendingDeletions.end(), std::back_inserter(persistentPendingDeletions), [](const QModelIndex &idx) {
        return QPersistentModelIndex(idx);
    });

    for (const QPersistentModelIndex &idx : persistentPendingDeletions) {
        const QString pluginName = idx.data(ThemesModel::PluginNameRole).toString();
        const QString location = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/plasma/desktoptheme");
        auto *job = KPackage::PackageJob::uninstall(QStringLiteral("Plasma/Theme"), pluginName, location);

        connect(job, &KJob::finished, this, [this, idx](KJob *job) {
            if (job->error()) {
                Q_EMIT showErrorMessage(i18n("Removing theme failed: %1", job->errorString()));
                m_model->setData(idx, false, ThemesModel::PendingDeletionRole);
            } else {
                m_model->removeRow(idx.row());
            }
        });
    }
}

#include "kcm.moc"
