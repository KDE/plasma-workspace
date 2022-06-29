/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KConfigWatcher>
#include <KNSCore/EntryWrapper>
#include <QColor>
#include <QDBusPendingCallWatcher>
#include <QPointer>

#include <KSharedConfig>

#include <KQuickAddons/ManagedConfigModule>

#include <optional>

#include "colorsmodel.h"
#include "colorssettings.h"

class QProcess;
class QTemporaryFile;

namespace KIO
{
class FileCopyJob;
}

class FilterProxyModel;
class ColorsData;

class KCMColors : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(ColorsModel *model READ model CONSTANT)
    Q_PROPERTY(FilterProxyModel *filteredModel READ filteredModel CONSTANT)
    Q_PROPERTY(ColorsSettings *colorsSettings READ colorsSettings CONSTANT)
    Q_PROPERTY(bool downloadingFile READ downloadingFile NOTIFY downloadingFileChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QColor lastUsedCustomAccentColor READ lastUsedCustomAccentColor WRITE setLastUsedCustomAccentColor NOTIFY lastUsedCustomAccentColorChanged)
    Q_PROPERTY(bool accentColorFromWallpaper READ accentColorFromWallpaper WRITE setAccentColorFromWallpaper NOTIFY accentColorFromWallpaperChanged)

public:
    KCMColors(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMColors() override;

    enum SchemeFilter {
        AllSchemes,
        LightSchemes,
        DarkSchemes,
    };
    Q_ENUM(SchemeFilter)

    ColorsModel *model() const;
    FilterProxyModel *filteredModel() const;
    ColorsSettings *colorsSettings() const;
    bool downloadingFile() const;

    Q_INVOKABLE void loadSelectedColorScheme();
    Q_INVOKABLE void knsEntryChanged(KNSCore::EntryWrapper *entry);

    QColor accentColor() const;
    void setAccentColor(const QColor &accentColor);
    void resetAccentColor();
    Q_SIGNAL void accentColorChanged();

    QColor lastUsedCustomAccentColor() const;
    void setLastUsedCustomAccentColor(const QColor &accentColor);
    Q_SIGNAL void lastUsedCustomAccentColorChanged();

    bool accentColorFromWallpaper() const;
    void setAccentColorFromWallpaper(bool boolean);
    Q_SIGNAL void accentColorFromWallpaperChanged();

    Q_INVOKABLE void installSchemeFromFile(const QUrl &url);

    Q_INVOKABLE void editScheme(const QString &schemeName, QQuickItem *ctx);

    // we take an extraneous reference to the accent colour here in order to have the bindings
    // re-evaluate when it changes
    Q_INVOKABLE QColor tinted(const QColor &color, const QColor &accent, bool tints, qreal tintFactor);
    Q_INVOKABLE QColor accentBackground(const QColor &accent, const QColor &background);
    Q_INVOKABLE QColor accentForeground(const QColor &accent, const bool &isActive);

public Q_SLOTS:
    void load() override;
    void save() override;

private Q_SLOTS:
    void wallpaperAccentColorArrivedSlot(QDBusPendingCallWatcher *call);

Q_SIGNALS:
    void downloadingFileChanged();

    void showSuccessMessage(const QString &message);
    void showErrorMessage(const QString &message);

    void showSchemeNotInstalledWarning(const QString &schemeName);

private:
    bool isSaveNeeded() const override;

    void saveColors();
    void processPendingDeletions();

    void applyWallpaperAccentColor();

    void installSchemeFile(const QString &path);

    ColorsModel *m_model;
    FilterProxyModel *m_filteredModel;
    ColorsData *m_data;

    bool m_selectedSchemeDirty = false;
    bool m_activeSchemeEdited = false;

    QProcess *m_editDialogProcess = nullptr;

    KSharedConfigPtr m_config;
    KConfigWatcher::Ptr m_configWatcher;

    std::unique_ptr<QTemporaryFile> m_tempInstallFile;
    QPointer<KIO::FileCopyJob> m_tempCopyJob;
};
