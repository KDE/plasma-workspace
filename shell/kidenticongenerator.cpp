/*
 *   Copyright 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kidenticongenerator.h"

#include <QHash>
#include <QPainter>
#include <QDebug>
#include <QCryptographicHash>

#include <kiconeffect.h>

#include <Plasma/Svg>
#include <Plasma/Theme>

#define VALUE_LIMIT_UP 192
#define VALUE_LIMIT_DOWN 64

class KIdenticonGenerator::Private {
public:
    QPixmap generatePattern(int size, quint32 hash, QIcon::Mode mode);

    QString elementName(const QString & element, QIcon::Mode mode);
    QColor colorForHash(quint32 hash) const;
    quint32 hash(const QString & data);

    static KIdenticonGenerator * instance;

    Plasma::Theme *theme;
    Plasma::Svg shapes;
    Plasma::Svg svg;
};

QPixmap KIdenticonGenerator::Private::generatePattern(int size, quint32 hash, QIcon::Mode mode)
{
    // We are dividing the pixmap into 9 blocks - 3 x 3
    int blockSize = size / 3;

    // pulling parts of the hash
    quint32 tmp = hash;

    quint8 block[4];
    block[0] = tmp & 31; tmp >>= 5;
    block[1] = tmp & 31; tmp >>= 5;
    block[2] = tmp & 31; tmp >>= 5;

    // Painting alpha channel
    QImage pixmapAlpha({size, size}, QImage::Format_ARGB32);
    pixmapAlpha.fill(Qt::black);

    QPainter painterAlpha(& pixmapAlpha);

    QRectF rect(0, 0, blockSize + 0.5, blockSize + 0.5);

    for (int i = 0; i < 4; i++) {
        // Painting the corner item
        rect.moveTopLeft(QPoint(0, 0));
        shapes.paint(& painterAlpha, rect, "shape" + QString::number(block[0] + 1));

        // Painting side item
        rect.moveTopLeft(QPoint(blockSize, 0));
        shapes.paint(& painterAlpha, rect, "shape" + QString::number(block[1] + 1));

        // Rotating the canvas to paint other edges
        painterAlpha.translate(size, 0);
        painterAlpha.rotate(90);
    }

    // Painting center item
    rect.moveTopLeft(QPoint(blockSize, blockSize));
    shapes.paint(& painterAlpha, rect, "shape" + QString::number(block[2] + 1));

    painterAlpha.end();

    // Painting final pixmap
    QImage pixmapResult(size, size, QImage::Format_ARGB32);
    pixmapResult.fill(Qt::transparent);

    // QRadialGradient gradient(50, 50, 100);
    // gradient.setColorAt(0, color.lighter());
    // gradient.setColorAt(1, color.darker());

    QPainter resultPainter(& pixmapResult);
    // resultPainter.fillRect(0, 0, size, size, gradient);
    svg.paint(& resultPainter, QRect(0, 0, size, size), elementName(QStringLiteral("content"), mode));

    resultPainter.end();

    pixmapResult.setAlphaChannel(pixmapAlpha);

    // QImage itmp = pixmapResult.toImage();
    // KIconEffect::colorize(itmp, colorForHash(hash), 1.0);
    // pixmapResult = pixmapResult.fromImage(itmp);

    return QPixmap::fromImage(pixmapResult);
}

QColor KIdenticonGenerator::Private::colorForHash(quint32 hash) const
{
    // Color is chosen according to hash
    QColor color;

    // Getting the value from color svg, but we must restrain it to
    // values in range from VALUE_LIMIT_DOWN to VALUE_LIMIT_UP

    int value = theme->color(Plasma::Theme::TextColor).value();
    if (value < VALUE_LIMIT_DOWN) {
        value = VALUE_LIMIT_DOWN;
    } else if (value > VALUE_LIMIT_UP) {
        value = VALUE_LIMIT_UP;
    }

    color.setHsv(
        hash % 359 + 1, // hue depending on hash
        250,            // high saturation level
        value
    );

    return color;

}

QString KIdenticonGenerator::Private::elementName(const QString & element, QIcon::Mode mode)
{
    QString prefix;

    switch (mode) {
        case QIcon::Normal:
            prefix = QStringLiteral("normal-");
            break;

        case QIcon::Disabled:
            prefix = QStringLiteral("disabled-");
            break;

        case QIcon::Selected:
            prefix = QStringLiteral("selected-");
            break;

        case QIcon::Active:
            prefix = QStringLiteral("active-");
            break;

        default:
            break;

    }

    if (svg.hasElement(prefix + element)) {
        return prefix + element;
    } else {
        return element;
    }
}

quint32 KIdenticonGenerator::Private::hash(const QString & data)
{
    // qHash function doesn't give random enough results
    // and gives similar hashes for similar strings.

    QByteArray bytes = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5);

    // Generating hash
    quint32 hash = 0;

    char * hashBytes = (char *) & hash;
    for (int i = 0; i < bytes.size(); i++) {
        // Using XOR for mixing the bytes because
        // it is fast and cryptographically safe
        // (more than enough for our use-case)
        hashBytes[i % 4] ^= bytes.at(i);
    }

    return hash;
}

KIdenticonGenerator * KIdenticonGenerator::Private::instance = NULL;

KIdenticonGenerator * KIdenticonGenerator::self()
{
    if (!Private::instance) {
        Private::instance = new KIdenticonGenerator();
    }

    return Private::instance;
}

KIdenticonGenerator::KIdenticonGenerator()
    : d(new Private())
{
    d->theme = new Plasma::Theme(0);
    // loading SVGs
    d->shapes.setImagePath(QStringLiteral("widgets/identiconshapes"));
    d->shapes.setContainsMultipleImages(true);

    d->svg.setImagePath(QStringLiteral("widgets/identiconsvg"));
    d->svg.setContainsMultipleImages(true);
}

KIdenticonGenerator::~KIdenticonGenerator()
{
    delete d->theme;
    delete d;
}

#define generateIconModes( PARAM ) \
    for (int omode = QIcon::Normal; omode <= QIcon::Selected; omode++) {   \
        QIcon::Mode mode = (QIcon::Mode)omode;                             \
        result.addPixmap(generatePixmap(size, PARAM, mode), mode);         \
    }

QIcon KIdenticonGenerator::generate(int size, quint32 hash)
{
    QIcon result;
    generateIconModes(hash);
    return result;
}

QIcon KIdenticonGenerator::generate(int size, const QString & data)
{
    QIcon result;
    generateIconModes(data);
    return result;
}

QIcon KIdenticonGenerator::generate(int size, const QIcon & icon)
{
    QIcon result;
    generateIconModes(icon);
    return result;
}

QPixmap KIdenticonGenerator::generatePixmap(int size, QString id, QIcon::Mode mode)
{
    return generatePixmap(size, d->hash(id), mode);
}

QPixmap KIdenticonGenerator::generatePixmap(int size, quint32 hash, QIcon::Mode mode)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    // Painting background and the pattern
    {
    QPainter painter(& pixmap);
    d->svg.paint(& painter, QRect(0, 0, size, size), d->elementName(QStringLiteral("background"), mode));
    painter.drawPixmap(0, 0, d->generatePattern(size, hash, mode));
    painter.end();
    }

    // coloring the painted image
    QImage itmp = pixmap.toImage();
    KIconEffect::colorize(itmp, d->colorForHash(hash), 1.0);
    if (mode == QIcon::Disabled) {
        KIconEffect::toGray(itmp, 0.7);
    }
    pixmap = pixmap.fromImage(itmp);

    // Drawing the overlay
    {
    QPainter painter(& pixmap);
    d->svg.paint(& painter, QRect(0, 0, size, size), d->elementName(QStringLiteral("overlay"), mode));
    }

    return pixmap;
}

QPixmap KIdenticonGenerator::generatePixmap(int size, const QIcon & icon, QIcon::Mode mode)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QRect paintRect(0, 0, size, size);

    // Painting background and the pattern
    QPainter painter(& pixmap);
    d->svg.paint(& painter, QRect(0, 0, size, size), d->elementName(QStringLiteral("background"), mode));

    icon.paint(& painter, paintRect, Qt::AlignCenter, mode);

    painter.end();

    return pixmap;
}

