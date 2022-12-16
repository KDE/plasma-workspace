/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QRunnable>

/**
 * A runnable that finds all available images in the specified paths.
 */
class ImageFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ImageFinder(const QStringList &paths, QObject *parent = nullptr);

    void run() override;

Q_SIGNALS:
    void imageFound(const QStringList &paths);

private:
    QStringList m_paths;
};
