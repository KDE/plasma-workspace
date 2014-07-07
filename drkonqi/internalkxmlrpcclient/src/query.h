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

#ifndef KXML_RPC_QUERY_H
#define KXML_RPC_QUERY_H

#include "kxmlrpcclientprivate_export.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <kio/job.h>

class QString;

/** Namespace for XmlRpc related classes */
namespace KXmlRpc {

/**
  @brief
  Query is a class that represents an individual XML-RPC call.

  This is an internal class and is only used by the KXmlRpc::Client class.
  @internal
 */
class KXMLRPCCLIENTPRIVATE_EXPORT Query : public QObject
{
  friend class Result;
  Q_OBJECT

  public:
    /**
      Constructs a query.

      @param id an optional id for the query.
      @param parent an optional parent for the query.
     */
    static Query *create( const QVariant &id = QVariant(), QObject *parent = 0 );

  public Q_SLOTS:
    /**
      Calls the specified method on the specified server with
      the given argument list.

      @param server the server to contact.
      @param method the method to call.
      @param args an argument list to pass to said method.
      @param jobMetaData additional arguments to pass to the KIO::Job.
     */
    void call( const QString &server, const QString &method,
               const QList<QVariant> &args,
               const QMap<QString, QString> &jobMetaData );

  Q_SIGNALS:
    /**
      A signal sent when we receive a result from the server.
     */
    void message( const QList<QVariant> &result, const QVariant &id );

    /**
      A signal sent when we receive an error from the server.
     */
    void fault( int, const QString &, const QVariant &id );

    /**
      A signal sent when a query finishes.
     */
    void finished( Query * );

  private:
    explicit Query( const QVariant &id, QObject *parent = 0 );
    ~Query();

    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void slotData( KIO::Job *, const QByteArray & ) )
    Q_PRIVATE_SLOT( d, void slotResult( KJob * ) )
};

} // namespace XmlRpc

#endif

