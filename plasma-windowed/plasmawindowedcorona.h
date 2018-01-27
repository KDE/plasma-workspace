/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef PLASMAWINDOWEDCORONA_H
#define PLASMAWINDOWEDCORONA_H

#include <Plasma/Corona>

class PlasmaWindowedCorona : public Plasma::Corona
{
    Q_OBJECT

public:
    explicit PlasmaWindowedCorona(QObject * parent = nullptr);
    QRect screenGeometry(int id) const override;

    void setHasStatusNotifier(bool stay);
    void loadApplet(const QString &applet, const QVariantList &arguments);

public Q_SLOTS:
    void load();
    void activateRequested(const QStringList &arguments, const QString &workingDirectory);

private:
    Plasma::Containment *m_containment = nullptr;
    bool m_hasStatusNotifier = false;
};

#endif
