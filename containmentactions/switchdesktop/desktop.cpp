/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
        if (m_virtualDesktopInfo->navigationWrappingAround()) {
            nextDesktopIndex = 0;
        } else {
            return;
        }
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
        if (m_virtualDesktopInfo->navigationWrappingAround()) {
            previousDesktopIndex = desktopIds.count() - 1;
        } else {
            return;
        }
    }

    m_virtualDesktopInfo->requestActivate(desktopIds.at(previousDesktopIndex));
}

K_PLUGIN_CLASS_WITH_JSON(SwitchDesktop, "plasma-containmentactions-switchdesktop.json")

#include "desktop.moc"
