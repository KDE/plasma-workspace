/*
 *   Copyright (C) 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "openwidgetassistant_p.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>

#include <QDebug>
#include <QUrl>
#include <kfilewidget.h>
#include <KMessageBox>
#include <KService>
#include <KServiceTypeTrader>
#include <KLocalizedString>

#include <KPackage/PackageStructure>
#include <KPackage/PackageLoader>

namespace Plasma
{

OpenWidgetAssistant::OpenWidgetAssistant(QWidget *parent)
    : KAssistantDialog(parent),
      m_fileWidget(nullptr),
      m_filePageWidget(nullptr)
{
    m_filePageWidget = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(m_filePageWidget);
    m_fileWidget = new KFileWidget(QUrl(), m_filePageWidget);
    m_fileWidget->setOperationMode(KFileWidget::Opening);
    m_fileWidget->setMode(KFile::File | KFile::ExistingOnly);
    connect(this, SIGNAL(user1Clicked()), m_fileWidget, SLOT(slotOk()));
    connect(m_fileWidget, SIGNAL(accepted()), this, SLOT(finished()));
    layout->addWidget(m_fileWidget);

    m_fileWidget->setFilter(QString());
    QStringList mimes;
    mimes << QStringLiteral("application/x-plasma");
    m_fileWidget->setMimeFilter(mimes);

    m_filePage = new KPageWidgetItem(m_filePageWidget, i18n("Select Plasmoid File"));
    addPage(m_filePage);

    resize(QSize(560, 400).expandedTo(minimumSizeHint()));
}

void OpenWidgetAssistant::slotHelpClicked()
{
    //enable it when doc will created
}

void OpenWidgetAssistant::finished()
{
    m_fileWidget->accept(); // how interesting .. accept() must be called before the state is set
    QString packageFilePath = m_fileWidget->selectedFile();
    if (packageFilePath.isEmpty()) {
        //TODO: user visible error handling
        qDebug() << "hm. no file path?";
        return;
    }

    KPackage::Package installer = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));

    if (!installer.install(packageFilePath)) {
        KMessageBox::error(this, i18n("Installing the package %1 failed.", packageFilePath),
                           i18n("Installation Failure"));
    }
}

} // Plasma namespace

