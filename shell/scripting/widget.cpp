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

#include "widget.h"
#include "scriptengine.h"

#include <QAction>
#include <QMetaEnum>
#include <QQuickItem>

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>

namespace WorkspaceScripting
{
class Widget::Private
{
public:
    Private()
    {
    }

    QPointer<Plasma::Applet> applet;
};

Widget::Widget(Plasma::Applet *applet, ScriptEngine *parent)
    : Applet(parent)
    , d(new Widget::Private)
{
    d->applet = applet;
    setCurrentConfigGroup(QStringList());
    setCurrentGlobalConfigGroup(QStringList());
}

Widget::~Widget()
{
    reloadConfigIfNeeded();
    delete d;
}

uint Widget::id() const
{
    if (d->applet) {
        return d->applet.data()->id();
    }

    return 0;
}

QString Widget::type() const
{
    if (d->applet) {
        return d->applet.data()->pluginMetaData().pluginId();
    }

    return QString();
}

void Widget::remove()
{
    if (d->applet) {
        d->applet.data()->destroy();
        d->applet.clear();
    }
}

void Widget::setGlobalShortcut(const QString &shortcut)
{
    if (d->applet) {
        d->applet.data()->setGlobalShortcut(QKeySequence(shortcut));
    }
}

QString Widget::globalShorcut() const
{
    if (d->applet) {
        return d->applet.data()->globalShortcut().toString();
    }

    return QString();
}

Plasma::Applet *Widget::applet() const
{
    return d->applet.data();
}

int Widget::index() const
{
    if (!d->applet) {
        return -1;
    }

    Plasma::Applet *applet = d->applet.data();
    Plasma::Containment *c = applet->containment();
    if (!c) {
        return -1;
    }

    /*QGraphicsLayout *layout = c->layout();
    if (!layout) {
        return - 1;
    }

    for (int i = 0; i < layout->count(); ++i) {
        if (layout->itemAt(i) == applet) {
            return i;
        }
    }*/

    return -1;
}

void Widget::setIndex(int index)
{
    Q_UNUSED(index)
    /*
    if (!d->applet) {
        return;
    }

    Plasma::Applet *applet = d->applet.data();
    Plasma::Containment *c = applet->containment();
    if (!c) {
        return;
    }
    //FIXME: this is hackish. would be nice to define this for gridlayouts too
    QGraphicsLinearLayout *layout = dynamic_cast<QGraphicsLinearLayout *>(c->layout());
    if (!layout) {
        return;
    }

    layout->insertItem(index, applet);*/
}

QJSValue Widget::geometry() const
{
    QQuickItem *appletItem = d->applet.data()->property("_plasma_graphicObject").value<QQuickItem *>();

    if (appletItem) {
        QJSValue rect = engine()->newObject();
        const QPointF pos = appletItem->mapToScene(QPointF(0, 0));
        rect.setProperty(QStringLiteral("x"), pos.x());
        rect.setProperty(QStringLiteral("y"), pos.y());
        rect.setProperty(QStringLiteral("width"), appletItem->width());
        rect.setProperty(QStringLiteral("height"), appletItem->height());
        return rect;
    }

    return QJSValue();
}

void Widget::setGeometry(const QJSValue &geometry)
{
    Q_UNUSED(geometry)
    /*if (d->applet) {
        d->applet.data()->setGeometry(geometry);
        KConfigGroup cg = d->applet.data()->config().parent();
        if (cg.isValid()) {
            cg.writeEntry("geometry", geometry);
        }
    }*/
}

void Widget::showConfigurationInterface()
{
    /* if (d->applet) {
         d->applet.data()->showConfigurationInterface();
     }*/
}

QString Widget::userBackgroundHints() const
{
    QMetaEnum hintEnum = QMetaEnum::fromType<Plasma::Types::BackgroundHints>();
    return hintEnum.valueToKey(applet()->userBackgroundHints());
}

void Widget::setUserBackgroundHints(QString hint)
{
    QMetaEnum hintEnum = QMetaEnum::fromType<Plasma::Types::BackgroundHints>();
    bool ok;
    int value = hintEnum.keyToValue(hint.toUtf8().constData(), &ok);
    if (ok) {
        applet()->setUserBackgroundHints(Plasma::Types::BackgroundHints(value));
    }
}

}
