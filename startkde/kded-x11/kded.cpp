/*
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kded_debug.h"

#include <QApplication>
#include <QHash>

#include <KConfigGroup>
#include <KDEDModule>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KSharedConfig>

#include <unistd.h>
#include <xcb/xfixes.h>

class Kded : public QObject
{
public:
    Kded()
    {
        const auto modules = KPluginMetaData::findPlugins(QStringLiteral("kf6/kded-x11"));
        for (const KPluginMetaData &module : modules) {
            const QString moduleId = module.pluginId();

            auto factoryResult = KPluginFactory::loadFactory(module);
            if (!factoryResult) {
                qCWarning(KDED).nospace() << "Could not load kded module " << moduleId << ": " << factoryResult.errorText;
                continue;
            }

            auto *kdedModule = factoryResult.plugin->create<KDEDModule>(this);
            if (!kdedModule) {
                qCWarning(KDED) << "Could not create kded module" << moduleId;
                continue;
            }

            kdedModule->setModuleName(moduleId);
            m_modules.insert(moduleId, kdedModule);
            qCDebug(KDED) << "Successfully loaded module" << moduleId;
        }
    }

    ~Kded() override
    {
        qDeleteAll(m_modules);
    }

private:
    QHash<QString, KDEDModule *> m_modules;
};

static void configureDisconnectMode()
{
    const auto nativeInterface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    Q_ASSERT(nativeInterface);
    xcb_connection_t *connection = nativeInterface->connection();

    const xcb_query_extension_reply_t *queryReply = xcb_get_extension_data(connection, &xcb_xfixes_id);
    if (!queryReply || !queryReply->present) {
        qWarning() << "No xfixes, kded-x11 may keep Xwayland around";
        return;
    }

    QScopedPointer<xcb_xfixes_query_version_reply_t, QScopedPointerPodDeleter> reply(
        xcb_xfixes_query_version_reply(connection, xcb_xfixes_query_version(connection, 6, 0), nullptr));
    if (!reply || reply->major_version < 6) {
        qWarning() << "xcb_xfixes_set_client_disconnect_mode is unsupported";
        return;
    }

    xcb_xfixes_set_client_disconnect_mode(connection, XCB_XFIXES_CLIENT_DISCONNECT_FLAGS_TERMINATE);
    xcb_flush(connection);
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication app(argc, argv);

    configureDisconnectMode();

    app.setQuitOnLastWindowClosed(false);
    app.setQuitLockEnabled(false);

    Kded kded;

    const pid_t pid = fork();
    if (pid < 0) {
        qCWarning(KDED) << "Could not fork into the background";
    } else if (pid > 0) {
        return 0;
    }

    return app.exec();
}
