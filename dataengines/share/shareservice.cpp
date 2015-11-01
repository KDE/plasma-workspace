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
#include <QDebug>
#include <QStandardPaths>

#include <kjsembed/kjsembed.h>
#include <kjs/JSVariableObject.h>

#include <KPackage/PackageLoader>


#include "shareservice.h"
#include "shareprovider.h"
#include "config-workspace.h"

ShareService::ShareService(ShareEngine *engine)
    : Plasma::Service(engine)
{
    setName(QStringLiteral("share"));
}

Plasma::ServiceJob *ShareService::createJob(const QString &operation,
                                            QMap<QString, QVariant> &parameters)
{
    return new ShareJob(destination(), operation, parameters, this);
}

ShareJob::ShareJob(const QString &destination, const QString &operation,
                   QMap<QString, QVariant> &parameters, QObject *parent)
    : Plasma::ServiceJob(destination, operation, parameters, parent),
      m_engine(new KJSEmbed::Engine()), m_provider(0)
{
}

ShareJob::~ShareJob()
{
    delete m_provider;
}

void ShareJob::start()
{
    //KService::Ptr service = KService::serviceByStorageId("plasma-share-pastebincom.desktop");
    KService::Ptr service = KService::serviceByStorageId(destination());
    if (!service) {
        showError(i18n("Could not find the provider with the specified destination"));
        return;
    }

    QString pluginName =
        service->property(QStringLiteral("X-KDE-PluginInfo-Name"), QVariant::String).toString();

    const QString path =
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, PLASMA_RELATIVE_DATA_INSTALL_DIR "/shareprovider/" + pluginName + '/' );

    if (path.isEmpty()) {
        showError(i18n("Invalid path for the requested provider"));
        return;
    }

    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/ShareProvider"));
    m_package.setPath(path);
    if (m_package.isValid()) {
        const QString mainscript = m_package.filePath("mainscript");

        m_provider = new ShareProvider(m_engine.data(), this);
        connect(m_provider, &ShareProvider::readyToPublish, this, &ShareJob::publish);
        connect(m_provider, &ShareProvider::finished,
                this, &ShareJob::showResult);
        connect(m_provider, &ShareProvider::finishedError,
                this, &ShareJob::showError);

        m_engine->addObject(m_provider, "provider");

        // set the main script file and load it
        KJSEmbed::Engine::ExitStatus status = m_engine->runFile(mainscript);

        // check for any errors
        if(status == KJSEmbed::Engine::Failure) {
            showError(i18n("Error trying to execute script"));
            return;
        }

        KJS::ExecState* execState = m_engine->interpreter()->execState();
        KJS::JSGlobalObject* scriptObject = m_engine->interpreter()->globalObject();

        // do the work together with the loaded plugin
        if (!scriptObject->hasProperty(execState, "url") || !scriptObject->hasProperty(execState, "contentKey") ||
            !scriptObject->hasProperty(execState, "setup")) {
            showError(i18n("Could not find all required functions"));
            return;
        }

        // call the methods from the plugin
        const QString url = m_engine->callMethod("url")->toString(execState).qstring();
        m_provider->setUrl(url);

        // setup the method (get/post)
        QVariant vmethod;
        if (scriptObject->hasProperty(execState, "method")) {
            vmethod = m_engine->callMethod("method")->toString(execState).qstring();
        }

        // default is POST (if the plugin does not specify one method)
        const QString method = vmethod.isValid() ? vmethod.toString() : QStringLiteral("POST");
        m_provider->setMethod(method);

        // setup the provider
        m_engine->callMethod("setup");

        // get the content from the parameters, set the url and add the file
        // then we can wait the signal to publish the information
        const QString contentKey = m_engine->callMethod("contentKey")->toString(execState).qstring();

        QVariant contents = parameters()[QStringLiteral("content")];
        if(contents.type() == QVariant::List) {
            QVariantList list = contents.toList();
            if (list.size() == 1) {
                contents = list.first();
            }
        }
        m_provider->addPostFile(contentKey, contents);
    }
}

void ShareJob::publish()
{
    m_provider->publish();
}

void ShareJob::showResult(const QString &url)
{
    setResult(url);
}

void ShareJob::showError(const QString &message)
{
    QString errorMsg = message;
    if (errorMsg.isEmpty()) {
        errorMsg = i18n("Unknown Error");
    }

    setError(1);
    setErrorText(message);
    emitResult();
}


