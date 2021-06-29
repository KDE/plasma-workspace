/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "verticalmenu.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

VerticalMenu::VerticalMenu(QWidget *parent)
    : QMenu(parent)
{
}

VerticalMenu::~VerticalMenu()
{
}
