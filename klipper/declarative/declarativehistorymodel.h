/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QIdentityProxyModel>
#include <qqmlregistration.h>

class HistoryModel;

/**
 * This class provides a view for history clip items in QML
 **/
class DeclarativeHistoryModel : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HistoryModel)

public:
    explicit DeclarativeHistoryModel(QObject *parent = nullptr);
    ~DeclarativeHistoryModel() override;

private:
    std::shared_ptr<HistoryModel> m_model;
};
