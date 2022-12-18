/*
    SPDX-FileCopyrightText: 2008, 2009 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KIO/ForwardingWorkerBase>

class DesktopProtocol : public KIO::ForwardingWorkerBase
{
    Q_OBJECT
public:
    DesktopProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~DesktopProtocol() override;

protected:
    void checkLocalInstall();
    QString desktopFile(KIO::UDSEntry &) const;
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
    KIO::WorkerResult listDir(const QUrl &url) override;
    void adjustUDSEntry(KIO::UDSEntry &entry, UDSEntryCreationMode creationMode) const override;
    KIO::WorkerResult rename(const QUrl &, const QUrl &, KIO::JobFlags flags) override;

private:
    KIO::WorkerResult fileSystemFreeSpace(const QUrl &url) override;
};
