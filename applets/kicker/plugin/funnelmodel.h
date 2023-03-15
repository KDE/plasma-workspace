/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "forwardingmodel.h"

class FunnelModel : public ForwardingModel
{
    Q_OBJECT
    Q_PROPERTY(bool sorted READ sorted NOTIFY sourceModelChanged)

public:
    explicit FunnelModel(QObject *parent = nullptr);
    ~FunnelModel() override;

    bool sorted() const;

    void setSourceModel(QAbstractItemModel *model) override;
};
