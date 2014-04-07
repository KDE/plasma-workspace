/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

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

#ifndef KIO_REMOTE_H
#define KIO_REMOTE_H

#include <kio/slavebase.h>
#include "remoteimpl.h"

class RemoteProtocol : public KIO::SlaveBase
{
public:
	RemoteProtocol(const QByteArray &protocol, const QByteArray &pool,
	               const QByteArray &app);
	virtual ~RemoteProtocol();
	virtual void listDir(const QUrl &url);
	virtual void stat(const QUrl &url);
	virtual void del(const QUrl &url, bool isFile);
	virtual void get(const QUrl &url);
	virtual void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags);
	virtual void symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags);

private:
	void listRoot();

	RemoteImpl m_impl;
};

#endif
