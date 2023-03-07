/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

#include <Plasma5Support/Service>

#include "playercontrol.h"
#include <QPointer>
#include <QWeakPointer>

class Multiplexer;
class PlayerControl;
class KActionCollection;

class MultiplexedService : public Plasma5Support::Service
{
    Q_OBJECT

public:
    explicit MultiplexedService(Multiplexer *multiplexer, QObject *parent = nullptr);

protected:
    Plasma5Support::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

public Q_SLOTS:
    void enableGlobalShortcuts();

private Q_SLOTS:
    void updateEnabledOperations();
    void activePlayerChanged(PlayerContainer *container);

private:
    QPointer<PlayerControl> m_control;
    KActionCollection *m_actionCollection = nullptr;
};
