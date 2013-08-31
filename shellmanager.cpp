/*
 *   Copyright (C) 2013 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "shellmanager.h"

#include <algorithm>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QTimer>

#include <QQmlEngine>
#include <QQmlComponent>

#include <config-prefix.h>
#include "shellcorona.h"
#include "shellpluginloader.h"

static const QString s_shellsDir(
        QString(CMAKE_INSTALL_PREFIX) + "/" + DATA_INSTALL_DIR + "/" + "plasma/shells/");
static const QString s_shellLoaderPath = QString("/contents/loader.qml");

//
// ShellManager
//

class ShellManager::Private {
public:
    Private()
        : currentHandler(nullptr)
    {
        shellUpdateDelay.setInterval(100);
        shellUpdateDelay.setSingleShot(true);
    }

    QList<QObject *> handlers;
    QObject * currentHandler;
    QTimer shellUpdateDelay;
    ShellCorona * corona;
};

ShellManager::ShellManager()
    : d(new Private())
{
    ShellPluginLoader::init();

    connect(
        &d->shellUpdateDelay, &QTimer::timeout,
        this, &ShellManager::updateShell
    );

    d->corona = new ShellCorona(this);

    connect(
        this,      &ShellManager::shellChanged,
        d->corona, &ShellCorona::setShell
    );

    loadHandlers();
}

ShellManager::~ShellManager()
{
    // if (d->currentHandler)
    //     d->currentHandler->unload();
}

void ShellManager::loadHandlers()
{
    // TODO: Use corona's qml engine when it switches from QScriptEngine
    static QQmlEngine * engine = new QQmlEngine(this);

    for (const auto & dir: QDir(s_shellsDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString qmlFile = s_shellsDir + dir + s_shellLoaderPath;
        // qDebug() << "Making a new instance of " << qmlFile;

        QQmlComponent handlerComponent(engine,
                QUrl::fromLocalFile(qmlFile)
            );
        auto handler = handlerComponent.create();

        // Writing out the errors
        for (const auto & error: handlerComponent.errors()) {
            qWarning() << "Error: " << error;
        }

        if (handler) {
            registerHandler(handler);
        }
    }

    updateShell();
}

void ShellManager::registerHandler(QObject * handler)
{
    // qDebug() << "We got the handler: " << handler->property("shell").toString();

    connect(
        handler, &QObject::destroyed,
        this,    &ShellManager::deregisterHandler
    );

    connect(
        handler, SIGNAL(willingChanged()),
        this,    SLOT(requestShellUpdate())
    );

    connect(
        handler, SIGNAL(priorityChanged()),
        this,    SLOT(requestShellUpdate())
    );

    d->handlers.push_back(handler);
}

void ShellManager::deregisterHandler(QObject * handler)
{
    if (d->handlers.contains(handler)) {
        d->handlers.removeAll(handler);

        handler->disconnect(this);
    }

    if (d->currentHandler == handler)
        d->currentHandler = nullptr;
}

void ShellManager::requestShellUpdate()
{
    d->shellUpdateDelay.start();
}

void ShellManager::updateShell()
{
    d->shellUpdateDelay.stop();

    if (d->handlers.isEmpty()) {
        qFatal("We have no shell handlers installed");
        return;
    }

    // Finding the handler that has the priority closest to zero.
    // We will return a handler even if there are no willing ones.

    auto handler =* std::min_element(d->handlers.cbegin(), d->handlers.cend(),
            [] (QObject * left, QObject * right)
            {
                auto willing = [] (QObject * handler)
                {
                    return handler->property("willing").toBool();
                };

                auto priority = [] (QObject * handler)
                {
                    return handler->property("priority").toInt();
                };

                return
                    // If one is willing and the other is not,
                    // return it - it has the priority
                    willing(left) && !willing(right) ? true :
                    !willing(left) && willing(right) ? false :
                    // otherwise just compare the priorities
                    priority(left) < priority(right);
            }
         );

    if (handler == d->currentHandler) return;

    // Activating the new handler and killing the old one
    if (d->currentHandler) {
        d->currentHandler->setProperty("loaded", false);
    }

    d->currentHandler = handler;
    d->currentHandler->setProperty("loaded", true);

    // d->corona->setShell(d->currentHandler->property("shell").toString());
    emit shellChanged(d->currentHandler->property("shell").toString());
}

ShellManager * ShellManager::instance()
{
    static ShellManager manager;
    return &manager;
}

