/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBindable>

#include <KDEDModule>

class PlayerContainer;
class Multiplexer;
class KActionCollection;

/**
 * This service allows to control media players when there is no media controller widget
 */
class MprisService : public KDEDModule
{
    Q_OBJECT

public:
    explicit MprisService(QObject *parent, const QList<QVariant> &);
    ~MprisService() override;

private Q_SLOTS:
    void onPlayPause();
    void onNext();
    void onPrevious();
    void onStop();
    void onPause();
    void onPlay();
    void onVolumeUp();
    void onVolumeDown();

private:
    void enableGlobalShortcuts();

    std::shared_ptr<Multiplexer> m_multiplexer;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MprisService, PlayerContainer *, m_activePlayer, nullptr)
    KActionCollection *m_actionCollection = nullptr;
};
