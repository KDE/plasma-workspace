/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractentry.h"

namespace KPeople
{
class PersonData;
}

class ContactEntry : public AbstractEntry
{
public:
    explicit ContactEntry(AbstractModel *owner, const QString &id);

    EntryType type() const override
    {
        return RunnableType;
    }

    bool isValid() const override;

    QIcon icon() const override;
    QString name() const override;

    QString id() const override;
    QUrl url() const override;

    bool hasActions() const override;
    QVariantList actions() const override;

    bool run(const QString &actionId = QString(), const QVariant &argument = QVariant()) override;

    static void showPersonDetailsDialog(const QString &id);

private:
    KPeople::PersonData *m_personData = nullptr;

    Q_DISABLE_COPY(ContactEntry)
};
