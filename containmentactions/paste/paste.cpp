/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

K_PLUGIN_CLASS_WITH_JSON(Paste, "plasma-containmentactions-paste.json")

#include "paste.moc"
