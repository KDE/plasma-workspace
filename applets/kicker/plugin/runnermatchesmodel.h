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
    Q_PROPERTY(QString description READ description CONSTANT)

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

    void setMatches(const QList<KRunner::QueryMatch> &matches);

    // AbstractModel *favoritesModel() override;

    Q_SIGNAL void requestUpdateQueryString(const QString &term);

private:
    const QString m_runnerId;
    QString m_name;
};
