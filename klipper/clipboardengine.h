/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Plasma5Support/DataEngine>

class Klipper;

class ClipboardEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT
public:
    ClipboardEngine(QObject *parent);
    ~ClipboardEngine() override;

    Plasma5Support::Service *serviceForSource(const QString &source) override;

private:
    Klipper *m_klipper;
};
