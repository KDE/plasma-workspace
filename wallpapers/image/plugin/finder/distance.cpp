/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "distance.h"

#include <limits>

#include <QSize>
#include <QString>

float distance(const QSize &size, const QSize &desired)
{
    const float desiredAspectRatio = (desired.height() > 0) ? desired.width() / static_cast<float>(desired.height()) : 0;
    const float candidateAspectRatio = (size.height() > 0) ? size.width() / static_cast<float>(size.height()) : std::numeric_limits<float>::max();

    float delta = size.width() - desired.width();
    delta = delta >= 0.0 ? delta : -delta * 2; // Penalize for scaling up

    return std::abs(candidateAspectRatio - desiredAspectRatio) * 25000 + delta;
}

QSize resSize(const QString &str)
{
    const int index = str.indexOf(QLatin1Char('x'));

    if (index != -1) {
        return QSize(QStringView(str).left(index).toInt(), QStringView(str).mid(index + 1).toInt());
    }

    return QSize();
}
