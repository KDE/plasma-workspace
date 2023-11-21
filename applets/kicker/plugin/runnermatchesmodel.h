/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"

#include <KRunner/QueryMatch>
#include <KRunner/ResultsModel>
#include <optional>

namespace KRunner
{
class RunnerManager;
}

class RunnerMatchesModel : public KRunner::ResultsModel
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(AbstractModel *favoritesModel READ favoritesModel WRITE setFavoritesModel NOTIFY favoritesModelChanged)

public:
    explicit RunnerMatchesModel(const QString &runnerId, const std::optional<QString> &name, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument);

    QString name() const
    {
        return m_name;
    }
    QString description() const
    {
        return name();
    }

    Q_SIGNAL void countChanged();
    int count() const
    {
        return rowCount();
    }

    AbstractModel *favoritesModel() const
    {
        return m_favoritesModel;
    }
    void setFavoritesModel(AbstractModel *model);
    Q_SIGNAL void favoritesModelChanged() const;

    void setMatches(const QList<KRunner::QueryMatch> &matches);

    QHash<int, QByteArray> roleNames() const override
    {
        return AbstractModel::staticRoleNames();
    }

    Q_SIGNAL void requestUpdateQueryString(const QString &term);

private:
    AbstractModel *m_favoritesModel = nullptr;
    const QString m_runnerId;
    QString m_name;
};
