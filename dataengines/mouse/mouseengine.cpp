/*
 *   Copyright © 2007 Fredrik Höglund <fredrik@kde.org>
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

#include "mouseengine.h"

#include <QCursor>

#ifdef HAVE_XFIXES
#include "cursornotificationhandler.h"
#endif

MouseEngine::MouseEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , timerId(0)
#ifdef HAVE_XFIXES
    , handler(nullptr)
#endif
{
    Q_UNUSED(args)
    init();
}

MouseEngine::~MouseEngine()
{
    if (timerId)
        killTimer(timerId);
#ifdef HAVE_XFIXES
    delete handler;
#endif
}

QStringList MouseEngine::sources() const
{
    QStringList list;

    list << QLatin1String("Position");
#ifdef HAVE_XFIXES
    list << QLatin1String("Name");
#endif

    return list;
}

void MouseEngine::init()
{
    if (!timerId)
        timerId = startTimer(40);

    // Init cursor position
    QPoint pos = QCursor::pos();
    setData(QLatin1String("Position"), QVariant(pos));
    lastPosition = pos;

#ifdef HAVE_XFIXES
    handler = new CursorNotificationHandler;
    connect(handler, &CursorNotificationHandler::cursorNameChanged, this, &MouseEngine::updateCursorName);

    setData(QLatin1String("Name"), QVariant(handler->cursorName()));
#endif
}

void MouseEngine::timerEvent(QTimerEvent *)
{
    QPoint pos = QCursor::pos();

    if (pos != lastPosition) {
        setData(QLatin1String("Position"), QVariant(pos));
        lastPosition = pos;
    }
}

void MouseEngine::updateCursorName(const QString &name)
{
    setData(QLatin1String("Name"), QVariant(name));
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(mouse, MouseEngine, "plasma-dataengine-mouse.json")

#include "mouseengine.moc"
