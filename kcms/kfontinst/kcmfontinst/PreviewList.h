#ifndef __PREVIEW_LIST_H__
#define __PREVIEW_LIST_H__

/*
 * KFontInst - KDE Font Installer
 *
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
        : itsName(name)
        , itsFile(file)
        , itsStyle(style)
        , itsIndex(index)
    {
    }

    const QString &name() const
    {
        return itsName;
    }
    quint32 style() const
    {
        return itsStyle;
    }
    const QString &file() const
    {
        return itsFile;
    }
    int index() const
    {
        return itsIndex;
    }

private:
    QString itsName, itsFile;
    quint32 itsStyle;
    int itsIndex;
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
        return itsItems.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 1;
    }
    void clear();
    void showFonts(const QModelIndexList &font);

private:
    QList<CPreviewListItem *> itsItems;
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
    CPreviewList *itsModel;
};

}

#endif
