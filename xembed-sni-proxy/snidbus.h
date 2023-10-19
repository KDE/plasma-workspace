/*
    SNI Dbus serialisers
    Copyright 2015  <davidedmundson@kde.org> David Edmundson

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QByteArray>
#include <QDBusArgument>
#include <QImage>
#include <QList>
#include <QString>

// Custom message type for DBus
struct KDbusImageStruct {
    KDbusImageStruct();
    KDbusImageStruct(const QImage &image);
    int width;
    int height;
    QByteArray data;
};

typedef QList<KDbusImageStruct> KDbusImageVector;

struct KDbusToolTipStruct {
    QString icon;
    KDbusImageVector image;
    QString title;
    QString subTitle;
};

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusImageStruct &icon);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusImageStruct &icon);

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusImageVector &iconVector);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusImageVector &iconVector);

const QDBusArgument &operator<<(QDBusArgument &argument, const KDbusToolTipStruct &toolTip);
const QDBusArgument &operator>>(const QDBusArgument &argument, KDbusToolTipStruct &toolTip);
