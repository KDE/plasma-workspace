/*
 * SPDX-FileCopyrightText: 2025 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "actioninterface.h"

class DevicesStateMonitor;

class CheckAction : public ActionInterface
{
    Q_OBJECT

    Q_INTERFACES(ActionInterface)

public:
    explicit CheckAction(const QString &udi, QObject *parent = nullptr);
    ~CheckAction() override;

    void triggered() override;

    bool isValid() const override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

private Q_SLOTS:
    void updateIsValid(const QString &udi);

private:
    std::shared_ptr<DevicesStateMonitor> m_stateMonitor;
};
