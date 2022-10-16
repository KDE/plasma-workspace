/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KConfig>
#include <KConfigGroup>
#include <KNSCore/EntryWrapper>
#include <KSharedConfig>

#include <QDir>
#include <QStandardItemModel>

#include <KPackage/Package>
#include <KQuickAddons/ManagedConfigModule>

#include "lookandfeelmanager.h"
#include "lookandfeelsettings.h"

class QQuickItem;
class LookAndFeelManager;

class KCMLookandFeel : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(LookAndFeelSettings *lookAndFeelSettings READ lookAndFeelSettings CONSTANT)
    Q_PROPERTY(QStandardItemModel *lookAndFeelModel READ lookAndFeelModel CONSTANT)
    Q_PROPERTY(LookAndFeelManager::AppearanceToApply appearanceToApply READ appearanceToApply WRITE setAppearanceToApply NOTIFY appearanceToApplyChanged RESET
                   resetAppearanceToApply)
    Q_PROPERTY(LookAndFeelManager::LayoutToApply layoutToApply READ layoutToApply WRITE setLayoutToApply NOTIFY layoutToApplyChanged RESET resetLayoutToApply)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole + 1,
        ScreenshotRole,
        FullScreenPreviewRole,
        DescriptionRole,
        HasSplashRole,
        HasLockScreenRole,
        HasRunCommandRole,
        HasLogoutRole,
        HasGlobalThemeRole,
        HasLayoutSettingsRole,
        HasDesktopLayoutRole,
        HasTitlebarLayoutRole,
        HasColorsRole,
        HasWidgetStyleRole,
        HasIconsRole,
        HasPlasmaThemeRole,
        HasCursorsRole,
        HasWindowSwitcherRole,
        HasDesktopSwitcherRole,
        HasWindowDecorationRole,
        HasFontsRole,
    };
    Q_ENUM(Roles)

    KCMLookandFeel(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~KCMLookandFeel() override;

    LookAndFeelSettings *lookAndFeelSettings() const;
    QStandardItemModel *lookAndFeelModel() const;

    Q_INVOKABLE int pluginIndex(const QString &pluginName) const;
    Q_INVOKABLE void knsEntryChanged(KNSCore::EntryWrapper *wrapper);
    Q_INVOKABLE void reloadConfig()
    {
        ManagedConfigModule::load();
    }

    LookAndFeelManager *lookAndFeel() const
    {
        return m_lnf;
    };
    void addKPackageToModel(const KPackage::Package &pkg);

    bool isSaveNeeded() const override;

    LookAndFeelManager::AppearanceToApply appearanceToApply() const;
    void setAppearanceToApply(LookAndFeelManager::AppearanceToApply items);
    void resetAppearanceToApply();
    LookAndFeelManager::LayoutToApply layoutToApply() const;
    void setLayoutToApply(LookAndFeelManager::LayoutToApply items);
    void resetLayoutToApply();

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void showConfirmation();
    void appearanceToApplyChanged();
    void layoutToApplyChanged();

private:
    // List only packages which provide at least one of the components
    QList<KPackage::Package> availablePackages(const QStringList &components);
    void loadModel();
    QDir cursorThemeDir(const QString &theme, const int depth);
    QStringList cursorSearchPaths();
    void cursorsChanged(const QString &name);

    LookAndFeelManager *const m_lnf;

    QStandardItemModel *m_model;
    KPackage::Package m_package;
    QStringList m_cursorSearchPaths;
};
