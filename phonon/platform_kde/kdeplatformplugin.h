/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PHONON_KDEPLATFORMPLUGIN_H
#define PHONON_KDEPLATFORMPLUGIN_H

#include <QObject>
#include <phonon/objectdescription.h>
#include <phonon/platformplugin.h>

namespace Phonon
{
class KdePlatformPlugin : public QObject, public PlatformPlugin
{
    Q_PLUGIN_METADATA(IID "3PlatformPlugin.phonon.kde.org" FILE "phononbackend.json")
    Q_INTERFACES(Phonon::PlatformPlugin)
    Q_OBJECT
public:
    KdePlatformPlugin();
    ~KdePlatformPlugin() override;

    AbstractMediaStream *createMediaStream(const QUrl &url, QObject *parent) override;

    QIcon icon(const QString &name) const override;
    void notification(const char *notificationName, const QString &text, const QStringList &actions, QObject *receiver, const char *actionSlot) const override;
    QString applicationName() const override;
    QObject *createBackend() override;
    QObject *createBackend(const QString &library, const QString &version) override;
    bool isMimeTypeAvailable(const QString &mimeType) const override;
    void saveVolume(const QString &outputName, qreal volume) override;
    qreal loadVolume(const QString &outputName) const override;

    QList<int> objectDescriptionIndexes(ObjectDescriptionType type) const override;
    QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType type, int index) const override;

signals:
    void objectDescriptionChanged(ObjectDescriptionType);
};

} // namespace Phonon

#endif // PHONON_KDEPLATFORMPLUGIN_H
