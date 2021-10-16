/*
    SPDX-FileCopyrightText: 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
    SPDX-FileCopyrightText: 2007 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-FileCopyrightText: 2016 Olivier Churlaud <olivier@churlaud.com>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "colors.h"

#include <QColor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QStandardPaths>

#include <KAboutData>
#include <KColorScheme>
#include <KColorUtils>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KWindowSystem>

#include <KIO/DeleteJob>
#include <KIO/FileCopyJob>
#include <KIO/JobUiDelegate>

#include <KNSCore/EntryWrapper>

#include <algorithm>

#include "krdb.h"

#include "colorsapplicator.h"
#include "colorsdata.h"
#include "colorsmodel.h"
#include "colorssettings.h"
#include "filterproxymodel.h"

#include "../kcms-common_p.h"

K_PLUGIN_FACTORY_WITH_JSON(KCMColorsFactory, "kcm_colors.json", registerPlugin<KCMColors>(); registerPlugin<ColorsData>();)

KCMColors::KCMColors(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_model(new ColorsModel(this))
    , m_filteredModel(new FilterProxyModel(this))
    , m_data(new ColorsData(this))
    , m_config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")))
{
    qmlRegisterUncreatableType<KCMColors>("org.kde.private.kcms.colors", 1, 0, "KCM", QStringLiteral("Cannot create instances of KCM"));
    qmlRegisterType<ColorsModel>();
    qmlRegisterType<FilterProxyModel>();
    qmlRegisterType<ColorsSettings>();

    KAboutData *about = new KAboutData(QStringLiteral("kcm_colors"), i18n("Colors"), QStringLiteral("2.0"), QString(), KAboutLicense::GPL);

    about->addAuthor(i18n("Kai Uwe Broulik"), QString(), QStringLiteral("kde@privat.broulik.de"));
    setAboutData(about);

    connect(m_model, &ColorsModel::pendingDeletionsChanged, this, &KCMColors::settingsChanged);

    connect(m_model, &ColorsModel::selectedSchemeChanged, this, [this](const QString &scheme) {
        m_selectedSchemeDirty = true;
        colorsSettings()->setColorScheme(scheme);
    });

    connect(colorsSettings(), &ColorsSettings::colorSchemeChanged, this, [this] {
        m_model->setSelectedScheme(colorsSettings()->colorScheme());
    });

    connect(colorsSettings(), &ColorsSettings::accentColorChanged, this, &KCMColors::accentColorChanged);

    connect(m_model, &ColorsModel::selectedSchemeChanged, m_filteredModel, &FilterProxyModel::setSelectedScheme);
    m_filteredModel->setSourceModel(m_model);
}

KCMColors::~KCMColors()
{
    m_config->markAsClean();
}

ColorsModel *KCMColors::model() const
{
    return m_model;
}

FilterProxyModel *KCMColors::filteredModel() const
{
    return m_filteredModel;
}

ColorsSettings *KCMColors::colorsSettings() const
{
    return m_data->settings();
}

QColor KCMColors::accentColor() const
{
    const QColor color = colorsSettings()->accentColor();
    if (!color.isValid()) {
        return QColor(Qt::transparent);
    }
    return color;
}

void KCMColors::setAccentColor(const QColor &accentColor)
{
    colorsSettings()->setAccentColor(accentColor);
    Q_EMIT settingsChanged();
}

bool KCMColors::downloadingFile() const
{
    return m_tempCopyJob;
}

void KCMColors::knsEntryChanged(KNSCore::EntryWrapper *entry)
{
    if (!entry) {
        return;
    }
    m_model->load();

    // If a new theme was installed, select the first color file in it
    QStringList installedThemes;
    const QString suffix = QStringLiteral(".colors");
    if (entry->entry().status() == KNS3::Entry::Installed) {
        for (const QString &path : entry->entry().installedFiles()) {
            const QString fileName = path.section(QLatin1Char('/'), -1, -1);

            const int suffixPos = fileName.indexOf(suffix);
            if (suffixPos != fileName.length() - suffix.length()) {
                continue;
            }

            installedThemes.append(fileName.left(suffixPos));
        }

        if (!installedThemes.isEmpty()) {
            // The list is sorted by (potentially translated) name
            // but that would require us parse every file, so this should be close enough
            std::sort(installedThemes.begin(), installedThemes.end());

            m_model->setSelectedScheme(installedThemes.constFirst());
        }
    }
}

void KCMColors::loadSelectedColorScheme()
{
    colorsSettings()->config()->reparseConfiguration();
    colorsSettings()->read();
    const QString schemeName = colorsSettings()->colorScheme();

    // If the scheme named in kdeglobals doesn't exist, show a warning and use default scheme
    if (m_model->indexOfScheme(schemeName) == -1) {
        m_model->setSelectedScheme(colorsSettings()->defaultColorSchemeValue());
        // These are normally synced but initially the model doesn't emit a change to avoid the
        // Apply button from being enabled without any user interaction. Sync manually here.
        m_filteredModel->setSelectedScheme(colorsSettings()->defaultColorSchemeValue());
        emit showSchemeNotInstalledWarning(schemeName);
    } else {
        m_model->setSelectedScheme(schemeName);
        m_filteredModel->setSelectedScheme(schemeName);
    }
    setNeedsSave(false);
}

void KCMColors::installSchemeFromFile(const QUrl &url)
{
    if (url.isLocalFile()) {
        installSchemeFile(url.toLocalFile());
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

    // Ideally we copied the file into the proper location right away but
    // (for some reason) we determine the file name from the "Name" inside the file
    m_tempCopyJob = KIO::file_copy(url, QUrl::fromLocalFile(m_tempInstallFile->fileName()), -1, KIO::Overwrite);
    m_tempCopyJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    emit downloadingFileChanged();

    connect(m_tempCopyJob, &KIO::FileCopyJob::result, this, [this, url](KJob *job) {
        if (job->error() != KJob::NoError) {
            emit showErrorMessage(i18n("Unable to download the color scheme: %1", job->errorText()));
            return;
        }

        installSchemeFile(m_tempInstallFile->fileName());
        m_tempInstallFile.reset();
    });
    connect(m_tempCopyJob, &QObject::destroyed, this, &KCMColors::downloadingFileChanged);
}

void KCMColors::installSchemeFile(const QString &path)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(path, KConfig::SimpleConfig);

    KConfigGroup group(config, "General");
    const QString name = group.readEntry("Name");

    if (name.isEmpty()) {
        emit showErrorMessage(i18n("This file is not a color scheme file."));
        return;
    }

    // Do not overwrite another scheme
    int increment = 0;
    QString newName = name;
    QString testpath;
    do {
        if (increment) {
            newName = name + QString::number(increment);
        }
        testpath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(newName));
        increment++;
    } while (!testpath.isEmpty());

    QString newPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/color-schemes/");

    if (!QDir().mkpath(newPath)) {
        emit showErrorMessage(i18n("Failed to create 'color-scheme' data folder."));
        return;
    }

    newPath += newName + QLatin1String(".colors");

    if (!QFile::copy(path, newPath)) {
        emit showErrorMessage(i18n("Failed to copy color scheme into 'color-scheme' data folder."));
        return;
    }

    // Update name
    KSharedConfigPtr config2 = KSharedConfig::openConfig(newPath, KConfig::SimpleConfig);
    KConfigGroup group2(config2, "General");
    group2.writeEntry("Name", newName);
    config2->sync();

    m_model->load();

    const auto results = m_model->match(m_model->index(0, 0), ColorsModel::SchemeNameRole, newName, 1, Qt::MatchExactly);
    if (!results.isEmpty()) {
        m_model->setSelectedScheme(newName);
    }

    emit showSuccessMessage(i18n("Color scheme installed successfully."));
}

void KCMColors::editScheme(const QString &schemeName, QQuickItem *ctx)
{
    if (m_editDialogProcess) {
        return;
    }

    QModelIndex idx = m_model->index(m_model->indexOfScheme(schemeName), 0);

    m_editDialogProcess = new QProcess(this);
    connect(m_editDialogProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitCode);
        Q_UNUSED(exitStatus);

        const auto savedThemes = QString::fromUtf8(m_editDialogProcess->readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        if (!savedThemes.isEmpty()) {
            m_model->load(); // would be cool to just reload/add the changed/new ones

            // If the currently active scheme was edited, consider settings dirty even if the scheme itself didn't change
            if (savedThemes.contains(colorsSettings()->colorScheme())) {
                m_activeSchemeEdited = true;
                settingsChanged();
            }

            m_model->setSelectedScheme(savedThemes.last());
        }

        m_editDialogProcess->deleteLater();
        m_editDialogProcess = nullptr;
    });

    QStringList args;
    args << idx.data(ColorsModel::SchemeNameRole).toString();
    if (idx.data(ColorsModel::RemovableRole).toBool()) {
        args << QStringLiteral("--overwrite");
    }

    if (ctx && ctx->window()) {
        // QQuickWidget, used for embedding QML KCMs, renders everything into an offscreen window
        // Qt is able to resolve this on its own when setting transient parents in-process.
        // However, since we pass the ID to an external process which has no idea of this
        // we need to resolve the actual window we end up showing in.
        if (QWindow *actualWindow = QQuickRenderControl::renderWindowFor(ctx->window())) {
            if (KWindowSystem::isPlatformX11()) {
                // TODO wayland: once we have foreign surface support
                args << QStringLiteral("--attach") << (QStringLiteral("x11:") + QString::number(actualWindow->winId()));
            }
        }
    }

    m_editDialogProcess->start(QStringLiteral("kcolorschemeeditor"), args);
}

bool KCMColors::isSaveNeeded() const
{
    return m_activeSchemeEdited || !m_model->match(m_model->index(0, 0), ColorsModel::PendingDeletionRole, true).isEmpty() || colorsSettings()->isSaveNeeded();
}

void KCMColors::load()
{
    ManagedConfigModule::load();
    m_model->load();

    m_config->markAsClean();
    m_config->reparseConfiguration();

    loadSelectedColorScheme();

    {
        KConfig cfg(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
        KConfigGroup group(m_config, "General");
        group = KConfigGroup(&cfg, "X11");
        m_applyToAlien = group.readEntry("exportKDEColors", true);
    }

    // If need save is true at the end of load() function, it will stay disabled forever.
    // setSelectedScheme() call due to unexisting scheme name in kdeglobals will trigger a need to save.
    // this following call ensure the apply button will work properly.
    setNeedsSave(false);
}

void KCMColors::save()
{
    // We need to save the colors change first, to avoid a situation,
    // when we announced that the color scheme has changed, but
    // the colors themselves in the color scheme have not yet
    if (m_selectedSchemeDirty || m_activeSchemeEdited || colorsSettings()->isSaveNeeded()) {
        saveColors();
    }
    ManagedConfigModule::save();
    notifyKcmChange(GlobalChangeType::PaletteChanged);
    m_activeSchemeEdited = false;

    processPendingDeletions();
}

void KCMColors::saveColors()
{
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(m_model->selectedScheme()));
    // hard to figure out why mutating from the colours settings
    // doesn't affect the applicator's view on config, but operating on the
    // globalConfig directly works.
    // code already a mess, so might as well just do what works.
    KSharedConfigPtr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));

    auto setGlobals = [=]() {
        globalConfig->group("General").writeEntry("AccentColor", QColor());
        if (accentColor() != QColor(Qt::transparent)) {
            globalConfig->group("General").writeEntry("AccentColor", accentColor(), KConfig::Notify);
        } else {
            globalConfig->group("General").deleteEntry("AccentColor", KConfig::Notify);
        }
    };

    setGlobals();
    applyScheme(path, colorsSettings()->config());
    m_selectedSchemeDirty = false;
    setGlobals();
}

QColor KCMColors::accentBackground(const QColor& accent, const QColor& background)
{
    return ::accentBackground(accent, background);
}

void KCMColors::processPendingDeletions()
{
    const QStringList pendingDeletions = m_model->pendingDeletions();

    for (const QString &schemeName : pendingDeletions) {
        Q_ASSERT(schemeName != m_model->selectedScheme());

        const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(schemeName));

        auto *job = KIO::del(QUrl::fromLocalFile(path), KIO::HideProgressInfo);
        // needs to block for it to work on "OK" where the dialog (kcmshell) closes
        job->exec();
    }

    m_model->removeItemsPendingDeletion();
}

#include "colors.moc"
