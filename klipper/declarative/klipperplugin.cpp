/*
     SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

     SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDBusMetaType>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include "previewimageprovider.h"

void qml_register_types_org_kde_plasma_private_clipboard();

class KlipperPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
    Q_DISABLE_COPY_MOVE(KlipperPlugin)
public:
    KlipperPlugin(QObject *parent = nullptr)
        : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_org_kde_plasma_private_clipboard;
        Q_UNUSED(registration);
    }

    void initializeEngine(QQmlEngine *engine, const char * /*uri*/) override
    {
        engine->addImageProvider(QStringLiteral("klipperpreview"), new PreviewImageProvider);
    }
};

#include "klipperplugin.moc"
