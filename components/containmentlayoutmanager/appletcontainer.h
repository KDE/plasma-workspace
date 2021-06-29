/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "itemcontainer.h"

#include <QPointer>
#include <QQmlParserStatus>

namespace PlasmaQuick
{
class AppletQuickItem;
}

class AppletContainer : public ItemContainer
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(PlasmaQuick::AppletQuickItem *applet READ applet NOTIFY appletChanged)

    Q_PROPERTY(QQmlComponent *busyIndicatorComponent READ busyIndicatorComponent WRITE setBusyIndicatorComponent NOTIFY busyIndicatorComponentChanged)

    Q_PROPERTY(QQmlComponent *configurationRequiredComponent READ configurationRequiredComponent WRITE setConfigurationRequiredComponent NOTIFY
                   configurationRequiredComponentChanged)

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
