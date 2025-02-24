/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KConfig>
#include <KConfigGroup>
#include <KNSCore/Entry>
#include <KSharedConfig>

#include <QDir>
#include <QStandardItemModel>

#include <KPackage/Package>
#include <KQuickManagedConfigModule>

#include "lookandfeelmanager.h"
#include "lookandfeelsettings.h"

class LookAndFeelManager;

class KCMLookandFeel : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(LookAndFeelSettings *lookAndFeelSettings READ lookAndFeelSettings CONSTANT)
    Q_PROPERTY(QStandardItemModel *lookAndFeelModel READ lookAndFeelModel CONSTANT)

    Q_PROPERTY(LookAndFeelManager::Contents themeContents READ themeContents NOTIFY themeContentsChanged)
    Q_PROPERTY(LookAndFeelManager::Contents selectedContents READ selectedContents WRITE setSelectedContents RESET resetSelectedContents NOTIFY
                   selectedContentsChanged)

    Q_PROPERTY(bool plasmaLocked READ isPlasmaLocked NOTIFY plasmaLockedChanged)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole + 1,
        ScreenshotRole,
        FullScreenPreviewRole,
        DescriptionRole,
        ContentsRole,
        PackagePathRole, //< Package root path
        UninstallableRole, //< Package is installed in local directory
    };
    Q_ENUM(Roles)

    KCMLookandFeel(QObject *parent, const KPluginMetaData &data);
    ~KCMLookandFeel() override;

    LookAndFeelSettings *lookAndFeelSettings() const;
    QStandardItemModel *lookAndFeelModel() const;

    /**
     * Removes the given row from the LookandFeel items.
     * It calls \QStandardItemModel::removeRow and removes local files
     *
     * @param row the given row from LookandFeel items
     * @param removeDependencies whether the dependencies should also be removed
     * @return Returns true if the row is removed; otherwise returns false.
     */
    Q_INVOKABLE bool removeRow(int row, bool removeDependencies = false);

    Q_INVOKABLE int pluginIndex(const QString &pluginName) const;
    Q_INVOKABLE void knsEntryChanged(const KNSCore::Entry &entry);
    Q_INVOKABLE void reloadConfig()
    {
        KQuickManagedConfigModule::load();
    }

    LookAndFeelManager *lookAndFeel() const
    {
        return m_lnf;
    };
    void addKPackageToModel(const KPackage::Package &pkg);

    bool isSaveNeeded() const override;

    LookAndFeelManager::Contents themeContents() const;

    LookAndFeelManager::Contents selectedContents() const;
    void setSelectedContents(LookAndFeelManager::Contents items);
    void resetSelectedContents();

    bool isPlasmaLocked() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void showConfirmation();
    void themeContentsChanged();
    void selectedContentsChanged();
    void plasmaLockedChanged();

private:
    // List only packages which provide at least one of the components
    QList<KPackage::Package> availablePackages(const QStringList &components);
    void loadModel();
    QDir cursorThemeDir(const QString &theme, const int depth);
    QStringList cursorSearchPaths();
    void cursorsChanged(const QString &name);

    LookAndFeelManager *const m_lnf;

    LookAndFeelManager::Contents m_themeContents;
    LookAndFeelManager::Contents m_selectedContents;

    QStandardItemModel *m_model;
    KPackage::Package m_package;
    QStringList m_cursorSearchPaths;
};
