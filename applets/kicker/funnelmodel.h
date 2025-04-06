/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "forwardingmodel.h"

#include <qqmlregistration.h>

class FunnelModel : public ForwardingModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit FunnelModel(QObject *parent = nullptr);
    ~FunnelModel() override;

    void setSourceModel(QAbstractItemModel *model) override;
};
