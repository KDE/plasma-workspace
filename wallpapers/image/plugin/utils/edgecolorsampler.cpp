/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "edgecolorsampler.h"

#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QtConcurrent>

EdgeColorSampler::EdgeColorSampler(QObject *parent)
    : QObject(parent)
{
}

QQuickItem *EdgeColorSampler::source() const
{
    return m_sourceItem;
}

void EdgeColorSampler::setSource(QQuickItem *source)
{
    if (m_sourceItem == source) {
        return;
    }

    m_sourceItem = source;
    Q_EMIT sourceChanged();

    update();
}

QColor EdgeColorSampler::color() const
{
    return m_color;
}

void EdgeColorSampler::update()
{
    if (!m_sourceItem) {
        return;
    }

    const QSizeF sourceSize = m_sourceItem->size();
    const QSizeF targetSize = sourceSize.scaled(200, 200, Qt::KeepAspectRatio);
    // When Image.Pad is chosen, paintedWidth > sourceSize.width
    const double paintedWidth = std::min(targetSize.width(), m_sourceItem->property("paintedWidth").toDouble() * targetSize.width() / sourceSize.width());
    const double paintedHeight = std::min(targetSize.height(), m_sourceItem->property("paintedHeight").toDouble() * targetSize.height() / sourceSize.height());

    if (std::islessequal(paintedWidth, 0.0) || std::islessequal(paintedHeight, 0.0)) {
        Q_EMIT grabFailed();
        return;
    }

    startX = (targetSize.width() - paintedWidth) / 2;
    endX = startX + paintedWidth - 1;
    startY = (targetSize.height() - paintedHeight) / 2;
    endY = startY + paintedHeight - 1;

    m_grabResult = m_sourceItem->grabToImage(targetSize.toSize());
    if (m_grabResult.isNull()) {
        Q_EMIT grabFailed();
        return;
    }

    connect(m_grabResult.data(), &QQuickItemGrabResult::ready, this, [this] {
        QtConcurrent::run(this, &EdgeColorSampler::extractColor);
    });
}

void EdgeColorSampler::extractColor()
{
    const auto sourceImage = m_grabResult->image();
    m_grabResult.clear();

    int r = 0, g = 0, b = 0, c = 0;
    int leftEdge = startX + 0.2 * (endX - startX + 1);
    int rightEdge = endX - 0.2 * (endX - startX + 1);
    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            if (x >= leftEdge && x < rightEdge) {
                // Left/Right edge only
                continue;
            }

            const QColor sampleColor = sourceImage.pixelColor(x, y);
            if (sampleColor.alpha() == 0) {
                continue;
            }

            c++;
            r += sampleColor.red();
            g += sampleColor.green();
            b += sampleColor.blue();
        }
    }

    if (c == 0) {
        return;
    }

    m_color = QColor(r / c, g / c, b / c, 255);
    Q_EMIT colorChanged();
}
