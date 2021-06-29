/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FUNNELMODEL_H
#define FUNNELMODEL_H

#include "forwardingmodel.h"

class FunnelModel : public ForwardingModel
{
    Q_OBJECT

public:
    explicit FunnelModel(QObject *parent = nullptr);
    ~FunnelModel() override;

    void setSourceModel(QAbstractItemModel *model) override;
};

#endif
