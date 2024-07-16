/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "actioninterface.h"

#include <devicestatemonitor_p.h>

class MountAction : public ActionInterface
{
    Q_OBJECT

    Q_INTERFACES(ActionInterface)

public:
    explicit MountAction(const QString &udi, QObject *parent = nullptr);
    ~MountAction() override;

    void triggered() override;

    bool isValid() const override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

private Q_SLOTS:
    void updateIsValid(const QString &udi);

private:
    bool m_supportsMTP;

    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
};
