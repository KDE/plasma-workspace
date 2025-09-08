/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipperinterface.h"
#include "klipper.h"

KlipperInterface::KlipperInterface(QObject *parent)
    : QObject(parent)
    , m_klipper(Klipper::self())
{
}

KlipperInterface::~KlipperInterface() = default;

void KlipperInterface::configure()
{
    m_klipper->slotConfigure();
}

#include "moc_klipperinterface.cpp"
