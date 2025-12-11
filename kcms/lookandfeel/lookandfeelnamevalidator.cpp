/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "lookandfeelnamevalidator.h"
#include "kcm.h"

LookAndFeelNameValidator::LookAndFeelNameValidator(QObject *parent)
    : QValidator(parent)
{
}

QAbstractItemModel *LookAndFeelNameValidator::model() const
{
    return m_model;
}

void LookAndFeelNameValidator::setModel(QAbstractItemModel *model)
{
    if (m_model != model) {
        m_model = model;
        Q_EMIT modelChanged();
    }
}

LookAndFeelNameValidator::State LookAndFeelNameValidator::validate(QString &input, int &position) const
{
    Q_UNUSED(position)

    if (!m_model) {
        return State::Acceptable;
    }

    const int packageCount = m_model->rowCount();
    for (int i = 0; i < packageCount; ++i) {
        const QModelIndex index = m_model->index(i, 0);

        const QString name = index.data(Qt::DisplayRole).toString();
        if (input == name) {
            return State::Intermediate;
        }

        const QString id = index.data(KCMLookandFeel::PluginNameRole).toString();
        if (input == id) {
            return State::Intermediate;
        }
    }

    return State::Acceptable;
}

#include "moc_lookandfeelnamevalidator.cpp"
