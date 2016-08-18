/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SCRIPTENGINE_V1
#define SCRIPTENGINE_V1

#include <QScriptEngine>
#include <QScriptValue>

#include <QFontMetrics>

#include <kactivities/controller.h>

#include "../shellcorona.h"
#include "scriptengine.h"

namespace WorkspaceScripting
{

class ScriptEngine::V1 {
public:
    static QScriptValue createActivity(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue setCurrentActivity(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue currentActivity(QScriptContext *controller, QScriptEngine *engine);
    static QScriptValue activities(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue setActivityName(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue activityName(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue loadSerializedLayout(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue newPanel(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue desktopsForActivity(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue desktops(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue desktopById(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue desktopForScreen(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue panelById(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue panels(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue fileExists(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue loadTemplate(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue applicationExists(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue defaultApplication(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue applicationPath(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue userDataPath(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue knownWallpaperPlugins(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue configFile(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue gridUnit();
    static QScriptValue createContainment(const QString &type, const QString &defautPlugin,
                                          QScriptContext *context, QScriptEngine *engine);
};

}

#endif

