/*
    SPDX-FileCopyrightText: 2009 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QTableView;

class ClipAction;
class ActionDetailModel;

class EditActionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditActionDialog(QWidget *parent);
    ~EditActionDialog() override = default;

    /**
     * Sets the action this dialog will work with
     */
    void setAction(ClipAction *act, int commandIdxToSelect = -1);

private Q_SLOTS:
    void onAddCommand();
    void onEditCommand();
    void onRemoveCommand();
    void onSelectionChanged();
    void slotAccepted();

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
    QLineEdit *m_regExpEdit;
    QLineEdit *m_descriptionEdit;
    QCheckBox *m_automaticCheck;

    QTableView *m_commandList;
    QPushButton *m_addCommandPb;
    QPushButton *m_editCommandPb;
    QPushButton *m_removeCommandPb;

    ClipAction *m_action;
    ActionDetailModel *m_model;
};
