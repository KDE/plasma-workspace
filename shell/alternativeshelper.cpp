/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "alternativeshelper.h"

#include <QQmlContext>
#include <QQmlEngine>

#include <KConfigLoader>

#include <Plasma/Containment>
#include <Plasma/PluginLoader>

AlternativesHelper::AlternativesHelper(Plasma::Applet *applet, QObject *parent)
    : QObject(parent)
    , m_applet(applet)
{
}

AlternativesHelper::~AlternativesHelper()
{
}

QStringList AlternativesHelper::appletProvides() const
{
    return m_applet->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"), QStringList());
}

QString AlternativesHelper::currentPlugin() const
{
    return m_applet->pluginMetaData().pluginId();
}

QQuickItem *AlternativesHelper::applet() const
{
    return m_applet->property("_plasma_graphicObject").value<QQuickItem *>();
}

void AlternativesHelper::loadAlternative(const QString &plugin)
{
    if (plugin == m_applet->pluginMetaData().pluginId() || m_applet->isContainment()) {
        return;
    }

    Plasma::Containment *cont = m_applet->containment();
    if (!cont) {
        return;
    }

    QQuickItem *appletItem = m_applet->property("_plasma_graphicObject").value<QQuickItem *>();
    QQuickItem *contItem = cont->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!appletItem || !contItem) {
        return;
    }

    // ensure the global shortcut is moved to the new applet
    const QKeySequence &shortcut = m_applet->globalShortcut();
    m_applet->setGlobalShortcut(QKeySequence()); // need to unmap the old one first

    const QPoint newPos = appletItem->mapToItem(contItem, QPointF(0, 0)).toPoint();

    Plasma::Applet *newApplet = nullptr;
    QMetaObject::invokeMethod(contItem,
                              "createApplet",
                              Q_RETURN_ARG(Plasma::Applet *, newApplet),
                              Q_ARG(QString, plugin),
                              Q_ARG(QVariantList, QVariantList()),
                              Q_ARG(QPoint, newPos));

    if (newApplet) {
        newApplet->setGlobalShortcut(shortcut);
        KConfigGroup newCg(newApplet->config());
        m_applet->config().copyTo(&newCg);
        // To let ConfigPropertyMap reload its config
        Q_EMIT newApplet->configScheme()->configChanged();
    }

    m_applet->destroy();
}

#include "moc_alternativeshelper.cpp"
