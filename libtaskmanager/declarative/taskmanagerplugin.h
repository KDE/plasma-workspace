/*
SPDX-FileCopyrightText: 2015-2016 Eike Hein <hein.org>

SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef TASKMANAGERPLUGIN_H
#define TASKMANAGERPLUGIN_H

#include <QQmlEngine>
#include <QQmlExtensionPlugin>

namespace TaskManager
{
class TaskManagerPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

}

#endif
