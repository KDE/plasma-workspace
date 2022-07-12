/*
    SPDX-FileCopyrightText: 2022 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QDebug>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

class MockAuthenticator : public QObject
{
    Q_OBJECT
public:
    MockAuthenticator(QObject *parent)
        : QObject(parent)
    {
    }
Q_SIGNALS:
    void promptForSecret(const QString &msg);
    void prompt(const QString &msg);
    void infoMessage(const QString &msg);
    void errorMessage(const QString &msg);
    void succeeded();
    void failed();

public Q_SLOTS:
    void tryUnlock();
    void respond(const QByteArray &response);
    void cancel();

private:
    bool inPrompt = false;
};

void MockAuthenticator::tryUnlock()
{
    QTimer::singleShot(0, this, [this]() {
        Q_EMIT promptForSecret("Password:");
        inPrompt = true;
    });
}

void MockAuthenticator::respond(const QByteArray &response)
{
    QTimer::singleShot(0, this, [this, response]() {
        if (!inPrompt) {
            return;
        }

        if (response == "myPassword") {
            Q_EMIT succeeded();
        } else {
            Q_EMIT(failed());
        }
        inPrompt = false;
    });
}

void MockAuthenticator::cancel()
{
    inPrompt = false;
}

class Setup : public QObject
{
    Q_OBJECT
public:
    Setup()
    {
    }
public Q_SLOTS:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty("authenticator", new MockAuthenticator(this));
        engine->rootContext()->setContextProperty("kscreenlocker_userName", "testUser");
        engine->rootContext()->setContextProperty("kscreenlocker_userImage", "");
        engine->rootContext()->setContextProperty("defaultToSwitchUser", false);

        // not part of kscreenlocker, but used for a test
        engine->rootContext()->setContextProperty("engine", engine);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(lockscreen, Setup)

#include "main.moc"
