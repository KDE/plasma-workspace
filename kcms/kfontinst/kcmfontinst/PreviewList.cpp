/*
    SPDX-FileCopyrightText: 2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PreviewList.h"
#include "Fc.h"
#include "FcEngine.h"
#include "FontList.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QPainter>
#include <QPixmapCache>
#include <QStyledItemDelegate>
#include <QTextStream>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

namespace KFI
{
static CFcEngine *theFcEngine = nullptr;

CPreviewList::CPreviewList(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QVariant CPreviewList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    CPreviewListItem *item = static_cast<CPreviewListItem *>(index.internalPointer());

    if (item) {
        switch (role) {
        case Qt::DisplayRole:
            return FC::createName(item->name(), item->style());
        default:
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags CPreviewList::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled;
}

QModelIndex CPreviewList::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        CPreviewListItem *item = m_items.value(row);

        if (item) {
            return createIndex(row, column, item);
        }
    }

    return QModelIndex();
}

QModelIndex CPreviewList::parent(const QModelIndex &) const
{
    return QModelIndex();
}

void CPreviewList::clear()
{
    Q_EMIT layoutAboutToBeChanged();
    qDeleteAll(m_items);
    m_items.clear();
    Q_EMIT layoutChanged();
}

void CPreviewList::showFonts(const QModelIndexList &fonts)
{
    clear();
    Q_EMIT layoutAboutToBeChanged();
    QModelIndex index;
    foreach (index, fonts) {
        CFontModelItem *mi = static_cast<CFontModelItem *>(index.internalPointer());
        CFontItem *font = mi->parent() ? static_cast<CFontItem *>(mi) : (static_cast<CFamilyItem *>(mi))->regularFont();

        if (font) {
            m_items.append(new CPreviewListItem(font->family(), font->styleInfo(), font->isEnabled() ? QString() : font->fileName(), font->index()));
        }
    }

    Q_EMIT layoutChanged();
}

class CPreviewListViewDelegate : public QStyledItemDelegate
{
public:
    CPreviewListViewDelegate(QObject *p, int previewSize)
        : QStyledItemDelegate(p)
        , m_previewSize(previewSize)
    {
    }
    ~CPreviewListViewDelegate() override
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const override
    {
        CPreviewListItem *item = static_cast<CPreviewListItem *>(idx.internalPointer());
        QStyleOptionViewItem opt(option);

        opt.rect.adjust(1, constBorder - 3, 0, -(1 + m_previewSize));

        QStyledItemDelegate::paint(painter, opt, idx);

        opt.rect.adjust(constBorder, option.rect.height() - (1 + m_previewSize), -constBorder, 0);
        painter->save();
        painter->setPen(QApplication::palette().color(QPalette::Text));
        QRect lineRect(opt.rect.adjusted(-1, 3, 0, 2));
        painter->drawLine(lineRect.bottomLeft(), lineRect.bottomRight());
        painter->setClipRect(option.rect.adjusted(constBorder, 0, -constBorder, 0));
        painter->drawPixmap(opt.rect.topLeft(), getPixmap(item));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &idx) const override
    {
        QSize sz(QStyledItemDelegate::sizeHint(option, idx));
        // int   pWidth(getPixmap(static_cast<CPreviewListItem *>(idx.internalPointer())).width());
        int pWidth(1536);

        return QSize((constBorder * 2) + pWidth, sz.height() + 1 + constBorder + m_previewSize);
    }

    QPixmap getPixmap(CPreviewListItem *item) const
    {
        QString key;
        QPixmap pix;
        QColor text(QApplication::palette().color(QPalette::Text));

        QTextStream(&key) << "kfi-" << item->name() << "-" << item->style() << "-" << text.rgba();

        if (!QPixmapCache::find(key, &pix)) {
            QColor bgnd(Qt::black);

            bgnd.setAlpha(0);
            // TODO: Ideally, for this preview we want the fonts to be of a set point size
            pix = QPixmap::fromImage(
                theFcEngine->drawPreview(item->file().isEmpty() ? item->name() : item->file(), item->style(), item->index(), text, bgnd, m_previewSize));
            QPixmapCache::insert(key, pix);
        }

        return pix;
    }

    int m_previewSize;
    static const int constBorder = 4;
};

CPreviewListView::CPreviewListView(CFcEngine *eng, QWidget *parent)
    : QTreeView(parent)
{
    theFcEngine = eng;

    QFont font;
    int pixelSize((int)(((font.pointSizeF() * QX11Info::appDpiY()) / 72.0) + 0.5));

    m_model = new CPreviewList(this);
    setModel(m_model);
    setItemDelegate(new CPreviewListViewDelegate(this, (pixelSize + 12) * 3));
    setSelectionMode(NoSelection);
    setVerticalScrollMode(ScrollPerPixel);
    setSortingEnabled(false);
    setAlternatingRowColors(false);
    setAcceptDrops(false);
    setDragEnabled(false);
    header()->setVisible(false);
    setRootIsDecorated(false);
    resizeColumnToContents(0);
}

void CPreviewListView::refreshPreviews()
{
    QPixmapCache::clear();
    repaint();
    resizeColumnToContents(0);
}

void CPreviewListView::showFonts(const QModelIndexList &fonts)
{
    m_model->showFonts(fonts);
    resizeColumnToContents(0);
}

void CPreviewListView::contextMenuEvent(QContextMenuEvent *ev)
{
    Q_EMIT showMenu(ev->pos());
}

}
