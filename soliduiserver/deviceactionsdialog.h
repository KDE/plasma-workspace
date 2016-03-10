/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DEVICEACTIONSDIALOG_H
#define DEVICEACTIONSDIALOG_H

#include <kdialog.h>
#include <solid/device.h>

#include "ui_deviceactionsdialogview.h"

class DeviceAction;

class DeviceActionsDialog : public KDialog
{
    Q_OBJECT

public:
    DeviceActionsDialog(QWidget *parent=0);
    ~DeviceActionsDialog() override;

    void setDevice(const Solid::Device &device);
    Solid::Device device() const;

    void setActions(const QList<DeviceAction*> &actions);
    QList<DeviceAction*> actions() const;

private Q_SLOTS:
    void slotOk();

private:
    void launchAction(DeviceAction *action);
    void updateActionsListBox();

    Ui::DeviceActionsDialogView m_view;

    Solid::Device m_device;
    QList<DeviceAction*> m_actions;
};

#endif
