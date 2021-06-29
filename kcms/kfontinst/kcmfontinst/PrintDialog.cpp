/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PrintDialog.h"
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace KFI
{
CPrintDialog::CPrintDialog(QWidget *parent)
    : QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Print Font Samples"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CPrintDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CPrintDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QFrame *page = new QFrame(this);
    QGridLayout *layout = new QGridLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *lbl = new QLabel(i18n("Select size to print font:"), page);
    lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    layout->addWidget(lbl, 0, 0);
    itsSize = new QComboBox(page);
    itsSize->insertItem(0, i18n("Waterfall"));
    itsSize->insertItem(1, i18n("12pt"));
    itsSize->insertItem(2, i18n("18pt"));
    itsSize->insertItem(3, i18n("24pt"));
    itsSize->insertItem(4, i18n("36pt"));
    itsSize->insertItem(5, i18n("48pt"));
    itsSize->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    layout->addWidget(itsSize, 0, 1);
    layout->addItem(new QSpacerItem(2, 2), 2, 0);

    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);
}

bool CPrintDialog::exec(int size)
{
    itsSize->setCurrentIndex(size);
    return QDialog::Accepted == QDialog::exec();
}

}
