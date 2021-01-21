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

#include "desktop.h"

#include <virtualdesktopinfo.h>

#include <QAction>

using namespace TaskManager;

SwitchDesktop::SwitchDesktop(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
    , m_virtualDesktopInfo(new VirtualDesktopInfo(this))
{
}

SwitchDesktop::~SwitchDesktop()
{
}

QList<QAction *> SwitchDesktop::contextualActions()
{
    const int numDesktops = m_virtualDesktopInfo->numberOfDesktops();
    const QVariantList &desktopIds = m_virtualDesktopInfo->desktopIds();
    const QStringList &desktopNames = m_virtualDesktopInfo->desktopNames();
    const QVariant &currentDesktop = m_virtualDesktopInfo->currentDesktop();

    QList<QAction *> actions;
    actions.reserve(numDesktops);

    if (m_actions.count() < numDesktops) {
        for (int i = m_actions.count(); i < numDesktops; ++i) {
            QString name = desktopNames.at(i);
            QAction *action = new QAction(this);
            connect(action, &QAction::triggered, this, &SwitchDesktop::switchTo);
            m_actions[i] = action;
        }
    } else if (m_actions.count() > numDesktops) {
        for (int i = m_actions.count(); i > numDesktops; --i) {
            delete m_actions.take(i - 1);
        }
    }

    for (int i = 0; i < numDesktops; ++i) {
        QAction *action = m_actions.value(i);
        action->setText(QStringLiteral("%1: %2").arg(QString::number(i), desktopNames.at(i)));
        action->setData(desktopIds.at(i));
        action->setEnabled(desktopIds.at(i) != currentDesktop);
        actions << action;
    }

    return actions;
}

void SwitchDesktop::switchTo()
{
    const QAction *action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }

    m_virtualDesktopInfo->requestActivate(action->data());
}

void SwitchDesktop::performNextAction()
{
    const QVariantList &desktopIds = m_virtualDesktopInfo->desktopIds();
    if (desktopIds.isEmpty()) {
        return;
    }

    const QVariant &currentDesktop = m_virtualDesktopInfo->currentDesktop();
    const int currentDesktopIndex = desktopIds.indexOf(currentDesktop);

    int nextDesktopIndex = currentDesktopIndex + 1;

    if (nextDesktopIndex == desktopIds.count()) {
        nextDesktopIndex = 0;
    }

    m_virtualDesktopInfo->requestActivate(desktopIds.at(nextDesktopIndex));
}

void SwitchDesktop::performPreviousAction()
{
    const QVariantList &desktopIds = m_virtualDesktopInfo->desktopIds();
    if (desktopIds.isEmpty()) {
        return;
    }
    const QVariant &currentDesktop = m_virtualDesktopInfo->currentDesktop();
    const int currentDesktopIndex = desktopIds.indexOf(currentDesktop);

    int previousDesktopIndex = currentDesktopIndex - 1;

    if (previousDesktopIndex < 0) {
        previousDesktopIndex = desktopIds.count() - 1;
    }

    m_virtualDesktopInfo->requestActivate(desktopIds.at(previousDesktopIndex));
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(switchdesktop, SwitchDesktop, "plasma-containmentactions-switchdesktop.json")

#include "desktop.moc"
