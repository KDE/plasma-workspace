/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KStatusNotifierItem>

class Klipper;

class KlipperTray : public KStatusNotifierItem
{
    Q_OBJECT

public:
    KlipperTray();
    ~KlipperTray() override;

public Q_SLOTS:
    void slotSetToolTipFromHistory();

private:
    Klipper *m_klipper;
};
