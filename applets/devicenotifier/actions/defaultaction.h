/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "actioninterface.h"

#include <QObject>

class DefaultAction : public ActionInterface
{
    Q_OBJECT

    Q_INTERFACES(ActionInterface)

public:
    explicit DefaultAction(const QString &udi, const QString &desktopFile, QObject *parent = nullptr);
    ~DefaultAction() override;

    QString name() const override;
    QString predicate() const override;

    bool isValid() const override;

    QString icon() const override;
    QString text() const override;

private:
    QString m_icon;
    QString m_text;

    bool m_isValid;

    QString m_predicate;
};
