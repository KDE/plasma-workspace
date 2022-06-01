/*
    main.h

    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    KDE Frameworks 5 port:
    SPDX-FileCopyrightText: 2013 Jonathan Riddell <jr@jriddell.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include <QCache>

class KIconTheme;
class IconsSettings;
class IconsData;

class QQuickItem;
class QTemporaryFile;

namespace KIO
{
class FileCopyJob;
}

class IconsModel;
class IconSizeCategoryModel;

class IconModule : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(IconsSettings *iconsSettings READ iconsSettings CONSTANT)
    Q_PROPERTY(IconsModel *iconsModel READ iconsModel CONSTANT)
    Q_PROPERTY(IconSizeCategoryModel *iconSizeCategoryModel READ iconSizeCategoryModel CONSTANT)
    Q_PROPERTY(bool downloadingFile READ downloadingFile NOTIFY downloadingFileChanged)

public:
    IconModule(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~IconModule() override;

    enum Roles {
        ThemeNameRole = Qt::UserRole + 1,
        DescriptionRole,
        RemovableRole,
        PendingDeletionRole,
    };

    IconsSettings *iconsSettings() const;
    IconsModel *iconsModel() const;
    IconSizeCategoryModel *iconSizeCategoryModel() const;

    bool downloadingFile() const;

    void load() override;
    Q_INVOKABLE void reloadConfig()
    {
        ManagedConfigModule::load();
    }

    void save() override;
    void defaults() override;

    Q_INVOKABLE void ghnsEntriesChanged();
    Q_INVOKABLE void installThemeFromFile(const QUrl &url);

    Q_INVOKABLE QList<int> availableIconSizes(int group) const;

    Q_INVOKABLE int pluginIndex(const QString &pluginName) const;

    // QML doesn't understand QList<QPixmap>, hence wrapped in a QVariantList
    Q_INVOKABLE QVariantList previewIcons(const QString &themeName, int size, qreal dpr, int limit = -1);

Q_SIGNALS:
    void downloadingFileChanged();

    void showSuccessMessage(const QString &message);
    void showErrorMessage(const QString &message);

    void showProgress(const QString &message);
    void hideProgress();

private:
    bool isSaveNeeded() const override;

    void processPendingDeletions();

    static QStringList findThemeDirs(const QString &archiveName);
    bool installThemes(const QStringList &themes, const QString &archiveName);
    void installThemeFile(const QString &path);

    static QPixmap getBestIcon(KIconTheme &theme, const QStringList &iconNames, int size, qreal dpr);

    IconsData *m_data;
    IconsModel *m_model;
    IconSizeCategoryModel *m_iconSizeCategoryModel;

    mutable QCache<QString, KIconTheme> m_kiconThemeCache;

    std::unique_ptr<QTemporaryFile> m_tempInstallFile;
    QPointer<KIO::FileCopyJob> m_tempCopyJob;
};
