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

#include <kactioncollection.h>
#include <kdeclarative/configpropertymap.h>
#include <klocalizedstring.h>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

#include "scriptengine.h"
#include "shellcorona.h"
#include "widget.h"

namespace WorkspaceScripting
{
class Containment::Private
{
public:
    QPointer<Plasma::Containment> containment;
    ShellCorona *corona;
    QString oldWallpaperPlugin;
    QString wallpaperPlugin;
    QString oldWallpaperMode;
    QString wallpaperMode;

    QString type;
    QString plugin;
};

Containment::Containment(Plasma::Containment *containment, ScriptEngine *engine)
    : Applet(engine)
    , d(new Containment::Private)
{
    d->containment = containment;
    d->corona = qobject_cast<ShellCorona *>(containment->corona());

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
        if (d->oldWallpaperPlugin != d->wallpaperPlugin || d->oldWallpaperMode != d->wallpaperMode) {
            containment->setWallpaper(d->wallpaperPlugin);
        }
    }

    reloadConfigIfNeeded();

    delete d;
}

ShellCorona *Containment::corona() const
{
    return d->corona;
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
    case Plasma::Types::MediaCenter:
        return QStringLiteral("mediacenter");
    case Plasma::Types::Horizontal:
        return QStringLiteral("horizontal");
    case Plasma::Types::Vertical:
        return QStringLiteral("vertical");
    case Plasma::Types::Application:
        return QStringLiteral("application");
    }

    return QStringLiteral("Planar");
}

QList<int> Containment::widgetIds() const
{
    // FIXME: the ints could overflow since Applet::id() returns a uint,
    //       however QScript deals with QList<uint> very, very poory
    QList<int> w;

    if (d->containment) {
        foreach (const Plasma::Applet *applet, d->containment.data()->applets()) {
            w.append(applet->id());
        }
    }

    return w;
}

QJSValue Containment::widgetById(const QJSValue &paramId) const
{
    if (!paramId.isNumber()) {
        return engine()->newError(i18n("widgetById requires an id"));
    }

    const uint id = paramId.toInt();

    if (d->containment) {
        foreach (Plasma::Applet *w, d->containment.data()->applets()) {
            if (w->id() == id) {
                return engine()->wrap(w);
            }
        }
    }

    return QJSValue();
}

QJSValue Containment::addWidget(const QJSValue &v, qreal x, qreal y, qreal w, qreal h, const QVariantList &args)
{
    if (!v.isString() && !v.isQObject()) {
        return engine()->newError(i18n("addWidget requires a name of a widget or a widget object"));
    }

    if (!d->containment) {
        return QJSValue();
    }

    QRectF geometry(x, y, w, h);

    Plasma::Applet *applet = nullptr;
    if (v.isString()) {
        // A position has been supplied: search for the containment's graphics object
        QQuickItem *containmentItem = nullptr;

        if (geometry.x() >= 0 && geometry.y() >= 0) {
            containmentItem = d->containment.data()->property("_plasma_graphicObject").value<QQuickItem *>();

            if (containmentItem) {
                QMetaObject::invokeMethod(containmentItem,
                                          "createApplet",
                                          Qt::DirectConnection,
                                          Q_RETURN_ARG(Plasma::Applet *, applet),
                                          Q_ARG(QString, v.toString()),
                                          Q_ARG(QVariantList, args),
                                          Q_ARG(QRectF, geometry));
            }
            if (applet) {
                return engine()->wrap(applet);
            }
            return engine()->newError(i18n("Could not create the %1 widget!", v.toString()));
        }

        // Case in which either:
        // * a geometry wasn't provided
        // * containmentItem wasn't found
        applet = d->containment.data()->createApplet(v.toString(), args);

        if (applet) {
            return engine()->wrap(applet);
        }

        return engine()->newError(i18n("Could not create the %1 widget!", v.toString()));
    } else if (Widget *widget = qobject_cast<Widget *>(v.toQObject())) {
        applet = widget->applet();
        d->containment.data()->addApplet(applet);
        return v;
    }

    return QJSValue();
}

QJSValue Containment::widgets(const QString &widgetType) const
{
    if (!d->containment) {
        return QJSValue();
    }

    QJSValue widgets = engine()->newArray();
    int count = 0;

    foreach (Plasma::Applet *widget, d->containment.data()->applets()) {
        if (widgetType.isEmpty() || widget->pluginMetaData().pluginId() == widgetType) {
            widgets.setProperty(count, engine()->wrap(widget));
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

    return d->containment.data()->pluginMetaData().pluginId();
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
