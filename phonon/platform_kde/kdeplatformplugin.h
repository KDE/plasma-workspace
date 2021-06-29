/*
    SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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

Q_SIGNALS:
    void objectDescriptionChanged(ObjectDescriptionType);
};

} // namespace Phonon
