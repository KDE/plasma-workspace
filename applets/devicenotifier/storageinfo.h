/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>

#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/Predicate>

// TODO: implement in libsolid2
template<class DevIface>
DevIface *getAncestorAs(const Solid::Device &device)
{
    for (Solid::Device parent = device.parent(); parent.isValid(); parent = parent.parent()) {
        if (parent.is<DevIface>()) {
            return parent.as<DevIface>();
        }
    }
    return nullptr;
}

class StorageInfo : public QObject
{
    Q_OBJECT

public:
    explicit StorageInfo(const QString &udi, QObject *parent = nullptr);
    ~StorageInfo() override;

    static Solid::Predicate predicate();

    bool isValid() const;

    bool isEncrypted() const;
    bool hasRemovableParent() const;
    bool isRemovable() const;
    QString type() const;
    QString icon() const;
    QString description() const;
    const Solid::Device &device() const;

private:
    bool m_isValid;
    bool m_isEncrypted;
    bool m_isRemovable;
    bool m_hasRemovableParent;
    QString m_type;
    QString m_icon;
    QString m_description;
    Solid::Device m_device;

    Solid::Predicate m_predicateDeviceMatch;

    static const Solid::Predicate m_encryptedPredicate;

    static const QList<Solid::DeviceInterface::Type> m_types;
};
