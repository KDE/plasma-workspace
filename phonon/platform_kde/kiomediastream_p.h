/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef KIOMEDIASTREAM_P_H
#define KIOMEDIASTREAM_P_H

#include "kiomediastream.h"
#include <kio/jobclasses.h>

namespace Phonon {

class KioMediaStreamPrivate
{
    Q_DECLARE_PUBLIC(KioMediaStream)
protected:
    KioMediaStream *q_ptr;

    KioMediaStreamPrivate(const QUrl &u)
        : url(u),
          endOfDataSent(false),
          seeking(false),
          reading(false),
          open(false),
          seekPosition(0),
          kiojob(nullptr)
    {
    }

    ~KioMediaStreamPrivate()
    {
        if (kiojob) {
            kiojob->kill();
            kiojob = nullptr;
        }
    }

    void _k_bytestreamNeedData();
    void _k_bytestreamEnoughData();
    void _k_bytestreamData(KIO::Job *, const QByteArray &);
    void _k_bytestreamResult(KJob *);
    void _k_bytestreamTotalSize(KJob *, qulonglong);
    void _k_bytestreamSeekStream(qint64);
    void _k_bytestreamFileJobOpen(KIO::Job *);
    void _k_bytestreamSeekDone(KIO::Job *, KIO::filesize_t);
    void _k_read();

    QUrl url;
    bool endOfDataSent;
    bool seeking;
    bool reading;
    bool open;
    qint64 seekPosition;
    KIO::SimpleJob *kiojob;
};

} // namespace Phonon

#endif // KIOMEDIASTREAM_P_H
