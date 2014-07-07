/******************************************************************************
 *   Copyright (C) 2003 - 2004 by Frerich Raabe <raabe@kde.org>               *
 *                                Tobias Koenig <tokoe@kde.org>               *
 *   Copyright (C) 2006 by Narayan Newton <narayannewton@gmail.com>           *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING.BSD'.                        *
 *****************************************************************************/
/**
  @file

  This file is part of KXmlRpc and defines our internal classes.

  @author Frerich Raabe <raabe@kde.org>
  @author Tobias Koenig <tokoe@kde.org>
  @author Narayan Newton <narayannewton@gmail.com>
*/

#include "query.h"

#include <klocalizedstring.h>

#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QVariant>
#include <QtXml/QDomDocument>

using namespace KXmlRpc;

/**
  @file

  Implementation of Query
**/

namespace KXmlRpc {

/**
  @brief
  Result is an internal class that represents a response
  from a XML-RPC server.

  This is an internal class and is only used by Query.
  @internal
 */
class Result
{
  friend class Query;
  friend class Query::Private;

  public:
    /**
      Constructs a result.
     */
    Result();

    /**
      Destroys a result.
     */
    ~Result();

    /**
      Returns true if the method call succeeded, false
      if there was an XML-RPC fault.

      @see errorCode(), errorString()
     */
    bool success() const;

    /**
      Returns the error code of the fault.

      @see success(), errorString()
     */
    int errorCode() const;

    /**
      Returns the error string that describes the fault.

      @see success, errorCode()
     */
    QString errorString() const;

    /**
      Returns the data sent to us from the server.
     */
    QList<QVariant> data() const;

  private:
    bool mSuccess;
    int mErrorCode;
    QString mErrorString;
    QList<QVariant> mData;
};

} // namespace KXmlRpcClient

KXmlRpc::Result::Result()
{
}

KXmlRpc::Result::~Result()
{
}

bool KXmlRpc::Result::success() const
{
  return mSuccess;
}

int KXmlRpc::Result::errorCode() const
{
  return mErrorCode;
}

QString KXmlRpc::Result::errorString() const
{
  return mErrorString;
}

QList<QVariant> KXmlRpc::Result::data() const
{
  return mData;
}

class Query::Private
{
  public:
    Private( Query *parent )
      : mParent( parent )
    {
    }

    bool isMessageResponse( const QDomDocument &doc ) const;
    bool isFaultResponse( const QDomDocument &doc ) const;

    Result parseMessageResponse( const QDomDocument &doc ) const;
    Result parseFaultResponse( const QDomDocument &doc ) const;

    QString markupCall( const QString &method, const QList<QVariant> &args ) const;
    QString marshal( const QVariant &value ) const;
    QVariant demarshal( const QDomElement &element ) const;

    void slotData( KIO::Job *job, const QByteArray &data );
    void slotResult( KJob *job );

    Query *mParent;
    QByteArray mBuffer;
    QVariant mId;
    QList<KJob*> mPendingJobs;
};

bool Query::Private::isMessageResponse( const QDomDocument &doc ) const
{
  return doc.documentElement().firstChild().toElement().tagName().toLower()
      == QLatin1String("params");
}

bool Query::Private::isFaultResponse( const QDomDocument &doc ) const
{
  return doc.documentElement().firstChild().toElement().tagName().toLower()
      == QLatin1String("fault");
}

Result Query::Private::parseMessageResponse( const QDomDocument &doc ) const
{
  Result response;
  response.mSuccess = true;

  QDomNode paramNode = doc.documentElement().firstChild().firstChild();
  while ( !paramNode.isNull() ) {
    response.mData << demarshal( paramNode.firstChild().toElement() );
    paramNode = paramNode.nextSibling();
  }

  return response;
}

Result Query::Private::parseFaultResponse( const QDomDocument &doc ) const
{
  Result response;
  response.mSuccess = false;

  QDomNode errorNode = doc.documentElement().firstChild().firstChild();
  const QVariant errorVariant = demarshal( errorNode.toElement() );
  response.mErrorCode = errorVariant.toMap() [ QLatin1String("faultCode") ].toInt();
  response.mErrorString = errorVariant.toMap() [ QLatin1String("faultString") ].toString();

  return response;
}

QString Query::Private::markupCall( const QString &cmd,
                                    const QList<QVariant> &args ) const
{
  QString markup = QLatin1String("<?xml version=\"1.0\" ?>\r\n<methodCall>\r\n");

  markup += QLatin1String("<methodName>") + cmd + QLatin1String("</methodName>\r\n");

  if ( !args.isEmpty() ) {

    markup += QLatin1String("<params>\r\n");
    QList<QVariant>::ConstIterator it = args.begin();
    QList<QVariant>::ConstIterator end = args.end();
    for ( ; it != end; ++it ) {
      markup += QLatin1String("<param>\r\n") + marshal( *it ) + QLatin1String("</param>\r\n");
    }
    markup += QLatin1String("</params>\r\n");
  }

  markup += QLatin1String("</methodCall>\r\n");

  return markup;
}

QString Query::Private::marshal( const QVariant &arg ) const
{
  switch ( arg.type() ) {

    case QVariant::String:
      return QLatin1String("<value><string><![CDATA[") + arg.toString() + QLatin1String("]]></string></value>\r\n");
    case QVariant::StringList:
      {
        QStringList data = arg.toStringList();
        QStringListIterator dataIterator( data );
        QString markup;
        markup += QLatin1String("<value><array><data>");
        while ( dataIterator.hasNext() ) {
          markup += QLatin1String("<value><string><![CDATA[") + dataIterator.next() + QLatin1String("]]></string></value>\r\n");
        }
        markup += QLatin1String("</data></array></value>");
        return markup;
      }
    case QVariant::Int:
      return QLatin1String("<value><int>") + QString::number( arg.toInt() ) + QLatin1String("</int></value>\r\n");
    case QVariant::Double:
      return QLatin1String("<value><double>") + QString::number( arg.toDouble() ) + QLatin1String("</double></value>\r\n");
    case QVariant::Bool:
      {
        QString markup = QLatin1String("<value><boolean>");
        markup += arg.toBool() ? QLatin1String("1") : QLatin1String("0");
        markup += QLatin1String("</boolean></value>\r\n");
        return markup;
      }
    case QVariant::ByteArray:
      return QString::fromLatin1(QByteArray(QByteArray("<value><base64>") + arg.toByteArray().toBase64() + QByteArray("</base64></value>\r\n")));
    case QVariant::DateTime:
      {
        return QLatin1String("<value><dateTime.iso8601>") +
          arg.toDateTime().toString( Qt::ISODate ) +
          QLatin1String("</dateTime.iso8601></value>\r\n");
      }
    case QVariant::List:
      {
        QString markup = QLatin1String("<value><array><data>\r\n");
        const QList<QVariant> args = arg.toList();
        QList<QVariant>::ConstIterator it = args.begin();
        QList<QVariant>::ConstIterator end = args.end();
        for ( ; it != end; ++it ) {
          markup += marshal( *it );
        }
        markup += QLatin1String("</data></array></value>\r\n");
        return markup;
      }
    case QVariant::Map:
      {
        QString markup = QLatin1String("<value><struct>\r\n");
        QMap<QString, QVariant> map = arg.toMap();
        QMap<QString, QVariant>::ConstIterator it = map.constBegin();
        QMap<QString, QVariant>::ConstIterator end = map.constEnd();
        for ( ; it != end; ++it ) {
          markup += QLatin1String("<member>\r\n");
          markup += QLatin1String("<name>") + it.key() + QLatin1String("</name>\r\n");
          markup += marshal( it.value() );
          markup += QLatin1String("</member>\r\n");
        }
        markup += QLatin1String("</struct></value>\r\n");
        return markup;
      }
    default:
      qWarning() << "Failed to marshal unknown variant type:" << arg.type();
  };

  return QString();
}

QVariant Query::Private::demarshal( const QDomElement &element ) const
{
  Q_ASSERT( element.tagName().toLower() == QLatin1String("value") );

  const QDomElement typeElement = element.firstChild().toElement();
  const QString typeName = typeElement.tagName().toLower();

  if ( typeName == QLatin1String("string") ) {
    return QVariant( typeElement.text() );
  } else if ( typeName == QLatin1String("i4") || typeName == QLatin1String("int") ) {
    return QVariant( typeElement.text().toInt() );
  } else if ( typeName == QLatin1String("double") ) {
    return QVariant( typeElement.text().toDouble() );
  } else if ( typeName == QLatin1String("boolean") ) {

    if ( typeElement.text().toLower() == QLatin1String("true") || typeElement.text() == QLatin1String("1") ) {
      return QVariant( true );
    } else {
      return QVariant( false );
    }
  } else if ( typeName == QLatin1String("base64") ) {
    return QVariant( QByteArray::fromBase64( typeElement.text().toLatin1() ) );
  } else if ( typeName == QLatin1String("datetime") || typeName == QLatin1String("datetime.iso8601") ) {
    QDateTime date;
    QString dateText = typeElement.text();
    // Test for broken use of Basic ISO8601 date and extended ISO8601 time
    if ( 17 <= dateText.length() && dateText.length() <= 18 &&
         dateText.at( 4 ) !=  QLatin1Char('-') && dateText.at( 11 ) ==  QLatin1Char(':') ) {
        if ( dateText.endsWith( QLatin1Char('Z') ) ) {
          date = QDateTime::fromString( dateText, QLatin1String("yyyyMMddTHH:mm:ssZ") );
        } else {
          date = QDateTime::fromString( dateText, QLatin1String("yyyyMMddTHH:mm:ss") );
        }
    } else {
      date = QDateTime::fromString( dateText, Qt::ISODate );
    }
    return QVariant( date );
  } else if ( typeName == QLatin1String("array") ) {
    QList<QVariant> values;
    QDomNode valueNode = typeElement.firstChild().firstChild();
    while ( !valueNode.isNull() ) {
      values << demarshal( valueNode.toElement() );
      valueNode = valueNode.nextSibling();
    }
    return QVariant( values );
  } else if ( typeName == QLatin1String("struct") ) {

    QMap<QString, QVariant> map;
    QDomNode memberNode = typeElement.firstChild();
    while ( !memberNode.isNull() ) {
      const QString key = memberNode.toElement().elementsByTagName(
                              QLatin1String("name") ).item( 0 ).toElement().text();
      const QVariant data = demarshal( memberNode.toElement().elementsByTagName(
                                       QLatin1String("value") ).item( 0 ).toElement() );
      map[ key ] = data;
      memberNode = memberNode.nextSibling();
    }
    return QVariant( map );
  } else {
    qWarning() << "Cannot demarshal unknown type" << typeName;
  }
  return QVariant();
}

void Query::Private::slotData( KIO::Job *, const QByteArray &data )
{
  unsigned int oldSize = mBuffer.size();
  mBuffer.resize( oldSize + data.size() );
  memcpy( mBuffer.data() + oldSize, data.data(), data.size() );
}

void Query::Private::slotResult( KJob *job )
{
  mPendingJobs.removeAll( job );

  if ( job->error() != 0 ) {
    emit mParent->fault( job->error(), job->errorString(), mId );
    emit mParent->finished( mParent );
    return;
  }

  QDomDocument doc;
  QString errMsg;
  int errLine, errCol;
  if ( !doc.setContent( mBuffer, false, &errMsg, &errLine, &errCol ) ) {
    emit mParent->fault( -1, i18n( "Received invalid XML markup: %1 at %2:%3",
                                   errMsg, errLine, errCol ), mId );
    emit mParent->finished( mParent );
    return;
  }

  mBuffer.truncate( 0 );

  if ( isMessageResponse( doc ) ) {
    emit mParent->message( parseMessageResponse( doc ).data(), mId );
  } else if ( isFaultResponse( doc ) ) {
    emit mParent->fault( parseFaultResponse( doc ).errorCode(),
                         parseFaultResponse( doc ).errorString(), mId );
  } else {
    emit mParent->fault( 1, i18n( "Unknown type of XML markup received" ),
                         mId );
  }

  emit mParent->finished( mParent );
}

Query *Query::create( const QVariant &id, QObject *parent )
{
  return new Query( id, parent );
}

void Query::call( const QString &server,
                  const QString &method,
                  const QList<QVariant> &args,
                  const QMap<QString, QString> &jobMetaData )
{

  const QString xmlMarkup = d->markupCall( method, args );

  QMap<QString, QString>::const_iterator mapIter;
  QByteArray postData;
  QDataStream stream( &postData, QIODevice::WriteOnly );
  stream.writeRawData( xmlMarkup.toUtf8().constData(), xmlMarkup.toUtf8().length() );

  KIO::TransferJob *job = KIO::http_post( QUrl( server ), postData, KIO::HideProgressInfo );

  if ( !job ) {
    qWarning() << "Unable to create KIO job for" << server;
    return;
  }

  job->addMetaData( QLatin1String("content-type"), QLatin1String("Content-Type: text/xml; charset=utf-8") );
  job->addMetaData( QLatin1String("ConnectTimeout"), QLatin1String("50") );

  for ( mapIter = jobMetaData.begin(); mapIter != jobMetaData.end(); ++mapIter ) {
    job->addMetaData( mapIter.key(), mapIter.value() );
  }

  connect( job, SIGNAL(data(KIO::Job*,QByteArray)),
           this, SLOT(slotData(KIO::Job*,QByteArray)) );
  connect( job, SIGNAL(result(KJob*)),
           this, SLOT(slotResult(KJob*)) );

  d->mPendingJobs.append( job );
}

Query::Query( const QVariant &id, QObject *parent )
  : QObject( parent ), d( new Private( this ) )
{
  d->mId = id;
}

Query::~Query()
{
  QList<KJob*>::Iterator it;
  for ( it = d->mPendingJobs.begin(); it != d->mPendingJobs.end(); ++it ) {
    ( *it )->kill();
  }
  delete d;
}

#include "moc_query.cpp"

