/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ui_config.h"
#include <plasma/containmentactions.h>

class QAction;

namespace TaskManager
{
class ActivityInfo;
class TasksModel;
class VirtualDesktopInfo;
}

class SwitchWindow : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    explicit SwitchWindow(QObject *parent, const QVariantList &args);
    ~SwitchWindow() override;

    void restore(const KConfigGroup &config) override;
    QWidget *createConfigurationInterface(QWidget *parent) override;
    void configurationAccepted() override;
    void save(KConfigGroup &config) override;

    void performNextAction() override;
    void performPreviousAction() override;
    void doSwitch(bool up);
    QList<QAction *> contextualActions() override;

private:
    void makeMenu();

private Q_SLOTS:
    void switchTo(QAction *action);

private:
    enum MenuMode {
        AllFlat = 0,
        DesktopSubmenus,
        CurrentDesktop,
    };

    QList<QAction *> m_actions;
    Ui::Config m_ui;
    MenuMode m_mode;

    TaskManager::VirtualDesktopInfo *const m_virtualDesktopInfo;

    static TaskManager::ActivityInfo *s_activityInfo;
    static TaskManager::TasksModel *s_tasksModel;
    static int s_instanceCount;
};
