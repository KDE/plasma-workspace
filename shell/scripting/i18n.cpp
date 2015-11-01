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

#include "i18n.h"

#include <QScriptContext>
#include <QScriptEngine>

#include <QDebug>
#include <klocalizedstring.h>

QScriptValue jsi18n(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 1) {
        // qDebug() << i18n("i18n() takes at least one argument");
        return engine->undefinedValue();
    }

    KLocalizedString message = ki18n(context->argument(0).toString().toUtf8());

    const int numArgs = context->argumentCount();
    for (int i = 1; i < numArgs; ++i) {
        message = message.subs(context->argument(i).toString());
    }

    return message.toString();
}

QScriptValue jsi18nc(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 2) {
        // qDebug() << i18n("i18nc() takes at least two arguments");
        return engine->undefinedValue();
    }

    KLocalizedString message = ki18nc(context->argument(0).toString().toUtf8(),
                                      context->argument(1).toString().toUtf8());

    const int numArgs = context->argumentCount();
    for (int i = 2; i < numArgs; ++i) {
        message = message.subs(context->argument(i).toString());
    }

    return message.toString();
}

QScriptValue jsi18np(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 2) {
        // qDebug() << i18n("i18np() takes at least two arguments");
        return engine->undefinedValue();
    }

    KLocalizedString message = ki18np(context->argument(0).toString().toUtf8(),
                                      context->argument(1).toString().toUtf8());

    const int numArgs = context->argumentCount();
    for (int i = 2; i < numArgs; ++i) {
        QScriptValue v = context->argument(i);
        if (v.isNumber()) {
            message = message.subs(v.toInt32());
        } else {
            message = message.subs(v.toString());
        }
    }

    return message.toString();
}

QScriptValue jsi18ncp(QScriptContext *context, QScriptEngine *engine)
{
    Q_UNUSED(engine)

    if (context->argumentCount() < 3) {
        // qDebug() << i18n("i18ncp() takes at least three arguments");
        return engine->undefinedValue();
    }

    KLocalizedString message = ki18ncp(context->argument(0).toString().toUtf8(),
                                       context->argument(1).toString().toUtf8(),
                                       context->argument(2).toString().toUtf8());

    const int numArgs = context->argumentCount();
    for (int i = 3; i < numArgs; ++i) {
        message = message.subs(context->argument(i).toString());
    }

    return message.toString();
}

void bindI18N(QScriptEngine *engine)
{
    QScriptValue global = engine->globalObject();
    global.setProperty(QStringLiteral("i18n"), engine->newFunction(jsi18n));
    global.setProperty(QStringLiteral("i18nc"), engine->newFunction(jsi18nc));
    global.setProperty(QStringLiteral("i18np"), engine->newFunction(jsi18np));
    global.setProperty(QStringLiteral("i18ncp"), engine->newFunction(jsi18ncp));
}

