/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QBindable>
#include <QClipboard>
#include <QDateTime>
#include <QSqlDatabase>

#include "klipper_export.h"

class KCoreConfigSkeleton;
class HistoryItem;
class SystemClipboard;
class UpdateDatabaseJob;

class KLIPPER_EXPORT HistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum RoleType {
        HistoryItemConstPtrRole = Qt::UserRole,
        UuidRole,
        TypeRole,
        TypeIntRole,
        ImageUrlRole,
        ImageSizeRole,
    };
    Q_ENUM(RoleType)

    [[nodiscard]] static std::shared_ptr<HistoryModel> self();
    ~HistoryModel() override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Remove (first) history item equal to item from history
     */
    bool remove(const QString &uuid);

    qsizetype maxSize() const;
    void setMaxSize(qsizetype size);

    /**
     * Clear history
     */
    void clear();
    void clearHistory();

    /**
     * Move the history in position pos to top
     */
    void moveToTop(const QString &uuid);
    void moveTopToBack();
    void moveBackToTop();

    int indexOf(const QString &uuid) const;
    int indexOf(const HistoryItem *item) const;

    /**
     * Traversal: Get first item
     */
    std::shared_ptr<const HistoryItem> first() const;

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    bool insert(const QMimeData *mimeData, qreal timestamp = 0);
    bool insert(const QString &text);

    /**
     * @short Loads history from disk.
     * Inserts items into clipboard without any checks
     * Used when restoring a saved history and internally.
     * Don't use this unless you're reasonable the list
     * should be reset.
     */
    bool loadHistory();
    bool saveClipboardHistory();

    void loadSettings(bool firstTime = false);

    int pendingJobs() const;

Q_SIGNALS:
    void changed(bool isTop = false);

    void actionInvoked(const std::shared_ptr<const HistoryItem> &item);

private Q_SLOTS:
    /**
     * Check data in clipboard, and if it passes these checks,
     * store the data in the clipboard history.
     */
    void checkClipData(QClipboard::Mode mode, const QMimeData *data);

    void slotIgnored(QClipboard::Mode mode);
    void slotReceivedEmptyClipboard(QClipboard::Mode mode);

private:
    explicit HistoryModel();

    static KCoreConfigSkeleton *settings();

    void moveToTop(qsizetype row);

    static void saveToFile(QStringView dbFolder, const QByteArray &data, QStringView newUuid, QStringView dataUuid);

    std::shared_ptr<SystemClipboard> m_clip;
    QList<std::shared_ptr<HistoryItem>> m_items;
    int m_pendingJobs = 0;
    QString m_dbFolder;
    QSqlDatabase m_db;
    qsizetype m_maxSize = 0;
    bool m_displayImages = false;
    bool m_bNoNullClipboard = true;
    bool m_bIgnoreSelection = true;
    bool m_bKeepContents = true;
    bool m_bSynchronize = false;
    bool m_bSelectionTextOnly = true;

    friend class DeclarativeHistoryModel;
    friend class HistoryModelTest;
};
