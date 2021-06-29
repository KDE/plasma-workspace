/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <Plasma/Service>

class HotplugEngine;

class HotplugService : public Plasma::Service
{
    Q_OBJECT

public:
    HotplugService(HotplugEngine *parent, const QString &source);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    HotplugEngine *m_engine;
    QString m_dest;
};
