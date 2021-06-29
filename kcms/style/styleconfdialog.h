/*
    KCMStyle's container dialog for custom style setup dialogs

    SPDX-FileCopyrightText: 2003 Maksim Orlovich <maksim.orlovich@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QDialog>
class QHBoxLayout;

class StyleConfigDialog : public QDialog
{
    Q_OBJECT
public:
    StyleConfigDialog(QWidget *parent, const QString &styleName);

    bool isDirty() const;

    void setMainWidget(QWidget *w);
public Q_SLOTS:
    void setDirty(bool dirty);

Q_SIGNALS:
    void defaults();
    void save();

private:
    void slotAccept();
    bool m_dirty;
    QHBoxLayout *mMainLayout = nullptr;
};
