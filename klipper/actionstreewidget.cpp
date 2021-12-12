/*
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "actionstreewidget.h"

ActionsTreeWidget::ActionsTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
    , m_actionsChanged(-1)
    , m_modified(false)
{
    // these signals indicate that something was changed in actions tree

    connect(this, &ActionsTreeWidget::itemChanged, this, &ActionsTreeWidget::onItemChanged);
    QAbstractItemModel *treeModel = model();
    if (treeModel) {
        connect(treeModel, &QAbstractItemModel::rowsInserted, this, &ActionsTreeWidget::onItemChanged);
        connect(treeModel, &QAbstractItemModel::rowsRemoved, this, &ActionsTreeWidget::onItemChanged);
    }
    setProperty("kcfg_propertyNotify", true);
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

    if (!m_modified) {
        m_actionsChanged = m_actionsChanged ? 1 : 0;
        m_modified = true;
        Q_EMIT changed();
    }
}

int ActionsTreeWidget::actionsChanged() const
{
    return m_actionsChanged;
}
