/*
This file is part of the KDE project.

SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KLIPPER_CLIPBOARDENGINE_H
#define KLIPPER_CLIPBOARDENGINE_H

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

#endif
