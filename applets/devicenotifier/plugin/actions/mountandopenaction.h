/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include <Solid/SolidNamespace>

#include "actioninterface.h"

#include <devicestatemonitor_p.h>

class MountAndOpenAction : public ActionInterface
{
    Q_OBJECT

    Q_INTERFACES(ActionInterface)

public:
    explicit MountAndOpenAction(const QString &udi, QObject *parent = nullptr);
    ~MountAndOpenAction() override;

    QString predicate() const override;
    bool isValid() const override;

    void triggered() override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

private Q_SLOTS:
    void updateAction(const QString &udi);

private:
    bool m_hasStorageAccess;
    bool m_isOpticalDisk;
    bool m_isRoot;

    bool m_hasPortableMediaPlayer;
    QStringList m_supportedProtocols;

    QString m_icon;
    QString m_text;

    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
};
