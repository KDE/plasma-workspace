/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "forwardingmodel.h"

namespace KPeople
{
class PersonData;
}

class RecentContactsModel : public ForwardingModel
{
    Q_OBJECT

public:
    explicit RecentContactsModel(QObject *parent = nullptr);
    ~RecentContactsModel() override;

    QString description() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

    bool hasActions() const override;
    QVariantList actions() const override;

private Q_SLOTS:
    void refresh() override;
    void buildCache();
    void personDataChanged();

private:
    void insertPersonData(const QString &id, int row);

    QHash<QString, KPeople::PersonData *> m_idToData;
    QHash<KPeople::PersonData *, int> m_dataToRow;
};
