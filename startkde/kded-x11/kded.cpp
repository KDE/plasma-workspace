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

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "xcb");
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication app(argc, argv);

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
