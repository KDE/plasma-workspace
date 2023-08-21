/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mprisservice.h"

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KPluginFactory>

#include "multiplexer.h"
#include "playercontainer.h"

K_PLUGIN_CLASS_WITH_JSON(MprisService, "mprisservice.json")

MprisService::MprisService(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_multiplexer(Multiplexer::self())
    , m_actionCollection(new KActionCollection(this, QStringLiteral("mediacontrol")))
{
    m_activePlayer.setBinding([this] {
        return m_multiplexer->activePlayer().value();
    });

    enableGlobalShortcuts();
}

MprisService::~MprisService()
{
}

void MprisService::onPlayPause()
{
    if (!m_activePlayer) {
        return;
    }

    const auto playbackStatus = m_activePlayer->playbackStatus();
    if (playbackStatus == PlaybackStatus::Playing) {
        if (m_activePlayer->canPause()) {
            m_activePlayer->Pause();
        }
    } else {
        if (m_activePlayer->canPlay()) {
            m_activePlayer->Play();
        }
    }
}

void MprisService::onNext()
{
    if (m_activePlayer && m_activePlayer->canGoNext()) {
        m_activePlayer->Next();
    }
}

void MprisService::onPrevious()
{
    if (m_activePlayer && m_activePlayer->canGoPrevious()) {
        m_activePlayer->Previous();
    }
}

void MprisService::onStop()
{
    if (m_activePlayer && m_activePlayer->canStop()) {
        m_activePlayer->Stop();
    }
}

void MprisService::onPause()
{
    if (m_activePlayer && m_activePlayer->canPause()) {
        m_activePlayer->Pause();
    }
}

void MprisService::onPlay()
{
    if (m_activePlayer && m_activePlayer->canPlay()) {
        m_activePlayer->Play();
    }
}

void MprisService::onVolumeUp()
{
    if (m_activePlayer && m_activePlayer->canControl()) {
        m_activePlayer->changeVolume(0.05, true);
    }
}

void MprisService::onVolumeDown()
{
    if (m_activePlayer && m_activePlayer->canControl()) {
        m_activePlayer->changeVolume(-0.05, true);
    }
}

void MprisService::enableGlobalShortcuts()
{
    m_actionCollection->setComponentDisplayName(i18nc("@title Name for global shortcuts category", "Media Controller"));

    auto addGlobalShortcut = [this](const QString &name, const QString &text, const QKeySequence &keySequence, void (MprisService::*callback)()) {
        QAction *action = m_actionCollection->addAction(name);
        action->setText(text);
        KGlobalAccel::setGlobalShortcut(action, keySequence);
        connect(action, &QAction::triggered, this, callback);
    };

    addGlobalShortcut(QStringLiteral("playpausemedia"), i18nc("@title shortcut", "Play/Pause media playback"), Qt::Key_MediaPlay, &MprisService::onPlayPause);
    addGlobalShortcut(QStringLiteral("nextmedia"), i18nc("@title shortcut", "Media playback next"), Qt::Key_MediaNext, &MprisService::onNext);
    addGlobalShortcut(QStringLiteral("previousmedia"), i18nc("@title shortcut", "Media playback previous"), Qt::Key_MediaPrevious, &MprisService::onPrevious);
    addGlobalShortcut(QStringLiteral("stopmedia"), i18nc("@title shortcut", "Stop media playback"), Qt::Key_MediaStop, &MprisService::onStop);
    addGlobalShortcut(QStringLiteral("pausemedia"), i18nc("@title shortcut", "Pause media playback"), Qt::Key_MediaPause, &MprisService::onPause);
    addGlobalShortcut(QStringLiteral("playmedia"), i18nc("@title shortcut", "Play media playback"), QKeySequence(), &MprisService::onPlay);
    addGlobalShortcut(QStringLiteral("mediavolumeup"), i18nc("@title shortcut", "Media volume up"), QKeySequence(), &MprisService::onVolumeUp);
    addGlobalShortcut(QStringLiteral("mediavolumedown"), i18nc("@title shortcut", "Media volume down"), QKeySequence(), &MprisService::onVolumeDown);
}

#include "moc_mprisservice.cpp"
#include "mprisservice.moc"
