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

#include <QWidget>
#include <QPixmap>
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>

#include "logouteffect.h"
#include "fadeeffect.h"
#include "curtaineffect.h"

#include <unistd.h> // for gethostname()


static bool localDisplay(Display *dpy)
{
    QByteArray display(XDisplayString(dpy));
    QByteArray hostPart = display.left(display.indexOf(':'));

    if (hostPart.isEmpty())
        return true;

    if (hostPart == "localhost")
        return true;

    if (hostPart == "127.0.0.1")
        return true;

    char name[2048];
    gethostname(name, sizeof(name));

    if (hostPart == name)
       return true;

    return false;
}

static bool supportedFormat(const QPixmap *pixmap)
{
    int depth = pixmap->depth();
    Visual *visual = (Visual*)pixmap->x11Info().visual();

    if (ImageByteOrder(pixmap->x11Info().display()) != LSBFirst)
        return false;

    // Assume this means the pixmap is ARGB32
    if (pixmap->hasAlphaChannel())
        return true;

    // 24/34 bit x8a8r8g8b8
    if ((depth == 24 || depth == 32) &&
        visual->red_mask   == 0x00ff0000 &&
        visual->green_mask == 0x0000ff00 &&
        visual->blue_mask  == 0x000000ff)
    {
        return true;
    }

    // 16 bit r5g6b5
    if (depth == 16 &&
        visual->red_mask   == 0xf800 &&
        visual->green_mask == 0x07e0 &&
        visual->blue_mask  == 0x001f)
    {
        return true;
    }

    return false;
}



// ----------------------------------------------------------------------------



LogoutEffect::LogoutEffect(QWidget *parent, QPixmap *pixmap)
    : QObject(parent), parent(parent), pixmap(pixmap)
{
}

LogoutEffect::~LogoutEffect()
{
}

LogoutEffect *LogoutEffect::create(QWidget *parent, QPixmap *pixmap)
{
    Display *dpy = parent->x11Info().display();

    if (!localDisplay(dpy) || !supportedFormat(pixmap))
        return new CurtainEffect(parent, pixmap);
    else
        return new FadeEffect(parent, pixmap);
}


