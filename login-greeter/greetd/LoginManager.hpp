/**
 * Copyright 2020-2022 Britanicus <marcusbritanicus@gmail.com>
 * This file is a part of QtGreet project (https://gitlab.com/marcusbritanicus/QtGreet)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 **/

#pragma once

#include <QObject>
#include <QString>
// #include "Global.hpp"

class LoginManager : public QObject
{
    Q_OBJECT;

public:
    LoginManager();

public Q_SLOTS:
    /** Make an attempt to authenticate the user @user using the password @passwd */
    virtual bool authenticate(QString user, QString passwd) = 0;

    /** User authentication was successful. Try to start the session */
    virtual bool startSession(QString command, QString type) = 0;

    /**
     * If authenticate/startSession failed, retrieve the error message.
     * The error message will be reset if another command is issued.
     */
    virtual QString errorMessage() = 0;
};
