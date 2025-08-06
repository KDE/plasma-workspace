/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "actioninterface.h"

#include <stateinfo.h>

class MountAction : public ActionInterface
{
    Q_OBJECT

public:
    explicit MountAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent = nullptr);
    ~MountAction() override;

    void triggered() override;

    bool isValid() const override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

private Q_SLOTS:
    void updateIsValid();

private:
    bool m_supportsMTP;
    bool m_hasStorageAccess;
};
