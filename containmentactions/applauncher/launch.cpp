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

#include "launch.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>

#include <QDebug>

#include <Plasma/PluginLoader>
#include <KRun>

AppLauncher::AppLauncher(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args),
      m_group(new KServiceGroup(QStringLiteral("/")))
{
}

AppLauncher::~AppLauncher()
{
}

void AppLauncher::init(const KConfigGroup &)
{
}

QList<QAction*> AppLauncher::contextualActions()
{
    qDeleteAll(m_actions);
    m_actions.clear();
    makeMenu(0, m_group);

    return m_actions;
}

void AppLauncher::makeMenu(QMenu *menu, const KServiceGroup::Ptr group)
{
    foreach (KSycocaEntry::Ptr p, group->entries(true, false, true)) {
        if (p->isType(KST_KService)) {
            const KService::Ptr service(static_cast<KService*>(p.data()));
            QAction *action = new QAction(QIcon::fromTheme(service->icon()), service->genericName().isEmpty() ? service->name() : service->genericName(), this);
            connect(action, &QAction::triggered, [action](){
                KService::Ptr service = KService::serviceByStorageId(action->data().toString());
                new KRun(QUrl("file://"+service->entryPath()), 0);
            });
            action->setData(service->storageId());
            if (menu) {
                menu->addAction(action);
            } else {
                m_actions << action;
            }
        } else if (p->isType(KST_KServiceGroup)) {
            const KServiceGroup::Ptr service(static_cast<KServiceGroup*>(p.data()));
            if (service->childCount() == 0) {
                continue;
            }
            QAction *action = new QAction(QIcon::fromTheme(service->icon()), service->caption(), this);
            QMenu *subMenu = new QMenu();
            makeMenu(subMenu, service);
            action->setMenu(subMenu);
            if (menu) {
                menu->addAction(action);
            } else {
                m_actions << action;
            }
        } else if (p->isType(KST_KServiceSeparator)) {
            if (menu) {
                menu->addSeparator();
            }
        }
    }
}


K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(applauncher, AppLauncher, "plasma-containmentactions-applauncher.json")


#include "launch.moc"
