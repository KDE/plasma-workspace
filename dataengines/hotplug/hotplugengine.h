/*
    SPDX-FileCopyrightText: 2007 Menard Alexis <darktears31@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <Plasma5Support/DataEngine>
#include <Plasma5Support/Service>

#include <Solid/Predicate>

class KDirWatch;

/**
 * This class is connected with solid, filter devices and provide signal with source for applet in Plasma
 */
class HotplugEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT

public:
    HotplugEngine(QObject *parent, const QVariantList &args);
    ~HotplugEngine() override;
    void init();
    Plasma5Support::Service *serviceForSource(const QString &source) override;

protected Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);

private:
    void handleDeviceAdded(Solid::Device &dev, bool added = true);
    void findPredicates();
    QStringList predicatesForDevice(Solid::Device &device) const;
    QVariantList actionsForPredicates(const QStringList &predicates) const;

private Q_SLOTS:
    void processNextStartupDevice();
    void updatePredicates(const QString &path);

private:
    QHash<QString, Solid::Predicate> m_predicates;
    QHash<QString, Solid::Device> m_startList;
    QHash<QString, Solid::Device> m_devices;
    Solid::Predicate m_encryptedPredicate;
    KDirWatch *m_dirWatch;
};
