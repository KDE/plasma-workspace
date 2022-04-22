/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <plasma/containmentactions.h>

class QAction;

namespace TaskManager
{
class VirtualDesktopInfo;
}

class SwitchDesktop : public Plasma::ContainmentActions
{
    Q_OBJECT
public:
    SwitchDesktop(QObject *parent, const QVariantList &args);
    ~SwitchDesktop() override;

    QList<QAction *> contextualActions() override;

    void performNextAction() override;
    void performPreviousAction() override;

private Q_SLOTS:
    void switchTo();

private:
    QHash<int, QAction *> m_actions;
    TaskManager::VirtualDesktopInfo *const m_virtualDesktopInfo;
};
