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

#include "GreetdManager.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <QDir>
#include <QFile>

#include <QCryptographicHash>
#include <QSettings>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

QString tmpPath = "/tmp/";
QSettings *sett; // this is not ok

/**
 * class Response
 * Converts the response recieved in the form of JSON into usable form
 */

Response::Response(QByteArray jsonResp)
{
    QJsonObject json = QJsonDocument::fromJson(jsonResp).object();

    if (!json.contains("type")) {
        return;
    }

    QString type = json["type"].toString();

    if (type == "success") {
        mType = Success;
    }

    else if (type == "error") {
        mType = Error;
    }

    else if (type == "auth_message") {
        mType = AuthMessage;
    }

    else {
        mType = Invalid;
    }

    if (mType == Error) {
        if (json["error_type"].toString() == "auth_error") {
            mErrorType = ErrorTypeAuthError;
        }

        else {
            mErrorType = ErrorTypeGeneric;
        }

        mErrorMsg = json["description"].toString();
    }

    if (mType == AuthMessage) {
        if (json["auth_message_type"].toString() == "visible") {
            mAuthType = AuthTypeVisible;
        }

        else if (json["auth_message_type"].toString() == "secret") {
            mAuthType = AuthTypeSecret;
        }

        else if (json["auth_message_type"].toString() == "info") {
            mAuthType = AuthTypeInfo;
        }

        else {
            mAuthType = AuthTypeError;
        }

        mAuthMsg = json["auth_message"].toString();
    }
}

Response::Type Response::responseType()
{
    return mType;
}

Response::AuthType Response::authType()
{
    return mAuthType;
}

QString Response::authMessage()
{
    return mAuthMsg;
}

Response::ErrorType Response::errorType()
{
    return mErrorType;
}

QString Response::errorMessage()
{
    return mErrorMsg;
}

/**
 * class GreetdLogin
 * This class perform the actual user authentication,
 * and then starts the session.
 */

GreetdLogin::GreetdLogin()
    : LoginManager()
{
    /** Nothing much */
    sett = new QSettings(); // what
}

bool GreetdLogin::authenticate(QString username, QString password)
{
    mErrorMsg.clear();

    Response resp = createSession(username);

    /**
     * Request failed: We probably failed to conenct to greetd.
     */
    if (resp.responseType() == Response::Invalid) {
        mErrorMsg = "Unable to communicate with greetd.";
        qDebug() << mErrorMsg;
        return false;
    }

    /**
     * Request successful: We can now start the session.
     * This may happen for password-less login.
     */
    if (resp.responseType() == Response::Success) {
        return true;
    }

    /**
     * Request failed: We can now start the session.
     * Something failed, we will cancel the session.
     */
    if (resp.responseType() == Response::Error) {
        if (resp.errorType() == Response::ErrorTypeGeneric) {
            qDebug() << "Generic error";
        }

        else {
            qDebug() << "Authentication error";
        }

        mErrorMsg = resp.errorMessage();
        resp = cancelSession();

        if (resp.responseType() != Response::Success) {
            mErrorMsg += "\nError while handling the previous error. Abort.";
        }

        qDebug() << mErrorMsg;

        return false;
    }

    /**
     * Request failed: We can now start the session.
     * Something failed, we will cancel the session.
     */
    if (resp.responseType() == Response::AuthMessage) {
        switch (resp.authType()) {
        /* We should never reach here. If we do, then abort. */
        case Response::AuthTypeInvalid: {
            qDebug() << "Not an auth_message response. Abort.";
            return false;
        }

        /**
         * User reply should be visible.
         * But this does not apply to us.
         * We already have a password which will be used.
         */
        case Response::AuthTypeVisible: {
            [[fallthrough]];
        }

        /**
         * User reply should be secret.
         * But this does not apply to us.
         * We already have a password which will be used.
         */
        case Response::AuthTypeSecret: {
            resp = postResponse(password);

            /** Authenticated! We can now start the session */
            if (resp.responseType() == Response::Success) {
                return true;
            }

            mErrorMsg = "Authentication failed.";
            resp = cancelSession();

            if (resp.responseType() == Response::Success) {
                mErrorMsg += "\nError while handling the previous error. Abort.";
            }

            qDebug() << mErrorMsg;

            return false;
        }

        /**
         * Providing info to the user.
         * Not applicable to us. Treat it as failure.
         * Cancel session and return false.
         */
        case Response::AuthTypeInfo: {
            [[fallthrough]];
        }

        /**
         * Providing info to the user.
         * Not applicable to us. Treat it as failure.
         * Cancel session and return false.
         */
        case Response::AuthTypeError: {
            mErrorMsg = resp.authMessage();
            resp = cancelSession();

            if (resp.responseType() != Response::Success) {
                mErrorMsg += "\nError while handling the previous error. Abort.";
            }

            qDebug() << mErrorMsg;

            return false;
        }
        }

        /** We should have never reached this point. */
        mErrorMsg = "Unknown error. Abort.";
        qDebug() << mErrorMsg;
        return false;
    }

    return false;
}

bool GreetdLogin::startSession(QString baseCmd, QString type)
{
    /** Get the log name */
    // QString logName = logPath + "/session/QtGreet-" + QDateTime::currentDateTime().toString( "ddMMyyyy-hhmmss" ) + ".log";

    QString cmd;

    if (type == "wayland") {
        cmd = baseCmd; //+ " > " + logName + " 2>&1";
    }

    else if (type == "X11") {
        cmd = getX11Session(baseCmd); // + " > " + logName + " 2>&1";
    }

    else {
        cmd = baseCmd;
    }

    /** Send the start_session request */
    Response resp = startSession(cmd, prepareEnv());

    if (resp.responseType() == Response::Success) {
        return true;
    }

    qDebug() << "Failed to launch the session:" << baseCmd;
    mErrorMsg = resp.errorMessage();
    resp = cancelSession();

    if (resp.responseType() != Response::Success) {
        mErrorMsg += "\nError while handling the previous error. Abort.";
    }

    qDebug() << mErrorMsg;

    return false;
}

QString GreetdLogin::errorMessage()
{
    return mErrorMsg;
}

QString GreetdLogin::getX11Session(QString base)
{
    QString xinit("xinit %1 -- %2 :%3 vt%4 -keeptty -noreset -novtswitch -auth %5/Xauth.%6");

    /* Arg2: Get the display */
    int display;

    for (display = 0; display < 64; display++) {
        QString x1 = QString(tmpPath + "/.X%1-lock").arg(display);
        QString x2 = QString(tmpPath + "/.X11-unix/X%1").arg(display);

        if (QFile::exists(x1) || QFile::exists(x2)) {
            continue;
        }

        else {
            break;
        }
    }

    /* Arg4: Random strings for server auth file */
    QString hash = QCryptographicHash::hash(QDateTime::currentDateTime().toString().toUtf8(), QCryptographicHash::Md5).toHex().left(10);

    return QString();
    // return xinit.arg( base ).arg( xrcPath ).arg( display ).arg( vtNum ).arg( tmpPath ).arg( hash );
}

QStringList GreetdLogin::prepareEnv()
{
    QStringList env;

    sett->beginGroup("Environment");
    for (QString key : sett->allKeys()) {
        env << QString("%1=%2").arg(key).arg(sett->value(key).toString());
    }
    sett->endGroup();

    QSettings envSett("/etc/environment", QSettings::IniFormat);

    for (QString key : envSett.allKeys()) {
        env << QString("%1=%2").arg(key).arg(envSett.value(key).toString());
    }

    QDir envDir("/etc/environment.d");

    for (QFileInfo info : envDir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name)) {
        QSettings s(info.filePath(), QSettings::IniFormat);
        for (QString key : s.allKeys()) {
            env << QString("%1=%2").arg(key).arg(s.value(key).toString());
        }
    }

    return env;
}

Response GreetdLogin::createSession(QString username)
{
    QVariantMap request;

    request["type"] = "create_session";
    request["username"] = username;

    QJsonDocument json;

    json.setObject(QJsonObject::fromVariantMap(request));

    QByteArray response = roundtrip(json.toJson().simplified());

    return Response(response);
}

Response GreetdLogin::postResponse(QString response)
{
    QVariantMap request;

    request["type"] = "post_auth_message_response";

    if (!response.isEmpty()) {
        request["response"] = response;
    }

    QJsonDocument json;

    json.setObject(QJsonObject::fromVariantMap(request));

    QByteArray respJson = roundtrip(json.toJson().simplified());

    return Response(respJson);
}

Response GreetdLogin::startSession(QString cmd, QStringList env)
{
    QVariantMap request;

    request["type"] = "start_session";
    request["cmd"] = QStringList() << cmd;

    if (env.count()) {
        request["env"] = env;
    }

    QJsonDocument json;

    json.setObject(QJsonObject::fromVariantMap(request));

    QByteArray response = roundtrip(json.toJson().simplified());

    return Response(response);
}

Response GreetdLogin::cancelSession()
{
    QVariantMap request;

    request["type"] = "cancel_session";

    QJsonDocument json;

    json.setObject(QJsonObject::fromVariantMap(request));

    QByteArray response = roundtrip(json.toJson().simplified());

    return Response(response);
}

bool GreetdLogin::connectToServer()
{
    if (mFD > 0) {
        close(mFD);
    }

    mFD = socket(AF_UNIX, SOCK_STREAM, 0);

    if (mFD < 0) {
        qDebug() << "Unable to open the socket";
        return false;
    }

    struct sockaddr_un addr;

    QString path = qgetenv("GREETD_SOCK");

    if (path.isEmpty()) {
        qDebug() << "Unable to retrieve GREETD_SOCK";
        close(mFD);

        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path.toLocal8Bit().constData());

    if (::connect(mFD, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        qDebug() << "Connection error:" << strerror(errno);
        close(mFD);
        return false;
    }

    return true;
}

QByteArray GreetdLogin::roundtrip(QByteArray payload)
{
    if (!connectToServer()) {
        return QByteArray();
    }

    /** 32-bit representation of the payload length */
    uint32_t length = payload.size();
    char *chrlen = (char *)&length;

    /** chrlen will be 4 bytes long */
    if (write(mFD, chrlen, 4) != 4) {
        qDebug() << "Error sending message";
        return QByteArray();
    }

    if (write(mFD, payload.constData(), length) != payload.size()) {
        qDebug() << "Error sending message";
        return QByteArray();
    }

    uint32_t retLen;
    char *chrRetLen = (char *)&retLen;

    if (read(mFD, chrRetLen, 4) != 4) {
        qDebug() << "Error reading response from the server";
        return QByteArray();
    }

    char message[retLen + 1] = {'\0'};

    memset(message, '\0', retLen + 1);

    if (read(mFD, message, retLen) != retLen) {
        qDebug() << "Error reading response from the server";
        return QByteArray();
    }

    QByteArray respJson = message;

    close(mFD);

    return respJson;
}
