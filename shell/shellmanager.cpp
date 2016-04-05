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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QList>
#include <QTimer>

#include <qplatformdefs.h>
#include <QQmlEngine>
#include <QQmlComponent>

//#include <config-prefix.h>
#include "shellcorona.h"
#include "config-workspace.h"

#include <KMessageBox>
#include <KLocalizedString>

static const QStringList s_shellsDirs(QStandardPaths::locateAll(QStandardPaths::QStandardPaths::GenericDataLocation,
                                                  PLASMA_RELATIVE_DATA_INSTALL_DIR "/shells/",
                                                  QStandardPaths::LocateDirectory));
static const QString s_shellLoaderPath = QStringLiteral("/contents/loader.qml");

bool ShellManager::s_forceWindowed = false;
bool ShellManager::s_standaloneOption = false;
QString ShellManager::s_fixedShell;
QString ShellManager::s_testModeLayout;

//
// ShellManager
//

class ShellManager::Private {
public:
    Private()
        : currentHandler(nullptr),
          corona(0)
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
    //we have to ensure this is executed after QCoreApplication::exec()
    QMetaObject::invokeMethod(this, "loadHandlers", Qt::QueuedConnection);
}

ShellManager::~ShellManager()
{
    // if (d->currentHandler)
    //     d->currentHandler->unload();
}

void ShellManager::loadHandlers()
{
    //this should be executed one single time in the app life cycle
    Q_ASSERT(!d->corona);

    d->corona = new ShellCorona(this);

    connect(
        this,      &ShellManager::shellChanged,
        d->corona, &ShellCorona::setShell
    );

    // TODO: Use corona's qml engine when it switches from QScriptEngine
    static QQmlEngine * engine = new QQmlEngine(this);

    for (const QString &shellsDir: s_shellsDirs) {
        for (const auto & dir: QDir(shellsDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString qmlFile = shellsDir + dir + s_shellLoaderPath;
            // qDebug() << "Making a new instance of " << qmlFile;

            //this shell is not valid, ignore it
            if (!QFile::exists(qmlFile)) {
                continue;
            }

            QQmlComponent handlerComponent(engine,
                    QUrl::fromLocalFile(qmlFile)
                );
            auto handler = handlerComponent.create();

            // Writing out the errors
            for (const auto & error: handlerComponent.errors()) {
                qWarning() << "Error: " << error;
            }

            if (handler) {
                handler->setProperty("pluginName", dir);
                // This property is useful for shells to launch themselves in some specific sessions
                // For example mediacenter shell can be launched when in plasma-mediacenter session
                handler->setProperty("currentSession", QString::fromUtf8(qgetenv("DESKTOP_SESSION")));
                registerHandler(handler);
            }
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
    const int removed = d->handlers.removeAll(handler);
    if (removed > 0) {
        handler->disconnect(this);
    }

    if (d->currentHandler == handler) {
        d->currentHandler = nullptr;
        updateShell();
    }
    handler->deleteLater();
}

void ShellManager::requestShellUpdate()
{
    d->shellUpdateDelay.start();
}

void ShellManager::updateShell()
{
    d->shellUpdateDelay.stop();

    if (d->handlers.isEmpty()) {
        KMessageBox::error(0, //wID, but we don't have a window yet
                           i18nc("Fatal error message body","All shell packages missing.\nThis is an installation issue, please contact your distribution"),
                           i18nc("Fatal error message title", "Plasma Cannot Start"));
        qCritical("We have no shell handlers installed");
        QCoreApplication::exit(-1);
    }

    QObject *handler = 0;

    if (!s_fixedShell.isEmpty()) {
        QList<QObject *>::const_iterator it = std::find_if (d->handlers.cbegin(), d->handlers.cend(), [=] (QObject *handler) {
            return handler->property("pluginName").toString() == s_fixedShell;
        });
        if (it != d->handlers.cend()) {
            handler = *it;
        } else {
            KMessageBox::error(0,
                               i18nc("Fatal error message body", "Shell package %1 cannot be found", s_fixedShell),
                               i18nc("Fatal error message title", "Plasma Cannot Start"));
            qCritical("Unable to find the shell plugin '%s'", qPrintable(s_fixedShell));
            QCoreApplication::exit(-1);
        }
    } else {
        // Finding the handler that has the priority closest to zero.
        // We will return a handler even if there are no willing ones.
        handler =* std::min_element(d->handlers.cbegin(), d->handlers.cend(),
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
    }

    if (handler == d->currentHandler) return;

    // Activating the new handler and killing the old one
    if (d->currentHandler) {
        d->currentHandler->setProperty("loaded", false);
    }

    // handler will never be null, unless there is no shells
    // available on the system, which is definitely not something
    // we want to support :)
    d->currentHandler = handler;
    d->currentHandler->setProperty("loaded", true);

    emit shellChanged(d->currentHandler->property("shell").toString());
}

ShellManager * ShellManager::instance()
{
    static ShellManager* manager = nullptr;
    if (!manager) {
         manager = new ShellManager;
    }
    return manager;
}

Plasma::Corona* ShellManager::corona() const
{
    return d->corona;
}
