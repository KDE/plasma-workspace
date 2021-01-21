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

#include <QDebug>

#include <KRun>
#include <Plasma/PluginLoader>

AppLauncher::AppLauncher(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
    , m_group(new KServiceGroup(QStringLiteral("/")))
{
}

AppLauncher::~AppLauncher()
{
}

void AppLauncher::init(const KConfigGroup &)
{
}

QList<QAction *> AppLauncher::contextualActions()
{
    qDeleteAll(m_actions);
    m_actions.clear();
    makeMenu(nullptr, m_group);

    return m_actions;
}

void AppLauncher::makeMenu(QMenu *menu, const KServiceGroup::Ptr group)
{
    const auto entries = group->entries(true, true, true);
    for (const KSycocaEntry::Ptr &p : entries) {
        if (p->isType(KST_KService)) {
            const KService::Ptr service(static_cast<KService *>(p.data()));

            QString text = service->name();
            if (!m_showAppsByName && !service->genericName().isEmpty()) {
                text = service->genericName();
            }

            QAction *action = new QAction(QIcon::fromTheme(service->icon()), text, this);
            connect(action, &QAction::triggered, [action]() {
                KService::Ptr service = KService::serviceByStorageId(action->data().toString());
                new KRun(QUrl("file://" + service->entryPath()), nullptr);
            });
            action->setData(service->storageId());
            if (menu) {
                menu->addAction(action);
            } else {
                m_actions << action;
            }
        } else if (p->isType(KST_KServiceGroup)) {
            const KServiceGroup::Ptr service(static_cast<KServiceGroup *>(p.data()));
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

QWidget *AppLauncher::createConfigurationInterface(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);
    widget->setWindowTitle(i18nc("plasma_containmentactions_applauncher", "Configure Application Launcher Plugin"));

    m_ui.showAppsByName->setChecked(m_showAppsByName);

    return widget;
}

void AppLauncher::configurationAccepted()
{
    m_showAppsByName = m_ui.showAppsByName->isChecked();
}

void AppLauncher::restore(const KConfigGroup &config)
{
    m_showAppsByName = config.readEntry(QStringLiteral("showAppsByName"), false);
}

void AppLauncher::save(KConfigGroup &config)
{
    config.writeEntry(QStringLiteral("showAppsByName"), m_showAppsByName);
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(applauncher, AppLauncher, "plasma-containmentactions-applauncher.json")

#include "launch.moc"
