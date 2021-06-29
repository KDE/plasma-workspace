/*
    SPDX-FileCopyrightText: 2008, 2009 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <kio/forwardingslavebase.h>

class DesktopProtocol : public KIO::ForwardingSlaveBase
{
    Q_OBJECT
public:
    DesktopProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~DesktopProtocol() override;

protected:
    void checkLocalInstall();
    QString desktopFile(KIO::UDSEntry &) const;
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
    void listDir(const QUrl &url) override;
    void prepareUDSEntry(KIO::UDSEntry &entry, bool listing = false) const override;
    void rename(const QUrl &, const QUrl &, KIO::JobFlags flags) override;

    void virtual_hook(int id, void *data) override;

private:
    void fileSystemFreeSpace(const QUrl &url);
};
