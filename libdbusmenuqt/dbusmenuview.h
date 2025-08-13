/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMenu>
#include <QPersistentModelIndex>

class DBusMenuModel;
class DBusMenuViewPrivate;

/**
 * The DBusMenuView provides a model/view implementation for dbus menus.
 *
 * Example usage
 *
 * @code
 * auto model = new DBusMenuModel(serviceName, objectPath);
 * model->setPrefetchSize(2);
 *
 * auto popup = new DBusMenuView();
 * popup->setRoot(model, QModelIndex());
 * popup->popup(globalPos);
 * @endcode
 */
class DBusMenuView : public QMenu
{
    Q_OBJECT

public:
    explicit DBusMenuView(QWidget *parent = nullptr);

    DBusMenuModel *model() const;
    QPersistentModelIndex rootIndex() const;
    void setRoot(DBusMenuModel *model, const QPersistentModelIndex &index);

private:
    void onAboutToShow();
    void onAboutToHide();

    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onModelReset();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

    QAction *actionByIndex(const QModelIndex &index) const;
    QMenu *menuByIndex(const QModelIndex &index);
    QModelIndex indexByAction(QAction *action) const;
    QModelIndex indexByMenu(QMenu *menu) const;

    void addAction(QMenu *menu, const QModelIndex &parent, int row);
    void buildSubTree(QMenu *menu, const QModelIndex &index);
    void pruneSubTree(QMenu *menu, QAction *action);

    std::unique_ptr<DBusMenuViewPrivate> d;
};
