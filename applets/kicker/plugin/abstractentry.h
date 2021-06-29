/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"

#include <QIcon>
#include <QUrl>

class AbstractEntry
{
public:
    explicit AbstractEntry(AbstractModel *owner);
    virtual ~AbstractEntry();

    enum EntryType {
        RunnableType,
        GroupType,
        SeparatorType,
    };

    virtual EntryType type() const = 0;

    AbstractModel *owner() const;

    virtual bool isValid() const;

    virtual QIcon icon() const;
    virtual QString name() const;
    virtual QString group() const;
    virtual QString description() const;

    virtual QString id() const;
    virtual QUrl url() const;

    virtual bool hasChildren() const;
    virtual AbstractModel *childModel() const;

    virtual bool hasActions() const;
    virtual QVariantList actions() const;

    virtual bool run(const QString &actionId = QString(), const QVariant &argument = QVariant());

protected:
    AbstractModel *m_owner;
};

class AbstractGroupEntry : public AbstractEntry
{
public:
    explicit AbstractGroupEntry(AbstractModel *owner);

    EntryType type() const override
    {
        return GroupType;
    }
};

class SeparatorEntry : public AbstractEntry
{
public:
    explicit SeparatorEntry(AbstractModel *owner);

    EntryType type() const override
    {
        return SeparatorType;
    }
};
