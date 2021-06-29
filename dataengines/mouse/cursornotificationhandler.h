/*
    SPDX-FileCopyrightText: 2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QMap>
#include <QWidget>

#include <X11/Xlib.h>
#include <fixx11h.h>

class CursorNotificationHandler : public QWidget
{
    Q_OBJECT

public:
    CursorNotificationHandler();
    ~CursorNotificationHandler() override;

    QString cursorName();

Q_SIGNALS:
    void cursorNameChanged(const QString &name);

protected:
    bool x11Event(XEvent *);

private:
    QString cursorName(Atom cursor);

private:
    bool haveXfixes;
    int fixesEventBase;
    Atom currentName;
    QMap<Atom, QString> names;
};
