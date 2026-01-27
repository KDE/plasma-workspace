/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    , m_clearHistoryAction(new QAction(this))
{
    m_toggleDoNotDisturbAction->setObjectName(QStringLiteral("toggle do not disturb"));
    m_toggleDoNotDisturbAction->setProperty("componentName", QStringLiteral("plasmashell"));
    m_toggleDoNotDisturbAction->setText(i18n("Toggle do not disturb"));
    m_toggleDoNotDisturbAction->setIcon(QIcon::fromTheme(QStringLiteral("notifications-disabled")));
    m_toggleDoNotDisturbAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(m_toggleDoNotDisturbAction, &QAction::triggered, this, &GlobalShortcuts::toggleDoNotDisturbTriggered);

    KGlobalAccel::self()->setGlobalShortcut(m_toggleDoNotDisturbAction, QKeySequence());

    m_clearHistoryAction->setObjectName(QStringLiteral("clear history"));
    m_clearHistoryAction->setProperty("componentName", QStringLiteral("plasmashell"));
    m_clearHistoryAction->setText(i18nc("@action", "Clear Notification History"));
    m_clearHistoryAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    m_clearHistoryAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(m_clearHistoryAction, &QAction::triggered, this, &GlobalShortcuts::clearHistoryTriggered);

    KGlobalAccel::self()->setGlobalShortcut(m_clearHistoryAction, QKeySequence());
}

GlobalShortcuts::~GlobalShortcuts() = default;

void GlobalShortcuts::showDoNotDisturbOsd(bool doNotDisturb) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall( //
        QStringLiteral("org.kde.plasmashell"),
        QStringLiteral("/org/kde/osdService"),
        QStringLiteral("org.kde.osdService"),
        QStringLiteral("showText"));

    const QString iconName = doNotDisturb ? QStringLiteral("notifications-disabled") : QStringLiteral("notifications");
    const QString text = doNotDisturb ? i18nc("OSD popup, keep short", "Notifications Off") //
                                      : i18nc("OSD popup, keep short", "Notifications On");

    msg.setArguments({iconName, text});

    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

void GlobalShortcuts::showNotificationsHistoryCleaned() const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                      QStringLiteral("/org/kde/osdService"),
                                                      QStringLiteral("org.kde.osdService"),
                                                      QStringLiteral("showText"));

    const QString iconName = QStringLiteral("edit-clear-history");
    const QString text = i18nc("@label OSD popup, keep short", "Notification history cleared");

    msg.setArguments({iconName, text});

    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

#include "moc_globalshortcuts.cpp"
