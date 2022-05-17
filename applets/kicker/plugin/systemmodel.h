/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"
#include "systementry.h"

class SystemModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit SystemModel(QObject *parent = nullptr);
    ~SystemModel() override;

    QString description() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

Q_SIGNALS:
    void sessionManagementStateChanged();

protected Q_SLOTS:
    void refresh() override;

private:
    void populate();

    QVector<SystemEntry *> m_entries;
    decltype(m_entries) m_invalidEntries;
};
