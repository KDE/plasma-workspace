/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObjectBindableProperty>
#include <QSortFilterProxyModel>
#include <QStack>

#include <qqmlregistration.h>

#include <devicestatemonitor_p.h>
#include <spacemonitor_p.h>

class DeviceFilterControl : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isVisible READ isVisible WRITE setIsVisible);
    Q_PROPERTY(DevicesType filterType READ filterType WRITE setFilterType)

    Q_PROPERTY(qsizetype deviceCount READ default NOTIFY deviceCountChanged BINDABLE bindableDeviceCount)
    Q_PROPERTY(qsizetype unmountableCount READ default NOTIFY unmountableCountChanged BINDABLE bindableUnmountableCount)

    Q_PROPERTY(QString lastUdi READ default NOTIFY lastUdiChanged BINDABLE bindableLastUdi)
    Q_PROPERTY(QString lastDescription READ default NOTIFY lastDescriptionChanged BINDABLE bindableLastDescription)
    Q_PROPERTY(QString lastIcon READ default NOTIFY lastIconChanged BINDABLE bindableLastIcon)
    Q_PROPERTY(bool lastDeviceAdded READ default NOTIFY lastDeviceAddedChanged BINDABLE bindableLastDeviceAdded)

public:
    enum DevicesType {
        All = 0,
        Removable,
        Unremovable,
    };

    Q_ENUM(DevicesType)

    Q_INVOKABLE void unmountAllRemovables();

    explicit DeviceFilterControl(QObject *parent = nullptr);
    ~DeviceFilterControl() override;

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

    bool isVisible() const;
    void setIsVisible(bool status);

    DevicesType filterType() const;
    void setFilterType(DevicesType type);

    QBindable<QString> bindableLastUdi();
    QBindable<QString> bindableLastDescription();
    QBindable<QString> bindableLastIcon();
    QBindable<bool> bindableLastDeviceAdded();

    QBindable<qsizetype> bindableDeviceCount();
    QBindable<qsizetype> bindableUnmountableCount();

Q_SIGNALS:
    void lastUdiChanged();
    void lastDescriptionChanged();
    void lastIconChanged();
    void lastDeviceAddedChanged();

    void deviceCountChanged();
    void unmountableCountChanged();

private Q_SLOTS:
    void onDeviceAdded(const QModelIndex &parent, int first, int last);
    void onDeviceRemoved(const QModelIndex &parent, int first, int last);
    void onModelReset();

    void onDeviceActionUnmountableChanged(const QString &udi, bool status);

private:
    void handleDeviceAdded(const QModelIndex &index);

    Q_OBJECT_BINDABLE_PROPERTY(DeviceFilterControl, QString, m_lastUdi, &DeviceFilterControl::lastUdiChanged);
    Q_OBJECT_BINDABLE_PROPERTY(DeviceFilterControl, QString, m_lastDescription, &DeviceFilterControl::lastDescriptionChanged);
    Q_OBJECT_BINDABLE_PROPERTY(DeviceFilterControl, QString, m_lastIcon, &DeviceFilterControl::lastIconChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DeviceFilterControl, bool, m_lastDeviceAdded, false, &DeviceFilterControl::lastDeviceAddedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DeviceFilterControl, qsizetype, m_deviceCount, 0, &DeviceFilterControl::deviceCountChanged);
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DeviceFilterControl, qsizetype, m_unmountableCount, 0, &DeviceFilterControl::unmountableCountChanged);

    DevicesType m_filterType;
    bool m_isVisible;
    bool m_modelReset;

    QSet<QString> m_unmountableDevices;
    QStack<QString> m_deviceOrder;

    std::shared_ptr<SpaceMonitor> m_spaceMonitor;
};
