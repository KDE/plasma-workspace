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

#include <QApplication>

#include <QAction>
#include <QDebug>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/DataEngine>
#include <Plasma/ServiceJob>

Q_DECLARE_METATYPE(QWeakPointer<Plasma::Containment>)

SwitchActivity::SwitchActivity(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
}

SwitchActivity::~SwitchActivity()
{
}

void SwitchActivity::makeMenu()
{
    qDeleteAll(m_actions);
    m_actions.clear();
    foreach (const QString &id, m_consumer.activities(KActivities::Info::Running)) {
        KActivities::Info info(id);
        QAction *action = new QAction(QIcon::fromTheme(info.icon()), info.name(), this);
        action->setData(id);

        if (id == m_consumer.currentActivity()) {
            QFont font = action->font();
            font.setBold(true);
            action->setFont(font);
        }

        connect(action, &QAction::triggered, [=]() {
            switchTo(action);
        });

        m_actions << action;
    }
}

QList<QAction *> SwitchActivity::contextualActions()
{
    makeMenu();

    return m_actions;
}

void SwitchActivity::switchTo(QAction *action)
{
    if (!action) {
        return;
    }

    m_controller.setCurrentActivity(action->data().toString());
}

void SwitchActivity::performNextAction()
{
    const QStringList activities = m_consumer.activities(KActivities::Info::Running);

    int i = activities.indexOf(m_consumer.currentActivity());

    i = (i + 1) % activities.size();
    m_controller.setCurrentActivity(activities[i]);
}

void SwitchActivity::performPreviousAction()
{
    const QStringList activities = m_consumer.activities(KActivities::Info::Running);

    int i = activities.indexOf(m_consumer.currentActivity());

    i = (i + activities.size() - 1) % activities.size();
    m_controller.setCurrentActivity(activities[i]);
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(switchactivity, SwitchActivity, "plasma-containmentactions-switchactivity.json")

#include "switch.moc"
