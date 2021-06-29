/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

K_PLUGIN_CLASS_WITH_JSON(AppLauncher, "plasma-containmentactions-applauncher.json")

#include "launch.moc"
