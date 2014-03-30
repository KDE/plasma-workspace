/*
   This file is part of the KDE project
   Copyright (C) 2008 by Dmitry Suzdalev <dimsuz@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "actionstreewidget.h"
#include <KDebug>

ActionsTreeWidget::ActionsTreeWidget(QWidget* parent)
    : QTreeWidget(parent), m_actionsChanged(-1), m_modified(false)
{
    // these signals indicate that something was changed in actions tree

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), SLOT(onItemChanged()));
    QAbstractItemModel *treeModel = model();
    if (treeModel)
    {
        connect(treeModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(onItemChanged()));
        connect(treeModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(onItemChanged()));
    }
}

void ActionsTreeWidget::onItemChanged()
{
    setActionsChanged(true);
}

void ActionsTreeWidget::resetModifiedState()
{
    m_modified = false;
    m_actionsChanged = -1;
}

void ActionsTreeWidget::setActionsChanged(int isChanged)
{
    Q_UNUSED(isChanged)

    if (!m_modified)
    {
        m_actionsChanged = m_actionsChanged ? 1 : 0;
        m_modified = true;
        emit changed();
    }
}

int ActionsTreeWidget::actionsChanged() const
{
    return m_actionsChanged;
}

