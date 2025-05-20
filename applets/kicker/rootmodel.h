/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "appsmodel.h"

class KAStatsFavoritesModel;
class RecentUsageModel;
class SystemModel;

class RootModel;

class GroupEntry : public AbstractGroupEntry
{
public:
    GroupEntry(AppsModel *parentModel, const QString &name, const QString &iconName, AbstractModel *childModel);

    QString icon() const override;
    QString name() const override;

    bool isNewlyInstalled() const override;

    bool hasChildren() const override;
    AbstractModel *childModel() const override;

private:
    QString m_name;
    QString m_iconName;
    QPointer<AbstractModel> m_childModel;
};

class AllAppsGroupEntry : public GroupEntry
{
public:
    AllAppsGroupEntry(AppsModel *parentModel, AbstractModel *childModel);

    bool isNewlyInstalled() const override;
};

class RootModel : public AppsModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *systemFavoritesModel READ systemFavoritesModel NOTIFY systemFavoritesModelChanged)
    Q_PROPERTY(bool showAllApps READ showAllApps WRITE setShowAllApps NOTIFY showAllAppsChanged)
    Q_PROPERTY(bool showAllAppsCategorized READ showAllAppsCategorized WRITE setShowAllAppsCategorized NOTIFY showAllAppsCategorizedChanged)
    Q_PROPERTY(bool showRecentApps READ showRecentApps WRITE setShowRecentApps NOTIFY showRecentAppsChanged)
    Q_PROPERTY(bool showRecentDocs READ showRecentDocs WRITE setShowRecentDocs NOTIFY showRecentDocsChanged)
    Q_PROPERTY(int recentOrdering READ recentOrdering WRITE setRecentOrdering NOTIFY recentOrderingChanged)
    Q_PROPERTY(bool showPowerSession READ showPowerSession WRITE setShowPowerSession NOTIFY showPowerSessionChanged)
    Q_PROPERTY(bool showFavoritesPlaceholder READ showFavoritesPlaceholder WRITE setShowFavoritesPlaceholder NOTIFY showFavoritesPlaceholderChanged)
    Q_PROPERTY(bool highlightNewlyInstalledApps READ highlightNewlyInstalledApps WRITE setHighlightNewlyInstalledApps NOTIFY highlightNewlyInstalledAppsChanged)

public:
    explicit RootModel(QObject *parent = nullptr);
    ~RootModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

    bool showAllApps() const;
    void setShowAllApps(bool show);

    bool showAllAppsCategorized() const;
    void setShowAllAppsCategorized(bool showCategorized);

    bool showRecentApps() const;
    void setShowRecentApps(bool show);

    bool showRecentDocs() const;
    void setShowRecentDocs(bool show);

    int recentOrdering() const;
    void setRecentOrdering(int ordering);

    bool showPowerSession() const;
    void setShowPowerSession(bool show);

    bool showFavoritesPlaceholder() const;
    void setShowFavoritesPlaceholder(bool show);

    bool highlightNewlyInstalledApps() const;
    void setHighlightNewlyInstalledApps(bool highlight);

    AbstractModel *favoritesModel() override;
    AbstractModel *systemFavoritesModel();

Q_SIGNALS:
    void refreshed() const;
    void systemFavoritesModelChanged() const;
    void showAllAppsChanged() const;
    void showAllAppsCategorizedChanged() const;
    void showRecentAppsChanged() const;
    void showRecentDocsChanged() const;
    void showPowerSessionChanged() const;
    void recentOrderingChanged() const;
    void recentAppsModelChanged() const;
    void showFavoritesPlaceholderChanged() const;
    void highlightNewlyInstalledAppsChanged() const;

protected Q_SLOTS:
    void refresh() override;

private:
    void refreshNewlyInstalledApps();
    void cleanupNewlyInstalledApps();

    KAStatsFavoritesModel *m_favorites;
    SystemModel *m_systemModel;

    bool m_showAllApps;
    bool m_showAllAppsCategorized;
    bool m_showRecentApps;
    bool m_showRecentDocs;
    int m_recentOrdering;
    bool m_showPowerSession;
    bool m_showFavoritesPlaceholder;

    bool m_highlightNewlyInstalledApps;
    QTimer *m_refreshNewlyInstalledAppsTimer;
    QTimer *m_cleanupNewlyInstalledAppsTimer;

    RecentUsageModel *m_recentAppsModel;
    RecentUsageModel *m_recentDocsModel;
};
