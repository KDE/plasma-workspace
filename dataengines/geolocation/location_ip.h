/*
 *   Copyright (C) 2009 Petri Damstén <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IP_H
#define IP_H

#include "geolocationprovider.h"

class Ip : public GeolocationProvider
{
    Q_OBJECT
public:
    explicit Ip(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~Ip() override;

    void update() override;

private:
    class Private;
    Private *const d;

};

#endif
