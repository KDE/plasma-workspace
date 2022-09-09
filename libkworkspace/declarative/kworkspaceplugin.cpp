/*
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QQmlExtensionPlugin>
#include <qqml.h>
#include <sessionmanagement.h>

class TaskManagerPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override
    {
        qmlRegisterType<SessionManagement>(uri, 1, 0, "SessionManagement");
    }
};

#include "kworkspaceplugin.moc"
