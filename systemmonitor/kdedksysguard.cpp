/*
 *   Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 *   Copyright (C) 2006  Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kdedksysguard.h"

#include <QAction>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <KLocalizedString>
#include <KPluginFactory>

#include <KActionCollection>
#include <KGlobalAccel>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

K_PLUGIN_CLASS_WITH_JSON(KDEDKSysGuard, "ksysguard.json")

KDEDKSysGuard::KDEDKSysGuard(QObject *parent, const QVariantList &)
    : KDEDModule(parent)
{
    QTimer::singleShot(0, this, &KDEDKSysGuard::init);
}

KDEDKSysGuard::~KDEDKSysGuard()
{
}

void KDEDKSysGuard::init()
{
    KActionCollection *actionCollection = new KActionCollection(this);

    QAction *action = actionCollection->addAction(QStringLiteral("Show System Activity"));
    action->setText(i18n("Show System Activity"));
    connect(action, &QAction::triggered, this, &KDEDKSysGuard::showTaskManager);

    KGlobalAccel::self()->setGlobalShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_Escape));
}

void KDEDKSysGuard::showTaskManager()
{
    QDBusConnection con = QDBusConnection::sessionBus();
    QDBusConnectionInterface *interface = con.interface();
    if (interface->isServiceRegistered(QStringLiteral("org.kde.systemmonitor"))) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.systemmonitor"),
                                                          QStringLiteral("/"),
                                                          QStringLiteral("org.qtproject.Qt.QWidget"),
                                                          QStringLiteral("close"));

        con.asyncCall(msg);
    } else {
        const QString exe = QStandardPaths::findExecutable(QStringLiteral("systemmonitor"));
        QProcess::startDetached(exe, QStringList());
    }
}

#include "kdedksysguard.moc"
