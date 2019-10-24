/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
 *   Copyright 2018 by Eike Hein <hein@kde.org>
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

#include "switch.h"

#include "abstracttasksmodel.h"
#include "activityinfo.h"
#include "tasksmodel.h"
#include "virtualdesktopinfo.h"

#include <QAction>
#include <QMenu>

using namespace TaskManager;

ActivityInfo *SwitchWindow::s_activityInfo = nullptr;
TasksModel *SwitchWindow::s_tasksModel = nullptr;int SwitchWindow::s_instanceCount = 0;

SwitchWindow::SwitchWindow(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
    , m_mode(AllFlat)
    , m_virtualDesktopInfo(new VirtualDesktopInfo(this))
{
    ++s_instanceCount;

    if (!s_activityInfo) {
        s_activityInfo = new ActivityInfo();
    }

    if (!s_tasksModel) {
        s_tasksModel = new TasksModel();

        s_tasksModel->setGroupMode(TasksModel::GroupDisabled);

        s_tasksModel->setActivity(s_activityInfo->currentActivity());
        s_tasksModel->setFilterByActivity(true);
        connect(s_activityInfo, &ActivityInfo::currentActivityChanged,
            this, []() { s_tasksModel->setActivity(s_activityInfo->currentActivity()); });
    }
}

SwitchWindow::~SwitchWindow()
{
    --s_instanceCount;

    if (!s_instanceCount) {
        delete s_activityInfo;
        s_activityInfo = nullptr;
        delete s_tasksModel;
        s_tasksModel = nullptr;
    }

    qDeleteAll(m_actions);
}

void SwitchWindow::restore(const KConfigGroup &config)
{
    m_mode = (MenuMode)config.readEntry("mode", (int)AllFlat);
}

QWidget *SwitchWindow::createConfigurationInterface(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);
    widget->setWindowTitle(i18nc("plasma_containmentactions_switchwindow", "Configure Switch Window Plugin"));
    switch (m_mode) {
        case AllFlat:
            m_ui.flatButton->setChecked(true);
            break;
        case DesktopSubmenus:
            m_ui.subButton->setChecked(true);
            break;
        case CurrentDesktop:
            m_ui.curButton->setChecked(true);
            break;
    }
    return widget;
}

void SwitchWindow::configurationAccepted()
{
    if (m_ui.flatButton->isChecked()) {
        m_mode = AllFlat;
    } else if (m_ui.subButton->isChecked()) {
        m_mode = DesktopSubmenus;
    } else {
        m_mode = CurrentDesktop;
    }
}

void SwitchWindow::save(KConfigGroup &config)
{
    config.writeEntry("mode", (int)m_mode);
}

void SwitchWindow::makeMenu()
{
    qDeleteAll(m_actions);
    m_actions.clear();

    if (s_tasksModel->rowCount() == 0) {
        return;
    }

    QMultiMap<QVariant, QAction *> desktops;
    QList<QAction *> allDesktops;

    // Make all the window actions.
    for (int i = 0; i < s_tasksModel->rowCount(); ++i) {
        const QModelIndex &idx = s_tasksModel->index(i, 0);

        if (!idx.data(AbstractTasksModel::IsWindow).toBool()) {
            continue;
        }

        const QString &name = idx.data().toString();

        if (name.isEmpty()) {
            continue;
        }

        QAction *action = new QAction(name, this);
        action->setIcon(idx.data(Qt::DecorationRole).value<QIcon>());
        action->setData(idx.data(AbstractTasksModel::WinIdList).toList());

        const QVariantList &desktopList = idx.data(AbstractTasksModel::VirtualDesktops).toList();

        for (const QVariant &desktop : desktopList) {
            desktops.insert(desktop, action);
        }

        if (idx.data(AbstractTasksModel::IsOnAllVirtualDesktops).toBool()) {
            allDesktops << action;
        }

        connect(action, &QAction::triggered, [=]() {
            switchTo(action);
        });
    }

    // Sort into menu(s).
    if (m_mode == CurrentDesktop) {
        const QVariant &currentDesktop = m_virtualDesktopInfo->currentDesktop();

        QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "Windows"), this);
        a->setSeparator(true);
        m_actions << a;
        m_actions << desktops.values(currentDesktop);
        m_actions << allDesktops;

    } else {
        const QVariantList &desktopIds = m_virtualDesktopInfo->desktopIds();
        const QStringList &desktopNames = m_virtualDesktopInfo->desktopNames();

        if (m_mode == AllFlat) {
            for (int i = 0; i < desktopIds.count(); ++i) {
                const QVariant &desktop = desktopIds.at(i);

                if (desktops.contains(desktop)) {
                    const QString &name = QStringLiteral("%1: %2").arg(QString::number(i + 1), desktopNames.at(i));
                    QAction *a = new QAction(name, this);
                    a->setSeparator(true);
                    m_actions << a;
                    m_actions << desktops.values(desktop);
                }
            }

            if (allDesktops.count()) {
                QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "All Desktops"), this);
                a->setSeparator(true);
                m_actions << a;
                m_actions << allDesktops;
            }
        } else { // Submenus.
            for (int i = 0; i < desktopIds.count(); ++i) {
                const QVariant &desktop = desktopIds.at(i);

                if (desktops.contains(desktop)) {
                    const QString &name = QStringLiteral("%1: %2").arg(QString::number(i + 1), desktopNames.at(i));
                    QMenu *subMenu = new QMenu(name);
                    subMenu->addActions(desktops.values(desktop));

                    QAction *a = new QAction(name, this);
                    a->setMenu(subMenu);
                    m_actions << a;
                }
            }

            if (allDesktops.count()) {
                QMenu *subMenu = new QMenu(i18nc("plasma_containmentactions_switchwindow", "All Desktops"));
                subMenu->addActions(allDesktops);
                QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "All Desktops"), this);
                a->setMenu(subMenu);
                m_actions << a;
            }
        }
    }
}

QList<QAction *> SwitchWindow::contextualActions()
{
    makeMenu();
    return m_actions;
}

void SwitchWindow::switchTo(QAction *action)
{
    const QVariantList &idList = action->data().toList();

    for (int i = 0; i < s_tasksModel->rowCount(); ++i) {
        const QModelIndex &idx = s_tasksModel->index(i, 0);

        if (idList == idx.data(AbstractTasksModel::WinIdList).toList()) {
            s_tasksModel->requestActivate(idx);

            return;
        }
    }
}

void SwitchWindow::performNextAction()
{
    doSwitch(true);
}

void SwitchWindow::performPreviousAction()
{
    doSwitch(false);
}

void SwitchWindow::doSwitch(bool up)
{
    const QModelIndex &activeTask = s_tasksModel->activeTask();

    if (!activeTask.isValid()) {
        return;
    }

    if (up) {
        const QModelIndex &next = activeTask.sibling(activeTask.row() + 1, 0);

        if (next.isValid()) {
            s_tasksModel->requestActivate(next);
        } else if (s_tasksModel->rowCount() > 1) {
            s_tasksModel->requestActivate(s_tasksModel->index(0, 0));
        }
    } else {
        const QModelIndex &previous = activeTask.sibling(activeTask.row() - 1, 0);

        if (previous.isValid()) {
            s_tasksModel->requestActivate(previous);
        } else if (s_tasksModel->rowCount() > 1) {
            s_tasksModel->requestActivate(s_tasksModel->index(s_tasksModel->rowCount() - 1, 0));
        }
    }
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(switchwindow, SwitchWindow, "plasma-containmentactions-switchwindow.json")

#include "switch.moc"
