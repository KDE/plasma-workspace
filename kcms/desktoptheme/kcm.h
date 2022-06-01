/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2016 David Rosca <nowrep@gmail.com>
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include "desktopthemesettings.h"
#include "themesmodel.h"

class QTemporaryFile;

namespace Plasma
{
class Theme;
}

namespace KIO
{
class FileCopyJob;
}

class QQuickItem;
class DesktopThemeData;
class FilterProxyModel;

class KCMDesktopTheme : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(DesktopThemeSettings *desktopThemeSettings READ desktopThemeSettings CONSTANT)
    Q_PROPERTY(FilterProxyModel *filteredModel READ filteredModel CONSTANT)
    Q_PROPERTY(ThemesModel *desktopThemeModel READ desktopThemeModel CONSTANT)
    Q_PROPERTY(bool downloadingFile READ downloadingFile NOTIFY downloadingFileChanged)
    Q_PROPERTY(bool canEditThemes READ canEditThemes CONSTANT)

public:
    KCMDesktopTheme(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMDesktopTheme() override;

    DesktopThemeSettings *desktopThemeSettings() const;
    ThemesModel *desktopThemeModel() const;
    FilterProxyModel *filteredModel() const;

    bool downloadingFile() const;

    bool canEditThemes() const;

    Q_INVOKABLE void installThemeFromFile(const QUrl &url);

    Q_INVOKABLE void applyPlasmaTheme(QQuickItem *item, const QString &themeName);

    Q_INVOKABLE void editTheme(const QString &themeName);

Q_SIGNALS:
    void downloadingFileChanged();

    void showSuccessMessage(const QString &message);
    void showErrorMessage(const QString &message);

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private:
    bool isSaveNeeded() const override;

    void processPendingDeletions();

    void installTheme(const QString &path);

    DesktopThemeData *m_data;

    ThemesModel *m_model;
    FilterProxyModel *m_filteredModel;
    QHash<QString, Plasma::Theme *> m_themes;
    bool m_haveThemeExplorerInstalled;

    std::unique_ptr<QTemporaryFile> m_tempInstallFile;
    QPointer<KIO::FileCopyJob> m_tempCopyJob;
};

Q_DECLARE_LOGGING_CATEGORY(KCM_DESKTOP_THEME)
