/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "DuplicatesDialog.h"
#include "ActionLabel.h"
#include "Fc.h"
#include "FcEngine.h"
#include "FontList.h"
#include <KFileItem>
#include <KFormat>
#include <KIconLoader>
#include <KMessageBox>
#include <KPropertiesDialog>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMimeDatabase>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>
#include <QWindow>

namespace KFI
{
enum EDialogColumns {
    COL_FILE,
    COL_TRASH,
    COL_SIZE,
    COL_DATE,
    COL_LINK,
};

CDuplicatesDialog::CDuplicatesDialog(QWidget *parent, CFontList *fl)
    : QDialog(parent)
    , m_fontList(fl)
{
    setWindowTitle(i18n("Duplicate Fonts"));
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::clicked, this, &CDuplicatesDialog::slotButtonClicked);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    setModal(true);

    QFrame *page = new QFrame(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(m_buttonBox);

    QGridLayout *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(page);
    m_view = new CFontFileListView(page);
    m_view->hide();
    layout->addWidget(m_actionLabel = new CActionLabel(this), 0, 0);
    layout->addWidget(m_label, 0, 1);
    m_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    layout->addWidget(m_view, 1, 0, 1, 2);
    m_fontFileList = new CFontFileList(this);
    connect(m_fontFileList, SIGNAL(finished()), SLOT(scanFinished()));
    connect(m_view, &CFontFileListView::haveDeletions, this, &CDuplicatesDialog::enableButtonOk);
}

int CDuplicatesDialog::exec()
{
    m_actionLabel->startAnimation();
    m_label->setText(i18n("Scanning for duplicate fonts. Please wait…"));
    m_fontFileList->start();
    return QDialog::exec();
}

void CDuplicatesDialog::scanFinished()
{
    m_actionLabel->stopAnimation();

    if (m_fontFileList->wasTerminated()) {
        m_fontFileList->wait();
        reject();
    } else {
        CFontFileList::TFontMap duplicates;

        m_fontFileList->getDuplicateFonts(duplicates);

        if (0 == duplicates.count()) {
            m_buttonBox->setStandardButtons(QDialogButtonBox::Close);
            m_label->setText(i18n("No duplicate fonts found."));
        } else {
            QSize sizeB4(size());

            m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Close);
            QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
            okButton->setDefault(true);
            okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
            okButton->setText(i18n("Delete Marked Files"));
            okButton->setEnabled(false);
            m_label->setText(i18np("%1 duplicate font found.", "%1 duplicate fonts found.", duplicates.count()));
            m_view->show();

            CFontFileList::TFontMap::ConstIterator it(duplicates.begin()), end(duplicates.end());
            QFont boldFont(font());

            boldFont.setBold(true);

            for (; it != end; ++it) {
                QStringList details;

                details << FC::createName(it.key().family, it.key().styleInfo);

                CFontFileListView::StyleItem *top = new CFontFileListView::StyleItem(m_view, details, it.key().family, it.key().styleInfo);

                QSet<QString>::ConstIterator fit((*it).begin()), fend((*it).end());
                int tt(0), t1(0);

                for (; fit != fend; ++fit) {
                    QFileInfo info(*fit);
                    details.clear();
                    details.append(*fit);
                    details.append("");
                    details.append(KFormat().formatByteSize(info.size()));
                    details.append(QLocale().toString(info.birthTime()));
                    if (info.isSymLink()) {
                        details.append(info.symLinkTarget());
                    }
                    new QTreeWidgetItem(top, details);
                    if (Misc::checkExt(*fit, "pfa") || Misc::checkExt(*fit, "pfb")) {
                        t1++;
                    } else {
                        tt++;
                    }
                }
                top->setData(COL_FILE, Qt::DecorationRole, QIcon::fromTheme(t1 > tt ? "application-x-font-type1" : "application-x-font-ttf"));
                top->setFont(COL_FILE, boldFont);
            }

            QTreeWidgetItem *item = nullptr;
            for (int i = 0; (item = m_view->topLevelItem(i)); ++i) {
                item->setExpanded(true);
            }

            m_view->setSortingEnabled(true);
            m_view->header()->resizeSections(QHeaderView::ResizeToContents);

            int width = (m_view->frameWidth() + 8) * 2 + style()->pixelMetric(QStyle::PM_LayoutLeftMargin) + style()->pixelMetric(QStyle::PM_LayoutRightMargin);

            for (int i = 0; i < m_view->header()->count(); ++i) {
                width += m_view->header()->sectionSize(i);
            }

            width = qMin(windowHandle()->screen()->size().width(), width);
            resize(width, height());
            QSize sizeNow(size());
            if (sizeNow.width() > sizeB4.width()) {
                int xmod = (sizeNow.width() - sizeB4.width()) / 2, ymod = (sizeNow.height() - sizeB4.height()) / 2;

                move(pos().x() - xmod, pos().y() - ymod);
            }
        }
    }
}

enum EStatus {
    STATUS_NO_FILES,
    STATUS_ALL_REMOVED,
    STATUS_ERROR,
    STATUS_USER_CANCELLED,
};

void CDuplicatesDialog::slotButtonClicked(QAbstractButton *button)
{
    switch (m_buttonBox->standardButton(button)) {
    case QDialogButtonBox::Ok: {
        QSet<QString> files = m_view->getMarkedFiles();
        int fCount = files.count();

        if (1 == fCount ? KMessageBox::PrimaryAction
                    == KMessageBox::warningTwoActions(this,
                                                      i18n("Are you sure you wish to delete:\n%1", *files.begin()),
                                                      QString(),
                                                      KStandardGuiItem::del(),
                                                      KStandardGuiItem::cancel())
                        : KMessageBox::PrimaryAction
                    == KMessageBox::warningTwoActionsList(this,
                                                          i18n("Are you sure you wish to delete:"),
                                                          files.values(),
                                                          QString(),
                                                          KStandardGuiItem::del(),
                                                          KStandardGuiItem::cancel())) {
            m_fontList->setSlowUpdates(true);

            CJobRunner runner(this);

            connect(&runner, &CJobRunner::configuring, m_fontList, &CFontList::unsetSlowUpdates);
            runner.exec(CJobRunner::CMD_REMOVE_FILE, m_view->getMarkedItems(), false);
            m_fontList->setSlowUpdates(false);
            m_view->removeFiles();
            files = m_view->getMarkedFiles();
            if (fCount != files.count()) {
                CFcEngine::setDirty();
            }
            if (0 == files.count()) {
                accept();
            }
        }
        break;
    }
    case QDialogButtonBox::Cancel:
    case QDialogButtonBox::Close:
        if (!m_fontFileList->wasTerminated()) {
            if (m_fontFileList->isRunning()) {
                if (KMessageBox::PrimaryAction
                    == KMessageBox::warningTwoActions(this, i18n("Cancel font scan?"), QString(), KStandardGuiItem::cancel(), KStandardGuiItem::cont())) {
                    m_label->setText(i18n("Canceling…"));

                    if (m_fontFileList->isRunning()) {
                        m_fontFileList->terminate();
                    } else {
                        reject();
                    }
                }
            } else {
                reject();
            }
        }
        break;
    default:
        break;
    }
}

void CDuplicatesDialog::enableButtonOk(bool on)
{
    QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setEnabled(on);
    }
}

static uint qHash(const CFontFileList::TFile &key)
{
    return qHash(key.name.toLower());
}

CFontFileList::CFontFileList(CDuplicatesDialog *parent)
    : QThread(parent)
    , m_terminated(false)
{
}

void CFontFileList::start()
{
    if (!isRunning()) {
        m_terminated = false;
        QThread::start();
    }
}

void CFontFileList::terminate()
{
    m_terminated = true;
}

void CFontFileList::getDuplicateFonts(TFontMap &map)
{
    map = m_map;

    if (!map.isEmpty()) {
        TFontMap::Iterator it(map.begin()), end(map.end());

        // Now re-iterate, and remove any entries that only have 1 file...
        for (it = map.begin(); it != end;) {
            if ((*it).count() < 2) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void CFontFileList::run()
{
    const QList<CFamilyItem *> &families(((CDuplicatesDialog *)parent())->fontList()->families());
    QList<CFamilyItem *>::ConstIterator it(families.begin()), end(families.end());

    for (; it != end; ++it) {
        QList<CFontItem *>::ConstIterator fontIt((*it)->fonts().begin()), fontEnd((*it)->fonts().end());

        for (; fontIt != fontEnd; ++fontIt) {
            if (!(*fontIt)->isBitmap()) {
                Misc::TFont font((*fontIt)->family(), (*fontIt)->styleInfo());
                FileCont::ConstIterator fileIt((*fontIt)->files().begin()), fileEnd((*fontIt)->files().end());

                for (; fileIt != fileEnd; ++fileIt) {
                    if (!Misc::isMetrics((*fileIt).path()) && !Misc::isBitmap((*fileIt).path())) {
                        m_map[font].insert((*fileIt).path());
                    }
                }
            }
        }
    }

    // if we have 2 fonts: /wibble/a.ttf and /wibble/a.TTF fontconfig only returns the 1st, so we
    // now iterate over fontconfig's list, and look for other matching fonts...
    if (!m_map.isEmpty() && !m_terminated) {
        // Create a map of folder -> set<files>
        TFontMap::Iterator it(m_map.begin()), end(m_map.end());
        QHash<QString, QSet<TFile>> folderMap;

        for (int n = 0; it != end && !m_terminated; ++it) {
            QStringList add;
            QSet<QString>::const_iterator fIt((*it).begin()), fEnd((*it).end());

            for (; fIt != fEnd && !m_terminated; ++fIt, ++n) {
                folderMap[Misc::getDir(*fIt)].insert(TFile(Misc::getFile(*fIt), it));
            }
        }

        // Go through our folder map, and check for file duplicates...
        QHash<QString, QSet<TFile>>::Iterator folderIt(folderMap.begin()), folderEnd(folderMap.end());

        for (; folderIt != folderEnd && !m_terminated; ++folderIt) {
            fileDuplicates(folderIt.key(), *folderIt);
        }
    }

    Q_EMIT finished();
}

void CFontFileList::fileDuplicates(const QString &folder, const QSet<TFile> &files)
{
    QDir dir(folder);

    dir.setFilter(QDir::Files | QDir::Hidden);

    QFileInfoList list(dir.entryInfoList());

    for (int i = 0; i < list.size() && !m_terminated; ++i) {
        QFileInfo fileInfo(list.at(i));

        // Check if this file is already know about - this will do a case-sensitive comparison
        if (!files.contains(TFile(fileInfo.fileName()))) {
            // OK, not found - this means it is a duplicate, but different case. So, find the
            // FontMap iterator, and update its list of files.
            QSet<TFile>::ConstIterator entry = files.find(TFile(fileInfo.fileName(), true));

            if (entry != files.end()) {
                (*((*entry).it)).insert(fileInfo.absoluteFilePath());
            }
        }
    }
}

inline void markItem(QTreeWidgetItem *item)
{
    item->setData(COL_TRASH, Qt::DecorationRole, QIcon::fromTheme("list-remove"));
}

inline void unmarkItem(QTreeWidgetItem *item)
{
    item->setData(COL_TRASH, Qt::DecorationRole, QVariant());
}

inline bool isMarked(QTreeWidgetItem *item)
{
    return item->data(COL_TRASH, Qt::DecorationRole).isValid();
}

CFontFileListView::CFontFileListView(QWidget *parent)
    : QTreeWidget(parent)
{
    QStringList headers;
    headers.append(i18n("Font/File"));
    headers.append("");
    headers.append(i18n("Size"));
    headers.append(i18n("Date"));
    headers.append(i18n("Links To"));
    setHeaderLabels(headers);
    headerItem()->setData(COL_TRASH, Qt::DecorationRole, QIcon::fromTheme("user-trash"));
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setSelectionMode(ExtendedSelection);
    sortByColumn(COL_FILE, Qt::AscendingOrder);
    setSelectionBehavior(SelectRows);
    setSortingEnabled(true);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);

    m_menu = new QMenu(this);
    if (!Misc::app(KFI_VIEWER).isEmpty()) {
        m_menu->addAction(QIcon::fromTheme("kfontview"), i18n("Open in Font Viewer"), this, &CFontFileListView::openViewer);
    }
    m_menu->addAction(QIcon::fromTheme("document-properties"), i18n("Properties"), this, &CFontFileListView::properties);
    m_menu->addSeparator();
    m_unMarkAct = m_menu->addAction(i18n("Unmark for Deletion"), this, &CFontFileListView::unmark);
    m_markAct = m_menu->addAction(QIcon::fromTheme("edit-delete"), i18n("Mark for Deletion"), this, &CFontFileListView::mark);

    connect(this, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)), SLOT(clicked(QTreeWidgetItem *, int)));
}

QSet<QString> CFontFileListView::getMarkedFiles()
{
    QTreeWidgetItem *root = invisibleRootItem();
    QSet<QString> files;

    for (int t = 0; t < root->childCount(); ++t) {
        QTreeWidgetItem *font = root->child(t);

        for (int c = 0; c < font->childCount(); ++c) {
            QTreeWidgetItem *file = font->child(c);

            if (isMarked(file)) {
                files.insert(file->text(0));
            }
        }
    }

    return files;
}

CJobRunner::ItemList CFontFileListView::getMarkedItems()
{
    QTreeWidgetItem *root = invisibleRootItem();
    CJobRunner::ItemList items;
    QString home(Misc::dirSyntax(QDir::homePath()));

    for (int t = 0; t < root->childCount(); ++t) {
        StyleItem *style = (StyleItem *)root->child(t);

        for (int c = 0; c < style->childCount(); ++c) {
            QTreeWidgetItem *file = style->child(c);

            if (isMarked(file)) {
                items.append(CJobRunner::Item(file->text(0), style->family(), style->value(), 0 != file->text(0).indexOf(home)));
            }
        }
    }

    return items;
}

void CFontFileListView::removeFiles()
{
    QTreeWidgetItem *root = invisibleRootItem();
    QList<QTreeWidgetItem *> removeFonts;

    for (int t = 0; t < root->childCount(); ++t) {
        QList<QTreeWidgetItem *> removeFiles;
        QTreeWidgetItem *font = root->child(t);

        for (int c = 0; c < font->childCount(); ++c) {
            QTreeWidgetItem *file = font->child(c);

            if (!Misc::fExists(file->text(0))) {
                removeFiles.append(file);
            }
        }

        QList<QTreeWidgetItem *>::ConstIterator it(removeFiles.begin()), end(removeFiles.end());

        for (; it != end; ++it) {
            delete (*it);
        }
        if (0 == font->childCount()) {
            removeFonts.append(font);
        }
    }

    QList<QTreeWidgetItem *>::ConstIterator it(removeFonts.begin()), end(removeFonts.end());
    for (; it != end; ++it) {
        delete (*it);
    }
}

void CFontFileListView::openViewer()
{
    // Number of fonts user has selected, before we ask if they really want to view them all...
    static const int constMaxBeforePrompt = 10;

    const QList<QTreeWidgetItem *> items(selectedItems());
    QSet<QString> files;

    for (QTreeWidgetItem *const item : items) {
        if (item->parent()) { // Then it is a file, not font name :-)
            files.insert(item->text(0));
        }
    }

    if (!files.isEmpty()
        && (files.count() < constMaxBeforePrompt
            || KMessageBox::PrimaryAction
                == KMessageBox::questionTwoActions(this,
                                                   i18np("Open font in font viewer?", "Open all %1 fonts in font viewer?", files.count()),
                                                   QString(),
                                                   KStandardGuiItem::open(),
                                                   KStandardGuiItem::cancel()))) {
        QSet<QString>::ConstIterator it(files.begin()), end(files.end());

        for (; it != end; ++it) {
            QStringList args;

            args << (*it);

            QProcess::startDetached(Misc::app(KFI_VIEWER), args);
        }
    }
}

void CFontFileListView::properties()
{
    const QList<QTreeWidgetItem *> items(selectedItems());
    KFileItemList files;
    QMimeDatabase db;

    for (QTreeWidgetItem *const item : items) {
        if (item->parent()) {
            files.append(
                KFileItem(QUrl::fromLocalFile(item->text(0)), db.mimeTypeForFile(item->text(0)).name(), item->text(COL_LINK).isEmpty() ? S_IFREG : S_IFLNK));
        }
    }

    if (!files.isEmpty()) {
        KPropertiesDialog dlg(files, this);
        dlg.exec();
    }
}

void CFontFileListView::mark()
{
    const QList<QTreeWidgetItem *> items(selectedItems());

    for (QTreeWidgetItem *const item : items) {
        if (item->parent()) {
            markItem(item);
        }
    }
    checkFiles();
}

void CFontFileListView::unmark()
{
    const QList<QTreeWidgetItem *> items(selectedItems());

    for (QTreeWidgetItem *const item : items) {
        if (item->parent()) {
            unmarkItem(item);
        }
    }
    checkFiles();
}

void CFontFileListView::selectionChanged()
{
    const QList<QTreeWidgetItem *> items(selectedItems());

    for (QTreeWidgetItem *const item : items) {
        if (!item->parent() && item->isSelected()) {
            item->setSelected(false);
        }
    }
}

void CFontFileListView::clicked(QTreeWidgetItem *item, int col)
{
    if (item && COL_TRASH == col && item->parent()) {
        if (isMarked(item)) {
            unmarkItem(item);
        } else {
            markItem(item);
        }
        checkFiles();
    }
}

void CFontFileListView::contextMenuEvent(QContextMenuEvent *ev)
{
    QTreeWidgetItem *item(itemAt(ev->pos()));

    if (item && item->parent()) {
        if (!item->isSelected()) {
            item->setSelected(true);
        }

        bool haveUnmarked(false), haveMarked(false);

        const QList<QTreeWidgetItem *> items(selectedItems());

        for (QTreeWidgetItem *const item : items) {
            if (item->parent() && item->isSelected()) {
                if (isMarked(item)) {
                    haveMarked = true;
                } else {
                    haveUnmarked = true;
                }
            }

            if (haveUnmarked && haveMarked) {
                break;
            }
        }

        m_markAct->setEnabled(haveUnmarked);
        m_unMarkAct->setEnabled(haveMarked);
        m_menu->popup(ev->globalPos());
    }
}

void CFontFileListView::checkFiles()
{
    // Need to check that if we mark a file that is linked to, then we also need
    // to mark the sym link.
    QSet<QString> marked(getMarkedFiles());

    if (marked.count()) {
        QTreeWidgetItem *root = invisibleRootItem();

        for (int t = 0; t < root->childCount(); ++t) {
            QTreeWidgetItem *font = root->child(t);

            for (int c = 0; c < font->childCount(); ++c) {
                QTreeWidgetItem *file = font->child(c);
                QString link(font->child(c)->text(COL_LINK));

                if (!link.isEmpty() && marked.contains(link)) {
                    if (!isMarked(file)) {
                        markItem(file);
                    }
                }
            }
        }

        Q_EMIT haveDeletions(true);
    } else {
        Q_EMIT haveDeletions(false);
    }
}
}
