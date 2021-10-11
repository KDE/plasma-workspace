#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FontList.h"
#include <KIO/Job>
#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QTreeView>
#include <QVariant>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QTextStream;
class QDomElement;

namespace KFI
{
class CGroupList;
class CFontItem;

class CGroupListItem
{
public:
    enum EType {
        ALL,
        PERSONAL,
        SYSTEM,
        UNCLASSIFIED,
        CUSTOM,
    };

    union Data {
        bool validated; // CUSTOM
        CGroupList *parent; // UNCLASSIFIED
    };

    CGroupListItem(const QString &name);
    CGroupListItem(EType type, CGroupList *p);

    const QString &name() const
    {
        return m_name;
    }
    void setName(const QString &n)
    {
        m_name = n;
    }
    QSet<QString> &families()
    {
        return m_families;
    }
    EType type() const
    {
        return m_type;
    }
    bool isCustom() const
    {
        return CUSTOM == m_type;
    }
    bool isAll() const
    {
        return ALL == m_type;
    }
    bool isUnclassified() const
    {
        return UNCLASSIFIED == m_type;
    }
    bool isPersonal() const
    {
        return PERSONAL == m_type;
    }
    bool isSystem() const
    {
        return SYSTEM == m_type;
    }
    bool validated() const
    {
        return isCustom() ? m_data.validated : true;
    }
    void setValidated()
    {
        if (isCustom())
            m_data.validated = true;
    }
    bool highlighted() const
    {
        return m_highlighted;
    }
    void setHighlighted(bool b)
    {
        m_highlighted = b;
    }
    bool hasFont(const CFontItem *fnt) const;
    CFamilyItem::EStatus status() const
    {
        return m_status;
    }
    void updateStatus(QSet<QString> &enabled, QSet<QString> &disabled, QSet<QString> &partial);
    bool load(QDomElement &elem);
    bool addFamilies(QDomElement &elem);
    void save(QTextStream &str);
    void addFamily(const QString &family)
    {
        m_families.insert(family);
    }
    void removeFamily(const QString &family)
    {
        m_families.remove(family);
    }
    bool hasFamily(const QString &family)
    {
        return m_families.contains(family);
    }

private:
    QSet<QString> m_families;
    QString m_name;
    EType m_type;
    Data m_data;
    bool m_highlighted;
    CFamilyItem::EStatus m_status;
};

class CGroupList : public QAbstractItemModel
{
    Q_OBJECT

public:
    CGroupList(QWidget *parent = nullptr);
    ~CGroupList() override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    void update(const QModelIndex &unHighlight, const QModelIndex &highlight);
    void updateStatus(QSet<QString> &enabled, QSet<QString> &disabled, QSet<QString> &partial);
    void setSysMode(bool sys);
    void rescan();
    void load();
    bool load(const QString &file);
    bool save();
    bool save(const QString &fileName, CGroupListItem *grp);
    void merge(const QString &file);
    void clear();
    QModelIndex index(CGroupListItem::EType t);
    void createGroup(const QString &name);
    bool removeGroup(const QModelIndex &idx);
    void removeFamily(const QString &family);
    bool removeFromGroup(CGroupListItem *grp, const QString &family);
    QString whatsThis() const;

    CGroupListItem *group(CGroupListItem::EType t)
    {
        return m_specialGroups[t];
    }
    bool exists(const QString &name, bool showDialog = true);

public Q_SLOTS:

    void addToGroup(const QModelIndex &group, const QSet<QString> &families);
    void removeFromGroup(const QModelIndex &group, const QSet<QString> &families);

Q_SIGNALS:

    void refresh();

private:
    void readGroupsFile();
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    CGroupListItem *find(const QString &name);
    QModelIndex createIdx(int r, int c, void *p)
    {
        return createIndex(r, c, p);
    }

private:
    QString m_fileName;
    time_t m_timeStamp;
    bool m_modified;
    QWidget *m_parent;
    QList<CGroupListItem *> m_groups;
    QMap<CGroupListItem::EType, CGroupListItem *> m_specialGroups;
    Qt::SortOrder m_sortOrder;

    friend class CGroupListItem;
    friend class CGroupListView;
};

class CGroupListView : public QTreeView
{
    Q_OBJECT

public:
    CGroupListView(QWidget *parent, CGroupList *model);
    ~CGroupListView() override
    {
    }

    QSize sizeHint() const override
    {
        return QSize(32, 32);
    }

    bool isCustom()
    {
        return CGroupListItem::CUSTOM == getType();
    }
    bool isUnclassified()
    {
        return CGroupListItem::UNCLASSIFIED == getType();
    }
    bool isSystem()
    {
        return CGroupListItem::SYSTEM == getType();
    }
    bool isPersonal()
    {
        return CGroupListItem::PERSONAL == getType();
    }
    CGroupListItem::EType getType();
    void controlMenu(bool del, bool en, bool dis, bool p, bool exp);

Q_SIGNALS:

    void del();
    void print();
    void enable();
    void disable();
    void zip();
    void moveFonts();
    void info(const QString &str);
    void addFamilies(const QModelIndex &group, const QSet<QString> &);
    void removeFamilies(const QModelIndex &group, const QSet<QString> &);
    void itemSelected(const QModelIndex &);
    void unclassifiedChanged();

private Q_SLOTS:

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void rename();
    void emitMoveFonts();

private:
    void contextMenuEvent(QContextMenuEvent *ev) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void drawHighlighter(const QModelIndex &idx);
    bool viewportEvent(QEvent *event) override;

private:
    QMenu *m_menu;
    QAction *m_deleteAct, *m_enableAct, *m_disableAct, *m_printAct, *m_renameAct, *m_exportAct;
    QModelIndex m_currentDropItem;
};
}
