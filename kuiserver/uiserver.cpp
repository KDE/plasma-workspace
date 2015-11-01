/*
  * This file is part of the KDE project
  * Copyright (C) 2009 Shaun Reich <shaun.reich@kdemail.net>
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2001 George Staikos <staikos@kde.org>
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

#include "uiserver.h"
#include "uiserver_p.h"

#include "progresslistmodel.h"
#include "progresslistdelegate.h"

#include <QWidget>
#include <QAction>
#include <QCloseEvent>
#include <qsystemtrayicon.h>
#include <QToolBar>
#include <kconfigdialog.h>

UiServer::UiServer(ProgressListModel* model)
        : KXmlGuiWindow(0), m_systemTray(0)
{
    //NOTE: if enough people really hate this dialog (having centralized information and such),
    //I imagine we could somehow forward it to the old dialogs, which would be displayed 1 for each job.
    //Or create our own. no worries, we'll see how it plays out..

    QString configure = i18n("Configure...");

    toolBar = addToolBar(configure);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *configureAction = toolBar->addAction(configure);
    configureAction->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    configureAction->setIconText(configure);

    connect(configureAction, &QAction::triggered, this, &UiServer::showConfigurationDialog);

    toolBar->addSeparator();

    listProgress = new QListView(this);
    listProgress->setAlternatingRowColors(true);
    listProgress->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listProgress->setUniformItemSizes(true);
    listProgress->setSelectionMode(QAbstractItemView::NoSelection);
    listProgress->setModel(model);

    setCentralWidget(listProgress);

    progressListDelegate = new ProgressListDelegate(this, listProgress);
    progressListDelegate->setSeparatorPixels(5);
    progressListDelegate->setLeftMargin(10);
    progressListDelegate->setRightMargin(10);
    progressListDelegate->setMinimumItemHeight(100);
    progressListDelegate->setMinimumContentWidth(300);
    progressListDelegate->setEditorHeight(20);
    listProgress->setItemDelegate(progressListDelegate);


    m_systemTray = new QSystemTrayIcon(this);
    m_systemTray->setIcon(QIcon::fromTheme(QStringLiteral("view-process-system")));
    m_systemTray->setToolTip(i18n("List of running file transfers/jobs (kuiserver)"));
    m_systemTray->show();
    resize(450, 450);
    applySettings();
}

UiServer::~UiServer()
{
}


void UiServer::updateConfiguration()
{
    Configuration::self()->save();
    applySettings();
}

void UiServer::applySettings()
{
    /* not used.
     int finishedIndex = tabWidget->indexOf(listFinished);
     if (Configuration::radioMove()) {
         if (finishedIndex == -1) {
             tabWidget->addTab(listFinished, i18n("Finished"));
         }
     } else if (finishedIndex != -1) {
         tabWidget->removeTab(finishedIndex);
     } */
}

void UiServer::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void UiServer::showConfigurationDialog()
{
    if (KConfigDialog::showDialog(QStringLiteral("configuration")))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, QStringLiteral("configuration"),
            Configuration::self());

    UIConfigurationDialog *configurationUI = new UIConfigurationDialog(0);

    dialog->addPage(configurationUI, i18n("Behavior"), QStringLiteral("configure"));

    connect(dialog, &KConfigDialog::settingsChanged, this, &UiServer::updateConfiguration);
    //dialog->button(KDialog::Help)->hide();
    dialog->show();
}

/// ===========================================================


UIConfigurationDialog::UIConfigurationDialog(QWidget *parent)
        : QWidget(parent)
{
    setupUi(this);
    adjustSize();
}

UIConfigurationDialog::~UIConfigurationDialog()
{
}


/// ===========================================================



//
