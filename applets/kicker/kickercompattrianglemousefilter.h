/*
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "trianglemousefilter.h"

class KickerCompatTriangleMouseFilter : public TriangleMouseFilter
{
    Q_OBJECT
    QML_ELEMENT

public:
    KickerCompatTriangleMouseFilter(QQuickItem *parent = nullptr);
    ~KickerCompatTriangleMouseFilter() = default;
};
