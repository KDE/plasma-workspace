/*
    SPDX-FileCopyrightText: 2022 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#define QT_FORCE_ASSERTS 1

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

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
        QQmlComponent abstractAuthenticatorsComp(engine, "org.kde.kscreenlocker", "AbstractAuthenticators", QQmlComponent::PreferSynchronous, engine);
        QObject *authenticator = abstractAuthenticatorsComp.create(engine->rootContext());
        Q_ASSERT(authenticator);
        engine->rootContext()->setContextProperty("authenticator", authenticator);
        engine->rootContext()->setContextProperty("kscreenlocker_userName", "testUser");
        engine->rootContext()->setContextProperty("kscreenlocker_userImage", "");
        engine->rootContext()->setContextProperty("defaultToSwitchUser", false);

        // not part of kscreenlocker, but used for a test
        engine->rootContext()->setContextProperty("engine", engine);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(lockscreen, Setup)

#include "main.moc"
