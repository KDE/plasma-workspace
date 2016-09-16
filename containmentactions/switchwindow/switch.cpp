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

#include "switch.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QTimer>

#include <QDebug>
#include <QMenu>
#include <KWindowSystem>

#include <Plasma/DataEngine>
#include <Plasma/Service>

#include "abstracttasksmodel.h"

SwitchWindow::SwitchWindow(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args),
      m_activityInfo(new TaskManager::ActivityInfo(this)),
      m_tasksModel(new TaskManager::TasksModel(this)),
      m_mode(AllFlat),
      m_clearOrderTimer(0)
{
    m_tasksModel->setGroupMode(TaskManager::TasksModel::GroupDisabled);

    m_tasksModel->setActivity(m_activityInfo->currentActivity());
    m_tasksModel->setFilterByActivity(true);
    connect(m_activityInfo, &TaskManager::ActivityInfo::currentActivityChanged,
        this, [this]() { m_tasksModel->setActivity(m_activityInfo->currentActivity()); });
}

SwitchWindow::~SwitchWindow()
{
}

void SwitchWindow::restore(const KConfigGroup &config)
{
    m_mode = (MenuMode)config.readEntry("mode", (int)AllFlat);
}

QWidget* SwitchWindow::createConfigurationInterface(QWidget* parent)
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

    if (m_tasksModel->rowCount() == 0) {
        return;
    }

    QMultiHash<int, QAction*> desktops;

    //make all the window actions
    for (int i = 0; i < m_tasksModel->rowCount(); ++i) {
        if (m_tasksModel->data(m_tasksModel->index(i, 0), TaskManager::AbstractTasksModel::IsStartup).toBool()) {
            qDebug() << "skipped fake task";
            continue;
        }

        QString name = m_tasksModel->data(m_tasksModel->index(i, 0), Qt::DisplayRole).toString();

        if (name.isEmpty()) {
            continue;
        }

        const QVariantList &idList = m_tasksModel->data(m_tasksModel->index(i, 0), TaskManager::AbstractTasksModel::LegacyWinIdList).toList();

        if (!idList.count()) {
            continue;
        }

        QAction *action = new QAction(name, this);
        action->setIcon(m_tasksModel->data(m_tasksModel->index(i, 0), Qt::DecorationRole).value<QIcon>());
        action->setData(idList.at(0));
        desktops.insert(m_tasksModel->data(m_tasksModel->index(i, 0), TaskManager::AbstractTasksModel::VirtualDesktop).toInt(), action);
        connect(action, &QAction::triggered, [=]() {
            switchTo(action);
        });
    }

    //sort into menu
    if (m_mode == CurrentDesktop) {
        int currentDesktop = KWindowSystem::currentDesktop();

        QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "Windows"), this);
        a->setSeparator(true);
        m_actions << a;
        m_actions << desktops.values(currentDesktop);
        m_actions << desktops.values(-1);

    } else {
        int numDesktops = KWindowSystem::numberOfDesktops();
        if (m_mode == AllFlat) {
            for (int i = 1; i <= numDesktops; ++i) {
                if (desktops.contains(i)) {
                    QString name = KWindowSystem::desktopName(i);
                    name = QStringLiteral("%1: %2").arg(i).arg(name);
                    QAction *a = new QAction(name, this);
                    a->setSeparator(true);
                    m_actions << a;
                    m_actions << desktops.values(i);
                }
            }
            if (desktops.contains(-1)) {
                QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "All Desktops"), this);
                a->setSeparator(true);
                m_actions << a;
                m_actions << desktops.values(-1);
            }

        } else { //submenus
            for (int i = 1; i <= numDesktops; ++i) {
                if (desktops.contains(i)) {
                    QString name = KWindowSystem::desktopName(i);
                    name = QStringLiteral("%1: %2").arg(i).arg(name);
                    QMenu *subMenu = new QMenu(name);
                    subMenu->addActions(desktops.values(i));

                    QAction *a = new QAction(name, this);
                    a->setMenu(subMenu);
                    m_actions << a;
                }
            }
            if (desktops.contains(-1)) {
                QMenu *subMenu = new QMenu(i18nc("plasma_containmentactions_switchwindow", "All Desktops"));
                subMenu->addActions(desktops.values(-1));
                QAction *a = new QAction(i18nc("plasma_containmentactions_switchwindow", "All Desktops"), this);
                a->setMenu(subMenu);
                m_actions << a;
            }
        }
    }
}

QList<QAction*> SwitchWindow::contextualActions()
{
    makeMenu();
    return m_actions;
}

void SwitchWindow::switchTo(QAction *action)
{
    int id = action->data().toInt();

    KWindowSystem::forceActiveWindow(id);
}

void SwitchWindow::clearWindowsOrder()
{
    qDebug() << "CLEARING>.......................";
    m_windowsOrder.clear();
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
    //TODO somehow find the "next" or "previous" window
    //without changing hte window order (don't want to always go between two windows)
    if (m_windowsOrder.isEmpty()) {
        m_windowsOrder = KWindowSystem::stackingOrder();
    } else {
        if (!m_clearOrderTimer) {
            m_clearOrderTimer = new QTimer(this);
            connect(m_clearOrderTimer, &QTimer::timeout, this, &SwitchWindow::clearWindowsOrder);
            m_clearOrderTimer->setSingleShot(true);
            m_clearOrderTimer->setInterval(1000);
        }

        m_clearOrderTimer->start();
    }

    const WId activeWindow = KWindowSystem::activeWindow();
    bool next = false;
    WId first = 0;
    WId last = 0;
    for (int i = 0; i < m_windowsOrder.count(); ++i) {
        const WId id = m_windowsOrder.at(i);
        const KWindowInfo info(id, NET::WMDesktop | NET::WMVisibleName | NET::WMWindowType);
        if (info.windowType(NET::NormalMask | NET::DialogMask | NET::UtilityMask) != -1 && info.isOnCurrentDesktop()) {
            if (next) {
                KWindowSystem::forceActiveWindow(id);
                return;
            }

            if (first == 0) {
                first = id;
            }

            if (id == activeWindow) {
                if (up) {
                    next = true;
                } else if (last) {
                    KWindowSystem::forceActiveWindow(last);
                    return;
                }
            }

            last = id;
        }
    }

    KWindowSystem::forceActiveWindow(up ? first : last);
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(switchwindow, SwitchWindow, "plasma-containmentactions-switchwindow.json")

#include "switch.moc"
