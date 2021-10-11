/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontInstInterface.h"
#include "FontInst.h"
#include "FontinstIface.h"
#include "config-fontinst.h"
#include "debug.h"
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QProcess>
#include <kio/global.h>

namespace KFI
{
FontInstInterface::FontInstInterface()
    : m_interface(new OrgKdeFontinstInterface(OrgKdeFontinstInterface::staticInterfaceName(), FONTINST_PATH, QDBusConnection::sessionBus(), nullptr))
    , m_active(false)
{
    FontInst::registerTypes();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(QLatin1String(OrgKdeFontinstInterface::staticInterfaceName()),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForOwnerChange,
                                                           this);

    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &FontInstInterface::dbusServiceOwnerChanged);
    connect(m_interface, &OrgKdeFontinstInterface::status, this, &FontInstInterface::status);
    connect(m_interface, &OrgKdeFontinstInterface::fontList, this, &FontInstInterface::fontList);
    connect(m_interface, &OrgKdeFontinstInterface::fontStat, this, &FontInstInterface::fontStat);

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(OrgKdeFontinstInterface::staticInterfaceName())) {
        QProcess::startDetached(QLatin1String(KFONTINST_LIB_EXEC_DIR "/fontinst"), QStringList());
    }
}

FontInstInterface::~FontInstInterface()
{
}

int FontInstInterface::install(const QString &file, bool toSystem)
{
    m_interface->install(file, true, toSystem, getpid(), true);
    return waitForResponse();
}

int FontInstInterface::uninstall(const QString &name, bool fromSystem)
{
    m_interface->uninstall(name, fromSystem, getpid(), true);
    return waitForResponse();
}

int FontInstInterface::reconfigure()
{
    m_interface->reconfigure(getpid(), false);
    return waitForResponse();
}

Families FontInstInterface::list(bool system)
{
    Families rv;
    m_interface->list(system ? FontInst::SYS_MASK : FontInst::USR_MASK, getpid());
    if (FontInst::STATUS_OK == waitForResponse()) {
        rv = m_families;
        m_families = Families();
    }
    return rv;
}

Family FontInstInterface::statFont(const QString &file, bool system)
{
    Family rv;
    m_interface->statFont(file, system ? FontInst::SYS_MASK : FontInst::USR_MASK, getpid());
    if (FontInst::STATUS_OK == waitForResponse()) {
        rv = *m_families.items.begin();
        m_families = Families();
    }
    return rv;
}

QString FontInstInterface::folderName(bool sys)
{
    if (!m_interface) {
        return QString();
    }

    QDBusPendingReply<QString> reply = m_interface->folderName(sys);

    reply.waitForFinished();
    return reply.isError() ? QString() : reply.argumentAt<0>();
}

int FontInstInterface::waitForResponse()
{
    m_status = FontInst::STATUS_OK;
    m_families = Families();
    m_active = true;

    m_eventLoop.exec();
    qCDebug(KCM_KFONTINST_KIO) << "Loop finished";
    return m_status;
}

void FontInstInterface::dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to)
{
    if (m_active && to.isEmpty() && !from.isEmpty() && name == QLatin1String(OrgKdeFontinstInterface::staticInterfaceName())) {
        qCDebug(KCM_KFONTINST_KIO) << "Service died :-(";
        m_status = FontInst::STATUS_SERVICE_DIED;
        m_eventLoop.quit();
    }
}

void FontInstInterface::status(int pid, int value)
{
    if (m_active && pid == getpid()) {
        qCDebug(KCM_KFONTINST_KIO) << "Status:" << value;
        m_status = value;
        m_eventLoop.quit();
    }
}

void FontInstInterface::fontList(int pid, const QList<KFI::Families> &families)
{
    if (m_active && pid == getpid()) {
        m_families = 1 == families.count() ? *families.begin() : Families();
        m_status = 1 == families.count() ? (int)FontInst::STATUS_OK : (int)KIO::ERR_DOES_NOT_EXIST;
        m_eventLoop.quit();
    }
}

void FontInstInterface::fontStat(int pid, const KFI::Family &font)
{
    if (m_active && pid == getpid()) {
        m_families = Families(font, false);
        m_status = font.styles().count() > 0 ? (int)FontInst::STATUS_OK : (int)KIO::ERR_DOES_NOT_EXIST;
        m_eventLoop.quit();
    }
}

}
