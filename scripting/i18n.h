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

#ifndef JAVASCRIPTBINDI18N_H
#define JAVASCRIPTBINDI18N_H

#include <QScriptValue>

class QScriptContext;
class QScriptEngine;

QScriptValue jsi18n(QScriptContext *context, QScriptEngine *engine);
QScriptValue jsi18nc(QScriptContext *context, QScriptEngine *engine);
QScriptValue jsi18np(QScriptContext *context, QScriptEngine *engine);
QScriptValue jsi18ncp(QScriptContext *context, QScriptEngine *engine);
void bindI18N(QScriptEngine *engine);

#endif

