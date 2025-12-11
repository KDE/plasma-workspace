/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QAbstractItemModel>
#include <QPointer>
#include <QValidator>

class LookAndFeelNameValidator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit LookAndFeelNameValidator(QObject *parent = nullptr);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    State validate(QString &input, int &position) const override;

Q_SIGNALS:
    void modelChanged();

private:
    QPointer<QAbstractItemModel> m_model;
};
