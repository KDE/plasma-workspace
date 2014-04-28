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

#include <kcrash.h>

static const QString s_shellsDir = QString(QStandardPaths::locate(QStandardPaths::QStandardPaths::GenericDataLocation,
                                                  PLASMA_RELATIVE_DATA_INSTALL_DIR "/shells/",
                                                  QStandardPaths::LocateDirectory));
static const QString s_shellLoaderPath = QString("/contents/loader.qml");

bool ShellManager::s_forceWindowed = false;
bool ShellManager::s_noRespawn = false;

int ShellManager::crashes = 0;
QString ShellManager::s_fixedShell = QString();

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
    // Using setCrashHandler, we end up in an infinite loop instead of quitting,
    // use setEmergencySaveFunction instead to avoid this.
    KCrash::setEmergencySaveFunction(ShellManager::crashHandler);
    QTimer::singleShot(15 * 1000, this, SLOT(resetCrashCount()));

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
            handler->setProperty("pluginName", dir);
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

    if (d->currentHandler == handler) {
        d->currentHandler = nullptr;
        updateShell();
    }
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

    QObject *handler = 0;

    if (!s_fixedShell.isEmpty()) {
        QList<QObject *>::const_iterator it = std::find_if (d->handlers.cbegin(), d->handlers.cend(), [=] (QObject *handler) {
            return handler->property("pluginName").toString() == s_fixedShell;
        });
        if (it != d->handlers.cend()) {
            handler = *it;
        } else {
            qCritical("Unable to find the shell plugin '%s'", qPrintable(s_fixedShell));
            QCoreApplication::exit();
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

    d->currentHandler = handler;
    d->currentHandler->setProperty("loaded", true);

    emit shellChanged(d->currentHandler->property("shell").toString());
}

ShellManager * ShellManager::instance()
{
    static ShellManager manager;
    return &manager;
}

void ShellManager::setCrashCount(int count)
{
    crashes = count;
}

void ShellManager::resetCrashCount()
{
    crashes = 0;
}

void ShellManager::crashHandler(int signal)
{
    /* plasma-shell restart logic as crash recovery
     *
     * We restart plasma-shell after crashes. When it crashes subsequently on startup,
     * and more than two times in a row, we give up in order to not endlessly loop.
     * Once the shell process stays alive for at least 15 seconds, we reset the crash
     * counter, so a later crash, at runtime rather than on startup will still be
     * recovered from.
     *
     * This logic is very similar as to how kwin handles it.
     */
    crashes++;
    fprintf(stderr, "Application::crashHandler() called with signal %d; recent crashes: %d\n", signal, crashes);
    char cmd[1024];
    sprintf(cmd, "%s --crashes %d &",
            QFile::encodeName(QCoreApplication::applicationFilePath()).constData(), crashes);

    if (crashes < 3 && !s_noRespawn) {
        sleep(1);
        system(cmd);
    } else {
        fprintf(stderr, "Too many crashes in short order or respawning disabled, not restarting automatically.\n");
    }

}
