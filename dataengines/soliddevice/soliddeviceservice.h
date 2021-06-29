/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <Plasma/Service>

class SolidDeviceEngine;

class SolidDeviceService : public Plasma::Service
{
    Q_OBJECT

public:
    SolidDeviceService(SolidDeviceEngine *parent, const QString &source);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    SolidDeviceEngine *m_engine;
    QString m_dest;
};
