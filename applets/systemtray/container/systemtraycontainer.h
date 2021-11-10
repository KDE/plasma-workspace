/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QQuickItem>

#include <Plasma/Applet>

/**
 * @brief Thin wrapping 'Plasma::Applet' for SystemTray.
 *
 * SystemTray is of 'Plasma::Containment' type. To have it presented as a widget in Plasma we need a wrapping applet.
 */
class SystemTrayContainer : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *internalSystray READ internalSystray NOTIFY internalSystrayChanged)

public:
    SystemTrayContainer(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~SystemTrayContainer() override;

    void init() override;

    QQuickItem *internalSystray();

protected:
    void constraintsEvent(Plasma::Types::Constraints constraints) override;
    void ensureSystrayExists();

Q_SIGNALS:
    void internalSystrayChanged();

private:
    QPointer<Plasma::Containment> m_innerContainment;
    QPointer<QQuickItem> m_internalSystray;
};
