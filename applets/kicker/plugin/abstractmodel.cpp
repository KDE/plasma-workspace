/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "abstractmodel.h"
#include "actionlist.h"

AbstractModel::AbstractModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

AbstractModel::~AbstractModel()
{
}

QHash<int, QByteArray> AbstractModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::DecorationRole, "decoration");
    roles.insert(Kicker::GroupRole, "group");
    roles.insert(Kicker::DescriptionRole, "description");
    roles.insert(Kicker::FavoriteIdRole, "favoriteId");
    roles.insert(Kicker::IsParentRole, "isParent");
    roles.insert(Kicker::IsSeparatorRole, "isSeparator");
    roles.insert(Kicker::HasChildrenRole, "hasChildren");
    roles.insert(Kicker::HasActionListRole, "hasActionList");
    roles.insert(Kicker::ActionListRole, "actionList");
    roles.insert(Kicker::UrlRole, "url");
    roles.insert(Kicker::DisabledRole, "disabled");
    roles.insert(Kicker::IsMultilineTextRole, "isMultilineText");

    return roles;
}

int AbstractModel::count() const
{
    return rowCount();
}

int AbstractModel::separatorCount() const
{
    return 0;
}

int AbstractModel::iconSize() const
{
    return m_iconSize;
}

void AbstractModel::setIconSize(int iconSize)
{
    if (m_iconSize != iconSize) {
        m_iconSize = iconSize;
        refresh();
    }
}

void AbstractModel::refresh()
{
}

QString AbstractModel::labelForRow(int row)
{
    return data(index(row, 0), Qt::DisplayRole).toString();
}

AbstractModel *AbstractModel::modelForRow(int row)
{
    Q_UNUSED(row)

    return nullptr;
}

int AbstractModel::rowForModel(AbstractModel *model)
{
    Q_UNUSED(model)

    return -1;
}

bool AbstractModel::hasActions() const
{
    return false;
}

QVariantList AbstractModel::actions() const
{
    return QVariantList();
}

AbstractModel *AbstractModel::favoritesModel()
{
    if (m_favoritesModel) {
        return m_favoritesModel;
    } else {
        AbstractModel *model = rootModel();

        if (model && model != this) {
            return model->favoritesModel();
        }
    }

    return nullptr;
}

void AbstractModel::setFavoritesModel(AbstractModel *model)
{
    if (m_favoritesModel != model) {
        m_favoritesModel = model;

        Q_EMIT favoritesModelChanged();
    }
}

AbstractModel *AbstractModel::rootModel()
{
    if (!parent()) {
        return nullptr;
    }

    QObject *p = this;
    AbstractModel *rootModel = nullptr;

    while (p) {
        if (qobject_cast<AbstractModel *>(p)) {
            rootModel = qobject_cast<AbstractModel *>(p);
        } else {
            return rootModel;
        }

        p = p->parent();
    }

    return rootModel;
}

QVariantList AbstractModel::sections() const
{
    return {};
}

void AbstractModel::entryChanged(AbstractEntry *entry)
{
    Q_UNUSED(entry)
}
