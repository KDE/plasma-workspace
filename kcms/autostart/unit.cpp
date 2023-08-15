/*
    SPDX-FileCopyrightText: 2023 Thenujan Sandramohan <sthenujan2002@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "unit.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDateTime>
#include <QFile>

#if HAVE_SYSTEMD
#include <systemd/sd-journal.h>
#endif

using namespace Qt::Literals::StringLiterals;

static const QMap<QString, QString> STATE_MAP = {
    {u"active"_s, i18nc("@label Entry is running right now", "Running")},
    {u"inactive"_s, i18nc("@label Entry is not running right now (exited without error)", "Not running")},
    {u"activating"_s, i18nc("@label Entry is being started", "Starting")},
    {u"deactivating"_s, i18nc("@label Entry is being stopped", "Stopping")},
    {u"failed"_s, i18nc("@label Entry has failed (exited with an error)", "Failed")},
};

Unit::Unit(QObject *parent, bool invalid)
    : QObject(parent)
{
    m_invalid = invalid;
}

Unit::~Unit()
{
}

void Unit::setId(const QString &id)
{
    m_id = id;
    loadAllProperties();
}

void Unit::loadAllProperties()
{
    // Get unit path using Manager interface
    auto message = QDBusMessage::createMethodCall(m_connSystemd, m_pathSysdMgr, m_ifaceMgr, "GetUnit");
    message.setArguments(QList<QVariant>{m_id});
    QDBusPendingCall async = m_sessionBus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &Unit::callFinishedSlot);
}

void Unit::callFinishedSlot(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QDBusObjectPath> reply = *call;
    if (reply.isError()) {
        m_invalid = true;
        Q_EMIT dataChanged();
        return;
    } else {
        m_dbusObjectPath = reply.argumentAt(0).value<QDBusObjectPath>();
    }
    call->deleteLater();
    QDBusMessage message =
        QDBusMessage::createMethodCall(m_connSystemd, m_dbusObjectPath.path(), QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));

    message << m_ifaceUnit;
    QDBusPendingCall newCall = m_sessionBus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(newCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &Unit::getAllCallback);
}

void Unit::getAllCallback(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    if (reply.isError()) {
        Q_EMIT error(i18n("Error occurred when receiving reply of GetAll call %1", reply.error().message()));
        call->deleteLater();
        return;
    }

    QVariantMap properties = reply.argumentAt<0>();
    call->deleteLater();

    m_activeState = STATE_MAP[properties[QStringLiteral("ActiveState")].toString()];
    m_description = properties[QStringLiteral("Description")].toString();
    qulonglong ActiveEnterTimestamp = properties[QStringLiteral("ActiveEnterTimestamp")].toULongLong();

    setActiveEnterTimestamp(ActiveEnterTimestamp);

    reloadLogs();
    QDBusConnection userbus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_connSystemd);
    userbus.connect(m_connSystemd,
                    m_dbusObjectPath.path(),
                    "org.freedesktop.DBus.Properties",
                    "PropertiesChanged",
                    this,
                    SLOT(dbusPropertiesChanged(QString, QVariantMap, QStringList)));
}

void Unit::setActiveEnterTimestamp(qulonglong ActiveEnterTimestamp)
{
    if (ActiveEnterTimestamp == 0) {
        m_timeActivated = QStringLiteral("N/A");
    } else {
        QDateTime dateTimeActivated;
        dateTimeActivated.setMSecsSinceEpoch(ActiveEnterTimestamp / 1000);
        m_timeActivated = dateTimeActivated.toString();
    }
}

void Unit::dbusPropertiesChanged(QString name, QVariantMap map, QStringList list)
{
    Q_UNUSED(name);
    Q_UNUSED(list);
    if (map.contains("ActiveEnterTimestamp")) {
        setActiveEnterTimestamp(map["ActiveEnterTimestamp"].toULongLong());
    }
    if (map.contains("ActiveState")) {
        m_activeState = STATE_MAP[map["ActiveState"].toString()];
    }

    Q_EMIT dataChanged();
}

void Unit::start()
{
    auto message = QDBusMessage::createMethodCall(m_connSystemd, m_dbusObjectPath.path(), m_ifaceUnit, "Start");
    message << QStringLiteral("replace");
    m_sessionBus.send(message);
}

void Unit::stop()
{
    auto message = QDBusMessage::createMethodCall(m_connSystemd, m_dbusObjectPath.path(), m_ifaceUnit, "Stop");
    message << QStringLiteral("replace");
    m_sessionBus.send(message);
}

QStringList Unit::getLastJournalEntries(const QString &unit)
{
#if HAVE_SYSTEMD
    sd_journal *journal;

    int returnValue = sd_journal_open(&journal, (SD_JOURNAL_LOCAL_ONLY | SD_JOURNAL_CURRENT_USER));
    if (returnValue != 0) {
        Q_EMIT journalError(i18n("Failed to open journal"));
        sd_journal_close(journal);
        return {};
    }

    sd_journal_flush_matches(journal);

    const QString match1 = QStringLiteral("USER_UNIT=%1").arg(unit);
    returnValue = sd_journal_add_match(journal, match1.toUtf8(), 0);
    if (returnValue != 0) {
        sd_journal_close(journal);
        return {};
    }

    sd_journal_add_disjunction(journal);

    const QString match2 = QStringLiteral("_SYSTEMD_USER_UNIT=%1").arg(unit);
    returnValue = sd_journal_add_match(journal, match2.toUtf8(), 0);
    if (returnValue != 0) {
        sd_journal_close(journal);
        return {};
    }

    returnValue = sd_journal_seek_tail(journal);
    if (returnValue != 0) {
        sd_journal_close(journal);
        return {};
    }

    QStringList reply;
    QString lastDateTime;
    // Fetch the last 50 entries
    for (int i = 0; i < 50; ++i) {
        returnValue = sd_journal_previous(journal);
        if (returnValue != 1) {
            // previous failed, no more entries
            sd_journal_close(journal);
            return reply;
        }
        QString line;

        // Get the date and time
        uint64_t time;
        returnValue = sd_journal_get_realtime_usec(journal, &time);

        if (returnValue == 0) {
            QDateTime date;
            date.setMSecsSinceEpoch(time / 1000);
            if (lastDateTime != date.toString()) {
                line.append(date.toString("yyyy.MM.dd hh:mm: "));
                lastDateTime = date.toString();
            } else {
                line.append(QString("&#8203;&#32;").repeated(33));
            }
        }

        // Color messages according to priority
        size_t length;
        const void *data;
        returnValue = sd_journal_get_data(journal, "PRIORITY", &data, &length);
        if (returnValue == 0) {
            int prio = QString::fromUtf8((const char *)data, length).section('=', 1).toInt();
            // Adding color to the logs is done in c++ so we can't add Kirigami color here so we add a string here
            //  and then replace it all with actual colors in qml code
            if (prio <= 3)
                line.append("<font color='Kirigami.Theme.negativeTextColor'>");
            else if (prio == 4)
                line.append("<font color='Kirigami.Theme.neutralTextColor'>");
            else
                line.append("<font>");
        }

        // Get the message itself
        returnValue = sd_journal_get_data(journal, "MESSAGE", &data, &length);
        if (returnValue == 0) {
            line.append(QString::fromUtf8((const char *)data, length).section('=', 1) + "</font>");
            reply << line;
        }
    }

    sd_journal_close(journal);

    return reply;
#else
    return QStringList();
#endif
}

void Unit::reloadLogs()
{
    QStringList logsList = getLastJournalEntries(m_id);

    logsList.removeAll({});
    m_logs = logsList.join(QStringLiteral("<br>"));

    Q_EMIT dataChanged();
}