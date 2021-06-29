/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
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
