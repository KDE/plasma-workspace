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

#include "LoginManager.hpp"

class Response
{
public:
    enum Type {
        Invalid = 0, /* Response was invalid */
        Success = 1, /* Request was a success */
        Error = 2, /* Request failed */
        AuthMessage = 3, /* Request succeeded, auth required */
    };

    enum AuthType {
        AuthTypeInvalid = 0, /* Not an Authentication Response */
        AuthTypeVisible = 1, /* Authentication in visible form */
        AuthTypeSecret = 2, /* Authentication in secret form */
        AuthTypeInfo = 3, /* Authentication request was Information */
        AuthTypeError = 4, /* Authentication failed */
    };

    enum ErrorType {
        ErrorTypeNone = 0, /* No Error */
        ErrorTypeGeneric = 1, /* Generic error */
        ErrorTypeAuthError = 2, /* Authentication error, typically bad credentials */
    };

    /** We will be given a json string. Parse it */
    Response(QByteArray);

    /** Get the response from greetd server */
    Response::Type responseType();

    /**
     * Auth type and message
     *  If responseType() != Response::Type::AuthMessage,
     *  authType() will return AuthTypeNone and authMessage()
     *  will return empty string.
     */
    Response::AuthType authType();
    QString authMessage();

    /**
     * Error type and message
     *  If responseType() != Response::Type::Error,
     *  errorType() will return ErrorTypeNone and errorMessage()
     *  will return empty string.
     */
    Response::ErrorType errorType();
    QString errorMessage();

private:
    Response::Type mType = Invalid;
    Response::AuthType mAuthType = AuthTypeInvalid;
    Response::ErrorType mErrorType = ErrorTypeNone;

    QString mAuthMsg;
    QString mErrorMsg;
};

class GreetdLogin : public LoginManager
{
    Q_OBJECT;

public:
    GreetdLogin();

    virtual bool authenticate(QString user, QString passwd) override;
    virtual bool startSession(QString command, QString type) override;
    virtual QString errorMessage() override;

private:
    /** Send create_session message to greetd-ipc */
    Response createSession(QString username);

    /** Send post_auth_message_response message to greetd-ipc */
    Response postResponse(QString response = QString());

    /** Send start_session message to greetd-ipc */
    Response startSession(QString cmd, QStringList);

    /** Send cancel_session message to greetd-ipc */
    Response cancelSession();

    bool connectToServer();
    QByteArray roundtrip(QByteArray);

    /** Get the X11 Session String */
    QString getX11Session(QString command);

    /** Prepare the environment variables */
    QStringList prepareEnv();

    /** GreetD socket FD */
    int mFD = -1;

    /** Error message */
    QString mErrorMsg;
};
