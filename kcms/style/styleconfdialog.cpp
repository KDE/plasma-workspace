/*
    KCMStyle's container dialog for custom style setup dialogs

    SPDX-FileCopyrightText: 2003 Maksim Orlovich <maksim.orlovich@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "styleconfdialog.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

StyleConfigDialog::StyleConfigDialog(QWidget *parent, const QString &styleName)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("StyleConfigDialog"));
    setModal(true);
    setWindowTitle(i18n("Configure %1", styleName));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *mainWidget = new QWidget(this);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, this);
    mainLayout->addWidget(mainWidget);

    mMainLayout = new QHBoxLayout(mainWidget);
    mMainLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &StyleConfigDialog::slotAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &StyleConfigDialog::defaults);
    mainLayout->addWidget(buttonBox);

    buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
    m_dirty = false;
}

bool StyleConfigDialog::isDirty() const
{
    return m_dirty;
}

void StyleConfigDialog::setDirty(bool dirty)
{
    m_dirty = dirty;
}

void StyleConfigDialog::slotAccept()
{
    Q_EMIT save();
    QDialog::accept();
}

void StyleConfigDialog::setMainWidget(QWidget *w)
{
    mMainLayout->addWidget(w);
}
