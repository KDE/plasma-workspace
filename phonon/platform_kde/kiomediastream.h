/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only

*/

#ifndef PHONON_KIOMEDIASTREAM_H
#define PHONON_KIOMEDIASTREAM_H

#include <Phonon/AbstractMediaStream>

class QUrl;

namespace Phonon
{
class KioMediaStreamPrivate;

class KioMediaStream : public AbstractMediaStream
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(KioMediaStream)
public:
    explicit KioMediaStream(const QUrl &url, QObject *parent = nullptr);
    ~KioMediaStream() override;

protected:
    void reset() override;
    void needData() override;
    void enoughData() override;
    void seekStream(qint64) override;

    KioMediaStreamPrivate *d_ptr;

private:
    Q_PRIVATE_SLOT(d_func(), void _k_bytestreamData(KIO::Job *, const QByteArray &))
    Q_PRIVATE_SLOT(d_func(), void _k_bytestreamResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void _k_bytestreamTotalSize(KJob *, qulonglong))
    Q_PRIVATE_SLOT(d_func(), void _k_bytestreamFileJobOpen(KIO::Job *))
    Q_PRIVATE_SLOT(d_func(), void _k_bytestreamSeekDone(KIO::Job *, KIO::filesize_t))
    Q_PRIVATE_SLOT(d_func(), void _k_read())
};

} // namespace Phonon

#endif // PHONON_KIOMEDIASTREAM_H
