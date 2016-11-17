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

#include "containment.h"

#include <QAction>
#include <QQuickItem>

#include <klocalizedstring.h>
#include <kactioncollection.h>
#include <kdeclarative/configpropertymap.h>

#include <Plasma/Corona>
#include <Plasma/Containment>
#include <Plasma/PluginLoader>

#include "scriptengine.h"
#include "widget.h"

namespace WorkspaceScripting
{

class Containment::Private
{
public:
    QPointer<Plasma::Containment> containment;
    QString oldWallpaperPlugin;
    QString wallpaperPlugin;
    QString oldWallpaperMode;
    QString wallpaperMode;
};

Containment::Containment(Plasma::Containment *containment, QObject *parent)
    : Applet(parent),
      d(new Containment::Private)
{
    d->containment = containment;
    setCurrentConfigGroup(QStringList());
    setCurrentGlobalConfigGroup(QStringList());
    if (containment) {
        d->oldWallpaperPlugin = d->wallpaperPlugin = containment->wallpaper();
    }
}

Containment::~Containment()
{
    if (d->containment) {
        Plasma::Containment *containment = d->containment.data();
        if (d->oldWallpaperPlugin != d->wallpaperPlugin ||
            d->oldWallpaperMode != d->wallpaperMode) {
            containment->setWallpaper(d->wallpaperPlugin);
        }
    }

    reloadConfigIfNeeded();

    delete d;
}

int Containment::screen() const
{
    if (!d->containment) {
        return -1;
    }

    return d->containment.data()->screen();
}

QString Containment::wallpaperPlugin() const
{
    return d->wallpaperPlugin;
}

void Containment::setWallpaperPlugin(const QString &wallpaperPlugin)
{
    d->wallpaperPlugin = wallpaperPlugin;
}


QString Containment::wallpaperMode() const
{
    return d->wallpaperMode;
}

void Containment::setWallpaperMode(const QString &wallpaperMode)
{
    d->wallpaperMode = wallpaperMode;
}

QString Containment::formFactor() const
{
    if (!d->containment) {
        return QStringLiteral("Planar");
    }

    switch (d->containment.data()->formFactor()) {
        case Plasma::Types::Planar:
            return QStringLiteral("planar");
            break;
        case Plasma::Types::MediaCenter:
            return QStringLiteral("mediacenter");
            break;
        case Plasma::Types::Horizontal:
            return QStringLiteral("horizontal");
            break;
        case Plasma::Types::Vertical:
            return QStringLiteral("vertical");
            break;
        case Plasma::Types::Application:
            return QStringLiteral("application");
            break;
    }

    return QStringLiteral("Planar");
}

QList<int> Containment::widgetIds() const
{
    //FIXME: the ints could overflow since Applet::id() returns a uint,
    //       however QScript deals with QList<uint> very, very poory
    QList<int> w;

    if (d->containment) {
        foreach (const Plasma::Applet *applet, d->containment.data()->applets()) {
            w.append(applet->id());
        }
    }

    return w;
}

QScriptValue Containment::widgetById(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("widgetById requires an id"));
    }

    const uint id = context->argument(0).toInt32();
    Containment *c = qobject_cast<Containment*>(context->thisObject().toQObject());

    if (!c) {
        return engine->undefinedValue();
    }

    if (c->d->containment) {
        foreach (Plasma::Applet *w, c->d->containment.data()->applets()) {
            if (w->id() == id) {
                ScriptEngine *env = ScriptEngine::envFor(engine);
                return env->wrap(w);
            }
        }
    }

    return engine->undefinedValue();
}

QScriptValue Containment::addWidget(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0) {
        return context->throwError(i18n("addWidget requires a name of a widget or a widget object"));
    }

    Containment *c = qobject_cast<Containment*>(context->thisObject().toQObject());

    if (!c || !c->d->containment) {
        return engine->undefinedValue();
    }

    QScriptValue v = context->argument(0);
    Plasma::Applet *applet = 0;
    QRectF geometry(-1, -1, -1, -1);
    if (context->argumentCount() > 4) {
        //The user provided a geometry as parameter
        if (context->argument(1).isNumber() &&
            context->argument(2).isNumber() &&
            context->argument(3).isNumber() &&
            context->argument(4).isNumber()) {
            //Try to reconstruct a rectangle from the object hat has been passed
            //It's expected a js object such as
            //addWidget("org.kde.plasma.analogclock", 0, 100, 300, 400);
            geometry = QRectF(context->argument(1).toNumber(),
                              context->argument(2).toNumber(),
                              context->argument(3).toNumber(),
                              context->argument(4).toNumber());
        }
    }
    if (v.isString()) {
        //A position has been supplied: search for the containment's graphics object
        QQuickItem *containmentItem = nullptr;

        if (geometry.x() >= 0 && geometry.y() >= 0) {
            containmentItem = c->d->containment.data()->property("_plasma_graphicObject").value<QQuickItem *>();
            Plasma::Applet *applet = nullptr;
            if (containmentItem) {
                QMetaObject::invokeMethod(containmentItem , "createApplet", Qt::DirectConnection, Q_RETURN_ARG(Plasma::Applet *, applet), Q_ARG(QString, v.toString()), Q_ARG(QVariantList, QVariantList()), Q_ARG(QRectF, geometry));
            }
            if (applet) {
                ScriptEngine *env = ScriptEngine::envFor(engine);
                return env->wrap(applet);
            }
        }

        //Case in which either:
        // * a geometry wasn't provided
        // * containmentItem wasn't found
        applet = c->d->containment.data()->createApplet(v.toString());

        if (applet) {
            ScriptEngine *env = ScriptEngine::envFor(engine);
            return env->wrap(applet);
        }
    } else if (Widget *widget = qobject_cast<Widget*>(v.toQObject())) {
        applet = widget->applet();
        c->d->containment.data()->addApplet(applet);
        return v;
    }

    return engine->undefinedValue();
}

QScriptValue Containment::widgets(QScriptContext *context, QScriptEngine *engine)
{
    Containment *c = qobject_cast<Containment*>(context->thisObject().toQObject());

    if (!c || !c->d->containment) {
        return engine->undefinedValue();
    }

    const QString widgetType = context->argumentCount() > 0 ? context->argument(0).toString() : QString();
    QScriptValue widgets = engine->newArray();
    ScriptEngine *env = ScriptEngine::envFor(engine);
    int count = 0;

    foreach (Plasma::Applet *widget, c->d->containment.data()->applets()) {
        if (widgetType.isEmpty() || widget->pluginInfo().pluginName() == widgetType) {
            widgets.setProperty(count, env->wrap(widget));
            ++count;
        }
    }

    widgets.setProperty(QStringLiteral("length"), count);
    return widgets;
}

uint Containment::id() const
{
    if (!d->containment) {
        return 0;
    }

    return d->containment.data()->id();
}

QString Containment::type() const
{
    if (!d->containment) {
        return QString();
    }

    return d->containment.data()->pluginInfo().pluginName();
}

void Containment::remove()
{
    if (d->containment) {
        d->containment.data()->destroy();
    }
}

void Containment::showConfigurationInterface()
{
    if (d->containment) {
        QAction *configAction = d->containment.data()->actions()->action(QStringLiteral("configure"));
        if (configAction && configAction->isEnabled()) {
            configAction->trigger();
        }
    }
}

Plasma::Applet *Containment::applet() const
{
    return d->containment.data();
}

Plasma::Containment *Containment::containment() const
{
    return d->containment.data();
}

}



