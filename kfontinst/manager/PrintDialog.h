#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QComboBox>
#include <QDialog>

namespace KFI
{
class CPrintDialog : public QDialog
{
public:
    CPrintDialog(QWidget *parent);

    bool exec(int size);
    int chosenSize()
    {
        return m_size->currentIndex();
    }

private:
    QComboBox *m_size;
};

}
