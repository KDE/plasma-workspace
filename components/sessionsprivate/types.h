/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QQmlEngine>

#include <sessionmanagement.h>

struct SessionManagementForeign {
    Q_GADGET
    QML_NAMED_ELEMENT(SessionManagement)
    QML_FOREIGN(SessionManagement)
};
