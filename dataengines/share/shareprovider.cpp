/***************************************************************************
 *   Copyright 2010 Artur Duque de Souza <asouza@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include <QFile>
#include <QXmlStreamReader>

#include <krandom.h>
#include <QDebug>
#include <QTemporaryFile>
#include <KIO/MimetypeJob>
#include <KIO/FileJob>
#include <kjsembed/kjsembed.h>
#include <kjsembed/variant_binding.h>
#include <QImage>
#include <QUrlQuery>
#include <QPixmap>

#include "shareprovider.h"

ShareProvider::ShareProvider(KJSEmbed::Engine* engine, QObject *parent)
    : QObject(parent), m_isBlob(false), m_isPost(true),
      m_engine(engine)
{
    // Just make the boundary random part long enough to be sure
    // it's not inside one of the arguments that we are sending
    m_boundary  = "----------";
    m_boundary += KRandom::randomString(55).toLatin1();
}

QString ShareProvider::method() const
{
    if (!m_isPost) {
        return QStringLiteral("GET");
    }
    return QStringLiteral("POST");
}

void ShareProvider::setMethod(const QString &method)
{
    if (method == QLatin1String("GET")) {
        m_isPost = false;
    } else {
        m_isPost = true;
    }
}

QUrl ShareProvider::url() const
{
    // the url that is set in this provider
    return m_url;
}

void ShareProvider::setUrl(const QString &url)
{
    // set the provider's url
    m_url = QUrl(url);
    m_service = m_url;
}

QString ShareProvider::parseXML(const QString &key, const QString &data)
{
    // this method helps plugins to parse results from webpages
    QXmlStreamReader xml(data);
    if (xml.hasError()) {
        return QString();
    }

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.name() == key) {
            QString url = xml.readElementText();
            return url;
        }
    }

    return QString();
}

void ShareProvider::addPostItem(const QString &key, const QString &value,
                                const QString &contentType)
{
    if (!m_isPost)
        return;

    // add a pair <item,value> in a post form
    QByteArray str;
    QString length = QString::number(value.length());

    str += "--";
    str += m_boundary;
    str += "\r\n";

    if (!key.isEmpty()) {
        str += "Content-Disposition: form-data; name=\"";
        str += key.toLatin1();
        str += "\"\r\n";
    }

    if (!contentType.isEmpty()) {
        str += "Content-Type: " + QByteArray(contentType.toLatin1());
        str += "\r\n";
        str += "Mime-version: 1.0 ";
        str += "\r\n";
    }

    str += "Content-Length: ";
    str += length.toLatin1();
    str += "\r\n\r\n";
    str += value.toUtf8();

    m_buffer.append(str);
    m_buffer.append("\r\n");
}

void ShareProvider::addPostFile(const QString &contentKey, const QVariant &content)
{
    // add a file in a post form (gets it using KIO)
    m_contentKey = contentKey;

    if(content.type() == QVariant::String) {
        m_content = content.toString();
        addPostItem(m_contentKey, m_content, QStringLiteral("text/plain"));
        addQueryItem(m_contentKey, m_content);
        emit readyToPublish();
    } else if(content.type() == QVariant::Url) {
        publishUrl(content.toUrl());
    } else if(content.type() == QVariant::Image) {
        QTemporaryFile* file = new QTemporaryFile(QStringLiteral("shareimage-XXXXXX.png"), this);
        bool b = file->open();
        Q_ASSERT(b);
        file->close();

        QImage image = content.value<QImage>();
        b = image.save(file->fileName());
        Q_ASSERT(b);
        publishUrl(QUrl::fromLocalFile(file->fileName()));
    } else if(content.type() == QVariant::Pixmap) {
        QTemporaryFile* file = new QTemporaryFile(QStringLiteral("sharepixmap-XXXXXX.png"), this);
        bool b = file->open();
        Q_ASSERT(b);
        file->close();

        QPixmap image = content.value<QPixmap>();
        b = image.save(file->fileName());
        Q_ASSERT(b);
        publishUrl(QUrl::fromLocalFile(file->fileName()));
    }
}

void ShareProvider::publishUrl(const QUrl& url)
{
    m_content = url.toString();

    KIO::MimetypeJob *mjob = KIO::mimetype(url, KIO::HideProgressInfo);
    connect(mjob, &KJob::finished, this, &ShareProvider::mimetypeJobFinished);
}

void ShareProvider::mimetypeJobFinished(KJob *job)
{
    KIO::MimetypeJob *mjob = qobject_cast<KIO::MimetypeJob *>(job);
    if (!job) {
        return;
    }

    if (mjob->error()) {
        qWarning() << "error when figuring out the file type";
        return;
    }

    // It's a valid file because there were no errors
    m_mimetype = mjob->mimetype();
    if (m_mimetype.isEmpty()) {
        // if we ourselves can't determine the mime of the file,
        // very unlikely the remote site will be able to identify it
        error(i18n("Could not detect the file's mimetype"));
        return;
    }

    // If it's not text then we should handle it later
    if (!m_mimetype.startsWith(QLatin1String("text/")))
        m_isBlob = true;

    // try to open the file
    KIO::FileJob *fjob = KIO::open(QUrl(m_content), QIODevice::ReadOnly);
    connect(fjob, &KIO::FileJob::open, this, &ShareProvider::openFile);
}

void ShareProvider::openFile(KIO::Job *job)
{
    // finished opening the file, now try to read it's content
    KIO::FileJob *fjob = static_cast<KIO::FileJob*>(job);
    fjob->read(fjob->size());
    connect(fjob, &KIO::FileJob::data,
            this, &ShareProvider::finishedContentData);
}

void ShareProvider::finishedContentData(KIO::Job *job, const QByteArray &data)
{
    // Close the job as we don't need it anymore.
    // NOTE: this is essential to ensure the job gets de-scheduled and deleted!
    job->disconnect(this);
    qobject_cast<KIO::FileJob *>(job)->close();

    if (data.length() == 0) {
        error(i18n("It was not possible to read the selected file"));
        return;
    }
    uploadData(data);
}

void ShareProvider::uploadData(const QByteArray& data)
{
    if (!m_isBlob) {
        // it's just text and we can return here using data()
        addPostItem(m_contentKey, QString::fromLocal8Bit(data), QStringLiteral("text/plain"));
        addQueryItem(m_contentKey, QString::fromLocal8Bit(data));
        emit readyToPublish();
        return;
    }

    // Add the special http post stuff with the content of the file
    QByteArray str;
    const QString fileSize = QString::number(data.size());
    str += "--";
    str += m_boundary;
    str += "\r\n";
    str += "Content-Disposition: form-data; name=\"";
    str += m_contentKey.toLatin1();
    str += "\"; ";
    str += "filename=\"";
    str += QFile::encodeName(QUrl(m_content).fileName()).replace(".tmp", ".jpg");
    str += "\"\r\n";
    str += "Content-Length: ";
    str += fileSize.toLatin1();
    str += "\r\n";
    str += "Content-Type: ";
    str +=  m_mimetype.toLatin1();
    str += "\r\n\r\n";

    m_buffer.append(str);
    m_buffer.append(data);
    m_buffer.append("\r\n");

    // tell the world that we are ready to publish
    emit readyToPublish();
}

void ShareProvider::readPublishData(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job);
    m_data.append(data);
}

void ShareProvider::finishedPublish(KJob *job)
{
    Q_UNUSED(job);
    if (m_data.length() == 0) {
        error(i18n("Service was not available"));
        return;
    }

    // process data. should be interpreted by the plugin.
    // plugin must call the right slots after processing the data.
    KJS::List kjsargs;
    kjsargs.append( KJSEmbed::convertToValue(m_engine->interpreter()->execState(), m_data) );
    m_engine->callMethod("handleResultData", kjsargs);
}

void ShareProvider::finishHeader()
{
    QByteArray str;
    str += "--";
    str += m_boundary;
    str += "--";
    m_buffer.append(str);
}

void ShareProvider::addQueryItem(const QString &key, const QString &value)
{
    // just add the item to the query's URL
    QUrlQuery uq(m_url);
    uq.addQueryItem(key, value);
    m_url.setQuery(uq);
}

void ShareProvider::publish()
{
    if (m_url.isEmpty()) {
        emit finishedError(i18n("You must specify a URL for this service"));
    }

    // clear the result data before publishing
    m_data.clear();

    // finish the http form
    if (m_isBlob) {
        finishHeader();
    }

    // Multipart is used to upload files
    KIO::TransferJob *tf;
    if (m_isBlob) {
        tf = KIO::http_post(m_service, m_buffer, KIO::HideProgressInfo);
        tf->addMetaData(QStringLiteral("content-type"),"Content-Type: multipart/form-data; boundary=" + m_boundary);
    } else {
        if (m_isPost) {
            tf = KIO::http_post(m_service,
                                m_url.query(QUrl::FullyEncoded).toLatin1(), KIO::HideProgressInfo);
            tf->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/x-www-form-urlencoded"));
        } else {
            QUrl url = m_service;
            url.setQuery(m_url.query());
            tf = KIO::get(url);
        }
    }

    connect(tf, &KIO::TransferJob::data,
            this, &ShareProvider::readPublishData);
    connect(tf, &KJob::result, this, &ShareProvider::finishedPublish);
    connect(tf, &KIO::TransferJob::redirection,
            this, &ShareProvider::redirected);
}

void ShareProvider::redirected(KIO::Job *job, const QUrl &to)
{
    Q_UNUSED(job)
    const QUrl toUrl(to);
    const QUrl serviceUrl(m_service);

    const QString toString(toUrl.toString(QUrl::StripTrailingSlash));
    const QString serviceString(serviceUrl.toString(QUrl::StripTrailingSlash));

    if (toString == serviceString) {
        return;
    }

    KJS::List kjsargs;
    kjsargs.append( KJSEmbed::convertToValue(m_engine->interpreter()->execState(), toString) );
    m_engine->callMethod("handleRedirection", kjsargs);
}

void ShareProvider::success(const QString &url)
{
    // notify the service that it worked and the result url
    emit finished(url);
}

void ShareProvider::error(const QString &msg)
{
    // notify the service that it didnt work and the error msg
    emit finishedError(msg);
}


