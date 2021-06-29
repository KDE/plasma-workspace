/*
SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLORCORRECTPLUGIN_H
#define COLORCORRECTPLUGIN_H

#include <QQmlExtensionPlugin>

namespace ColorCorrect
{
class ColorCorrectPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

}

#endif // COLORCORRECTPLUGIN_H
