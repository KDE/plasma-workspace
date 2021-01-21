/*
 *   Copyright (C) 2007 Tobias Koenig <tokoe@kde.org>
 *   Copyright (C) 2008 Marco Martin <notmart@gmail.com>
 *   Copyright (C) 2013 Andrea Scarpino <scarpino@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FAVICONPROVIDER_H
#define FAVICONPROVIDER_H

#include <QObject>

class QImage;

/**
 * This class provides a favicon for a given url
 */
class FaviconProvider : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new favicon provider.
     *
     * @param parent The parent object.
     * @param url The provider URL.
     */
    FaviconProvider(QObject *parent, const QString &url);

    /**
     * Destroys the favicon provider.
     */
    ~FaviconProvider() override;

    /**
     * Returns the requested image.
     *
     * @note This method returns only a valid image after the
     *       finished() signal has been emitted.
     */
    QImage image() const;

    /**
     * Returns the identifier of the comic request (name + date).
     */
    QString identifier() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever a request has been finished
     * successfully.
     *
     * @param provider The provider which emitted the signal.
     */
    void finished(FaviconProvider *provider);

    /**
     * This signal is emitted whenever an error has occurred.
     *
     * @param provider The provider which emitted the signal.
     */
    void error(FaviconProvider *provider);

private:
    QString m_url;

    class Private;
    Private *const d;
};

#endif
