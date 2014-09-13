/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Sebastian Kügler <sebas@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/


#ifndef AKONADIENGINE_H
#define AKONADIENGINE_H

#include <plasma/dataengine.h>

#include <Akonadi/Item>
#include <Akonadi/Monitor>

#include <kmime/kmime_message.h>
#include <kabc/addressee.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

class KJob;

class AkonadiEngine : public Plasma::DataEngine
{
    Q_OBJECT

    public:
        AkonadiEngine( QObject* parent, const QVariantList& args );
        ~AkonadiEngine();
        QStringList sources() const;

    protected:
        bool sourceRequestEvent(const QString &name);

    private Q_SLOTS:

        void stopMonitor(const QString &name);

        void fetchEmailCollectionDone(KJob* job); // done retrieving whole collection
        void fetchContactCollectionDone(KJob* job); // done retrieving a whole contact collection

        void emailItemsReceived(const Akonadi::Item::List &items);

        void fetchEmailCollectionsDone(KJob* job); // got list of collections
        void fetchContactCollectionsDone(KJob* job);

        void emailItemAdded(const Akonadi::Item &item, const QString &collection = QString());
        void contactItemAdded(const Akonadi::Item & item);

    private:
        void initEmailMonitor();
        void initContactMonitor();
        // useful for debugging
        void printMessage(MessagePtr msg);
        void printContact(const QString &source, const KABC::Addressee &a);

        Akonadi::Monitor* m_emailMonitor;
        Akonadi::Monitor* m_contactMonitor;

        QHash<KJob*, QString> m_jobCollections;
};

K_EXPORT_PLASMA_DATAENGINE(akonadi, AkonadiEngine)

#endif
