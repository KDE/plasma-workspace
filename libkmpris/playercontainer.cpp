/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "playercontainer.h"

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QVariantMap>

#include <KDesktopFile>

#include "dbusproperties.h"
#include "libkmpris_debug.h"
#include "mprisplayer.h"
#include "mprisroot.h"

AbstractPlayerContainer::AbstractPlayerContainer(QObject *parent)
    : QObject(parent)
{
}

AbstractPlayerContainer::~AbstractPlayerContainer()
{
}
bool AbstractPlayerContainer::canControl() const
{
    return m_canControl.value();
}

bool AbstractPlayerContainer::canGoNext() const
{
    return m_effectiveCanGoNext.value();
}

bool AbstractPlayerContainer::canGoPrevious() const
{
    return m_effectiveCanGoPrevious.value();
}

bool AbstractPlayerContainer::canPause() const
{
    return m_effectiveCanPause.value();
}

bool AbstractPlayerContainer::canPlay() const
{
    return m_effectiveCanPlay.value();
}

bool AbstractPlayerContainer::canStop() const
{
    return m_effectiveCanStop.value();
}

bool AbstractPlayerContainer::canSeek() const
{
    return m_effectiveCanSeek.value();
}

LoopStatus::Status AbstractPlayerContainer::loopStatus() const
{
    return m_loopStatus.value();
}

double AbstractPlayerContainer::maximumRate() const
{
    return m_maximumRate.value();
}

double AbstractPlayerContainer::minimumRate() const
{
    return m_minimumRate.value();
}

PlaybackStatus::Status AbstractPlayerContainer::playbackStatus() const
{
    return m_playbackStatus.value();
}

qlonglong AbstractPlayerContainer::position() const
{
    return m_position.value();
}

double AbstractPlayerContainer::rate() const
{
    return m_rate.value();
}

ShuffleStatus::Status AbstractPlayerContainer::shuffle() const
{
    return m_shuffle.value();
}

double AbstractPlayerContainer::volume() const
{
    return m_volume.value();
}

QString AbstractPlayerContainer::track() const
{
    return m_track.value();
}

QString AbstractPlayerContainer::artist() const
{
    return m_artist.value();
}

QString AbstractPlayerContainer::artUrl() const
{
    return m_artUrl.value();
}

QString AbstractPlayerContainer::album() const
{
    return m_album.value();
}

double AbstractPlayerContainer::length() const
{
    return m_length;
}

unsigned AbstractPlayerContainer::instancePid() const
{
    return m_instancePid;
}

unsigned AbstractPlayerContainer::kdePid() const
{
    return m_kdePid.value();
}

bool AbstractPlayerContainer::canQuit() const
{
    return m_canQuit.value();
}

bool AbstractPlayerContainer::canRaise() const
{
    return m_canRaise.value();
}

bool AbstractPlayerContainer::canSetFullscreen() const
{
    return m_canSetFullscreen.value();
}

QString AbstractPlayerContainer::desktopEntry() const
{
    return m_desktopEntry;
}

bool AbstractPlayerContainer::fullscreen() const
{
    return m_fullscreen.value();
}

PlayerContainer::PlayerContainer(const QString &busAddress, QObject *parent)
    : AbstractPlayerContainer(parent)
    , m_dbusAddress(busAddress)
    , m_propsIface(new OrgFreedesktopDBusPropertiesInterface(busAddress, MPRIS2_PATH, QDBusConnection::sessionBus(), this))
    , m_playerIface(new OrgMprisMediaPlayer2PlayerInterface(busAddress, MPRIS2_PATH, QDBusConnection::sessionBus(), this))
    , m_rootIface(new OrgMprisMediaPlayer2Interface(busAddress, MPRIS2_PATH, QDBusConnection::sessionBus(), this))
{
    Q_ASSERT(busAddress.startsWith(MPRIS2_PREFIX));

    // MPRIS specifies, that in case a player supports several instances, each additional
    // instance after the first one is supposed to append ".instance<pid>" at the end of
    // its dbus address. So instances of media players, which implement this correctly
    // can have one D-Bus connection per instance and can be identified by their pids.
    if (QDBusReply<unsigned> pidReply = QDBusConnection::sessionBus().interface()->servicePid(busAddress); pidReply.isValid()) {
        m_instancePid = pidReply.value();
    }

    initBindings();

    connect(m_propsIface, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &PlayerContainer::onPropertiesChanged);
    connect(m_playerIface, &OrgMprisMediaPlayer2PlayerInterface::Seeked, this, &PlayerContainer::onSeeked);

    refresh();
}

PlayerContainer::~PlayerContainer()
{
}

void PlayerContainer::setLoopStatus(LoopStatus::Status value)
{
    if (m_loopStatus == value) {
        return;
    }

    QVariant result;
    switch (value) {
    case LoopStatus::None:
        result = QStringLiteral("None");
        break;
    case LoopStatus::Playlist:
        result = QStringLiteral("Playlist");
        break;
    case LoopStatus::Track:
        result = QStringLiteral("Track");
        break;
    default:
        Q_UNREACHABLE();
    }
    m_propsIface->Set(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(), QStringLiteral("LoopStatus"), QDBusVariant(result));
    // Emit changed signals in onPropertiesChanged
}

void PlayerContainer::setPosition(qlonglong value)
{
    if (m_position == value) {
        return;
    }

    m_playerIface->SetPosition(QDBusObjectPath(m_trackId.value()), value);
}

void PlayerContainer::setRate(double value)
{
    if (m_rate == value) {
        return;
    }

    m_propsIface->Set(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(), QStringLiteral("Rate"), QDBusVariant(QVariant(value)));
    // Emit changed signals in onPropertiesChanged
}

void PlayerContainer::setShuffle(ShuffleStatus::Status value)
{
    if (m_shuffle == value) {
        return;
    }

    m_propsIface->Set(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(),
                      QStringLiteral("Shuffle"),
                      QDBusVariant(QVariant(value == ShuffleStatus::On)));
    // Emit changed signals in onPropertiesChanged
}

void PlayerContainer::setVolume(double value)
{
    if (m_volume == value) {
        return;
    }

    m_propsIface->Set(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(), QStringLiteral("Volume"), QDBusVariant(QVariant(value)));
    // Emit changed signals in onPropertiesChanged
}

void PlayerContainer::Next()
{
    Q_ASSERT(m_canGoNext.value());
    if (!m_canGoNext.value()) {
        return;
    }
    m_playerIface->Next();
    updatePosition();
}

void PlayerContainer::OpenUri(const QString &Uri)
{
    m_playerIface->OpenUri(Uri);
}

void PlayerContainer::Pause()
{
    Q_ASSERT(m_canPause.value());
    if (!m_canPause.value()) {
        return;
    }
    m_playerIface->Pause();
}

void PlayerContainer::Play()
{
    Q_ASSERT(m_canPlay.value());
    if (!m_canPlay.value()) {
        return;
    }
    m_playerIface->Play();
}

void PlayerContainer::PlayPause()
{
    Q_ASSERT(m_canPlay.value());
    Q_ASSERT(m_canPause.value());
    if (!m_canPlay.value() || !m_canPause.value()) {
        return;
    }
    m_playerIface->PlayPause();
}

void PlayerContainer::Previous()
{
    Q_ASSERT(m_canGoPrevious.value());
    if (!m_canGoPrevious.value()) {
        return;
    }
    m_playerIface->Previous();
    updatePosition();
}

void PlayerContainer::Seek(qlonglong Offset)
{
    Q_ASSERT(m_canSeek.value());
    if (!m_canSeek.value()) {
        return;
    }
    m_playerIface->Seek(Offset);
}

void PlayerContainer::Stop()
{
    Q_ASSERT(m_canStop.value());
    if (!m_canStop.value()) {
        return;
    }
    m_playerIface->Stop();
}

void PlayerContainer::setFullscreen(bool value)
{
    if (m_fullscreen == value) {
        return;
    }

    m_propsIface->Set(OrgMprisMediaPlayer2Interface::staticInterfaceName(), QStringLiteral("Fullscreen"), QDBusVariant(QVariant(value)));
    // Emit changed signals in onPropertiesChanged
}

void PlayerContainer::setPlaybackStatus(PlaybackStatus::Status value)
{
    if (m_playbackStatus == value) {
        return;
    }

    switch (value) {
    case PlaybackStatus::Playing:
        Play();
        break;
    case PlaybackStatus::Paused:
        Pause();
        break;
    case PlaybackStatus::Stopped:
        Stop();
        break;
    default:
#ifdef __cpp_lib_unreachable
        std::unreachable();
#else
        Q_UNREACHABLE();
#endif
    }

    // Emit changed signals in onPropertiesChanged
}

bool AbstractPlayerContainer::hasTrackList() const
{
    return m_hasTrackList;
}

QString AbstractPlayerContainer::identity() const
{
    return m_identity;
}

QStringList AbstractPlayerContainer::supportedMimeTypes() const
{
    return m_supportedMimeTypes;
}

QStringList AbstractPlayerContainer::supportedUriSchemes() const
{
    return m_supportedUriSchemes;
}

void PlayerContainer::Quit()
{
    Q_ASSERT(m_canQuit.value());
    if (!m_canQuit.value()) {
        return;
    }
    m_rootIface->Quit();
}
void PlayerContainer::Raise()
{
    Q_ASSERT(m_canRaise.value());
    if (!m_canRaise.value()) {
        return;
    }
    m_rootIface->Raise();
}

QString AbstractPlayerContainer::iconName() const
{
    return m_iconName;
}

void PlayerContainer::refresh()
{
    // despite these calls being async, we should never update values in the
    // wrong order (eg: a stale GetAll response overwriting a more recent value
    // from a PropertiesChanged signal) due to D-Bus message ordering guarantees.

    QDBusPendingCall async = m_propsIface->GetAll(OrgMprisMediaPlayer2Interface::staticInterfaceName());
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PlayerContainer::onGetPropsFinished);
    ++m_fetchesPending;

    async = m_propsIface->GetAll(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName());
    watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &PlayerContainer::onGetPropsFinished);
    ++m_fetchesPending;
}

void PlayerContainer::updatePosition()
{
    QDBusPendingCall call = m_propsIface->Get(OrgMprisMediaPlayer2PlayerInterface::staticInterfaceName(), QStringLiteral("Position"));
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariant> propsReply = *watcher;
        watcher->deleteLater();
        if (!propsReply.isValid() && propsReply.error().type() != QDBusError::NotSupported) {
            qCWarning(MPRIS2) << m_dbusAddress << "does not implement" << OrgFreedesktopDBusPropertiesInterface::staticInterfaceName()
                              << "correctly. Error message was" << propsReply.error().name() << propsReply.error().message();
            return;
        }

        m_position = propsReply.value().toLongLong();
        Q_EMIT positionChanged();
    });
}

void PlayerContainer::changeVolume(double delta, bool showOSD)
{
    // Not relying on property/setProperty to avoid doing blocking DBus calls
    const double newVolume = qBound(0.0, m_volume + delta, std::max(m_volume.value(), 1.0));
    const double oldVolume = m_volume;

    // Update the container value right away so when calling this method in quick succession
    // (mouse wheeling the tray icon) next call already gets the new value
    m_volume = newVolume;

    QDBusPendingCall call = m_propsIface->Set(m_playerIface->interface(), QStringLiteral("Volume"), QDBusVariant(newVolume));
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, oldVolume, showOSD](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        watcher->deleteLater();
        if (!reply.isValid()) {
            m_volume = oldVolume;
            return;
        }

        if (showOSD) {
            QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                              QStringLiteral("/org/kde/osdService"),
                                                              QStringLiteral("org.kde.osdService"),
                                                              QStringLiteral("mediaPlayerVolumeChanged"));

            msg.setArguments({qRound(m_volume * 100), m_identity, m_iconName});

            QDBusConnection::sessionBus().asyncCall(msg);
        }
    });
}

void PlayerContainer::initBindings()
{
    // Since the bindings are already used in QML, move them to C++ for better efficiency and consistency
    m_effectiveCanGoNext.setBinding([this] {
        return m_canControl.value() && m_canGoNext.value();
    });
    m_effectiveCanGoPrevious.setBinding([this] {
        return m_canControl.value() && m_canGoPrevious.value();
    });
    m_effectiveCanPlay.setBinding([this] {
        return m_canControl.value() && m_canPlay.value();
    });
    m_effectiveCanPause.setBinding([this] {
        return m_canControl.value() && m_canPause.value();
    });
    m_effectiveCanStop.setBinding([this] {
        return m_canControl.value() && m_canStop.value();
    });
    m_effectiveCanSeek.setBinding([this] {
        return m_canControl.value() && m_canSeek.value();
    });

    // Fake canStop property
    m_canStop.setBinding([this] {
        return m_canControl.value() && m_playbackStatus.value() > PlaybackStatus::Stopped;
    });

    // Metadata
    m_track.setBinding([this] {
        if (!m_xesamTitle.value().isEmpty()) {
            return m_xesamTitle.value();
        }
        const QString xesamUrl = m_xesamUrl.value();
        if (xesamUrl.isEmpty()) {
            return QString();
        }
        if (int lastSlashPos = xesamUrl.lastIndexOf(QLatin1Char('/')); lastSlashPos < 0 || lastSlashPos == xesamUrl.size() - 1) {
            return QString();
        } else {
            const QString lastUrlPart = xesamUrl.sliced(lastSlashPos + 1);
            return QUrl::fromEncoded(lastUrlPart.toLatin1()).toString();
        }
    });
    m_artist.setBinding([this] {
        if (!m_xesamArtist.value().empty()) {
            return m_xesamArtist.value().join(QLatin1String(", "));
        }
        if (!m_xesamAlbumArtist.value().empty()) {
            return m_xesamAlbumArtist.value().join(QLatin1String(", "));
        }
        return QString();
    });
    m_album.setBinding([this] {
        if (!m_xesamAlbum.value().isEmpty()) {
            return m_xesamAlbum.value();
        }
        const QString xesamUrl = m_xesamUrl.value();
        if (!xesamUrl.startsWith(QLatin1String("file:///"))) {
            return QString();
        }
        const QStringList urlParts = xesamUrl.split(QLatin1Char('/'));
        if (urlParts.size() < 3) {
            return QString();
        }
        // if we play a local file without title and artist, show its containing folder instead
        const auto lastFolderPathIt = std::next(urlParts.crbegin());
        if (!lastFolderPathIt->isEmpty()) {
            return *lastFolderPathIt;
        }
        return QString();
    });

    m_notifiers.reserve(2);
    auto callback = [this] {
        updatePosition();
    };
    m_notifiers.emplace_back(m_rate.addNotifier(callback));
    m_notifiers.emplace_back(m_playbackStatus.addNotifier(callback));
}

void PlayerContainer::updateFromMap(const QVariantMap &map)
{
    auto updateSingleProperty = [this]<typename T>(T &property, const QVariant &value, QMetaType::Type expectedType, void (PlayerContainer::*signal)()) {
        if (value.metaType().id() != expectedType) {
            qCWarning(MPRIS2) << m_dbusAddress << "exports" << value.metaType() << "but it should be" << QMetaType(expectedType);
        }
        if (T newProperty = value.value<T>(); property != newProperty) {
            property = newProperty;
            Q_EMIT(this->*signal)();
        }
    };

    QString oldTrackId;

    for (auto it = map.cbegin(); it != map.cend(); it = std::next(it)) {
        const QString &propName = it.key();

        if (propName == QLatin1String("Identity")) {
            updateSingleProperty(m_identity, it.value(), QMetaType::QString, &PlayerContainer::identityChanged);
        } else if (propName == QLatin1String("DesktopEntry")) {
            m_iconName = KDesktopFile(it.value().toString() + QLatin1String(".desktop")).readIcon();
            if (m_iconName.isEmpty()) {
                m_iconName = QStringLiteral("emblem-music-symbolic");
            }
            updateSingleProperty(m_desktopEntry, it.value(), QMetaType::QString, &PlayerContainer::desktopEntryChanged);
        } else if (propName == QLatin1String("SupportedUriSchemes")) {
            updateSingleProperty(m_supportedUriSchemes, it.value(), QMetaType::QStringList, &PlayerContainer::supportedUriSchemesChanged);
        } else if (propName == QLatin1String("SupportedMimeTypes")) {
            updateSingleProperty(m_supportedMimeTypes, it.value(), QMetaType::QStringList, &PlayerContainer::supportedMimeTypesChanged);
        } else if (propName == QLatin1String("Fullscreen")) {
            m_fullscreen = it->toBool();
        } else if (propName == QLatin1String("HasTrackList")) {
            m_hasTrackList = it->toBool();
        } else if (propName == QLatin1String("PlaybackStatus")) {
            if (const QString newValue = it->toString(); newValue == QLatin1String("Stopped")) {
                m_playbackStatus = PlaybackStatus::Stopped;
            } else if (newValue == QLatin1String("Paused")) {
                m_playbackStatus = PlaybackStatus::Paused;
            } else if (newValue == QLatin1String("Playing")) {
                m_playbackStatus = PlaybackStatus::Playing;
            } else {
                m_playbackStatus = PlaybackStatus::Unknown;
            }
        } else if (propName == QLatin1String("LoopStatus")) {
            if (const QString newValue = it.value().toString(); newValue == QLatin1String("Playlist")) {
                m_loopStatus = LoopStatus::Playlist;
            } else if (newValue == QLatin1String("Track")) {
                m_loopStatus = LoopStatus::Track;
            } else {
                m_loopStatus = LoopStatus::None;
            }
        } else if (propName == QLatin1String("Shuffle")) {
            m_shuffle = it->toBool() ? ShuffleStatus::On : ShuffleStatus::Off;
        } else if (propName == QLatin1String("Rate")) {
            m_rate = it->toDouble();
        } else if (propName == QLatin1String("MinimumRate")) {
            m_minimumRate = it->toDouble();
        } else if (propName == QLatin1String("MaximumRate")) {
            m_maximumRate = it->toDouble();
        } else if (propName == QLatin1String("Volume")) {
            m_volume = it->toDouble();
        } else if (propName == QLatin1String("Position")) {
            m_position = it->toLongLong();
        } else if (propName == QLatin1String("Metadata")) {
            oldTrackId = m_trackId.value();
            QDBusArgument arg = it->value<QDBusArgument>();
            if (arg.currentType() != QDBusArgument::MapType || arg.currentSignature() != QLatin1String("a{sv}")) {
                continue;
            }

            QVariantMap map;
            arg >> map;

            m_trackId = map.value(QStringLiteral("mpris:trackid")).value<QDBusObjectPath>().path();
            m_xesamTitle = map.value(QStringLiteral("xesam:title")).toString();
            m_xesamUrl = map.value(QStringLiteral("xesam:url")).toString();
            m_xesamArtist = map.value(QStringLiteral("xesam:artist")).toStringList();
            m_xesamAlbumArtist = map.value(QStringLiteral("xesam:albumArtist")).toStringList();
            m_xesamAlbum = map.value(QStringLiteral("xesam:album")).toString();
            m_artUrl = map.value(QStringLiteral("mpris:artUrl")).toString();
            m_length = map.value(QStringLiteral("mpris:length")).toDouble();
            m_kdePid = map.value(QStringLiteral("kde:pid")).toUInt();
        }
        // we give out CanControl, as this may completely
        // change the UI of the widget
        else if (propName == QLatin1String("CanControl")) {
            m_canControl = it->toBool();
        } else if (propName == QLatin1String("CanSeek")) {
            m_canSeek = it->toBool();
        } else if (propName == QLatin1String("CanGoNext")) {
            m_canGoNext = it->toBool();
        } else if (propName == QLatin1String("CanGoPrevious")) {
            m_canGoPrevious = it->toBool();
        } else if (propName == QLatin1String("CanRaise")) {
            m_canRaise = it->toBool();
        } else if (propName == QLatin1String("CanSetFullscreen")) {
            m_canSetFullscreen = it->toBool();
        } else if (propName == QLatin1String("CanQuit")) {
            m_canQuit = it->toBool();
        } else if (propName == QLatin1String("CanPlay")) {
            m_canPlay = it->toBool();
        } else if (propName == QLatin1String("CanPause")) {
            m_canPause = it->toBool();
        }
    }

    if (map.contains(QStringLiteral("Position"))) {
        return;
    }

    if (m_position != 0.0 && (m_playbackStatus == PlaybackStatus::Stopped || (!oldTrackId.isEmpty() && m_trackId.value() != oldTrackId))) {
        // assume the position has reset to 0, since this is really the
        // only sensible value for a stopped track
        updatePosition();
    }
}

void PlayerContainer::onGetPropsFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> propsReply = *watcher;
    watcher->deleteLater();

    if (m_fetchesPending < 1) {
        // we already failed
        Q_EMIT initialFetchFailed(this);
        return;
    }

    if (propsReply.isError()) {
        qCWarning(MPRIS2) << m_dbusAddress << "does not implement" << OrgFreedesktopDBusPropertiesInterface::staticInterfaceName() << "correctly"
                          << "Error message was" << propsReply.error().name() << propsReply.error().message();
        m_fetchesPending = 0;
        Q_EMIT initialFetchFailed(this);
        return;
    }

    updateFromMap(propsReply.value());

    if (--m_fetchesPending == 0) {
        Q_EMIT initialFetchFinished(this);
    }
}

void PlayerContainer::onPropertiesChanged(const QString &, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (!invalidatedProperties.empty()) {
        refresh();
    } else {
        updateFromMap(changedProperties);
    }
}

void PlayerContainer::onSeeked(qlonglong position)
{
    m_position = position;
}

#include "moc_playercontainer.cpp"
