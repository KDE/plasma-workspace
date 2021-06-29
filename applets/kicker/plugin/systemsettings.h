/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SYSTEMSETTINGS_H
#define SYSTEMSETTINGS_H

#include <QObject>

class SystemSettings : public QObject
{
    Q_OBJECT

public:
    explicit SystemSettings(QObject *parent = nullptr);
    ~SystemSettings() override;

    Q_INVOKABLE QString picturesLocation() const;
};

#endif
