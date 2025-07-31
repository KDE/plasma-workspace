/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <actioninterface.h>

#include <stateinfo.h>

class OpenWithFileManagerAction : public ActionInterface
{
    Q_OBJECT

public:
    explicit OpenWithFileManagerAction(const std::shared_ptr<StorageInfo> &storageInfo, const std::shared_ptr<StateInfo> &stateInfo, QObject *parent);
    ~OpenWithFileManagerAction() override;

    QString predicate() const override;
    bool isValid() const override;

    QString name() const override;
    QString icon() const override;
    QString text() const override;

private Q_SLOTS:
    void updateIsValid(const QString &udi);

private:
    QString m_icon;
    QString m_text;

    bool m_isActionValid;

    std::shared_ptr<StateInfo> m_stateInfo;
};
