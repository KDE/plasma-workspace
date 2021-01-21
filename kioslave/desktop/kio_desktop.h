/* This file is part of the KDE project
   Copyright (C) 2008, 2009 Fredrik Höglund <fredrik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KIO_DESKTOP_H
#define KIO_DESKTOP_H

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

#endif
