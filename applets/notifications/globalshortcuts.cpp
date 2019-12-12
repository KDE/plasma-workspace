/*
 * Copyright 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License or any later version accepted by the membership of
 * KDE e.V. (or its successor approved by the membership of KDE
 * e.V.), which shall act as a proxy defined in Section 14 of
 * version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "globalshortcuts.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>

#include <KLocalizedString>

#include <KGlobalAccel>

GlobalShortcuts::GlobalShortcuts(QObject *parent)
    : QObject(parent)
    , m_toggleDoNotDisturbAction(new QAction(this))
{
    m_toggleDoNotDisturbAction->setObjectName(QStringLiteral("toggle do not disturb"));
    m_toggleDoNotDisturbAction->setProperty("componentName", QStringLiteral("plasmashell"));
    m_toggleDoNotDisturbAction->setText(i18n("Toggle do not disturb"));
    m_toggleDoNotDisturbAction->setIcon(QIcon::fromTheme(QStringLiteral("notifications-disabled")));
    m_toggleDoNotDisturbAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(m_toggleDoNotDisturbAction, &QAction::triggered, this, &GlobalShortcuts::toggleDoNotDisturbTriggered);

    KGlobalAccel::self()->setGlobalShortcut(m_toggleDoNotDisturbAction, QKeySequence());
}

GlobalShortcuts::~GlobalShortcuts() = default;

void GlobalShortcuts::showDoNotDisturbOsd(bool doNotDisturb) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.plasmashell"),
        QStringLiteral("/org/kde/osdService"),
        QStringLiteral("org.kde.osdService"),
        QStringLiteral("showText")
    );

    const QString iconName = doNotDisturb ? QStringLiteral("notifications-disabled") : QStringLiteral("notifications");
    const QString text = doNotDisturb ? i18nc("OSD popup, keep short", "Notifications Off")
                                      : i18nc("OSD popup, keep short", "Notifications On");

    msg.setArguments({iconName, text});

    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}
