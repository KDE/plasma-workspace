/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QIcon>
#include <QObject>
#include <QObjectBindableProperty>
#include <qqmlregistration.h>

#include "applicationdata_p.h"

using InhibitionInfo = QPair<QString, QString>;

class PowerManagmentControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<QVariantMap> inhibitions READ default NOTIFY inhibitionsChanged BINDABLE bindableInhibitions)
    Q_PROPERTY(bool hasInhibition READ default NOTIFY hasInhibitionChanged BINDABLE bindableHasInhibition)
    Q_PROPERTY(bool isLidPresent READ default NOTIFY isLidPresentChanged BINDABLE bindableIsLidPresent)
    Q_PROPERTY(bool triggersLidAction READ default NOTIFY triggersLidActionChanged BINDABLE bindableTriggersLidAction)

public:
    explicit PowerManagmentControl(QObject *parent = nullptr);
    ~PowerManagmentControl();

    Q_INVOKABLE void beginSuppressingSleep(QString reason);
    Q_INVOKABLE void stopSuppressingSleep();
    Q_INVOKABLE void beginSuppressingScreenPowerManagement(QString reason);
    Q_INVOKABLE void stopSuppressingScreenPowerManagement();
    Q_INVOKABLE void releaseInhibition(uint cookie);

Q_SIGNALS:
    void inhibitionsChanged(QList<QVariantMap> inhibitions);
    void hasInhibitionChanged(bool status);
    void isLidPresentChanged(bool status);
    void triggersLidActionChanged(bool status);

private Q_SLOTS:
    void onInhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed);
    void onHasInhibitionChanged(bool status);

private:
    QBindable<QList<QVariantMap>> bindableInhibitions();
    QBindable<bool> bindableHasInhibition();
    QBindable<bool> bindableIsLidPresent();
    QBindable<bool> bindableTriggersLidAction();

    Q_OBJECT_BINDABLE_PROPERTY(PowerManagmentControl, QList<QVariantMap>, m_inhibitions, &PowerManagmentControl::inhibitionsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerManagmentControl, bool, m_hasInhibition, &PowerManagmentControl::hasInhibitionChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl, bool, m_isLidPresent, false, &PowerManagmentControl::bindableIsLidPresent)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl, bool, m_triggersLidAction, false, &PowerManagmentControl::bindableTriggersLidAction)

    uint m_sleepInhibitionCookie = -1;
    uint m_lockInhibitionCookie = -1;

    ApplicationData m_data;
};
