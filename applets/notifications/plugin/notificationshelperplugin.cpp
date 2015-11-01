/*
    Copyright (C) 2014  Martin Klapetek <mklapetek@kde.org>

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

#include "notificationshelperplugin.h"
#include "notificationshelper.h"

#include <QtQml>
#include <QProcess>
#include <QQmlNetworkAccessManagerFactory>
#include <QUrl>

class NoAccessNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override {
        QNetworkAccessManager *manager = new QNetworkAccessManager(parent);
        manager->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
        return manager;
    }
};

class UrlHelper : public QObject {
    Q_OBJECT
    public:
        Q_INVOKABLE bool isUrlValid(const QString &url) const {
           return QUrl::fromUserInput(url).isValid();
        }
};

static QObject *urlcheck_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new UrlHelper();
}

class ProcessRunner : public QObject
{
    Q_OBJECT
    public:
        Q_INVOKABLE void runNotificationsKCM() const {
            QProcess::startDetached(QStringLiteral("kcmshell5"), QStringList() << QStringLiteral("kcmnotify"));
        }
};

static QObject *processrunner_singleton_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return new ProcessRunner();
}

void NotificationsHelperPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.notifications"));

    qmlRegisterType<NotificationsHelper>(uri, 1, 0, "NotificationsHelper");
    qmlRegisterSingletonType<UrlHelper>(uri, 1, 0, "UrlHelper", urlcheck_singletontype_provider);
    qmlRegisterSingletonType<ProcessRunner>(uri, 1, 0, "ProcessRunner", processrunner_singleton_provider);
}

void NotificationsHelperPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.notifications"));

    auto oldFactory = engine->networkAccessManagerFactory();
    engine->setNetworkAccessManagerFactory(nullptr);
    delete oldFactory;
    engine->setNetworkAccessManagerFactory(new NoAccessNetworkAccessManagerFactory);
}

#include "notificationshelperplugin.moc"
