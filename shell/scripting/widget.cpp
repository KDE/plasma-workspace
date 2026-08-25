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
#include <PlasmaQuick/AppletQuickItem>

namespace WorkspaceScripting
{
class Widget::Private
{
public:
    Private() = default;

    QPointer<Plasma::Applet> applet;
};

Widget::Widget(Plasma::Applet *applet, ScriptEngine *parent)
    : Applet(parent)
    , d(new Widget::Private)
{
    d->applet = applet;
    setCurrentConfigGroup(QStringList());
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

    return {};
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

    return {};
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
    if (!c || (c->containmentType() != Plasma::Containment::Panel && c->containmentType() != Plasma::Containment::CustomPanel)) {
        return -1;
    }

    KConfigGroup cg = c->config();
    cg = KConfigGroup(&cg, QStringLiteral("General"));
    const QString orderString = cg.readEntry("AppletOrder", QString());
    auto parts = orderString.split(u';');
    QList<int> order;
    std::ranges::transform(parts, std::back_inserter(order), [](const QString &s) {
        return s.toInt();
    });

    return order.indexOf(d->applet->id());
}

void Widget::setIndex(int index)
{
    if (!d->applet) {
        return;
    }

    Plasma::Containment *c = d->applet->containment();
    if (!c || (c->containmentType() != Plasma::Containment::Panel && c->containmentType() != Plasma::Containment::CustomPanel)) {
        return;
    }

    KConfigGroup cg = c->config();
    cg = KConfigGroup(&cg, QStringLiteral("General"));
    const QString orderString = cg.readEntry("AppletOrder", QString());
    auto parts = orderString.split(u';');
    QList<int> order;
    order.reserve(parts.size());
    std::ranges::transform(std::as_const(parts), std::back_inserter(order), [](const QString &s) {
        return s.toInt();
    });

    order.removeAll(d->applet->id());

    order.insert(std::min(std::max(0, index), int(order.length()) - 1), d->applet->id());

    parts.clear();
    std::ranges::transform(std::as_const(order), std::back_inserter(parts), [](int val) {
        return QString::number(val);
    });

    QQuickItem *containmentItem = PlasmaQuick::AppletQuickItem::itemForApplet(c);

    bool success = QMetaObject::invokeMethod(containmentItem, "reorderApplets", Q_ARG(QVariant, QVariant::fromValue(order)));

    if (!success) {
        qWarning() << "Impossible to invoke reorderApplets() on the panel";
    }
}

QJSValue Widget::geometry() const
{
    QQuickItem *appletItem = PlasmaQuick::AppletQuickItem::itemForApplet(d->applet);

    if (appletItem) {
        QJSValue rect = engine()->newObject();
        const QPointF pos = appletItem->mapToScene(QPointF(0, 0));
        rect.setProperty(QStringLiteral("x"), pos.x());
        rect.setProperty(QStringLiteral("y"), pos.y());
        rect.setProperty(QStringLiteral("width"), appletItem->width());
        rect.setProperty(QStringLiteral("height"), appletItem->height());
        return rect;
    }

    return {};
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
    if (d->applet) {
        QAction *configAction = d->applet->internalAction(QStringLiteral("configure"));
        if (configAction && configAction->isEnabled()) {
            configAction->trigger();
        }
    }
}

QString Widget::userBackgroundHints() const
{
    QMetaEnum hintEnum = QMetaEnum::fromType<Plasma::Types::BackgroundHints>();
    return QString::fromLatin1(hintEnum.valueToKey(applet()->userBackgroundHints()));
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

QString Widget::launchErrorMessage() const
{
    return applet()->launchErrorMessage();
}
}

#include "moc_widget.cpp"
