/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KNSCore/EntryWrapper>
#include <QPointer>
#include <QScopedPointer>

#include <KSharedConfig>

#include <KQuickAddons/ManagedConfigModule>

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

public:
    KCMColors(QObject *parent, const QVariantList &args);
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
    Q_INVOKABLE void installSchemeFromFile(const QUrl &url);

    Q_INVOKABLE void editScheme(const QString &schemeName, QQuickItem *ctx);

public Q_SLOTS:
    void load() override;
    void save() override;

Q_SIGNALS:
    void downloadingFileChanged();

    void showSuccessMessage(const QString &message);
    void showErrorMessage(const QString &message);

    void showSchemeNotInstalledWarning(const QString &schemeName);

private:
    bool isSaveNeeded() const override;

    void saveColors();
    void processPendingDeletions();

    void installSchemeFile(const QString &path);

    ColorsModel *m_model;
    FilterProxyModel *m_filteredModel;
    ColorsData *m_data;

    bool m_selectedSchemeDirty = false;
    bool m_activeSchemeEdited = false;

    bool m_applyToAlien = true;

    QProcess *m_editDialogProcess = nullptr;

    KSharedConfigPtr m_config;

    QScopedPointer<QTemporaryFile> m_tempInstallFile;
    QPointer<KIO::FileCopyJob> m_tempCopyJob;
};
