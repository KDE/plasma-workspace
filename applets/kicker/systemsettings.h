/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class SystemSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit SystemSettings(QObject *parent = nullptr);
    ~SystemSettings() override;

    Q_INVOKABLE QString picturesLocation() const;
};
