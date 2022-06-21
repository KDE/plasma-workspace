/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PACKAGEFINDER_H
#define PACKAGEFINDER_H

#include <QObject>
#include <QRunnable>
#include <QSize>
#include <QTime>

#include <KPackage/Package>

#include "utils/dynamictype.h"

class QJsonValue;

namespace KPackage
{
class ImagePackage;
}

/**
 * A runnable that finds KPackage wallpapers.
 */
class PackageFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    PackageFinder(const QStringList &paths, const QSize &targetSize, QObject *parent = nullptr);

    void run() override;

    static QString packageDisplayName(const KPackage::Package &b);

Q_SIGNALS:
    void packageFound(const QList<KPackage::ImagePackage> &packages);

private:
    QStringList m_paths;
    QSize m_targetSize;
};

#endif // PACKAGEFINDER_H
