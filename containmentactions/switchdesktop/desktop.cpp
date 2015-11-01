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

#include "desktop.h"

#include <QAction>
#include <QWheelEvent>

#include <QDebug>
#include <KWindowSystem>
#include <KLocalizedString>

SwitchDesktop::SwitchDesktop(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
}

SwitchDesktop::~SwitchDesktop()
{
    qDeleteAll(m_actions);
}


QList<QAction*> SwitchDesktop::contextualActions()
{
    QList<QAction*> list;

    const int numDesktops = KWindowSystem::numberOfDesktops();
    const int currentDesktop = KWindowSystem::currentDesktop();

    //Is it either the first time or the desktop number changed?
    if (m_actions.count() < numDesktops) {
        for (int i = m_actions.count() + 1; i <= numDesktops; ++i) {
            QString name = KWindowSystem::desktopName(i);
            QAction *action = new QAction(QStringLiteral("%1: %2").arg(i).arg(name), this);
            connect(action, &QAction::triggered, this, &SwitchDesktop::switchTo);
            action->setData(i);
            m_actions[i] = action;
        }

    } else if (m_actions.count() > numDesktops) {
        for (int i = numDesktops +1; i <= m_actions.count(); ++i) {
            delete m_actions[i];
            m_actions.remove(i);
        }
    }

    for (int i = 1; i <= numDesktops; ++i) {
        QAction *action = m_actions.value(i);
        action->setEnabled(i != currentDesktop);
        list << action;
    }

    return list;
}

void SwitchDesktop::switchTo()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const int desktop = action->data().toInt();
    KWindowSystem::setCurrentDesktop(desktop);
}

void SwitchDesktop::performNextAction()
{
    const int numDesktops = KWindowSystem::numberOfDesktops();
    const int currentDesktop = KWindowSystem::currentDesktop();
    KWindowSystem::setCurrentDesktop(currentDesktop % numDesktops + 1);
}

void SwitchDesktop::performPreviousAction()
{
    const int numDesktops = KWindowSystem::numberOfDesktops();
    const int currentDesktop = KWindowSystem::currentDesktop();
    KWindowSystem::setCurrentDesktop((numDesktops + currentDesktop - 2) % numDesktops + 1);
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(switchdesktop, SwitchDesktop, "plasma-containmentactions-switchdesktop.json")

#include "desktop.moc"
