/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SWITCHWINDOW_HEADER
#define SWITCHWINDOW_HEADER

#include "ui_config.h"
#include <plasma/containmentactions.h>

// libtaskmanager
#include <activityinfo.h>
#include <tasksmodel.h>

class QAction;
class QTimer;

namespace LegacyTaskManager
{
    class Task;
} // namespace LegacyTaskManager

class SwitchWindow : public Plasma::ContainmentActions
{
    Q_OBJECT
    public:
        SwitchWindow(QObject* parent, const QVariantList& args);
        ~SwitchWindow() override;

        void restore(const KConfigGroup &config) override;
        QWidget* createConfigurationInterface(QWidget* parent) override;
        void configurationAccepted() override;
        void save(KConfigGroup &config) override;

        void performNextAction() override;
        void performPreviousAction() override;
        void doSwitch(bool up);
        QList<QAction*> contextualActions() override;

    private:
        void makeMenu();

    private Q_SLOTS:
        void clearWindowsOrder();
        void switchTo(QAction *action);

    private:
        enum MenuMode {
            AllFlat = 0,
            DesktopSubmenus,
            CurrentDesktop
        };

        QList <QAction *> m_actions;
        TaskManager::ActivityInfo *m_activityInfo;
        TaskManager::TasksModel *m_tasksModel;
        Ui::Config m_ui;
        MenuMode m_mode;
        QTimer *m_clearOrderTimer;
        QList<WId> m_windowsOrder;
};

#endif
