/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IMAGECOLORS_H
#define IMAGECOLORS_H

#include <QColor>
#include <QObject>
#include <QQuickItemGrabResult>

class QQuickItem;

struct ColorStat {
    std::vector<QRgb> colors;
    QRgb centroid = 0;
};

/**
 * This class is used to extract wallpaper accent color.
 */
class ImageColors : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *source READ sourceItem WRITE setSourceItem NOTIFY sourceChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit ImageColors(QObject *parent = nullptr);

    void setSourceItem(QQuickItem *source);
    QQuickItem *sourceItem() const;

    QColor accentColor() const;

    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;

public Q_SLOTS:
    Q_INVOKABLE void update();

Q_SIGNALS:
    void accentColorChanged();
    void backgroundColorChanged();
    void sourceChanged();

private Q_SLOTS:
    void slotGrabResultReady();

private:
    void generateAccentColor(const QImage &image);
    void generateDominantColor(std::vector<ColorStat> &clusters, std::vector<QRgb> &samples);

    void positionColor(QRgb rgb, std::vector<ColorStat> &clusters);

    qreal luminance(const QColor &color) const;

    static const int s_minimumSquareDistance = 32000;

    QPointer<QQuickItem> m_sourceItem;
    QColor m_accentColor;
    QColor m_backgroundColor;

    QSharedPointer<QQuickItemGrabResult> m_grabResult;
};

#endif // IMAGECOLORS_H
