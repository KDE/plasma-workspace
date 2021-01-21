/* This file is part of the KDE project
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>
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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <KConfigDialog>

#include "urlgrabber.h"

#include "ui_actionsconfig.h"
#include "ui_generalconfig.h"

class KConfigSkeleton;
class KShortcutsEditor;
class Klipper;
class KEditListWidget;
class KActionCollection;
class EditActionDialog;

class GeneralWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GeneralWidget(QWidget *parent);
    void updateWidgets();

private:
    Ui::GeneralWidget m_ui;
};

class ActionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ActionsWidget(QWidget *parent);

    void setActionList(const ActionList &);
    void setExcludedWMClasses(const QStringList &);

    ActionList actionList() const;
    QStringList excludedWMClasses() const;

    void resetModifiedState();

private Q_SLOTS:
    void onSelectionChanged();
    void onAddAction();
    void onEditAction();
    void onDeleteAction();
    void onAdvanced();

private:
    void updateActionItem(QTreeWidgetItem *item, ClipAction *action);
    void updateActionListView();

    Ui::ActionsWidget m_ui;
    EditActionDialog *m_editActDlg;

    /**
     * List of actions this page works with
     */
    ActionList m_actionList;

    QStringList m_exclWMClasses;
};

// only for use inside ActionWidget
class AdvancedWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedWidget(QWidget *parent = nullptr);
    ~AdvancedWidget() override;

    void setWMClasses(const QStringList &items);
    QStringList wmClasses() const;

private:
    KEditListWidget *editListBox;
};

class ConfigDialog : public KConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget *parent, KConfigSkeleton *config, const Klipper *klipper, KActionCollection *collection);
    ~ConfigDialog() override;

private:
    // reimp
    void updateWidgets() override;
    // reimp
    void updateSettings() override;
    // reimp
    void updateWidgetsDefault() override;

private:
    GeneralWidget *m_generalPage;
    ActionsWidget *m_actionsPage;
    KShortcutsEditor *m_shortcutsWidget;

    const Klipper *m_klipper;
};

#endif // CONFIGDIALOG_H
