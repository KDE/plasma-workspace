/*
    SPDX-FileCopyrightText: 2022 Jonathan Marten <jjm@keelhaul.me.uk>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <qdialog.h>

#include "urlgrabber.h"

class QLineEdit;
class QRadioButton;
class KIconButton;

class EditCommandDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCommandDialog(const ClipCommand &command, QWidget *parent);
    ~EditCommandDialog() override = default;

    /**
     * Retrieves the updated command
     */
    const ClipCommand &command() const
    {
        return (m_command);
    }

private Q_SLOTS:
    void slotAccepted();
    void slotUpdateButtons();

private:
    /**
     * Updates dialog's widgets according to values in m_command
     */
    void updateWidgets();

    /**
     * Saves values from widgets to m_command
     */
    void saveCommand();

private:
    ClipCommand m_command;

    QLineEdit *m_commandEdit;
    QLineEdit *m_descriptionEdit;

    QRadioButton *m_ignoreRadio;
    QRadioButton *m_appendRadio;
    QRadioButton *m_replaceRadio;
    KIconButton *m_iconButton;

    QPushButton *m_okButton;
};
