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


#include "akonadiengine.h"


#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>
#include <akonadi/kmime/messageparts.h>

#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <kabc/picture.h>
#include <kabc/key.h>

using namespace Akonadi;

AkonadiEngine::AkonadiEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent),
    m_emailMonitor(0),
    m_contactMonitor(0)
{
    Q_UNUSED(args);
    setMaxSourceCount( 512 ); // Guard against loading thousands of emails
}

void AkonadiEngine::initEmailMonitor()
{
    m_emailMonitor = new Monitor( this );
    m_emailMonitor->setMimeTypeMonitored("message/rfc822");
    //m_emailMonitor->setCollectionMonitored(Collection::root(), false);
    m_emailMonitor->itemFetchScope().fetchFullPayload( true );
    connect(m_emailMonitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
            SLOT(emailItemAdded(Akonadi::Item)) );
    connect(m_emailMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
            SLOT(emailItemAdded(Akonadi::Item)) );
    // remove the monitor on a source that's not used
    connect(this, SIGNAL(sourceRemoved(QString)), SLOT(stopMonitor(QString)));
}

void AkonadiEngine::initContactMonitor()
{
    m_contactMonitor = new Monitor( this );
    m_contactMonitor->setMimeTypeMonitored("text/directory");
    m_contactMonitor->setCollectionMonitored(Collection::root(), false);
    m_contactMonitor->itemFetchScope().fetchFullPayload();
    connect(m_contactMonitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
            SLOT(contactItemAdded(Akonadi::Item)) );
    connect(m_contactMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
            SLOT(contactItemAdded(Akonadi::Item)) );
    // remove the monitor on a source that's not used
    connect(this, SIGNAL(sourceRemoved(QString)), SLOT(stopMonitor(QString)));
}


AkonadiEngine::~AkonadiEngine()
{
}

QStringList AkonadiEngine::sources() const
{
    return QStringList() << "EmailCollections" << "ContactCollections";
}

void AkonadiEngine::fetchEmailCollectionsDone(KJob* job)
{
    // called when the job fetching email collections from Akonadi emits result()
    if ( job->error() ) {
        qDebug() << "Job Error:" << job->errorString();
    } else {
        CollectionFetchJob* cjob = static_cast<CollectionFetchJob*>( job );
        int i = 0;
        foreach( const Collection &collection, cjob->collections() ) {
            if (collection.contentMimeTypes().contains("message/rfc822")) {
                //qDebug() << "EmailCollection setting data:" << collection.name() << collection.url() << collection.contentMimeTypes();
                i++;
                setData("EmailCollections", QString("EmailCollection-%1").arg(collection.id()), collection.name());
            }
        }
        qDebug() << i << "Email collections are in now";
        scheduleSourcesUpdated();
    }
}

void AkonadiEngine::fetchContactCollectionsDone(KJob* job)
{
    // called when the job fetching contact collections from Akonadi emits result()
    if ( job->error() ) {
        qDebug() << "Job Error:" << job->errorString();
    } else {
        CollectionFetchJob* cjob = static_cast<CollectionFetchJob*>( job );
        int i = 0;
        foreach( const Collection &collection, cjob->collections() ) {
            if (collection.contentMimeTypes().contains("text/directory")) {
                //qDebug() << "ContactCollection setting data:" << collection.name() << collection.url() << collection.contentMimeTypes();
                i++;
                setData("ContactCollections", QString("ContactCollection-%1").arg(collection.id()), collection.name());
            }
        }
        qDebug() << i << "Contact collections are in now";
        scheduleSourcesUpdated();
    }
}

bool AkonadiEngine::sourceRequestEvent(const QString &name)
{
    qDebug() << "Source requested:" << name << sources();

    if (name == "EmailCollections") {
        Collection emailCollection(Collection::root());
        emailCollection.setContentMimeTypes(QStringList() << "message/rfc822");
        CollectionFetchJob *fetch = new CollectionFetchJob( emailCollection, CollectionFetchJob::Recursive);
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchEmailCollectionsDone(KJob*)) );
        // For async data fetching, it's mandatory to set the data source to empty before returning true
        setData(name, DataEngine::Data());
        return true;

    } else if (name.startsWith(QString("EmailCollection-"))) {
        qlonglong id = name.split('-')[1].toLongLong();
        ItemFetchJob* fetch = new ItemFetchJob( Collection( id ), this );
        if (!m_emailMonitor) {
            initEmailMonitor();
        }
        m_emailMonitor->setCollectionMonitored(Collection( id ), true);
        fetch->fetchScope().fetchPayloadPart( MessagePart::Envelope );
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchEmailCollectionDone(KJob*)) );
        connect( fetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(emailItemsReceived(Akonadi::Item::List)) );
        m_jobCollections[fetch] = name;
        setData(name, DataEngine::Data());
        return true;

    } else if (name.startsWith(QString("Email-"))) {
        qlonglong id = name.split('-')[1].toLongLong();
        ItemFetchJob* fetch = new ItemFetchJob( Item( id ), this );
        if (!m_emailMonitor) {
            initEmailMonitor();
        }
        m_emailMonitor->setItemMonitored(Item( id ), true);
        fetch->fetchScope().fetchFullPayload( true );
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchEmailCollectionDone(KJob*)) );
        connect( fetch, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(emailItemsReceived(Akonadi::Item::List)) );
        m_jobCollections[fetch] = name;
        setData(name, DataEngine::Data());
        return true;

    } else if (name == "ContactCollections") {
        Collection contactCollection(Collection::root());
        contactCollection.setContentMimeTypes(QStringList() << "text/directory");

        CollectionFetchJob* fetch = new CollectionFetchJob( contactCollection, CollectionFetchJob::Recursive);
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchContactCollectionsDone(KJob*)) );
        setData(name, DataEngine::Data());
        return true;

    } else if (name.startsWith(QString("ContactCollection-"))) {
        qlonglong id = name.split('-')[1].toLongLong();
        ItemFetchJob *fetch = new ItemFetchJob( Collection( id ), this );
        if (!m_contactMonitor) {
            initContactMonitor();
        }
        m_contactMonitor->setCollectionMonitored(Collection( id ), true); // FIXME: should be contacts monitor
        fetch->fetchScope().fetchFullPayload();
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchContactCollectionDone(KJob*)) );
        setData(name, DataEngine::Data());
        return true;

    } else if (name.startsWith(QString("Contact-"))) {
        qDebug() << "Fetching contact" << name;
        qlonglong id = name.split('-')[1].toLongLong();
        ItemFetchJob *fetch = new ItemFetchJob( Item( id ), this );
        if (!m_contactMonitor) {
            initContactMonitor();
        }
        m_contactMonitor->setItemMonitored(Item( id ), true); // FIXME: should be contacts monitor
        fetch->fetchScope().fetchFullPayload();
        connect( fetch, SIGNAL(result(KJob*)), SLOT(fetchContactCollectionDone(KJob*)) );
        setData(name, DataEngine::Data());
        return true;

    // We don't understand the request.
    qDebug() << "Don't know what to do with:" << name;
    return false;
}

void AkonadiEngine::stopMonitor(const QString &name)
{
    if (name.startsWith(QString("EmailCollection-"))) {
        // Stop monitoring this one
        qlonglong id = name.split('-')[1].toLongLong();
        m_emailMonitor->setCollectionMonitored( Collection( id ), false);
        qDebug() << "Removed monitor from:" << name << id;
    }
}

void AkonadiEngine::emailItemsReceived(const Akonadi::Item::List &items)
{
    foreach (const Akonadi::Item &item, items) {
        emailItemAdded(item);
    }
}

void AkonadiEngine::emailItemAdded(const Akonadi::Item &item, const QString &collection)
{
    if ( !item.hasPayload<MessagePtr>() ) {
        return;
    }
    MessagePtr msg = item.payload<MessagePtr>();
    if (msg) {
        QString source = QString::number( item.id() );
        source = "Email-" + source;
        //qDebug() << "new source adding:" << source << item.url() << msg->subject()->asUnicodeString();

        setData( source, "Id", item.id() );
        setData( source, "Collection", collection);
        setData( source, "Url", item.url().url() );
        setData( source, "Subject", msg->subject()->asUnicodeString() );
        setData( source, "From", msg->from()->asUnicodeString() );
        setData( source, "DateTime", msg->date()->dateTime().date() );
        setData( source, "To", msg->to()->asUnicodeString() );
        setData( source, "Cc", msg->cc()->asUnicodeString() );
        setData( source, "Bcc", msg->bcc()->asUnicodeString() );
        setData( source, "Body", QString(msg->mainBodyPart()->body()));
        // Flags
        //qDebug() << item.flags();
        setData( source, "Flag-New", !item.hasFlag("\\Seen") );
        setData( source, "Flag-Task", item.hasFlag("\\Task") ); // not in Akonadi!
        setData( source, "Flag-Important", item.hasFlag("important") );
        setData( source, "Flag-Attachment", item.hasFlag("has_attachment") );
        setData( source, "Flag-Spam", item.hasFlag("spam") );
        setData( source, "Flag-Draft", item.hasFlag("\\Draft") );
        setData( source, "Flag-Answered", item.hasFlag("\\Answered") );
        setData( source, "Flag-Deleted", item.hasFlag("\\Deleted") );
        setData( source, "Flag-Flagged", item.hasFlag("\\Flagged") );

        if (!collection.isEmpty()) {
            setData( collection, source, msg->subject()->asUnicodeString());
        }
        printMessage(msg);
        scheduleSourcesUpdated();
    }
}

void AkonadiEngine::printMessage(const MessagePtr msg)
{
    return;
    qDebug() << "sub" << msg->subject()->asUnicodeString();
    return;
    qDebug() << "=============== New Item" << msg->from()->asUnicodeString() << msg->subject()->asUnicodeString();
    qDebug() << "sub" << msg->subject()->asUnicodeString();
    qDebug() << "from" << msg->from()->asUnicodeString();
    qDebug() << "date" << msg->date()->dateTime().date();
    qDebug() << "to" << msg->to()->asUnicodeString();
    qDebug() << "cc" << msg->cc()->asUnicodeString();
    qDebug() << "bcc" << msg->bcc()->asUnicodeString();
    qDebug() << "body" << msg->mainBodyPart()->body();
}

void AkonadiEngine::fetchEmailCollectionDone(KJob* job)
{
    if ( job->error() ) {
        qDebug() << "Job Error:" << job->errorString();
        return;
    }
    const QString col = m_jobCollections[job];
    Item::List items = static_cast<ItemFetchJob*>( job )->items();
    foreach ( const Item &item, items ) {
        emailItemAdded(item, col);
    }
    m_jobCollections.remove(job);
    scheduleSourcesUpdated();
}

void AkonadiEngine::fetchContactCollectionDone(KJob* job)
{
    if ( job->error() ) {
        return;
    }
    Item::List items = static_cast<ItemFetchJob*>( job )->items();
    foreach ( const Item &item, items ) {
        contactItemAdded( item );
    }
}


void AkonadiEngine::printContact(const QString &source, const KABC::Addressee &a)
{
    qDebug() << "-----------------------------------";
    qDebug() << source;
    qDebug() << "name" << a.name();
    qDebug() << "formattedName" << a.formattedName();
    qDebug() << "nameLabel" << a.nameLabel();
    qDebug() << "given" << a.givenName();
    qDebug() << "familyName" << a.familyName();
    qDebug() << "realName" << a.realName();
    qDebug() << "organization" << a.organization();
    qDebug() << "department" << a.department();
    qDebug() << "role" << a.role();
    qDebug() << "emails" << a.emails();
    qDebug() << "fullEmail" << a.fullEmail();
    qDebug() << "photoUrl" << a.photo().url();
    qDebug() << "note" << a.note();

    QStringList phoneNumbers;
    foreach (const KABC::PhoneNumber &pn, a.phoneNumbers()) {
        const QString key = QString("Phone-%1").arg(pn.typeLabel());
        qDebug() << key << a.phoneNumber(pn.type()).number();
        phoneNumbers << a.phoneNumber(pn.type()).number();
    }
    qDebug() << "phoneNumbers" << phoneNumbers;

    qDebug() << "additionalName" << a.additionalName();

}

void AkonadiEngine::contactItemAdded( const Akonadi::Item &item )
{
    if (item.hasPayload<KABC::Addressee>()) {
        //qDebug() << item.id() << "item has payload ...";
        KABC::Addressee a = item.payload<KABC::Addressee>();
        if (!a.isEmpty()) {
            const QString source = QString("Contact-%1").arg(item.id());
            setData(source, "Id", item.id());
            setData(source, "Url", item.url().url());

            // Name and related
            setData(source, "Name", a.formattedName());
            setData(source, "GivenName", a.givenName());
            setData(source, "FamilyName", a.familyName());
            setData(source, "NickName", a.nickName());
            setData(source, "RealName", a.realName());
            setData(source, "AdditionalName", a.additionalName());

            // Organization and related
            setData(source, "Organization", a.organization());
            setData(source, "Department", a.department());
            setData(source, "Role", a.role());

            // EMail and related
            setData(source, "Emails", a.emails());
            setData(source, "FullEmail", a.fullEmail());

            // Phone and related
            QStringList phoneNumbers;
            foreach (const KABC::PhoneNumber &pn, a.phoneNumbers()) {
                const QString key = QString("Phone-%1").arg(pn.typeLabel());
                setData(source, key, a.phoneNumber(pn.type()).number());
                phoneNumbers << a.phoneNumber(pn.type()).number();
            }
            setData(source, "PhoneNumbers", phoneNumbers);

            // Personal
            setData(source, "Birthday", a.birthday());
            setData(source, "Photo", a.photo().data());
            setData(source, "PhotoUrl", a.photo().url());
            setData(source, "Latitude", a.geo().latitude());
            setData(source, "Longitude", a.geo().longitude());

            // addresses

            // categories

            // note,
            setData(source, "Note", a.note());
            // prefix

            // ...
            //printContact(source, a);

            scheduleSourcesUpdated();
        }
    }
}


