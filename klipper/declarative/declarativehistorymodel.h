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

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)

public:
    explicit DeclarativeHistoryModel(QObject *parent = nullptr);
    ~DeclarativeHistoryModel() override;

    QString currentText() const;

    Q_INVOKABLE void moveToTop(const QString &uuid);

    Q_INVOKABLE void remove(const QString &uuid);
    Q_INVOKABLE void clearHistory();

    Q_INVOKABLE void invokeAction(const QString &uuid);

Q_SIGNALS:
    void countChanged();
    void currentTextChanged();

private:
    std::shared_ptr<HistoryModel> m_model;
};
