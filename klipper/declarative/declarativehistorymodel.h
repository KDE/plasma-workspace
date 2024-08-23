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

    /**
     * When enabled, temporarily stops adding items to the history
     */
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
    explicit DeclarativeHistoryModel(QObject *parent = nullptr);
    ~DeclarativeHistoryModel() override;

    QString currentText() const;

    bool paused() const;
    void setPaused(bool status);

    Q_INVOKABLE void moveToTop(const QByteArray &uuid);

    Q_INVOKABLE void remove(const QByteArray &uuid);
    Q_INVOKABLE void clearHistory();

    Q_INVOKABLE void invokeAction(const QByteArray &uuid);

Q_SIGNALS:
    void countChanged();
    void currentTextChanged();
    void pausedChanged();

private:
    std::shared_ptr<HistoryModel> m_model;
};
