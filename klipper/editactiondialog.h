/*
   This file is part of the KDE project
   Copyright (C) 2009 by Dmitry Suzdalev <dimsuz@gmail.com>

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

#ifndef EDIT_ACTION_DIALOG_H
#define EDIT_ACTION_DIALOG_H

#include <QDialog>

namespace Ui
{
    class EditActionDialog;
}

class ClipAction;
class ActionDetailModel;

class EditActionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditActionDialog(QWidget* parent);
    ~EditActionDialog() override;

    /**
     * Sets the action this dialog will work with
     */
    void setAction(ClipAction* act, int commandIdxToSelect = -1);

private Q_SLOTS:
    void onAddCommand();
    void onRemoveCommand();
    void onSelectionChanged();
    void slotAccepted();
//    void onItemChanged( QTreeWidgetItem*, int );

private:
    /**
     * Updates dialog's widgets according to values
     * in m_action.
     * If commandIdxToSelect != -1 this command will be preselected
     */
    void updateWidgets(int commandIdxToSelect);

    /**
     * Saves a values from widgets to action
     */
    void saveAction();

private:
    Ui::EditActionDialog* m_ui;

    ClipAction* m_action;
    ActionDetailModel* m_model;
};
#endif
