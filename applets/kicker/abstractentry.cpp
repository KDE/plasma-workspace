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

AbstractEntry::~AbstractEntry() = default;

AbstractModel *AbstractEntry::owner() const
{
    return m_owner;
}

bool AbstractEntry::isValid() const
{
    return true;
}

QString AbstractEntry::icon() const
{
    return {};
}

QString AbstractEntry::name() const
{
    return {};
}

QString AbstractEntry::compactName() const
{
    return name();
}

QString AbstractEntry::group() const
{
    return {};
}

QString AbstractEntry::description() const
{
    return {};
}

QString AbstractEntry::id() const
{
    return {};
}

QUrl AbstractEntry::url() const
{
    return {};
}

QDate AbstractEntry::firstSeen() const
{
    return {};
}

bool AbstractEntry::isNewlyInstalled() const
{
    return false;
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
    return {};
}

bool AbstractEntry::run(const QString &actionId, const QVariant &argument)
{
    Q_UNUSED(actionId)
    Q_UNUSED(argument)

    return false;
}

void AbstractEntry::reload()
{
    return;
}

void AbstractEntry::refreshLabels()
{
    return;
}

AbstractGroupEntry::AbstractGroupEntry(AbstractModel *owner)
    : AbstractEntry(owner)
{
}

SeparatorEntry::SeparatorEntry(AbstractModel *owner)
    : AbstractEntry(owner)
{
}
