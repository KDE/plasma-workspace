/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QSortFilterProxyModel>
#include <qqmlregistration.h>

class HistoryModel;

/**
 * This class provides a view for history clip items in QML
 **/
class DeclarativeHistoryModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HistoryModel)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)

    // Whether to show pinned items only
    Q_PROPERTY(bool pinnedOnly READ pinnedOnly WRITE setPinnedOnly NOTIFY pinnedOnlyChanged)

    // Whether to prioritize pinned items over other items except for the first item (current clipboard)
    Q_PROPERTY(bool pinnedPrioritized READ pinnedPrioritized WRITE setPinnedPrioritized NOTIFY pinnedPrioritizedChanged)

public:
    explicit DeclarativeHistoryModel(QObject *parent = nullptr);
    ~DeclarativeHistoryModel() override;

    QString currentText() const;

    bool pinnedOnly() const;
    void setPinnedOnly(bool value);

    bool pinnedPrioritized() const;
    void setPinnedPrioritized(bool value);

    Q_INVOKABLE void moveToTop(const QString &uuid);

    Q_INVOKABLE void remove(const QString &uuid);
    Q_INVOKABLE void clearHistory();

    Q_INVOKABLE void invokeAction(const QString &uuid);

Q_SIGNALS:
    void countChanged();
    void currentTextChanged();
    void pinnedOnlyChanged();
    void pinnedPrioritizedChanged();

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

    std::shared_ptr<HistoryModel> m_model;

    bool m_pinnedOnly = false;
    bool m_pinnedPrioritized = false;
};
