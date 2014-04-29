/*
 * Copyright © 2008 Fredrik Höglund <fredrik@kde.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "curtaineffect.h"
#include "curtaineffect.moc"

#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include "config-workspace.h"

#include <qimageblitz.h>

CurtainEffect::CurtainEffect(QWidget *parent, QPixmap *pixmap)
    : LogoutEffect(parent, pixmap)
{
}

void CurtainEffect::start()
{
    currentY = 0;
    nextFrame();
    emit initialized();
}

void CurtainEffect::nextFrame()
{
    QImage image = QPixmap::grabWindow(QApplication::desktop()->winId(), 0, currentY,
                                       parent->width(), 10 ).toImage();
    Blitz::intensity(image, -0.4);
    Blitz::grayscale(image);

    QPainter painter(pixmap);
    painter.drawImage(0, currentY, image);
    painter.end();

    currentY += 10;
    parent->update(0, 0, parent->width(), currentY);

    QTimer::singleShot(5, this, SLOT(nextFrame()));
}

