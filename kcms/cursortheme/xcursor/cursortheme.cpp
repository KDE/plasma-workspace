/*
    SPDX-FileCopyrightText: 2006-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QImage>
#include <QStyle>

#include "cursortheme.h"

#include <config-X11.h>

#if HAVE_X11
#include <private/qtx11extras_p.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#endif

CursorTheme::CursorTheme(const QString &title, const QString &description)
{
    setTitle(title);
    setDescription(description);
    setSample(QStringLiteral("left_ptr"));
    setIsHidden(false);
    setIsWritable(false);
}

QPixmap CursorTheme::icon() const
{
    if (m_icon.isNull())
        m_icon = createIcon();

    return m_icon;
}

static int nominalCursorSize(int iconSize)
{
    for (int i = 512; i > 8; i /= 2) {
        if (i < iconSize)
            return i;

        if ((i * .75) < iconSize)
            return int(i * .75);
    }

    return 8;
}

QPixmap CursorTheme::createIcon() const
{
    int iconSize = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
    int cursorSize = nominalCursorSize(iconSize);
    QSize size = QSize(iconSize, iconSize);

    QPixmap pixmap = createIcon(cursorSize);

    if (!pixmap.isNull()) {
        // Scale the pixmap if it's larger than the preferred icon size
        if (pixmap.width() > size.width() || pixmap.height() > size.height())
            pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return pixmap;
}

QPixmap CursorTheme::createIcon(int size) const
{
    QPixmap pixmap;
    auto image = loadImage(sample(), size);

    if (!image.has_value() && sample() != QLatin1String("left_ptr"))
        image = loadImage(QStringLiteral("left_ptr"), size);

    if (image.has_value()) {
        pixmap = QPixmap::fromImage(std::move(image->image));
    }

    return pixmap;
}

void CursorTheme::setCursorName(qulonglong cursor, const QString &name) const
{
#ifdef HAVE_XFIXES

    if (haveXfixes()) {
        XFixesSetCursorName(QX11Info::display(), cursor, QFile::encodeName(name).constData());
    }
#endif
}

bool CursorTheme::haveXfixes()
{
    bool result = false;

#ifdef HAVE_XFIXES
    if (!QX11Info::isPlatformX11()) {
        return result;
    }
    int event_base, error_base;
    if (XFixesQueryExtension(QX11Info::display(), &event_base, &error_base)) {
        int major, minor;
        XFixesQueryVersion(QX11Info::display(), &major, &minor);
        result = (major >= 2);
    }
#endif

    return result;
}
