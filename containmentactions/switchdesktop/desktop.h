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

#ifndef SWITCHDESKTOP_HEADER
#define SWITCHDESKTOP_HEADER

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
    TaskManager::VirtualDesktopInfo *m_virtualDesktopInfo;
};

#endif
