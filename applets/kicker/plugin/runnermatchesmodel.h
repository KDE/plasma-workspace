/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"

#include <KRunner/QueryMatch>

namespace Plasma
{
class RunnerManager;
}

class RunnerMatchesModel : public AbstractModel
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT)

public:
    explicit RunnerMatchesModel(const QString &runnerId, const QString &name, Plasma::RunnerManager *manager, QObject *parent = nullptr);

    QString description() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

    QString runnerId() const
    {
        return m_runnerId;
    }
    QString name() const
    {
        return m_name;
    }

    void setMatches(const QList<Plasma::QueryMatch> &matches);

    AbstractModel *favoritesModel() override;

    Q_SIGNAL void requestUpdateQueryString(const QString &term);

private:
    QString m_runnerId;
    QString m_name;
    Plasma::RunnerManager *m_runnerManager;
    QList<Plasma::QueryMatch> m_matches;
};
