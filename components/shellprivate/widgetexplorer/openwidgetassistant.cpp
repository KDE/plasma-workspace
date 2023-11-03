/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "openwidgetassistant_p.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include <KFileWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <QDebug>
#include <QUrl>

#include <KPackage/PackageJob>
#include <KPackage/PackageLoader>

namespace Plasma
{
OpenWidgetAssistant::OpenWidgetAssistant(QWidget *parent)
    : KAssistantDialog(parent)
    , m_fileWidget(nullptr)
    , m_filePageWidget(new QWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout(m_filePageWidget);
    m_fileWidget = new KFileWidget(QUrl(), m_filePageWidget);
    m_fileWidget->setOperationMode(KFileWidget::Opening);
    m_fileWidget->setMode(KFile::File | KFile::ExistingOnly);
    connect(this, SIGNAL(user1Clicked()), m_fileWidget, SLOT(slotOk()));
    connect(m_fileWidget, SIGNAL(accepted()), this, SLOT(finished()));
    layout->addWidget(m_fileWidget);

    m_fileWidget->setFilters({KFileFilter::fromMimeType(QStringLiteral("application/x-plasma"))});

    m_filePage = new KPageWidgetItem(m_filePageWidget, i18n("Select Plasmoid File"));
    addPage(m_filePage);

    resize(QSize(560, 400).expandedTo(minimumSizeHint()));
}

void OpenWidgetAssistant::slotHelpClicked()
{
    // enable it when doc will created
}

void OpenWidgetAssistant::finished()
{
    m_fileWidget->accept(); // how interesting .. accept() must be called before the state is set
    const QString packageFilePath = m_fileWidget->selectedFile();
    if (packageFilePath.isEmpty()) {
        // TODO: user visible error handling
        qDebug() << "hm. no file path?";
        return;
    }

    auto job = KPackage::PackageJob::install(QStringLiteral("Plasma/Applet"), packageFilePath);
    connect(job, &KJob::result, this, [packageFilePath, this](KJob *job) {
        if (job->error()) {
            KMessageBox::error(this, i18n("Installing the package %1 failed.", packageFilePath), i18n("Installation Failure"));
        }
    });
}

} // Plasma namespace
