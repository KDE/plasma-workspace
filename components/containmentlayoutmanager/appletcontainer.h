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

#pragma once

#include "itemcontainer.h"

#include <QQmlParserStatus>
#include <QPointer>


namespace PlasmaQuick {
    class AppletQuickItem;
}

class AppletContainer: public ItemContainer
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(PlasmaQuick::AppletQuickItem *applet READ applet NOTIFY appletChanged)

    Q_PROPERTY(QQmlComponent *busyIndicatorComponent READ busyIndicatorComponent WRITE setBusyIndicatorComponent NOTIFY busyIndicatorComponentChanged)

    Q_PROPERTY(QQmlComponent *configurationRequiredComponent READ configurationRequiredComponent WRITE setConfigurationRequiredComponent NOTIFY configurationRequiredComponentChanged)

public:
    AppletContainer(QQuickItem *parent = nullptr);
    ~AppletContainer();

    PlasmaQuick::AppletQuickItem *applet();

    QQmlComponent *busyIndicatorComponent() const;
    void setBusyIndicatorComponent(QQmlComponent *comp);

    QQmlComponent *configurationRequiredComponent() const;
    void setConfigurationRequiredComponent(QQmlComponent *comp);

protected:
    void componentComplete() override;

Q_SIGNALS:
    void appletChanged();
    void busyIndicatorComponentChanged();
    void configurationRequiredComponentChanged();

private:
    void connectBusyIndicator();
    void connectConfigurationRequired();

    QPointer<PlasmaQuick::AppletQuickItem> m_appletItem;
    QPointer<QQmlComponent> m_busyIndicatorComponent;
    QQuickItem *m_busyIndicatorItem = nullptr;
    QPointer<QQmlComponent> m_configurationRequiredComponent;
    QQuickItem *m_configurationRequiredItem = nullptr;
};

