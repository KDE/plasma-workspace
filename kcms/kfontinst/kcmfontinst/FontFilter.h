#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-FileCopyrightText: 2019 Guo Yunhe <i@guoyunhe.me>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QActionGroup>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

namespace KFI
{
class CFontFilter : public QWidget
{
    Q_OBJECT

public:
    enum ECriteria {
        CRIT_FAMILY,
        CRIT_STYLE,
        CRIT_FOUNDRY,
        CRIT_FONTCONFIG,
        CRIT_FILETYPE,
        CRIT_FILENAME,
        CRIT_LOCATION,
        CRIT_WS,

        NUM_CRIT,
    };

    CFontFilter(QWidget *parent);
    ~CFontFilter() override
    {
    }

    void setFoundries(const QSet<QString> &currentFoundries);

Q_SIGNALS:

    void criteriaChanged(int crit, qulonglong ws, const QStringList &ft);
    void queryChanged(QString text);

private Q_SLOTS:

    void filterChanged();
    void textChanged(const QString &text);
    void ftChanged(const QString &ft);
    void wsChanged(const QString &writingSystemName);
    void foundryChanged(const QString &foundry);

private:
    void addAction(ECriteria crit, bool on);
    void setCriteria(ECriteria crit);

private:
    QPushButton *m_menuButton;
    QHBoxLayout *m_layout;
    QMenu *m_menu;
    QLineEdit *m_lineEdit;
    ECriteria m_currentCriteria;
    QFontDatabase::WritingSystem m_currentWs;
    QStringList m_currentFileTypes;
    QIcon m_icons[NUM_CRIT];
    QString m_texts[NUM_CRIT];
    QAction *m_actions[NUM_CRIT];
    QActionGroup *m_actionGroup;
};

}
