/*
    SPDX-FileCopyrightText: 2024 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <kdedmodule.h>

class DonationMessage : public KDEDModule
{
    Q_OBJECT
public:
    DonationMessage(QObject *parent, const QList<QVariant> &);

private:
    void donate();
    void suppressForThisYear();
};
