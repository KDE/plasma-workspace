/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "containmentinterface.h"

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>

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
            const Plasma::Applet *taskManager = findTaskManagerApplet(containment);

            if (!taskManager) {
                return false;
            }

            QQuickItem *rootItem = firstPlasmaGraphicObjectChild(taskManager);

            if (!rootItem) {
                return false;
            }

            QVariant ret;
            QMetaObject::invokeMethod(rootItem, "hasLauncher", Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
            return !ret.toBool();
        }

        break;
    }
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
            QQuickItem *rootItem = findPlasmaGraphicObjectChildIf(containment, [](QQuickItem *item) {
                return item->objectName() == QLatin1String("folder");
            });

            if (!rootItem) {
                return;
            }

            QMetaObject::invokeMethod(rootItem, "addLauncher", Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
        } else {
            containment->createApplet(QStringLiteral("org.kde.plasma.icon"), QVariantList() << entryPath);
        }

        break;
    }
    case Panel: {
        if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            containment->createApplet(QStringLiteral("org.kde.plasma.icon"), QVariantList() << entryPath);
        }

        break;
    }
    case TaskManager: {
        if (containment->pluginMetaData().pluginId() == QLatin1String("org.kde.panel")) {
            const Plasma::Applet *taskManager = findTaskManagerApplet(containment);

            if (!taskManager) {
                return;
            }

            QQuickItem *rootItem = firstPlasmaGraphicObjectChild(taskManager);

            if (!rootItem) {
                return;
            }

            QMetaObject::invokeMethod(rootItem, "addLauncher", Q_ARG(QVariant, QUrl::fromLocalFile(entryPath)));
        }

        break;
    }
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
        containment->actions()->action(QStringLiteral("lock widgets"))->trigger();
    }
}

template<class UnaryPredicate>
QQuickItem *ContainmentInterface::findPlasmaGraphicObjectChildIf(const Plasma::Applet *applet, UnaryPredicate predicate)
{
    QQuickItem *gObj = qobject_cast<QQuickItem *>(applet->property("_plasma_graphicObject").value<QObject *>());

    if (!gObj) {
        return nullptr;
    }

    const QList<QQuickItem *> children = gObj->childItems();
    const auto found = std::find_if(children.cbegin(), children.cend(), predicate);
    return found != children.cend() ? *found : nullptr;
}

QQuickItem *ContainmentInterface::firstPlasmaGraphicObjectChild(const Plasma::Applet *applet)
{
    return findPlasmaGraphicObjectChildIf(applet, [](QQuickItem *) {
        return true;
    });
}

Plasma::Applet *ContainmentInterface::findTaskManagerApplet(Plasma::Containment *containment)
{
    const QList<Plasma::Applet *> applets = containment->applets();
    const auto found = std::find_if(applets.cbegin(), applets.cend(), [](const Plasma::Applet *applet) {
        return m_knownTaskManagers.contains(applet->pluginMetaData().pluginId());
    });
    return found != applets.cend() ? *found : nullptr;
}
