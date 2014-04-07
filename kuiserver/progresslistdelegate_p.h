/*
  * This file is part of the KDE project
  * Copyright (C) 2007, 2006 Rafael Fernández López <ereslibre@kde.org>
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

#ifndef PROGRESSLISTDELEGATE_P_H
#define PROGRESSLISTDELEGATE_P_H

#include "progresslistmodel.h"

#include <QtCore/QList>
#include <QtCore/QObject>

#include <QListView>
#include <QPushButton>
#include <QProgressBar>

class QModelIndex;
class QString;

class ProgressListDelegate::Private
{
public:
    Private(QListView *listView)
        : separatorPixels(0),
        leftMargin(0),
        rightMargin(0),
        minimumItemHeight(0),
        minimumContentWidth(0),
        editorHeight(0),
        iconWidth(0),
        listView(listView),
        progressBar(new QProgressBar(0))
    {
    }

    ~Private() {
        delete progressBar;
    }

    QString getIcon(const QModelIndex &index) const;
    QString getSizeTotal(const QModelIndex &index) const;
    QString getSizeProcessed(const QModelIndex &index) const;
    qlonglong getTimeTotal(const QModelIndex &index) const;
    qlonglong getTimeProcessed(const QModelIndex &index) const;
    QString getSpeed(const QModelIndex &index) const;
    int getPercent(const QModelIndex &index) const;
    QString getInfoMessage(const QModelIndex &index) const;
    int getCurrentLeftMargin(int fontHeight) const;

public:
    int separatorPixels;
    int leftMargin;
    int rightMargin;
    int minimumItemHeight;
    int minimumContentWidth;
    int editorHeight;
    int iconWidth;
    QListView *listView;
    QProgressBar *progressBar;
};

#endif // PROGRESSLISTDELEGATE_P_H
