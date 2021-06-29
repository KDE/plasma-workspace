/*
    SPDX-FileCopyrightText: 2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

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
    explicit EditActionDialog(QWidget *parent);
    ~EditActionDialog() override;

    /**
     * Sets the action this dialog will work with
     */
    void setAction(ClipAction *act, int commandIdxToSelect = -1);

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
    Ui::EditActionDialog *m_ui;

    ClipAction *m_action;
    ActionDetailModel *m_model;
};