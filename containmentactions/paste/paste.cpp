/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
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

#include "paste.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include <Plasma/Containment>

#include <QAction>
#include <QDebug>

Paste::Paste(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
    m_action = new QAction(this);
    QObject::connect(m_action, &QAction::triggered, this, &Paste::doPaste);
}

QList<QAction *> Paste::contextualActions()
{
    QList<QAction *> actions;
    actions << m_action;

    return actions;
}

void Paste::doPaste()
{
    qWarning() << "Paste at" << m_action->data();

    if (!m_action->data().canConvert<QPoint>()) {
        return;
    }

    QPoint pos = m_action->data().value<QPoint>();
    Plasma::Containment *c = containment();
    Q_ASSERT(c);

    // get the actual graphic object of the containment
    QObject *graphicObject = c->property("_plasma_graphicObject").value<QObject *>();
    if (!graphicObject) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    // FIXME: can be the const_cast avoided?
    QMimeData *mimeData = const_cast<QMimeData *>(clipboard->mimeData(QClipboard::Selection));
    // TODO if that's not supported (ie non-linux) should we try clipboard instead of selection?

    graphicObject->metaObject()
        ->invokeMethod(graphicObject, "processMimeData", Qt::DirectConnection, Q_ARG(QMimeData *, mimeData), Q_ARG(int, pos.x()), Q_ARG(int, pos.y()));
}

K_EXPORT_PLASMA_CONTAINMENTACTIONS_WITH_JSON(paste, Paste, "plasma-containmentactions-paste.json")

#include "paste.moc"
