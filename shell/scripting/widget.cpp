/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
        return d->applet->id();
    }

    return 0;
}

QString Widget::type() const
{
    if (d->applet) {
        return d->applet->pluginMetaData().pluginId();
    }

    return QString();
}

void Widget::remove()
{
    if (d->applet) {
        d->applet->destroy();
        d->applet.clear();
    }
}

void Widget::setGlobalShortcut(const QString &shortcut)
{
    if (d->applet) {
        d->applet->setGlobalShortcut(QKeySequence(shortcut));
    }
}

QString Widget::globalShorcut() const
{
    if (d->applet) {
        return d->applet->globalShortcut().toString();
    }

    return QString();
}

Plasma::Applet *Widget::applet() const
{
    return d->applet;
}

int Widget::index() const
{
    if (!d->applet) {
        return -1;
    }

    Plasma::Containment *c = d->applet->containment();
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

    Plasma::Containment *c = d->applet->containment();
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
    QQuickItem *appletItem = d->applet->property("_plasma_graphicObject").value<QQuickItem *>();

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
        d->applet->setGeometry(geometry);
        KConfigGroup cg = d->applet->config().parent();
        if (cg.isValid()) {
            cg.writeEntry("geometry", geometry);
        }
    }*/
}

void Widget::showConfigurationInterface()
{
    /* if (d->applet) {
         d->applet->showConfigurationInterface();
     }*/
}

QString Widget::userBackgroundHints() const
{
    QMetaEnum hintEnum = QMetaEnum::fromType<Plasma::Types::BackgroundHints>();
    return hintEnum.valueToKey(applet()->userBackgroundHints());
}

void Widget::setUserBackgroundHints(const QString &hint)
{
    QMetaEnum hintEnum = QMetaEnum::fromType<Plasma::Types::BackgroundHints>();
    bool ok;
    int value = hintEnum.keyToValue(hint.toUtf8().constData(), &ok);
    if (ok) {
        applet()->setUserBackgroundHints(Plasma::Types::BackgroundHints(value));
    }
}

}
