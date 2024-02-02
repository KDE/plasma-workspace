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

    bool hasChildren() const override;
    AbstractModel *childModel() const override;

private:
    QString m_name;
    QString m_iconName;
    QPointer<AbstractModel> m_childModel;
};

class RootModel : public AppsModel
{
    Q_OBJECT

    Q_PROPERTY(QObject *systemFavoritesModel READ systemFavoritesModel NOTIFY systemFavoritesModelChanged)
    Q_PROPERTY(bool showAllApps READ showAllApps WRITE setShowAllApps NOTIFY showAllAppsChanged)
    Q_PROPERTY(bool showAllAppsCategorized READ showAllAppsCategorized WRITE setShowAllAppsCategorized NOTIFY showAllAppsCategorizedChanged)
    Q_PROPERTY(bool showRecentApps READ showRecentApps WRITE setShowRecentApps NOTIFY showRecentAppsChanged)
    Q_PROPERTY(bool showRecentDocs READ showRecentDocs WRITE setShowRecentDocs NOTIFY showRecentDocsChanged)
    Q_PROPERTY(int recentOrdering READ recentOrdering WRITE setRecentOrdering NOTIFY recentOrderingChanged)
    Q_PROPERTY(bool showPowerSession READ showPowerSession WRITE setShowPowerSession NOTIFY showPowerSessionChanged)
    Q_PROPERTY(bool showFavoritesPlaceholder READ showFavoritesPlaceholder WRITE setShowFavoritesPlaceholder NOTIFY showFavoritesPlaceholderChanged)

public:
    explicit RootModel(QObject *parent = nullptr);
    ~RootModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE QVariantList actionList(int row) override;
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

protected Q_SLOTS:
    void refresh() override;

private:
    KAStatsFavoritesModel *m_favorites;
    SystemModel *m_systemModel;

    bool m_showAllApps;
    bool m_showAllAppsCategorized;
    bool m_showRecentApps;
    bool m_showRecentDocs;
    int m_recentOrdering;
    bool m_showPowerSession;
    bool m_showFavoritesPlaceholder;

    RecentUsageModel *m_recentAppsModel;
    RecentUsageModel *m_recentDocsModel;
};
