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

#ifndef FADEEFFECT_H
#define FADEEFFECT_H

#include <QObject>
#include <QTime>

#include "logouteffect.h"
#include <X11/Xlib.h>


class QWidget;
class QPixmap;
class BlendingThread;


/**
 * This class implements an effect that will slowly fade the screen from color to grayscale.
 *
 * This class is X11 specific, and currently only works on little-endian systems where
 * the root window has a 24/32 or 16 bit r5g6b5 pixel format.
 */
class FadeEffect : public LogoutEffect
{
    Q_OBJECT

public:
    /**
     * Creates the FadeEffect.
     *
     * This effect will grab a screenshot and then slowly fade it to gray,
     * rendering each frame on the pixmap you pass to the constructor.
     * The animation will begin when you call start(), and each time the
     * pixmap is updated, update() will be called in the @p parent widget,
     * which must then paint the pixmap.
     *
     * The actual blending is done in a separate thread to keep the event
     * loop alive.
     *
     * The FadeEffect can safely be deleted at any time, even while the
     * effect is running, but it's important that the pixmap isn't deleted
     * before the effect object.
     *
     * @param parent The parent widget, which will draw the pixmap.
     * @param pixmap The pixmap the effect will be rendered to.
     */
    FadeEffect(QWidget *parent, QPixmap *pixmap);

    /**
     * Destroys the fade effect.
     */
    ~FadeEffect();

    /**
     * Starts the animation.
     */
    void start();

private Q_SLOTS:
    /** @internal */
    void grabImageSection();

    /** @internal */
    void nextFrame();

private:
    BlendingThread *blender;
    int alpha;
    int currentY;
    bool done;
    XImage *image;
    GC gc;
    QTime time;
};

#endif

