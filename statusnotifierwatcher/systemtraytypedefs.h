/*

    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVector>

struct KDbusImageStruct {
    int width;
    int height;
    QByteArray data;
};

typedef QVector<KDbusImageStruct> KDbusImageVector;

struct KDbusToolTipStruct {
    QString icon;
    KDbusImageVector image;
    QString title;
    QString subTitle;
};

Q_DECLARE_METATYPE(KDbusImageStruct)
Q_DECLARE_METATYPE(KDbusImageVector)
Q_DECLARE_METATYPE(KDbusToolTipStruct)
