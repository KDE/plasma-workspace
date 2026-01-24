/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "plasmashellwaydroidapplicationinterface.h"

#include <QObject>
#include <QString>

#include <qqmlregistration.h>

/**
 * This class provides a DBus client interface for individual Waydroid application.
 * It connects to WaydroidApplicationDBusObject instances via DBus.
 *
 * @author Florian RICHER <florian.richer@protonmail.com>
 */
class WaydroidApplicationDBusClient : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("")
    QML_ELEMENT

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString packageName READ packageName NOTIFY packageNameChanged)

public:
    typedef std::shared_ptr<WaydroidApplicationDBusClient> Ptr;

    explicit WaydroidApplicationDBusClient(const QDBusObjectPath &objectPath, QObject *parent = nullptr);

    QString name() const;
    QString packageName() const;
    QDBusObjectPath objectPath() const;

private Q_SLOTS:
    void updateName();
    void updatePackageName();

Q_SIGNALS:
    void nameChanged();
    void packageNameChanged();

private:
    OrgKdePlasmashellWaydroidApplicationInterface *m_interface;
    QDBusServiceWatcher *m_watcher;

    QString m_packageName;
    QString m_name;
    QDBusObjectPath m_objectPath;
    bool m_connected{false};

    void connectSignals();
};