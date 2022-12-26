#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Family.h"
#include "FcQuery.h"
#include "File.h"
#include "FontFilter.h"
#include "FontInst.h"
#include "JobRunner.h"
#include "Misc.h"
#include "Style.h"
#include <KFileItem>
#include <KIO/Job>
#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QModelIndex>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QUrl>
#include <QVariant>

class QMenu;
class QMimeData;
class QTimer;

#define KFI_FONT_DRAG_MIME "kfontinst/fontlist"

namespace KFI
{
class CFontItem;
class CFontItem;
class CFamilyItem;
class CGroupListItem;
class Style;

enum EColumns {
    COL_FONT,
    COL_STATUS,

    NUM_COLS,
};

typedef QList<CFamilyItem *> CFamilyItemCont;
typedef QList<CFontItem *> CFontItemCont;
typedef QHash<QString, CFamilyItem *> CFamilyItemHash;

class CFontList : public QAbstractItemModel
{
    Q_OBJECT

private:
    enum EMsgType {
        MSG_ADD,
        MSG_DEL,

        NUM_MSGS_TYPES,
    };

public:
    static const QStringList fontMimeTypes;

public:
    static QStringList compact(const QStringList &fonts);

    CFontList(QWidget *parent = nullptr);
    ~CFontList() override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QStringList mimeTypes() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int row(const CFamilyItem *fam) const
    {
        return m_families.indexOf((CFamilyItem *)fam);
    }
    void forceNewPreviews();
    const CFamilyItemCont &families() const
    {
        return m_families;
    }
    QModelIndex createIndex(int row, int column, void *data = nullptr) const
    {
        return QAbstractItemModel::createIndex(row, column, data);
    }
    bool hasFamily(const QString &family)
    {
        return nullptr != findFamily(family);
    }
    void refresh(bool allowSys, bool allowUser);
    bool allowSys() const
    {
        return m_allowSys;
    }
    bool allowUser() const
    {
        return m_allowUser;
    }
    void getFamilyStats(QSet<QString> &enabled, QSet<QString> &disabled, QSet<QString> &partial);
    void getFoundries(QSet<QString> &foundries) const;
    QString whatsThis() const;
    void setSlowUpdates(bool slow);
    bool slowUpdates() const
    {
        return m_slowUpdates;
    }

Q_SIGNALS:

    void listingPercent(int p);

public Q_SLOTS:

    void unsetSlowUpdates();
    void load();

private Q_SLOTS:

    void dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to);
    void fontList(int pid, const QList<KFI::Families> &families);
    void fontsAdded(const KFI::Families &families);
    void fontsRemoved(const KFI::Families &families);

private:
    void storeSlowedMessage(const Families &families, EMsgType type);
    void actionSlowedUpdates(bool sys);
    void addFonts(const FamilyCont &families, bool sys);
    void removeFonts(const FamilyCont &families, bool sys);
    CFamilyItem *findFamily(const QString &familyName);

private:
    CFamilyItemCont m_families;
    CFamilyItemHash m_familyHash;
    bool m_blockSignals, m_allowSys, m_allowUser, m_slowUpdates;
    static int theirPreviewSize;
    FamilyCont m_slowedMsgs[NUM_MSGS_TYPES][FontInst::FOLDER_COUNT];
};

class CFontModelItem
{
public:
    CFontModelItem(CFontModelItem *p)
        : m_parent(p)
        , m_isSystem(false)
    {
    }
    virtual ~CFontModelItem()
    {
    }

    CFontModelItem *parent() const
    {
        return m_parent;
    }
    bool isFamily() const
    {
        return nullptr == m_parent;
    }
    bool isFont() const
    {
        return nullptr != m_parent;
    }
    bool isSystem() const
    {
        return m_isSystem;
    }
    void setIsSystem(bool sys)
    {
        m_isSystem = sys;
    }
    virtual int rowNumber() const = 0;

protected:
    CFontModelItem *m_parent;
    bool m_isSystem;
};

class CFamilyItem : public CFontModelItem
{
public:
    enum EStatus {
        ENABLED,
        PARTIAL,
        DISABLED,
    };

    CFamilyItem(CFontList &p, const Family &f, bool sys);
    ~CFamilyItem() override;

    bool operator==(const CFamilyItem &other) const
    {
        return m_name == other.m_name;
    }

    bool addFonts(const StyleCont &styles, bool sys);
    const QString &name() const
    {
        return m_name;
    }
    const CFontItemCont &fonts() const
    {
        return m_fonts;
    }
    void addFont(CFontItem *font, bool update = true);
    void removeFont(CFontItem *font, bool update);
    void refresh();
    bool updateStatus();
    bool updateRegularFont(CFontItem *font);
    CFontItem *findFont(quint32 style, bool sys);
    int rowNumber() const override
    {
        return m_parent.row(this);
    }
    int row(const CFontItem *font) const
    {
        return m_fonts.indexOf((CFontItem *)font);
    }
    EStatus status() const
    {
        return m_status;
    }
    EStatus realStatus() const
    {
        return m_realStatus;
    }
    CFontItem *regularFont()
    {
        return m_regularFont;
    }
    int fontCount() const
    {
        return m_fontCount;
    }
    void getFoundries(QSet<QString> &foundries) const;
    bool slowUpdates() const
    {
        return m_parent.slowUpdates();
    }

private:
    bool usable(const CFontItem *font, bool root);

private:
    QString m_name;
    CFontItemCont m_fonts;
    int m_fontCount;
    EStatus m_status, m_realStatus;
    CFontItem *m_regularFont; // 'RegularFont' is font nearest to 'Regular' style, and used for previews.
    CFontList &m_parent;
};

class CFontItem : public CFontModelItem
{
public:
    CFontItem(CFontModelItem *p, const Style &s, bool sys);
    ~CFontItem() override
    {
    }

    void refresh();
    QString name() const
    {
        return family() + QString::fromLatin1(", ") + m_styleName;
    }
    bool isEnabled() const
    {
        return m_enabled;
    }
    bool isHidden() const
    {
        return !m_enabled;
    }
    bool isBitmap() const
    {
        return !m_style.scalable();
    }
    const QString &fileName() const
    {
        return (*m_style.files().begin()).path();
    }
    const QString &style() const
    {
        return m_styleName;
    }
    quint32 styleInfo() const
    {
        return m_style.value();
    }
    int index() const
    {
        return (*m_style.files().begin()).index();
    }
    const QString &family() const
    {
        return (static_cast<CFamilyItem *>(parent()))->name();
    }
    int rowNumber() const override
    {
        return (static_cast<CFamilyItem *>(parent()))->row(this);
    }
    const FileCont &files() const
    {
        return m_style.files();
    }
    qulonglong writingSystems() const
    {
        return m_style.writingSystems();
    }
    QUrl url() const
    {
        return CJobRunner::encode(family(), styleInfo(), isSystem());
    }
    void removeFile(const File &f)
    {
        m_style.remove(f);
    }
    void removeFiles(const FileCont &f)
    {
        m_style.removeFiles(f);
    }
    void addFile(const File &f)
    {
        m_style.add(f);
    }
    void addFiles(const FileCont &f)
    {
        m_style.addFiles(f);
    }

private:
    QString m_styleName;
    Style m_style;
    bool m_enabled;
};

class CFontListSortFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CFontListSortFilterProxy(QObject *parent, QAbstractItemModel *model);
    ~CFontListSortFilterProxy() override
    {
    }

    QVariant data(const QModelIndex &idx, int role) const override;
    bool acceptFont(CFontItem *fnt, bool checkFontText) const;
    bool acceptFamily(CFamilyItem *fam) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    void setFilterGroup(CGroupListItem *grp);
    CGroupListItem *filterGroup()
    {
        return m_group;
    }

    void setFilterText(const QString &text);
    void setFilterCriteria(CFontFilter::ECriteria crit, qulonglong ws, const QStringList &ft);

private Q_SLOTS:

    void timeout();
    void fcResults();

Q_SIGNALS:

    void refresh();

private:
    QString filterText() const
    {
        return CFontFilter::CRIT_FONTCONFIG == m_filterCriteria ? (m_fcQuery ? m_fcQuery->font() : QString()) : m_filterText;
    }

private:
    CGroupListItem *m_group;
    QString m_filterText;
    CFontFilter::ECriteria m_filterCriteria;
    qulonglong m_filterWs;
    QStringList m_filterTypes;
    QTimer *m_timer;
    CFcQuery *m_fcQuery;
};

class CFontListView : public QTreeView
{
    Q_OBJECT

public:
    CFontListView(QWidget *parent, CFontList *model);
    ~CFontListView() override
    {
    }

    void getFonts(CJobRunner::ItemList &urls, QStringList &fontNames, QSet<Misc::TFont> *fonts, bool selected, bool getEnabled = true, bool getDisabled = true);
    QSet<QString> getFiles();
    void getPrintableFonts(QSet<Misc::TFont> &items, bool selected);
    void setFilterGroup(CGroupListItem *grp);
    void stats(int &enabled, int &disabled, int &partial);
    void selectedStatus(bool &enabled, bool &disabled);
    QModelIndexList allFonts();
    void selectFirstFont();
    QModelIndexList getSelectedItems();

Q_SIGNALS:

    void del();
    void print();
    void enable();
    void disable();
    void fontsDropped(const QSet<QUrl> &);
    void itemsSelected(const QModelIndexList &);
    void refresh();
    void reload();

public Q_SLOTS:

    void listingPercent(int percent);
    void refreshFilter();
    void filterText(const QString &text);
    void filterCriteria(int crit, qulonglong ws, const QStringList &ft);

private Q_SLOTS:

    void setSortColumn(int col);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void itemCollapsed(const QModelIndex &index);
    void view();

private:
    QModelIndexList allIndexes();
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;
    bool viewportEvent(QEvent *event) override;

private:
    CFontListSortFilterProxy *m_proxy;
    CFontList *m_model;
    QMenu *m_menu;
    QAction *m_deleteAct, *m_enableAct, *m_disableAct, *m_printAct, *m_viewAct;
    bool m_allowDrops;
};

}
