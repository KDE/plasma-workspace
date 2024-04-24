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

struct SolidInhibition {
    uint cookie;
    QString appName;
    QString reason;
};

class PowerManagmentControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<QVariantMap> inhibitions READ default NOTIFY inhibitionsChanged BINDABLE bindableInhibitions)
    Q_PROPERTY(bool hasInhibition READ default NOTIFY hasInhibitionChanged BINDABLE bindableHasInhibition)
    Q_PROPERTY(bool isLidPresent READ default NOTIFY isLidPresentChanged BINDABLE bindableIsLidPresent)
    Q_PROPERTY(bool triggersLidAction READ default NOTIFY triggersLidActionChanged BINDABLE bindableTriggersLidAction)
    Q_PROPERTY(bool isManuallyInhibited READ default NOTIFY isManuallyInhibitedChanged BINDABLE bindableIsManuallyInhibited)
    Q_PROPERTY(bool isManuallyInhibitedError READ default WRITE default NOTIFY isManuallyInhibitedErrorChanged BINDABLE bindableIsManuallyInhibitedError)
    Q_PROPERTY(bool isSilent READ isSilent WRITE setIsSilent)

public:
    Q_INVOKABLE void inhibit(const QString &reason);
    Q_INVOKABLE void uninhibit();
    Q_INVOKABLE void releaseInhibition(uint cookie);

    explicit PowerManagmentControl(QObject *parent = nullptr);
    ~PowerManagmentControl() override;

Q_SIGNALS:
    void inhibitionsChanged(const QList<QVariantMap> &inhibitions);
    void hasInhibitionChanged(bool status);
    void isLidPresentChanged(bool status);
    void triggersLidActionChanged(bool status);
    void isManuallyInhibitedChanged(bool status);
    void isManuallyInhibitedErrorChanged(bool status);

private Q_SLOTS:
    void onInhibitionsChanged(const QList<uint> &added, const QList<uint> &removed);
    void onHasInhibitionChanged(bool status);
    void onIsManuallyInhibitedChanged(bool status);
    void onisManuallyInhibitedErrorChanged(bool status);

private:
    bool isSilent();
    void setIsSilent(bool status);
    void updateInhibitions(const QList<SolidInhibition> &inhibitions);

    QBindable<QList<QVariantMap>> bindableInhibitions();
    QBindable<bool> bindableHasInhibition();
    QBindable<bool> bindableIsLidPresent();
    QBindable<bool> bindableTriggersLidAction();
    QBindable<bool> bindableIsManuallyInhibited();
    QBindable<bool> bindableIsManuallyInhibitedError();

    Q_OBJECT_BINDABLE_PROPERTY(PowerManagmentControl, QList<QVariantMap>, m_inhibitions, &PowerManagmentControl::inhibitionsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PowerManagmentControl, bool, m_hasInhibition, &PowerManagmentControl::hasInhibitionChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl, bool, m_isLidPresent, false, &PowerManagmentControl::isLidPresentChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl, bool, m_triggersLidAction, false, &PowerManagmentControl::triggersLidActionChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl, bool, m_isManuallyInhibited, false, &PowerManagmentControl::isManuallyInhibitedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(PowerManagmentControl,
                                         bool,
                                         m_isManuallyInhibitedError,
                                         false,
                                         &PowerManagmentControl::isManuallyInhibitedErrorChanged)

    bool m_isSilent = false;

    ApplicationData m_data;
};
