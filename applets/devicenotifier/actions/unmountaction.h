/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "actioninterface.h"

#include "stateinfo.h"

class UnmountAction : public ActionInterface
{
    Q_OBJECT

public:
    explicit UnmountAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent = nullptr);
    ~UnmountAction() override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

    bool isValid() const override;

    void triggered() override;

private Q_SLOTS:
    void updateIsValid(const QString &udi);

private:
    bool m_hasStorageAccess;
    bool m_isRoot;
};
