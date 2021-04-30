/*
    Copyright (C) 2021 Kai Uwe Broulik <kde@broulik.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <KService>

namespace KIO
{
class MimeTypeFinderJob;
}

class Application
{
    Q_GADGET

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(bool valid READ isValid CONSTANT)

public:
    Application();
    explicit Application(const KService::Ptr &service);

    QString name() const;
    QString iconName() const;
    bool isValid() const;

private:
    KService::Ptr m_service;
};
Q_DECLARE_METATYPE(Application)

class FileInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int error READ error NOTIFY errorChanged)

    Q_PROPERTY(QString mimeType READ mimeType NOTIFY mimeTypeChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY mimeTypeChanged)
    Q_PROPERTY(Application preferredApplication READ preferredApplication NOTIFY mimeTypeChanged)

public:
    explicit FileInfo(QObject *parent = nullptr);
    ~FileInfo() override;

    QUrl url() const;
    void setUrl(const QUrl &url);
    Q_SIGNAL void urlChanged(const QUrl &url);

    bool busy() const;
    Q_SIGNAL void busyChanged(bool busy);

    int error() const;
    Q_SIGNAL void errorChanged(bool error);

    QString mimeType() const;
    Q_SIGNAL void mimeTypeChanged();

    QString iconName() const;
    Q_SIGNAL void iconNameChanged(const QString &iconName);

    Application preferredApplication() const;
    Q_SIGNAL void preferredApplicationChanged();

private:
    void reload();
    void mimeTypeFound(const QString &mimeType);

    void setBusy(bool busy);
    void setError(int error);

    QUrl m_url;

    QPointer<KIO::MimeTypeFinderJob> m_job;
    bool m_busy = false;
    int m_error = 0;

    QString m_mimeType;
    QString m_iconName;

    Application m_preferredApplication;
};
