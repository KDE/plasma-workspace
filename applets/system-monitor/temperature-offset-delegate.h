/*
 *   Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TEMPERATURE_OFFSET_DELEGATE_H
#define TEMPERATURE_OFFSET_DELEGATE_H

#include <QItemDelegate>

/**
 * Item delegate that uses a spinbox to edit real numbers.
 */
class TemperatureOffsetDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    TemperatureOffsetDelegate(QObject *parent = 0);

    /// reimplemented from QItemDelegate.
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
            const QModelIndex &index) const;
    /// reimplemented from QItemDelegate.
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    /// reimplemented from QItemDelegate.
    void setModelData(QWidget *editor, QAbstractItemModel *model,
            const QModelIndex &index) const;
    /// reimplemented from QItemDelegate.
    void updateEditorGeometry(QWidget *editor,
            const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // TEMPERATURE_OFFSET_DELEGATE_H
