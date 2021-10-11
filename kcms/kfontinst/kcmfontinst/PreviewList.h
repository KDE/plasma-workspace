#pragma once

/*
 * SPDX-FileCopyrightText: 2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QAbstractItemModel>
#include <QTreeView>

class QContextMenuEvent;

namespace KFI
{
class CFcEngine;

class CPreviewListItem
{
public:
    CPreviewListItem(const QString &name, quint32 style, const QString &file, int index)
        : m_name(name)
        , m_file(file)
        , m_style(style)
        , m_index(index)
    {
    }

    const QString &name() const
    {
        return m_name;
    }
    quint32 style() const
    {
        return m_style;
    }
    const QString &file() const
    {
        return m_file;
    }
    int index() const
    {
        return m_index;
    }

private:
    QString m_name, m_file;
    quint32 m_style;
    int m_index;
};

class CPreviewList : public QAbstractItemModel
{
    Q_OBJECT

public:
    CPreviewList(QObject *parent = nullptr);
    ~CPreviewList() override
    {
        clear();
    }

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_items.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 1;
    }
    void clear();
    void showFonts(const QModelIndexList &font);

private:
    QList<CPreviewListItem *> m_items;
};

class CPreviewListView : public QTreeView
{
    Q_OBJECT

public:
    CPreviewListView(CFcEngine *eng, QWidget *parent);
    ~CPreviewListView() override
    {
    }

    void refreshPreviews();
    void showFonts(const QModelIndexList &fonts);
    void contextMenuEvent(QContextMenuEvent *ev) override;

Q_SIGNALS:

    void showMenu(const QPoint &pos);

private:
    CPreviewList *m_model;
};

}
