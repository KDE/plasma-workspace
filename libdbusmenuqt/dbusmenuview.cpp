/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbusmenuview.h"
#include "dbusmenumodel.h"
#include "dbusmenuview_p.h"

#include <QActionGroup>

DBusMenuView::DBusMenuView(QWidget *parent)
    : QMenu(parent)
    , d(std::make_unique<DBusMenuViewPrivate>())
{
    connect(this, &QMenu::aboutToShow, this, &DBusMenuView::onAboutToShow);
    connect(this, &QMenu::aboutToHide, this, &DBusMenuView::onAboutToHide);
}

DBusMenuModel *DBusMenuView::model() const
{
    return d->model;
}

QPersistentModelIndex DBusMenuView::rootIndex() const
{
    return d->rootIndex;
}

void DBusMenuView::setRoot(DBusMenuModel *model, const QPersistentModelIndex &index)
{
    if (d->model != model) {
        if (d->model) {
            disconnect(d->model, &QAbstractItemModel::rowsInserted, this, &DBusMenuView::onRowsInserted);
            disconnect(d->model, &QAbstractItemModel::rowsMoved, this, &DBusMenuView::onRowsMoved);
            disconnect(d->model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &DBusMenuView::onRowsAboutToBeRemoved);
            disconnect(d->model, &QAbstractItemModel::modelReset, this, &DBusMenuView::onModelReset);
            disconnect(d->model, &QAbstractItemModel::dataChanged, this, &DBusMenuView::onDataChanged);
        }

        if (model) {
            connect(model, &QAbstractItemModel::rowsInserted, this, &DBusMenuView::onRowsInserted);
            connect(model, &QAbstractItemModel::rowsMoved, this, &DBusMenuView::onRowsMoved);
            connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &DBusMenuView::onRowsAboutToBeRemoved);
            connect(model, &QAbstractItemModel::modelReset, this, &DBusMenuView::onModelReset);
            connect(model, &QAbstractItemModel::dataChanged, this, &DBusMenuView::onDataChanged);
        }

        if (isVisible()) {
            model->open(d->rootIndex);
            d->model->close(d->rootIndex);
        }
    } else if (d->rootIndex != index) {
        if (isVisible()) {
            d->model->close(d->rootIndex);
            d->model->open(index);
        }
    } else {
        return;
    }

    d->model = model;
    d->rootIndex = index;
    d->rootIndexValid = index.isValid();

    onModelReset();
}

QAction *DBusMenuView::actionByIndex(const QModelIndex &index) const
{
    return d->actions.value(index);
}

QMenu *DBusMenuView::menuByIndex(const QModelIndex &index)
{
    if (d->rootIndex == index && d->rootIndexValid == index.isValid()) {
        return this;
    } else if (auto action = actionByIndex(index)) {
        return action->menu();
    } else {
        return nullptr;
    }
}

QModelIndex DBusMenuView::indexByAction(QAction *action) const
{
    return d->actions.key(action);
}

QModelIndex DBusMenuView::indexByMenu(QMenu *menu) const
{
    if (menu == this) {
        return d->rootIndex;
    } else {
        return indexByAction(menu->menuAction());
    }
}

void DBusMenuView::addAction(QMenu *menu, const QModelIndex &parentIndex, int row)
{
    const QModelIndex actionIndex = d->model->index(row, 0, parentIndex);
    if (actionIndex.data(DBusMenuModel::SeparatorRole).toBool()) {
        QAction *separator = menu->addSeparator();
        d->actions.insert(actionIndex, separator);
    } else {
        QAction *action = menu->addAction(actionIndex.data(Qt::DecorationRole).value<QIcon>(), actionIndex.data(Qt::DisplayRole).value<QString>());
        d->actions.insert(actionIndex, action);

        action->setEnabled(actionIndex.data(DBusMenuModel::EnabledRole).value<bool>());
        action->setVisible(actionIndex.data(DBusMenuModel::VisibleRole).value<bool>());
        action->setShortcut(actionIndex.data(DBusMenuModel::ShortcutRole).value<QKeySequence>());

        const QString toggleType = actionIndex.data(DBusMenuModel::ToggleTypeRole).value<QString>();
        const bool checked = actionIndex.data(DBusMenuModel::CheckedRole).value<bool>();
        if (toggleType == QLatin1String("radio")) {
            action->setCheckable(true);
            action->setChecked(checked);

            auto group = new QActionGroup(action);
            group->addAction(action);
        } else if (toggleType == QLatin1String("checkmark")) {
            action->setCheckable(true);
            action->setChecked(checked);
        }

        connect(action, &QAction::triggered, this, [this, action]() {
            d->model->click(indexByAction(action));
        });

        if (actionIndex.data(DBusMenuModel::SubmenuRole).toBool()) {
            QMenu *submenu = new QMenu(menu);
            connect(submenu, &QMenu::aboutToShow, this, [this, submenu]() {
                d->model->open(indexByMenu(submenu));
            });
            connect(submenu, &QMenu::aboutToHide, this, [this, submenu]() {
                d->model->close(indexByMenu(submenu));
            });
            action->setMenu(submenu);

            buildSubTree(submenu, actionIndex);
        }
    }
}

static void activateNextAction(QMenu *menu, QAction *referenceAction)
{
    const auto actions = menu->actions();
    const int referenceIndex = actions.indexOf(referenceAction);
    if (referenceIndex != -1) {
        const int nextIndex = (referenceIndex + 1) % actions.size();
        if (nextIndex != referenceIndex) {
            menu->setActiveAction(actions[nextIndex]);
        }
    }
}

static bool isPlaceholderAction(const QAction *action)
{
    return action->property("placeholder").toBool();
}

static QAction *maybeAddPlaceholder(QMenu *menu)
{
    const auto actions = menu->actions();
    for (QAction *action : actions) {
        if (isPlaceholderAction(action)) {
            return action;
        }
    }

    QAction *placeholder = menu->addAction(QString());
    placeholder->setProperty("placeholder", true);
    return placeholder;
}

void DBusMenuView::buildSubTree(QMenu *menu, const QModelIndex &index)
{
    if (const int rowCount = d->model->rowCount(index)) {
        const auto previousActions = menu->actions();
        for (int row = 0; row < rowCount; ++row) {
            addAction(menu, index, row);
        }

        for (QAction *previousAction : previousActions) {
            if (menu->activeAction() == previousAction) {
                activateNextAction(menu, previousAction);
            }

            pruneSubTree(menu, previousAction);
        }
    } else {
        // Placeholder is needed to prevent QMenu from hiding if it becomes empty.
        QAction *placeholderAction = maybeAddPlaceholder(menu);
        for (const auto actions = menu->actions(); QAction *action : actions) {
            if (action != placeholderAction) {
                pruneSubTree(menu, action);
            }
        }
    }
}

void DBusMenuView::pruneSubTree(QMenu *menu, QAction *action)
{
    if (QMenu *submenu = action->menu()) {
        const QList<QAction *> actions = submenu->actions();
        for (QAction *action : actions) {
            pruneSubTree(submenu, action);
        }

        submenu->hide();
        delete submenu;
    }

    menu->removeAction(action);
    d->actions.removeIf([action](const auto it) {
        return *it == action;
    });

    delete action;
}

void DBusMenuView::onAboutToShow()
{
    d->model->open(d->rootIndex);
}

void DBusMenuView::onAboutToHide()
{
    d->model->close(d->rootIndex);
}

void DBusMenuView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    QMenu *menu = menuByIndex(parent);
    if (!menu) {
        return;
    }

    for (int row = first; row <= last; ++row) {
        addAction(menu, parent, row);
    }

    const auto actions = menu->actions();
    for (QAction *action : actions) {
        if (isPlaceholderAction(action)) {
            if (menu->activeAction() == action) {
                activateNextAction(menu, action);
            }

            menu->removeAction(action);
            delete action;
        }
    }
}

void DBusMenuView::onRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    QMenu *sourceMenu = menuByIndex(sourceParent);
    if (!sourceMenu) {
        return;
    }

    QMenu *destinationMenu = menuByIndex(destinationParent);
    if (!destinationMenu) {
        return;
    }

    const QList<QAction *> sourceActions = sourceMenu->actions();
    const QList<QAction *> destinationActions = destinationMenu->actions();

    QList<QAction *> actions;
    for (int row = sourceStart; row <= sourceEnd; ++row) {
        sourceMenu->removeAction(sourceActions[row]);
        actions.append(sourceActions[row]);
    }

    int beforeRow = destinationRow;
    if (sourceMenu != destinationMenu) {
        beforeRow += sourceEnd - sourceStart + 1;
    }

    if (destinationActions.size() == beforeRow) {
        destinationMenu->addActions(actions);
    } else {
        destinationMenu->insertActions(destinationActions[beforeRow], actions);
    }
}

void DBusMenuView::onRowsAboutToBeRemoved(const QModelIndex &parentIndex, int first, int last)
{
    QMenu *menu = menuByIndex(parentIndex);
    if (!menu) {
        return;
    }

    const auto actions = menu->actions();
    for (int row = last; row >= first; --row) {
        pruneSubTree(menu, actions[row]);
    }
}

void DBusMenuView::onModelReset()
{
    d->actions.clear();
    buildSubTree(this, d->rootIndex);
}

void DBusMenuView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    QMenu *menu = menuByIndex(topLeft.parent());
    if (!menu) {
        return;
    }

    const QModelIndex parentIndex = topLeft.parent();
    const int firstRow = topLeft.row();
    const int lastRow = bottomRight.row();

    QList<int> effectiveRoles = roles;
    if (effectiveRoles.isEmpty()) {
        effectiveRoles = model()->roleNames().keys();
    }

    for (int row = firstRow; row <= lastRow; ++row) {
        const QModelIndex actionIndex = d->model->index(row, 0, parentIndex);
        QAction *action = menu->actions().at(row);
        for (const int &role : effectiveRoles) {
            switch (role) {
            case Qt::DecorationRole:
                action->setIcon(actionIndex.data(Qt::DecorationRole).value<QIcon>());
                break;

            case Qt::DisplayRole:
                action->setText(actionIndex.data(Qt::DisplayRole).value<QString>());
                break;

            case DBusMenuModel::EnabledRole:
                action->setEnabled(actionIndex.data(DBusMenuModel::EnabledRole).value<bool>());
                break;

            case DBusMenuModel::VisibleRole:
                action->setVisible(actionIndex.data(DBusMenuModel::VisibleRole).value<bool>());
                break;

            case DBusMenuModel::CheckedRole:
                action->setChecked(actionIndex.data(DBusMenuModel::CheckedRole).value<bool>());
                break;

            case DBusMenuModel::ShortcutRole:
                action->setShortcut(actionIndex.data(DBusMenuModel::ShortcutRole).value<QKeySequence>());
                break;
            }
        }
    }
}

#include "moc_dbusmenuview.cpp"
