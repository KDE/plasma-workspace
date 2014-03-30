/*
 * Copyright 2012 Gregor Taetzner gregor@freenet.de
 *
 * This program is free software; you can redistribute it and/or        *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PACKAGEKITSERVICE_H
#define PACKAGEKITSERVICE_H

#include <Plasma/Service>


class PackagekitService : public Plasma::Service
{
    Q_OBJECT
public:
    PackagekitService(QObject* parent = 0);
    Plasma::ServiceJob* createJob(const QString& operation, QMap< QString, QVariant >& parameters);
};

#endif // PACKAGEKITSERVICE_H
