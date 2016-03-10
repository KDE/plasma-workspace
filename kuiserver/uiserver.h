/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2000 Matej Koss <koss@miesto.sk>
  *                    David Faure <faure@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#ifndef UISERVER_H
#define UISERVER_H

#include <QListView>

#include <kxmlguiwindow.h>

#include "jobview.h"


#include <kuiserversettings.h>

class ProgressListModel;
class ProgressListDelegate;
class QToolBar;
class QSystemTrayIcon;

class UiServer : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit UiServer(ProgressListModel* model);
    ~UiServer() override;

public Q_SLOTS:
    void updateConfiguration();
    void applySettings();

protected:
    void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
    void showConfigurationDialog();

private:
    ProgressListDelegate *progressListDelegate;
    QListView *listProgress;

    QToolBar *toolBar;
    QSystemTrayIcon *m_systemTray;

};

#endif // UISERVER_H
