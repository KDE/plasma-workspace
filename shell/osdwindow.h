/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <PlasmaQuick/PlasmaWindow>
#include <qqmlregistration.h>

class OsdWindow : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit OsdWindow();
};
