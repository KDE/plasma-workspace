/*
    SPDX-FileCopyrightText: 2014 Martin Yrjölä <martin.yrjola@gmail.com>
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QQmlExtensionPlugin>

class TriangleMouseFilterPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};
