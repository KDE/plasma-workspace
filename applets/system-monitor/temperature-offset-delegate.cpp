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

#include "temperature-offset-delegate.h"

#include <knuminput.h>

TemperatureOffsetDelegate::TemperatureOffsetDelegate(QObject *parent) : QItemDelegate(parent)
{
}

QWidget *TemperatureOffsetDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
     const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    KDoubleNumInput *input = new KDoubleNumInput(parent);
    input->setMinimum(-1000);
    input->setMaximum(1000);
    input->setDecimals(1);
    input->setSingleStep(0.1);
    input->setSliderEnabled(false);

    return input;
}

void TemperatureOffsetDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KDoubleNumInput *input = static_cast<KDoubleNumInput*>(editor);
    input->setValue(index.model()->data(index, Qt::EditRole).toDouble());
}

void TemperatureOffsetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    KDoubleNumInput *input = static_cast<KDoubleNumInput*>(editor);
    model->setData(index, input->value(), Qt::EditRole);
}

void TemperatureOffsetDelegate::updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}
