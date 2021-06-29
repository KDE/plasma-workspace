/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

#include <Plasma/Service>

#include "playercontrol.h"
#include <QWeakPointer>

class Multiplexer;
class PlayerControl;
class KActionCollection;

class MultiplexedService : public Plasma::Service
{
    Q_OBJECT

public:
    explicit MultiplexedService(Multiplexer *multiplexer, QObject *parent = nullptr);

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

public Q_SLOTS:
    void enableGlobalShortcuts();

private Q_SLOTS:
    void updateEnabledOperations();
    void activePlayerChanged(PlayerContainer *container);

private:
    QPointer<PlayerControl> m_control;
    KActionCollection *m_actionCollection = nullptr;
};
