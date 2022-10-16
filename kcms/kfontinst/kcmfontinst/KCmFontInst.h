#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "GroupList.h"
#include "JobRunner.h"
#include <KCModule>
#include <KConfig>
#include <KIO/Job>
#include <KNSWidgets/Button>
#include <QSet>
#include <QUrl>

class QPushButton;
class QProgressDialog;
class QTemporaryDir;
class QAction;
class QLabel;
class QMenu;
class QProcess;
class QSplitter;

namespace KFI
{
class CFontFilter;
class CFontList;
class CFontPreview;
class CUpdateDialog;
class CFontListView;
class CProgressBar;
class CPreviewListView;

class CKCmFontInst : public KCModule
{
    Q_OBJECT

public:
    explicit CKCmFontInst(QWidget *parent = nullptr, const QVariantList &list = QVariantList());
    ~CKCmFontInst() override;

public Q_SLOTS:

    QString quickHelp() const override;
    void previewMenu(const QPoint &pos);
    void splitterMoved();
    void fontsSelected(const QModelIndexList &list);
    void groupSelected(const QModelIndex &index);
    void addFonts();
    void deleteFonts();
    void moveFonts();
    void zipGroup();
    void enableFonts();
    void disableFonts();
    void addGroup();
    void removeGroup();
    void enableGroup();
    void disableGroup();
    void changeText();
    void duplicateFonts();
    void downloadFonts(const QList<KNSCore::Entry> &changedEntries);
    void print();
    void printGroup();
    void listingPercent(int p);
    void refreshFontList();
    void refreshFamilies();
    void showInfo(const QString &info);
    void setStatusBar();
    void addFonts(const QSet<QUrl> &src);

private:
    void removeDeletedFontsFromGroups();
    void selectGroup(CGroupListItem::EType grp);
    void print(bool all);
    void toggleGroup(bool enable);
    void toggleFonts(bool enable, const QString &grp = QString());
    void toggleFonts(CJobRunner::ItemList &urls, const QStringList &fonts, bool enable, const QString &grp);
    void selectMainGroup();
    void doCmd(CJobRunner::ECommand cmd, const CJobRunner::ItemList &urls, bool system = false);

private:
    QSplitter *m_groupSplitter, *m_previewSplitter;
    CFontPreview *m_preview;
    CPreviewListView *m_previewList;
    KConfig m_config;
    QLabel *m_statusLabel;
    CProgressBar *m_listingProgress;
    CFontList *m_fontList;
    CFontListView *m_fontListView;
    CGroupList *m_groupList;
    CGroupListView *m_groupListView;
    QPushButton *m_deleteGroupControl, *m_enableGroupControl, *m_disableGroupControl, *m_addFontControl, *m_deleteFontControl, *m_scanDuplicateFontsControl;
    KNSWidgets::Button *m_getNewFontsControl;
    CFontFilter *m_filter;
    QString m_lastStatusBarMsg;
    KIO::Job *m_job;
    QProgressDialog *m_progress;
    CUpdateDialog *m_updateDialog;
    QTemporaryDir *m_tempDir;
    QProcess *m_printProc;
    QSet<QString> m_deletedFonts;
    QList<QUrl> m_modifiedUrls;
    CJobRunner *m_runner;
    QMenu *m_previewMenu, *m_previewListMenu;
    QWidget *m_previewWidget;
    bool m_previewHidden;
};

}
