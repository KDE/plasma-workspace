/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "containmentinterface.h"

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <PlasmaQuick/AppletQuickItem>

#include <KActionCollection>

// FIXME HACK TODO: Unfortunately we have no choice but to hard-code a list of
// applets we know to expose the correct interface right now -- this is slated
// for replacement with some form of generic service.
QStringList ContainmentInterface::m_knownTaskManagers{
    QLatin1String("org.kde.plasma.taskmanager"),
    QLatin1String("org.kde.plasma.icontasks"),
    QLatin1String("org.kde.plasma.expandingiconstaskmanager"),
};

ContainmentInterface::ContainmentInterface(QObject *parent)
    : QObject(parent)
{
}

ContainmentInterface::~ContainmentInterface()
{
}

bool ContainmentInterface::mayAddLauncher(QObject *appletInterface, ContainmentInterface::Target target, const QString &entryPath)
{
    if (!appletInterface) {
        return false;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();
    Plasma::Containment *containment = applet->containment();

    if (!containment) {
        return false;
    }

    Plasma::Corona *corona = containment->corona();

    if (!corona) {
        return false;
    }

    switch (target) {
    case Desktop: {
        containment = corona->containmentForScreen(containment->screen(), QString(), QString());

        if (containment) {
            return (containment->immutability() == Plasma::Types::Mutable);
        }

        break;
    }
    case Panel: {
        if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            return (containment->immutability() == Plasma::Types::Mutable);
        }

        break;
    }
    case TaskManager: {
        if (!entryPath.isEmpty() && containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            auto *taskManager = findTaskManagerApplet(containment);

            if (!taskManager) {
                return false;
            }

            auto *taskManagerQuickItem = PlasmaQuick::AppletQuickItem::itemForApplet(taskManager);

            if (!taskManagerQuickItem) {
                return false;
            }

            return taskManagerQuickItem->property("supportsLaunchers").toBool();
        }

        break;
    }
    }

    return false;
}

bool ContainmentInterface::hasLauncher(QObject *appletInterface, ContainmentInterface::Target target, const QString &entryPath)
{
    // Only the task manager supports toggle-able launchers
    if (target != TaskManager) {
        return false;
    }
    if (!appletInterface) {
        return false;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();
    Plasma::Containment *containment = applet->containment();

    if (!containment) {
        return false;
    }

    if (!entryPath.isEmpty() && containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
        auto *taskManager = findTaskManagerApplet(containment);

        if (!taskManager) {
            return false;
        }

        auto *taskManagerQuickItem = PlasmaQuick::AppletQuickItem::itemForApplet(taskManager);

        if (!taskManagerQuickItem) {
            return false;
        }

        QVariant ret;
        QMetaObject::invokeMethod(taskManagerQuickItem, "hasLauncher", Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
        return ret.toBool();
    }

    return false;
}

void ContainmentInterface::addLauncher(QObject *appletInterface, ContainmentInterface::Target target, const QString &entryPath)
{
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();
    Plasma::Containment *containment = applet->containment();

    if (!containment) {
        return;
    }

    Plasma::Corona *corona = containment->corona();

    if (!corona) {
        return;
    }

    switch (target) {
    case Desktop: {
        containment = corona->containmentForScreen(containment->screen(), QString(), QString());

        if (!containment) {
            return;
        }

        const QStringList &containmentProvides = containment->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"), QStringList());

        if (containmentProvides.contains(QLatin1String("org.kde.plasma.filemanagement"))) {
            auto *folderQuickItem = PlasmaQuick::AppletQuickItem::itemForApplet(containment);

            if (!folderQuickItem) {
                return;
            }

            QMetaObject::invokeMethod(folderQuickItem, "addLauncher", Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
        } else {
            containment->createApplet(QStringLiteral("org.kde.plasma.icon"), QVariantList() << QUrl::fromLocalFile(entryPath));
        }

        break;
    }
    case Panel: {
        if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            containment->createApplet(QStringLiteral("org.kde.plasma.icon"), QVariantList() << QUrl::fromLocalFile(entryPath));
        }

        break;
    }
    case TaskManager: {
        if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            auto *taskManager = findTaskManagerApplet(containment);

            if (!taskManager) {
                return;
            }

            auto *taskManagerQuickItem = PlasmaQuick::AppletQuickItem::itemForApplet(taskManager);

            if (!taskManagerQuickItem) {
                return;
            }

            QMetaObject::invokeMethod(taskManagerQuickItem, "addLauncher", Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
        }

        break;
    }
    }
}

void ContainmentInterface::removeLauncher(QObject *appletInterface, ContainmentInterface::Target target, const QString &entryPath)
{
    // Only the task manager supports toggle-able launches
    if (target != TaskManager) {
        return;
    }
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();
    Plasma::Containment *containment = applet->containment();

    if (!containment) {
        return;
    }

    if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
        auto *taskManager = findTaskManagerApplet(containment);

        if (!taskManager) {
            return;
        }

        auto *taskManagerQuickItem = PlasmaQuick::AppletQuickItem::itemForApplet(taskManager);

        if (!taskManagerQuickItem) {
            return;
        }

        QMetaObject::invokeMethod(taskManagerQuickItem, "removeLauncher", Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
    }
}

QObject *ContainmentInterface::screenContainment(QObject *appletInterface)
{
    if (!appletInterface) {
        return nullptr;
    }

    const Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();
    Plasma::Containment *containment = applet->containment();

    if (!containment) {
        return nullptr;
    }

    Plasma::Corona *corona = containment->corona();

    if (!corona) {
        return nullptr;
    }

    return corona->containmentForScreen(containment->screen(), QString(), QString());
}

bool ContainmentInterface::screenContainmentMutable(QObject *appletInterface)
{
    const Plasma::Containment *containment = static_cast<const Plasma::Containment *>(screenContainment(appletInterface));

    if (containment) {
        return (containment->immutability() == Plasma::Types::Mutable);
    }

    return false;
}

void ContainmentInterface::ensureMutable(Plasma::Containment *containment)
{
    if (containment && containment->immutability() != Plasma::Types::Mutable) {
        containment->internalAction(QStringLiteral("lock widgets"))->trigger();
    }
}

Plasma::Applet *ContainmentInterface::findTaskManagerApplet(Plasma::Containment *containment)
{
    const QList<Plasma::Applet *> applets = containment->applets();
    const auto found = std::find_if(applets.cbegin(), applets.cend(), [](const Plasma::Applet *applet) {
        return m_knownTaskManagers.contains(applet->pluginMetaData().pluginId());
    });
    return found != applets.cend() ? *found : nullptr;
}
