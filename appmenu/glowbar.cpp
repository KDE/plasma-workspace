/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "glowbar.h"

#include <X11/extensions/shape.h>
#include <fixx11h.h>

#include <Plasma/Svg>
#include <KWindowSystem>
#include <KDebug>

#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QX11Info>


GlowBar::GlowBar()
    : QWidget(0),
      m_svg(new Plasma::Svg(this))
{
    m_svg->setImagePath("widgets/glowbar");

    setWindowFlags(Qt::Tool | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    KWindowSystem::setType(winId(), NET::Dock);

    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::transparent);
    setPalette(pal);

    setInputMask();
}

GlowBar::~GlowBar()
{
}

void GlowBar::paintEvent(QPaintEvent*)
{
    QPixmap l, r, c;
    QPoint pixmapPosition(0, 0);

    m_buffer.fill(QColor(0, 0, 0, int(qreal(255)*0.3)));
    QPainter p(&m_buffer);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    l = m_svg->pixmap("bottomleft");
    r = m_svg->pixmap("bottomright");
    c = m_svg->pixmap("bottom");
    p.drawPixmap(pixmapPosition, l);
    p.drawTiledPixmap(QRect(l.width(), pixmapPosition.y(), width() - l.width() - r.width(), c.height()), c);
    p.drawPixmap(QPoint(width() - r.width(), pixmapPosition.y()), r);
    p.end();
    p.begin(this);
    p.drawPixmap(QPoint(0, 0), m_buffer);
}

void GlowBar::setPixmap(const QPoint pos, uint width)
{
    QRect zone = QRect(pos, QSize(width, 10));
    setGeometry(zone);
    m_buffer = QPixmap(zone.size());
}

void GlowBar::setInputMask()
{
    // Create an empty input mask to achieve click-through effect
    // Thanks to MacSlow for this!
    Pixmap mask;

    mask = XCreatePixmap(QX11Info::display(),
            winId(),
            1, /* width  */
            1, /* height */
            1  /* depth  */);
    XShapeCombineMask(QX11Info::display(),
            winId(),
            ShapeInput,
            0, /* x-offset */
            0, /* y-offset */
            mask,
            ShapeSet);
    XFreePixmap(QX11Info::display(), mask);
}
#include "glowbar.moc"
