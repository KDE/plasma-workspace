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
#ifndef ACTIONSTREEWIDGET_H
#define ACTIONSTREEWIDGET_H

#include <QTreeWidget>

/**
 * Custom tree widget class to make KConfigDialog properly
 * highlight Apply button when some action is changed.
 * We achieve this by adding custom type handling to KConfigDialogManager
 * and by adding a somewhat dummy config entry which gets changed whenever
 * some action is changed in treewidget.
 * KConfigDialog watches this entry for changes and highlights Apply when needed
 *
 * @see KConfigDialogManager
 */
class ActionsTreeWidget : public QTreeWidget
{
    Q_OBJECT

    // this property is int instead of (more logical) bool, because we need a custom handling of
    // "default state" and because of our custom use of this property:
    //
    // We indicate that changes were made to this widget by changing this int value.
    // We use it as "if this value is *CHANGED SOMEHOW*, this means that some changes were made to action list",
    // If we'd make this property bool, KConfigDialog would highlight "Defaults" button whenever
    // this property becomes false, but this is not the way we use this property.
    // So we change it from 0 to 1 periodically when something changes. Both 0, 1 values indicate
    // change.
    //
    // We set it to default only when resetModifiedState() is called, i.e. when Apply btn is being
    // clicked
    //
    // Hope this explains it.
    // Yeah, this class is a trick :) If there's a better way to properly
    // update KConfigDialog buttons whenever "some change occurs to QTreeWidget", let me know (dimsuz)
    Q_PROPERTY(int actionsChanged READ actionsChanged WRITE setActionsChanged USER true)

public:
    explicit ActionsTreeWidget(QWidget *parent = nullptr);

    void setActionsChanged(int);
    int actionsChanged() const;

    void resetModifiedState();

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void onItemChanged();

private:
    int m_actionsChanged;
    bool m_modified;
};

#endif
