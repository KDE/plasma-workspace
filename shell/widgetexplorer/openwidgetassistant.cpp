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

#include <Plasma/PackageStructure>
#include <Plasma/PluginLoader>

namespace Plasma
{

OpenWidgetAssistant::OpenWidgetAssistant(QWidget *parent)
    : KAssistantDialog(parent),
      m_fileDialog(0),
      m_filePageWidget(0)
{
    QWidget *selectWidget = new QWidget(this);
    QVBoxLayout *selectLayout = new QVBoxLayout(selectWidget);
    QLabel *selectLabel = new QLabel(selectWidget);
    selectLabel->setText(i18n("Select the type of widget to install from the list below."));
    m_widgetTypeList = new QListWidget(selectWidget);
    m_widgetTypeList->setSelectionMode(QAbstractItemView::SingleSelection);
    //m_widgetTypeList->setSelectionBehavior(QAbstractItemView::SelectItems);
    connect(m_widgetTypeList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(next()));
    connect(m_widgetTypeList, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemChanged()));

    QString constraint("'Applet' in [X-Plasma-ComponentTypes] and exist [X-Plasma-PackageFormat]");
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/ScriptEngine", constraint);

    QListWidgetItem * item = new QListWidgetItem(QIcon::fromTheme("plasma"), i18n("Plasmoid: Native plasma widget"), m_widgetTypeList);
    item->setSelected(true);
    m_widgetTypeList->setCurrentItem(item);

    foreach (const KService::Ptr &offer, offers) {
        QString text(offer->name());
        if (!offer->comment().isEmpty()) {
            text.append(": ").append(offer->comment());
        }

        item = new QListWidgetItem(text, m_widgetTypeList);
        item->setData(PackageStructureRole, offer->property("X-KDE-PluginInfo-Name"));

        if (!offer->icon().isEmpty()) {
            item->setIcon(QIcon::fromTheme(offer->icon()));
        }
    }

    selectLayout->addWidget(selectLabel);
    selectLayout->addWidget(m_widgetTypeList);

    m_typePage = new KPageWidgetItem(selectWidget, i18n("Install New Widget From File"));
    m_typePage->setIcon(QIcon::fromTheme("plasma"));
    addPage(m_typePage);

    m_filePageWidget = new QWidget(this);
    m_filePage = new KPageWidgetItem(m_filePageWidget, i18n("Select File"));
    addPage(m_filePage);

    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)), SLOT(prepPage(KPageWidgetItem*,KPageWidgetItem*)));

    //connect( this, SIGNAL(helpClicked()), this, SLOT(slotHelpClicked()) );
    //m_widgetTypeList->setFocus();
    resize(QSize(560, 400).expandedTo(minimumSizeHint()));
}


void OpenWidgetAssistant::prepPage(KPageWidgetItem *current, KPageWidgetItem *before)
{
    Q_UNUSED(before);
    if (m_widgetTypeList->selectedItems().isEmpty()) {
        return;
    }

    if (current != m_filePage) {
        return;
    }

    if (!m_fileDialog) {
        QVBoxLayout *layout = new QVBoxLayout(m_filePageWidget);
        m_fileDialog = new KFileWidget(QUrl(), m_filePageWidget);
        m_fileDialog->setOperationMode(KFileWidget::Opening);
        m_fileDialog->setMode(KFile::File | KFile::ExistingOnly);
        connect(this, SIGNAL(user1Clicked()), m_fileDialog, SLOT(slotOk()));
        connect(m_fileDialog, SIGNAL(accepted()), this, SLOT(finished()));
        //m_fileDialog->setWindowFlags(Qt::Widget);
        layout->addWidget(m_fileDialog);
    }

    QListWidgetItem *item = m_widgetTypeList->selectedItems().first();
    Q_ASSERT(item);

    QString type = item->data(PackageStructureRole).toString();

    m_fileDialog->setFilter(QString());
    if (!type.isEmpty()) {
        QString constraint = QString("'%1' == [X-KDE-PluginInfo-Name]").arg(type);
        KService::List offers = KServiceTypeTrader::self()->query("Plasma/PackageStructure", constraint);

        qDebug() << "looking for a Plasma/PackageStructure with" << constraint << type;
        Q_ASSERT(offers.count() > 0);

    } else {
        QStringList mimes;
        mimes << "application/x-plasma";
        m_fileDialog->setMimeFilter(mimes);
    }
}

void OpenWidgetAssistant::slotHelpClicked()
{
    //enable it when doc will created
}

void OpenWidgetAssistant::finished()
{
    m_fileDialog->accept(); // how interesting .. accept() must be called before the state is set
    QString packageFilePath = m_fileDialog->selectedFile();
    if (packageFilePath.isEmpty()) {
        //TODO: user visible error handling
        qDebug() << "hm. no file path?";
        return;
    }

    Plasma::Package installer = Plasma::PluginLoader::self()->loadPackage("Plasma/Plasmoid");

    if (!installer.install(packageFilePath)) {
        KMessageBox::error(this, i18n("Installing the package %1 failed.", packageFilePath),
                           i18n("Installation Failure"));
    }
}

} // Plasma namespace

#include "openwidgetassistant.moc"
