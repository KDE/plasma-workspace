/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "switch.h"

#include <QApplication>

#include <QAction>
#include <QDebug>

#include <Plasma/Containment>
#include <Plasma/Corona>

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

K_PLUGIN_CLASS_WITH_JSON(SwitchActivity, "plasma-containmentactions-switchactivity.json")

#include "switch.moc"
