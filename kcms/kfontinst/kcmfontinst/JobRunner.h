/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "FontInstInterface.h"
#include <QDialog>
#include <QUrl>

class QLabel;
class QProgressBar;
class QStackedWidget;
class QCloseEvent;
class QCheckBox;
class QDialogButtonBox;
class QAbstractButton;
class QTemporaryDir;

namespace KFI
{
class CActionLabel;

class CJobRunner : public QDialog
{
    Q_OBJECT

public:
    struct Item : public QUrl {
        enum EType {
            TYPE1_FONT,
            TYPE1_AFM,
            TYPE1_PFM,
            OTHER_FONT,
        };

        Item(const QUrl &u = QUrl(), const QString &n = QString(), bool dis = false);
        Item(const QString &file, const QString &family, quint32 style, bool system);
        QString displayName() const
        {
            return name.isEmpty() ? url() : name;
        }
        QString name,
            fileName; // Only required so that we can sort an ItemList so that afm/pfms follow after pfa/pfbs
        EType type;
        bool isDisabled;

        bool operator<(const Item &o) const;
    };

    typedef QList<Item> ItemList;

    enum ECommand {
        CMD_INSTALL,
        CMD_DELETE,
        CMD_ENABLE,
        CMD_DISABLE,
        CMD_UPDATE,
        CMD_MOVE,
        CMD_REMOVE_FILE,
    };

    explicit CJobRunner(QWidget *parent, int xid = 0);
    ~CJobRunner() override;

    static FontInstInterface *dbus();
    static QString folderName(bool sys);
    static void startDbusService();

    static QUrl encode(const QString &family, quint32 style, bool system);

    static void getAssociatedUrls(const QUrl &url, QList<QUrl> &list, bool afmAndPfm, QWidget *widget);
    int exec(ECommand cmd, const ItemList &urls, bool destIsSystem);

Q_SIGNALS:

    void configuring();

private Q_SLOTS:

    void doNext();
    void checkInterface();
    void dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to);
    void dbusStatus(int pid, int status);
    void slotButtonClicked(QAbstractButton *button);

private:
    void contineuToNext(bool cont);
    void closeEvent(QCloseEvent *e) override;
    void setPage(int page, const QString &msg = QString());
    QString fileName(const QUrl &url);
    QString errorString(int value) const;

private:
    ECommand m_cmd;
    ItemList m_urls;
    ItemList::ConstIterator m_it, m_end, m_prev;
    bool m_destIsSystem;
    QLabel *m_statusLabel, *m_skipLabel, *m_errorLabel;
    QProgressBar *m_progress;
    bool m_autoSkip, m_cancelClicked, m_modified;
    QTemporaryDir *m_tempDir;
    QString m_currentFile;
    CActionLabel *m_actionLabel;
    QStackedWidget *m_stack;
    int m_lastDBusStatus;
    QCheckBox *m_dontShowFinishedMsg;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_skipButton;
    QPushButton *m_autoSkipButton;
};

}
