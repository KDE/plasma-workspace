/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2016-2017 Ivan Cukic <ivan.cukic@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "placeholdermodel.h"

#include <QPointer>
#include <qqmlregistration.h>

#include <KConfig>

class PlaceholderModel;

namespace KActivities
{
class Consumer;

namespace Stats::Terms
{
class Activity;
} // namespace Stats::Terms

} // namespace KActivities

class KAStatsFavoritesModel : public PlaceholderModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QStringList favorites READ favorites WRITE setFavorites NOTIFY favoritesChanged)
    Q_PROPERTY(int maxFavorites READ maxFavorites WRITE setMaxFavorites NOTIFY maxFavoritesChanged)

    Q_PROPERTY(QObject *activities READ activities CONSTANT)

public:
    explicit KAStatsFavoritesModel(QObject *parent = nullptr);
    ~KAStatsFavoritesModel() override;

    QString description() const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

    bool enabled() const;
    void setEnabled(bool enable);

    QStringList favorites() const;
    void setFavorites(const QStringList &favorites);

    int maxFavorites() const;
    void setMaxFavorites(int max);

    Q_INVOKABLE bool isFavorite(const QString &id) const;

    Q_INVOKABLE void addFavorite(const QString &id, int index = -1);
    Q_INVOKABLE void removeFavorite(const QString &id);

    Q_INVOKABLE void addFavoriteTo(const QString &id, const QString &activityId, int index = -1);
    Q_INVOKABLE void removeFavoriteFrom(const QString &id, const QString &activityId);

    Q_INVOKABLE void setFavoriteOn(const QString &id, const QString &activityId);

    Q_INVOKABLE void portOldFavorites(const QStringList &ids);

    Q_INVOKABLE QStringList linkedActivitiesFor(const QString &id) const;

    Q_INVOKABLE void moveRow(int from, int to);

    Q_INVOKABLE void initForClient(const QString &client);

    QObject *activities() const;
    Q_INVOKABLE QString activityNameForId(const QString &activityId) const;

    AbstractModel *favoritesModel() override;

public Q_SLOTS:
    void refresh() override;

Q_SIGNALS:
    void enabledChanged();
    void favoritesChanged();
    void maxFavoritesChanged();

private:
    class Private;
    Private *d;

    AbstractEntry *favoriteFromId(const QString &id) const;

    void addFavoriteTo(const QString &id, const KActivities::Stats::Terms::Activity &activityId, int index = -1);
    void removeFavoriteFrom(const QString &id, const KActivities::Stats::Terms::Activity &activityId);

    bool m_enabled;

    int m_maxFavorites;

    KActivities::Consumer *m_activities;
};
