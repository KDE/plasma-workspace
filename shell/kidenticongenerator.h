/*
 *   Copyright 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KIDENTICONGENERATOR_H
#define KIDENTICONGENERATOR_H

#include <QPixmap>
#include <QIcon>

#include <Plasma/Svg>

class KIdenticonGenerator {
public:
    static KIdenticonGenerator * self();
    ~KIdenticonGenerator();

    QPixmap generatePixmap(int size, QString id, QIcon::Mode mode = QIcon::Normal);
    QPixmap generatePixmap(int size, quint32 hash, QIcon::Mode mode = QIcon::Normal);
    QPixmap generatePixmap(int size, const QIcon & icon, QIcon::Mode mode = QIcon::Normal);

    QIcon generate(int size, const QString & data);
    QIcon generate(int size, quint32 hash);
    QIcon generate(int size, const QIcon & icon);

private:
    KIdenticonGenerator();

    class Private;
    Private * const d;
};

#endif // KIDENTICONGENERATOR_H
