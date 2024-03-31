/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QIdentityProxyModel>
#include <qqmlregistration.h>

class HistoryModel;

class ProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HistoryModel)

public:
    ProxyModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

private:
    std::shared_ptr<HistoryModel> *m_model;
};
