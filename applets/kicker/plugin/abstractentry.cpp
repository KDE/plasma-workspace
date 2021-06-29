/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractentry.h"

#include <QDebug>

AbstractEntry::AbstractEntry(AbstractModel *owner)
    : m_owner(owner)
{
}

AbstractEntry::~AbstractEntry()
{
}

AbstractModel *AbstractEntry::owner() const
{
    return m_owner;
}

bool AbstractEntry::isValid() const
{
    return true;
}

QIcon AbstractEntry::icon() const
{
    return QIcon();
}

QString AbstractEntry::name() const
{
    return QString();
}

QString AbstractEntry::group() const
{
    return QString();
}

QString AbstractEntry::description() const
{
    return QString();
}

QString AbstractEntry::id() const
{
    return QString();
}

QUrl AbstractEntry::url() const
{
    return QUrl();
}

bool AbstractEntry::hasChildren() const
{
    return false;
}

AbstractModel *AbstractEntry::childModel() const
{
    return nullptr;
}

bool AbstractEntry::hasActions() const
{
    return false;
}

QVariantList AbstractEntry::actions() const
{
    return QVariantList();
}

bool AbstractEntry::run(const QString &actionId, const QVariant &argument)
{
    Q_UNUSED(actionId)
    Q_UNUSED(argument)

    return false;
}

AbstractGroupEntry::AbstractGroupEntry(AbstractModel *owner)
    : AbstractEntry(owner)
{
}

SeparatorEntry::SeparatorEntry(AbstractModel *owner)
    : AbstractEntry(owner)
{
}
