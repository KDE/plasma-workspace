/*
  * This file is part of the KDE project
  * Copyright (C) 2006-2008 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
*/

#ifndef PROGRESSLISTDELEGATE_H
#define PROGRESSLISTDELEGATE_H

#include <QModelIndex>

#include <kwidgetitemdelegate.h>

class QListView;

class ProgressListDelegate
        : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    explicit ProgressListDelegate(QObject *parent = 0, QListView *listView = 0);
    ~ProgressListDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setSeparatorPixels(int separatorPixels);
    void setLeftMargin(int leftMargin);
    void setRightMargin(int rightMargin);
    void setMinimumItemHeight(int minimumItemHeight);
    void setMinimumContentWidth(int minimumContentWidth);
    void setEditorHeight(int editorHeight);

protected:
    QList<QWidget*> createItemWidgets(const QModelIndex &index) const override;
    void updateItemWidgets(const QList<QWidget*> widgets,
                                   const QStyleOptionViewItem &option,
                                   const QPersistentModelIndex &index) const override;

private Q_SLOTS:
    void slotPauseResumeClicked();
    void slotCancelClicked();
    void slotClearClicked();

private:
    class Private;
    Private *d;
};

#endif // PROGRESSLISTDELEGATE_H
