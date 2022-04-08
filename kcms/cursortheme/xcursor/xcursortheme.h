/*
    SPDX-FileCopyrightText: 2006-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#pragma once

#include <QHash>

#include "cursortheme.h"

class QDir;

struct _XcursorImage;
struct _XcursorImages;

typedef _XcursorImage XcursorImage;
typedef _XcursorImages XcursorImages;

/**
 * The XCursorTheme class is a CursorTheme implementation for Xcursor themes.
 */
class XCursorTheme : public CursorTheme
{
public:
    /**
     * Initializes itself from the @p dir information, and parses the
     * index.theme file if the dir has one.
     */
    XCursorTheme(const QDir &dir);
    ~XCursorTheme() override
    {
    }

    const QStringList inherits() const
    {
        return m_inherits;
    }

    QImage loadImage(const QString &name, int size = 0) const override;
    std::vector<CursorImage> loadImages(const QString &name, int size = 0) const override;
    qulonglong loadCursor(const QString &name, int size = 0) const override;

    /** Returns the size that the XCursor library would use if no
        cursor size is given. This depends mainly on Xft.dpi. */
    int defaultCursorSize() const override;

protected:
    XCursorTheme(const QString &title, const QString &desc)
        : CursorTheme(title, desc)
    {
    }
    void setInherits(const QStringList &val)
    {
        m_inherits = val;
    }

private:
    XcursorImage *xcLoadImage(const QString &name, int size) const;
    XcursorImages *xcLoadImages(const QString &name, int size) const;
    void parseIndexFile();
    QString findAlternative(const QString &name) const;

    QStringList m_inherits;
    static QHash<QString, QString> alternatives;
};
