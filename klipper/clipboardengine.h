/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Plasma/DataEngine>

class Klipper;

class ClipboardEngine : public Plasma::DataEngine
{
    Q_OBJECT
public:
    ClipboardEngine(QObject *parent, const QVariantList &args);
    ~ClipboardEngine() override;

    Plasma::Service *serviceForSource(const QString &source) override;

private:
    Klipper *m_klipper;
};
