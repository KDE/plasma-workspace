/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef SHELLPRIVATEPLUGIN_H
#define SHELLPRIVATEPLUGIN_H

#include <QQmlExtensionPlugin>

class PlasmaShellPrivatePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

#endif
