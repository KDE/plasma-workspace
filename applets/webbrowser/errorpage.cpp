/* This file is part of the KDE project
 *
 * Copyright (C) Hamish Rodda, Urs Wolfer, Davide Bettio
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QNetworkReply>
#include <QTextDocument>

#include <QDebug>
#include <kio/global.h>
#include <KIconLoader>
#include <KLocale>
#include <KStandardDirs>

#include "errorpage.h"

int webKitErrorToKIOError(int eValue)
{
    switch (eValue){
        case QNetworkReply::NoError:
            return 0;

        case QNetworkReply::ConnectionRefusedError:
            return KIO::ERR_COULD_NOT_CONNECT;

        case QNetworkReply::HostNotFoundError:
            return KIO::ERR_UNKNOWN_HOST;

        case QNetworkReply::TimeoutError:
            return KIO::ERR_SERVER_TIMEOUT;

        case QNetworkReply::OperationCanceledError:
            return KIO::ERR_ABORTED;

        case QNetworkReply::ProxyNotFoundError:
            return KIO::ERR_UNKNOWN_PROXY_HOST;

        case QNetworkReply::ContentAccessDenied:
            return KIO::ERR_ACCESS_DENIED;

        case QNetworkReply::ContentOperationNotPermittedError:
            return KIO::ERR_WRITE_ACCESS_DENIED;

        case QNetworkReply::ContentNotFoundError:
            return KIO::ERR_NO_CONTENT;

        case QNetworkReply::AuthenticationRequiredError:
            return KIO::ERR_COULD_NOT_AUTHENTICATE;

        case QNetworkReply::ProtocolUnknownError:
            return KIO::ERR_UNSUPPORTED_PROTOCOL;

        case QNetworkReply::ProtocolInvalidOperationError:
            return KIO::ERR_UNSUPPORTED_ACTION;

        default:
            return KIO::ERR_UNKNOWN;
    }
}

/*
 * from khtml_part code
 */
QString errorPageHtml( int errorCode, const QString& text, const KUrl& reqUrl )
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( errorCode, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = Qt::escape( reqUrl.prettyUrl() );
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(),
                                                KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "khtml/error.html" ) );
  QFile file( filename );
  bool isOpened = file.open( QIODevice::ReadOnly );
  if ( !isOpened )
    kWarning(6050) << "Could not open error html template:" << filename;

  QString html = QString( QLatin1String( file.readAll() ) );

  html.replace( QLatin1String( "TITLE" ), i18n( "Error: %1 - %2", errorName, url ) );
  html.replace( QLatin1String( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  KUrl iconUrl(KIconLoader::global()->iconPath( "dialog-warning", -KIconLoader::SizeHuge ));
  iconUrl.setProtocol("file://");
  html.replace( QLatin1String( "ICON_PATH" ), iconUrl.url());

  QString doc = QLatin1String( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QLatin1String( "</h1><h2>" );
  doc += errorName;
  doc += QLatin1String( "</h2>" );
  if ( !techName.isNull() ) {
    doc += QLatin1String( "<h2>" );
    doc += i18n( "Technical Reason: " );
    doc += techName;
    doc += QLatin1String( "</h2>" );
  }
  doc += QLatin1String( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QLatin1String( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QLatin1String( "</li><li>" );
  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QLatin1String( "</li><li>" );
  }
  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QLatin1String( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QLatin1String( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QLatin1String( "</h3><p>" );
  doc += description;
  doc += QLatin1String( "</p>" );
  if ( causes.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }
  if ( solutions.count() ) {
    doc += QLatin1String( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QLatin1String( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QLatin1String( "</li></ul>" );
  }

  html.replace( QLatin1String("TEXT"), doc );
  
  return html;
}
