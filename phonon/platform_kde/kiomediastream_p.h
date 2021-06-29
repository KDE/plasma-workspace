/*
    SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "kiomediastream.h"

#include <QUrl>

#include <KIO/Job>

namespace Phonon
{
class KioMediaStreamPrivate
{
    Q_DECLARE_PUBLIC(KioMediaStream)
protected:
    KioMediaStream *q_ptr;

    KioMediaStreamPrivate(const QUrl &u)
        : url(u)
        , endOfDataSent(false)
        , seeking(false)
        , reading(false)
        , open(false)
        , seekPosition(0)
        , kiojob(nullptr)
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
