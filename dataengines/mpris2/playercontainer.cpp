/*
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "playercontainer.h"

#include <dbusproperties.h>
#include <mprisplayer.h>
#include <mprisroot.h>

#define MPRIS2_PATH "/org/mpris/MediaPlayer2"
#define POS_UPD_STRING "Position last updated (UTC)"

#include <KDesktopFile>

#include <QDBusConnection>
#include <QDateTime>

#include "debug.h"

static QVariant::Type expPropType(const QString& propName)
{
    if (propName == QLatin1String("Identity"))
        return QVariant::String;
    else if (propName == QLatin1String("DesktopEntry"))
        return QVariant::String;
    else if (propName == QLatin1String("SupportedUriSchemes"))
        return QVariant::StringList;
    else if (propName == QLatin1String("SupportedMimeTypes"))
        return QVariant::StringList;
    else if (propName == QLatin1String("Fullscreen"))
        return QVariant::Bool;
    else if (propName == QLatin1String("PlaybackStatus"))
        return QVariant::String;
    else if (propName == QLatin1String("LoopStatus"))
        return QVariant::String;
    else if (propName == QLatin1String("Shuffle"))
        return QVariant::Bool;
    else if (propName == QLatin1String("Rate"))
        return QVariant::Double;
    else if (propName == QLatin1String("MinimumRate"))
        return QVariant::Double;
    else if (propName == QLatin1String("MaximumRate"))
        return QVariant::Double;
    else if (propName == QLatin1String("Volume"))
        return QVariant::Double;
    else if (propName == QLatin1String("Position"))
        return QVariant::LongLong;
    else if (propName == QLatin1String("Metadata"))
        return QVariant::Map;
    // we give out CanControl, as this may completely
    // change the UI of the widget
    else if (propName == QLatin1String("CanControl"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanSeek"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanGoNext"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanGoPrevious"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanRaise"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanQuit"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanPlay"))
        return QVariant::Bool;
    else if (propName == QLatin1String("CanPause"))
        return QVariant::Bool;
    return QVariant::Invalid;
}

static PlayerContainer::Cap capFromName(const QString& capName)
{
    if (capName == QLatin1String("CanQuit"))
        return PlayerContainer::CanQuit;
    else if (capName == QLatin1String("CanRaise"))
        return PlayerContainer::CanRaise;
    else if (capName == QLatin1String("CanSetFullscreen"))
        return PlayerContainer::CanSetFullscreen;
    else if (capName == QLatin1String("CanControl"))
        return PlayerContainer::CanControl;
    else if (capName == QLatin1String("CanPlay"))
        return PlayerContainer::CanPlay;
    else if (capName == QLatin1String("CanPause"))
        return PlayerContainer::CanPause;
    else if (capName == QLatin1String("CanSeek"))
        return PlayerContainer::CanSeek;
    else if (capName == QLatin1String("CanGoNext"))
        return PlayerContainer::CanGoNext;
    else if (capName == QLatin1String("CanGoPrevious"))
        return PlayerContainer::CanGoPrevious;
    return PlayerContainer::NoCaps;
}

PlayerContainer::PlayerContainer(const QString& busAddress, QObject* parent)
    : DataContainer(parent)
    , m_caps(NoCaps)
    , m_fetchesPending(0)
    , m_dbusAddress(busAddress)
    , m_currentRate(0.0)
{
    Q_ASSERT(!busAddress.isEmpty());
    Q_ASSERT(busAddress.startsWith(QLatin1String("org.mpris.MediaPlayer2.")));

    // MPRIS specifies, that in case a player supports several instances, each additional
    // instance after the first one is supposed to append ".instance<pid>" at the end of
    // its dbus address. So instances of media players, which implement this correctly
    // can have one D-Bus connection per instance and can be identified by their pids.
    QDBusReply<uint> pidReply = QDBusConnection::sessionBus().interface()->servicePid(busAddress);
    if (pidReply.isValid()) {
        setData("InstancePid", pidReply.value());
    }

    m_propsIface = new OrgFreedesktopDBusPropertiesInterface(
            busAddress, MPRIS2_PATH,
            QDBusConnection::sessionBus(), this);

    m_playerIface = new OrgMprisMediaPlayer2PlayerInterface(
            busAddress, MPRIS2_PATH,
            QDBusConnection::sessionBus(), this);

    m_rootIface = new OrgMprisMediaPlayer2Interface(
            busAddress, MPRIS2_PATH,
            QDBusConnection::sessionBus(), this);

    connect(m_propsIface, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged,
            this,         &PlayerContainer::propertiesChanged);

    connect(m_playerIface, &OrgMprisMediaPlayer2PlayerInterface::Seeked,
            this,          &PlayerContainer::seeked);

    refresh();
}

void PlayerContainer::refresh()
{
    // despite these calls being async, we should never update values in the
    // wrong order (eg: a stale GetAll response overwriting a more recent value
    // from a PropertiesChanged signal) due to D-Bus message ordering guarantees.

    QDBusPendingCall async = m_propsIface->GetAll(OrgMprisMediaPlayer2Interface::staticInterfaceName());
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,    &PlayerContainer::getPropsFinished);
    ++m_fetchesPending;

    async = m_propsIface->GetAll(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName());
    watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,    &PlayerContainer::getPropsFinished);
    ++m_fetchesPending;
}

static bool decodeUri(QVariantMap &map, const QString& entry) {
    if (map.contains(entry)) {
        QString urlString = map.value(entry).toString();
        QUrl url = QUrl::fromEncoded(urlString.toUtf8());
        if (!url.isValid()) {
            // try to be lenient
            url = QUrl(urlString);
        }
        if (url.isValid()) {
            map.insert(entry, QVariant(url));
            return true;
        } else {
            map.remove(entry);
            return false;
        }
    }
    // count it as a success if it doesn't exist
    return true;
}

void PlayerContainer::copyProperty(const QString& propName, const QVariant& _value, QVariant::Type expType, UpdateType updType)
{
    QVariant value = _value;
    // we protect our users from bogus values
    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
        if (expType == QVariant::Map) {
            QDBusArgument arg = value.value<QDBusArgument>();
            // Bug 374531: MapType fits all kinds of maps but we crash when we try to stream the arg into a
            // QVariantMap below but get a wrong signature, e.g. a{ss} instead of the expected a{sv}
            if (arg.currentType() != QDBusArgument::MapType || arg.currentSignature() != QLatin1String("a{sv}")) {
                qCWarning(MPRIS2) << m_dbusAddress << "exports" << propName
                    << "with the wrong type; it should be D-Bus type \"a{sv}\" instead of " << arg.currentSignature();
                return;
            }
            QVariantMap map;
            arg >> map;
            if (propName == QLatin1String("Metadata")) {
                if (!decodeUri(map, QLatin1String("mpris:artUrl"))) {
                    qCWarning(MPRIS2) << m_dbusAddress << "has an invalid URL for the mpris:artUrl entry of the \"Metadata\" property";
                }
                if (!decodeUri(map, QLatin1String("xesam:url"))) {
                    qCWarning(MPRIS2) << m_dbusAddress << "has an invalid URL for the xesam:url entry of the \"Metadata\" property";
                }
            }
            value = QVariant(map);
        }
    }
    if (value.type() != expType) {
        const char * gotTypeCh = QDBusMetaType::typeToSignature(value.userType());
        QString gotType = gotTypeCh ? QString::fromLatin1(gotTypeCh) : QStringLiteral("<unknown>");
        const char * expTypeCh = QDBusMetaType::typeToSignature(expType);
        QString expType = expTypeCh ? QString::fromLatin1(expTypeCh) : QStringLiteral("<unknown>");

        qCWarning(MPRIS2) << m_dbusAddress << "exports" << propName
            << "as D-Bus type" << gotType
            << "but it should be D-Bus type" << expType;
    }
    if (value.convert(expType)) {
        if (propName == QLatin1String("Position")) {

            setData(POS_UPD_STRING, QDateTime::currentDateTimeUtc());

        } else if (propName == QLatin1String("Metadata")) {

            if (updType == UpdatedSignal) {
                const QString oldTrackId = data().value(QStringLiteral("Metadata")).toMap().value(QStringLiteral("mpris:trackid")).toString();
                const QString newTrackId = value.toMap().value(QStringLiteral("mpris:trackid")).toString();
                if (oldTrackId != newTrackId) {
                    setData(QStringLiteral("Position"), static_cast<qlonglong>(0));
                    setData(POS_UPD_STRING, QDateTime::currentDateTimeUtc());
                }
            }

            if (value.toMap().value(QStringLiteral("mpris:length")).toLongLong() <= 0) {
                QMap<QString, QVariant> metadataMap = value.toMap();
                metadataMap.remove(QStringLiteral("mpris:length"));
                value = QVariant(metadataMap);
            }

        } else if (propName == QLatin1String("Rate") &&
                data().value(QStringLiteral("PlaybackStatus")).toString() == QLatin1String("Playing")) {

            if (data().contains(QLatin1String("Position")))
                recalculatePosition();
            m_currentRate = value.toDouble();

        } else if (propName == QLatin1String("PlaybackStatus")) {

            if (data().contains(QLatin1String("Position")) && data().contains(QLatin1String("PlaybackStatus"))) {
                updatePosition();
            }

            // update the effective rate
            if (data().contains(QLatin1String("Rate"))) {
                if (value.toString() == QLatin1String("Playing"))
                    m_currentRate = data().value(QStringLiteral("Rate")).toDouble();
                else
                    m_currentRate = 0.0;
            }
            if (value.toString() == QLatin1String("Stopped")) {
                // assume the position has reset to 0, since this is really the
                // only sensible value for a stopped track
                setData(QStringLiteral("Position"), static_cast<qint64>(0));
                setData(POS_UPD_STRING, QDateTime::currentDateTimeUtc());
            }
        } else if (propName == QLatin1String("DesktopEntry")) {
            QString filename = value.toString() + QLatin1String(".desktop");
            KDesktopFile desktopFile(filename);
            QString iconName = desktopFile.readIcon();
            if (!iconName.isEmpty()) {
                setData(QStringLiteral("Desktop Icon Name"), iconName);
            }
        }
        setData(propName, value);
    }
}

void PlayerContainer::updateFromMap(const QVariantMap& map, UpdateType updType)
{
    QMap<QString, QVariant>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
        QVariant::Type type = expPropType(i.key());
        if (type != QVariant::Invalid) {
            copyProperty(i.key(), i.value(), type, updType);
        }

        Cap cap = capFromName(i.key());
        if (cap != NoCaps) {
            if (i.value().type() == QVariant::Bool) {
                if (i.value().toBool()) {
                    m_caps |= cap;
                } else {
                    m_caps &= ~cap;
                }
            } else {
                const char * gotTypeCh = QDBusMetaType::typeToSignature(i.value().userType());
                QString gotType = gotTypeCh ? QString::fromLatin1(gotTypeCh) : QStringLiteral("<unknown>");

                qCWarning(MPRIS2) << m_dbusAddress << "exports" << i.key()
                    << "as D-Bus type" << gotType
                    << "but it should be D-Bus type \"b\"";
            }
        }
        // fake the CanStop capability
        if (cap == CanControl || i.key() == QLatin1String("PlaybackStatus")) {
            if ((m_caps & CanControl) && i.value().toString() != QLatin1String("Stopped")) {
                qCDebug(MPRIS2) << "Enabling stop action";
                m_caps |= CanStop;
            } else {
                qCDebug(MPRIS2) << "Disabling stop action";
                m_caps &= ~CanStop;
            }
        }
        ++i;
    }
}

void PlayerContainer::getPropsFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariantMap> propsReply = *watcher;
    watcher->deleteLater();

    if (m_fetchesPending < 1) {
        // we already failed
        return;
    }

    if (propsReply.isError()) {
        qCWarning(MPRIS2) << m_dbusAddress << "does not implement"
            << OrgFreedesktopDBusPropertiesInterface::staticInterfaceName()
            << "correctly" << "Error message was" << propsReply.error().name() << propsReply.error().message();
        m_fetchesPending = 0;
        emit initialFetchFailed(this);
        return;
    }

    updateFromMap(propsReply.value(), FetchAll);
    checkForUpdate();

    --m_fetchesPending;
    if (m_fetchesPending == 0) {
        emit initialFetchFinished(this);
    }
}

void PlayerContainer::updatePosition()
{
    QDBusPendingCall async = m_propsIface->Get(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(), QStringLiteral("Position"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this,    &PlayerContainer::getPositionFinished);
}

void PlayerContainer::getPositionFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QVariant> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(MPRIS2) << m_dbusAddress << "does not implement"
            << OrgFreedesktopDBusPropertiesInterface::staticInterfaceName()
            << "correctly";
        qCDebug(MPRIS2) << "Error message was" << propsReply.error().name() << propsReply.error().message();
        return;
    }

    setData(QStringLiteral("Position"), propsReply.value().toLongLong());
    setData(POS_UPD_STRING, QDateTime::currentDateTimeUtc());
    checkForUpdate();
}

void PlayerContainer::propertiesChanged(
        const QString& interface,
        const QVariantMap& changedProperties,
        const QStringList& invalidatedProperties)
{
    Q_UNUSED(interface)

    updateFromMap(changedProperties, UpdatedSignal);
    if (!invalidatedProperties.isEmpty()) {
        refresh();
    }
    checkForUpdate();
}

void PlayerContainer::seeked(qlonglong position)
{
    setData(QStringLiteral("Position"), position);
    setData(POS_UPD_STRING, QDateTime::currentDateTimeUtc());
    checkForUpdate();
}

void PlayerContainer::recalculatePosition()
{
    Q_ASSERT(data().contains("Position"));

    qint64 pos = data().value(QStringLiteral("Position")).toLongLong();
    QDateTime lastUpdated = data().value(POS_UPD_STRING).toDateTime();
    QDateTime now = QDateTime::currentDateTimeUtc();
    qint64 diff = lastUpdated.msecsTo(now) * 1000;
    qint64 newPos = pos + static_cast<qint64>(diff * m_currentRate);
    setData(QStringLiteral("Position"), newPos);
    setData(POS_UPD_STRING, now);
}

