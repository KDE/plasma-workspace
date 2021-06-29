/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2013 Andrea Scarpino <scarpino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
