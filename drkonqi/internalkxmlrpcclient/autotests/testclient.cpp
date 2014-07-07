/*
    This file is part of the kxmlrpc library.

    Copyright (c) 2006 Narayan Newton <narayannewton@gmail.com>

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

#include <qtest.h>

#include "testclient.h"

QTEST_GUILESS_MAIN( TestClient )

#include <kxmlrpcclient/client.h>
using namespace KXmlRpc;

void TestClient::testValidity()
{
  Client *c = new Client();
  c->setUrl( QUrl( QLatin1String("http://test:pass@fake.com/rpc2") ) );
  c->setUserAgent( QLatin1String("Fake/1.0/MozillaCompat") );
  c->setDigestAuthEnabled( true );
  QVERIFY( c->url() == QUrl( QLatin1String("http://test:pass@fake.com/rpc2" )) );
  QVERIFY( c->userAgent() == QLatin1String("Fake/1.0/MozillaCompat") );
  QVERIFY( c->isDigestAuthEnabled() == true );

  Client *other = new Client( QUrl( QLatin1String("http://test:pass@fake.com/rpc2") ) );
  QVERIFY( c->url() == other->url() );

  delete c;
  delete other;
}
