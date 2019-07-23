/*
 *   Copyright 2019 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "appletcontainer.h"

#include <QQmlEngine>
#include <QQmlContext>

#include <Plasma/Applet>
#include <PlasmaQuick/AppletQuickItem>

AppletContainer::AppletContainer(QQuickItem *parent)
    : ItemContainer(parent)
{
    connect(this, &AppletContainer::contentItemChanged, this, [this]() {
        if (m_appletItem) {
            disconnect(m_appletItem->applet(), &Plasma::Applet::busyChanged, this, nullptr);
        }
        m_appletItem = qobject_cast<PlasmaQuick::AppletQuickItem *>(contentItem());

        connectBusyIndicator();
        connectConfigurationRequired();

        emit appletChanged();
    });
}

AppletContainer::~AppletContainer()
{
}

void AppletContainer::componentComplete()
{
    connectBusyIndicator();
    connectConfigurationRequired();
    ItemContainer::componentComplete();
}

PlasmaQuick::AppletQuickItem *AppletContainer::applet()
{
    return m_appletItem;
}

QQmlComponent *AppletContainer::busyIndicatorComponent() const
{
    return m_busyIndicatorComponent;
}

void AppletContainer::setBusyIndicatorComponent(QQmlComponent *component)
{
    if (m_busyIndicatorComponent == component) {
        return;
    }

    m_busyIndicatorComponent = component;

    if (m_busyIndicatorItem) {
        m_busyIndicatorItem->deleteLater();
        m_busyIndicatorItem = nullptr;
    }

    emit busyIndicatorComponentChanged();
}

void AppletContainer::connectBusyIndicator()
{
    if (m_appletItem && !m_busyIndicatorItem) {
        Q_ASSERT(m_appletItem->applet());
        connect(m_appletItem->applet(), &Plasma::Applet::busyChanged, this, [this] () {
            if (!m_busyIndicatorComponent || !m_appletItem->applet()->isBusy() || m_busyIndicatorItem) {
                return;
            }

            QQmlContext *context = QQmlEngine::contextForObject(this);
            Q_ASSERT(context);
            QObject *instance = m_busyIndicatorComponent->beginCreate(context);
            m_busyIndicatorItem = qobject_cast<QQuickItem *>(instance);

            if (!m_busyIndicatorItem) {
                qWarning() << "Error: busyIndicatorComponent not of Item type";
                if (instance) {
                    instance->deleteLater();
                }
                return;
            }

            m_busyIndicatorItem->setParentItem(this);
            m_busyIndicatorItem->setZ(999);
            m_busyIndicatorComponent->completeCreate();
        });
    }
}

QQmlComponent *AppletContainer::configurationRequiredComponent() const
{
    return m_configurationRequiredComponent;
}

void AppletContainer::setConfigurationRequiredComponent(QQmlComponent *component)
{
    if (m_configurationRequiredComponent == component) {
        return;
    }

    m_configurationRequiredComponent = component;

    if (m_configurationRequiredItem) {
        m_configurationRequiredItem->deleteLater();
        m_configurationRequiredItem = nullptr;
    }

    emit configurationRequiredComponentChanged();
}

void AppletContainer::connectConfigurationRequired()
{
    if (m_appletItem && !m_configurationRequiredItem) {
        Q_ASSERT(m_appletItem->applet());

        auto syncConfigRequired = [this] () {
            if (!m_configurationRequiredComponent || !m_appletItem->applet()->configurationRequired() || m_configurationRequiredItem) {
                return;
            }

            QQmlContext *context = QQmlEngine::contextForObject(this);
            Q_ASSERT(context);
            QObject *instance = m_configurationRequiredComponent->beginCreate(context);
            m_configurationRequiredItem = qobject_cast<QQuickItem *>(instance);

            if (!m_configurationRequiredItem) {
                qWarning() << "Error: configurationRequiredComponent not of Item type";
                if (instance) {
                    instance->deleteLater();
                }
                return;
            }

            m_configurationRequiredItem->setParentItem(this);
            m_configurationRequiredItem->setZ(998);
            m_configurationRequiredComponent->completeCreate();
        };

        connect(m_appletItem->applet(), &Plasma::Applet::configurationRequiredChanged, this, syncConfigRequired);

        if (m_appletItem->applet()->configurationRequired()) {
            syncConfigRequired();
        }
    }
}

#include "moc_appletcontainer.cpp"
