/*
 *   Copyright 2007 Richard J. Moore <rich@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptContext>
#include <QtCore/QRectF>
#include "backportglobal.h"

Q_DECLARE_METATYPE(QRectF*)
Q_DECLARE_METATYPE(QRectF)

static QScriptValue ctor(QScriptContext *ctx, QScriptEngine *eng)
{
    if (ctx->argumentCount() == 4)
    {
        qreal x = ctx->argument(0).toNumber();
        qreal y = ctx->argument(1).toNumber();
        qreal width = ctx->argument(2).toNumber();
        qreal height = ctx->argument(3).toNumber();
        return qScriptValueFromValue(eng, QRectF(x, y, width, height));
    }

    return qScriptValueFromValue(eng, QRectF());
}

static QScriptValue adjust(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, adjust);
    qreal dx1 = ctx->argument(0).toNumber();
    qreal dy1 = ctx->argument(1).toNumber();
    qreal dx2 = ctx->argument(2).toNumber();
    qreal dy2 = ctx->argument(3).toNumber();

    self->adjust(dx1, dy1, dx2, dy2);
    return QScriptValue();
}

static QScriptValue adjusted(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, adjusted);
    qreal dx1 = ctx->argument(0).toNumber();
    qreal dy1 = ctx->argument(1).toNumber();
    qreal dx2 = ctx->argument(2).toNumber();
    qreal dy2 = ctx->argument(3).toNumber();

    QRectF tmp = self->adjusted(dx1, dy1, dx2, dy2);
    return qScriptValueFromValue(eng, tmp);
}

static QScriptValue bottom(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, bottom);

    if (ctx->argumentCount() > 0) {
        int bottom = ctx->argument(0).toInt32();
        self->setBottom(bottom);
    }

    return QScriptValue(eng, self->bottom());
}

static QScriptValue top(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, top);

    if (ctx->argumentCount() > 0) {
        int top = ctx->argument(0).toInt32();
        self->setTop(top);
    }

    return QScriptValue(eng, self->top());
}

static QScriptValue contains(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, contains);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    return QScriptValue(eng, self->contains(x, y));
}

static QScriptValue height(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, height);

    if (ctx->argumentCount() > 0) {
        int height = ctx->argument(0).toInt32();
        self->setHeight(height);
    }

    return QScriptValue(eng, self->height());
}

static QScriptValue empty(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, empty);
    return QScriptValue(eng, self->isEmpty());
}

static QScriptValue null(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, null);
    return QScriptValue(eng, self->isNull());
}

static QScriptValue valid(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, valid);
    return QScriptValue(eng, self->isValid());
}

static QScriptValue left(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, left);

    if (ctx->argumentCount() > 0) {
        int left = ctx->argument(0).toInt32();
        self->setLeft(left);
    }

    return QScriptValue(eng, self->left());
}

static QScriptValue moveBottom(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveBottom);
    qreal bottom = ctx->argument(0).toNumber();
    self->moveBottom(bottom);
    return QScriptValue();
}

static QScriptValue moveLeft(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveLeft);
    qreal left = ctx->argument(0).toNumber();
    self->moveBottom(left);
    return QScriptValue();
}

static QScriptValue moveRight(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveRight);
    qreal right = ctx->argument(0).toNumber();
    self->moveBottom(right);
    return QScriptValue();
}


static QScriptValue moveTo(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveTo);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    self->moveTo(x, y);
    return QScriptValue();
}

static QScriptValue moveTop(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, moveTop);
    qreal top = ctx->argument(0).toNumber();
    self->moveTop(top);
    return QScriptValue();
}

static QScriptValue right(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, right);

    if (ctx->argumentCount() > 0) {
        int right = ctx->argument(0).toInt32();
        self->setRight(right);
    }

    return QScriptValue(eng, self->right());
}

static QScriptValue setCoords(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setCoords);
    qreal x1 = ctx->argument(0).toNumber();
    qreal y1 = ctx->argument(1).toNumber();
    qreal x2 = ctx->argument(2).toNumber();
    qreal y2 = ctx->argument(3).toNumber();
    self->setCoords(x1, y1, x2, y2);
    return QScriptValue();
}

static QScriptValue setRect(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, setRect);
    qreal x = ctx->argument(0).toNumber();
    qreal y = ctx->argument(1).toNumber();
    qreal width = ctx->argument(2).toNumber();
    qreal height = ctx->argument(3).toNumber();
    self->setRect(x, y, width, height);
    return QScriptValue();
}

static QScriptValue translate(QScriptContext *ctx, QScriptEngine *)
{
    DECLARE_SELF(QRectF, translate);
    qreal dx = ctx->argument(0).toNumber();
    qreal dy = ctx->argument(1).toNumber();
    self->translate(dx, dy);
    return QScriptValue();
}

static QScriptValue width(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, width);

    if (ctx->argumentCount() > 0) {
        int width = ctx->argument(0).toInt32();
        self->setWidth(width);
    }

    return QScriptValue(eng, self->width());
}

static QScriptValue x(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, x);

    if (ctx->argumentCount() > 0) {
        int x = ctx->argument(0).toInt32();
        self->setX(x);
    }

    return QScriptValue(eng, self->x());
}

static QScriptValue y(QScriptContext *ctx, QScriptEngine *eng)
{
    DECLARE_SELF(QRectF, y);

    if (ctx->argumentCount() > 0) {
        int y = ctx->argument(0).toInt32();
        self->setY(y);
    }

    return QScriptValue(eng, self->y());
}

/* Not Implemented Yet */
// QPointF bottomLeft () const
// QPointF bottomRight () const
// QPointF center () const
// bool contains ( const QPointF & point ) const
// bool contains ( const QRectF & rectangle ) const
// void getCoords ( qreal * x1, qreal * y1, qreal * x2, qreal * y2 ) const
// void getRect ( qreal * x, qreal * y, qreal * width, qreal * height ) const
// QRectF intersected ( const QRectF & rectangle ) const
// bool intersects ( const QRectF & rectangle ) const
// void moveBottomLeft ( const QPointF & position )
// void moveBottomRight ( const QPointF & position )
// void moveCenter ( const QPointF & position )
// void moveTo ( const QPointF & position )
// void moveTopLeft ( const QPointF & position )
// void moveTopRight ( const QPointF & position )
// QRectF normalized () const
// void setBottomLeft ( const QPointF & position )
// void setBottomRight ( const QPointF & position )
// void setSize ( const QSizeF & size )
// void setTopLeft ( const QPointF & position )
// void setTopRight ( const QPointF & position )
// QSizeF size () const
// QRect toAlignedRect () const
// QRect toRect () const
// QPointF topLeft () const
// QPointF topRight () const
// void translate ( const QPointF & offset )
// QRectF translated ( qreal dx, qreal dy ) const
// QRectF translated ( const QPointF & offset ) const
// QRectF united ( const QRectF & rectangle ) const

QScriptValue constructQRectFClass(QScriptEngine *eng)
{
    QScriptValue proto = qScriptValueFromValue(eng, QRectF());
    QScriptValue::PropertyFlags getter = QScriptValue::PropertyGetter;
    QScriptValue::PropertyFlags setter = QScriptValue::PropertySetter;

    proto.setProperty(QStringLiteral("adjust"), eng->newFunction(adjust));
    proto.setProperty(QStringLiteral("adjusted"), eng->newFunction(adjusted), getter);
    proto.setProperty(QStringLiteral("translate"), eng->newFunction(translate));
    proto.setProperty(QStringLiteral("setCoords"), eng->newFunction(setCoords));
    proto.setProperty(QStringLiteral("setRect"), eng->newFunction(setRect));

    proto.setProperty(QStringLiteral("contains"), eng->newFunction(contains));

    proto.setProperty(QStringLiteral("moveBottom"), eng->newFunction(moveBottom));
    proto.setProperty(QStringLiteral("moveLeft"), eng->newFunction(moveLeft));
    proto.setProperty(QStringLiteral("moveRight"), eng->newFunction(moveRight));
    proto.setProperty(QStringLiteral("moveTo"), eng->newFunction(moveTo));
    proto.setProperty(QStringLiteral("moveTop"), eng->newFunction(moveTop));

    proto.setProperty(QStringLiteral("empty"), eng->newFunction(empty), getter);
    proto.setProperty(QStringLiteral("null"), eng->newFunction(null), getter);
    proto.setProperty(QStringLiteral("valid"), eng->newFunction(valid), getter);

    proto.setProperty(QStringLiteral("left"), eng->newFunction(left), getter | setter);
    proto.setProperty(QStringLiteral("top"), eng->newFunction(top), getter | setter);
    proto.setProperty(QStringLiteral("bottom"), eng->newFunction(bottom), getter | setter);
    proto.setProperty(QStringLiteral("right"), eng->newFunction(right), getter | setter);
    proto.setProperty(QStringLiteral("height"), eng->newFunction(height), getter | setter);
    proto.setProperty(QStringLiteral("width"), eng->newFunction(width), getter | setter);
    proto.setProperty(QStringLiteral("x"), eng->newFunction(x), getter | setter);
    proto.setProperty(QStringLiteral("y"), eng->newFunction(y), getter | setter);

    eng->setDefaultPrototype(qMetaTypeId<QRectF>(), proto);
    eng->setDefaultPrototype(qMetaTypeId<QRectF*>(), proto);

    return eng->newFunction(ctor, proto);
}
