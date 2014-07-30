/* This file is part of the KDE Project
   Copyright (c) 2014 Marco Martin <mart@kde.org>
   Copyright (c) 2014 Vishesh Handa <me@vhanda.in>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcm.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#include <KLocalizedString>
#include <Plasma/Package>

K_PLUGIN_FACTORY(KCMSplashScreenFactory, registerPlugin<KCMSplashScreen>();)

KCMSplashScreen::KCMSplashScreen(QWidget* parent, const QVariantList& args)
    : KCModule(parent, args)
    , m_config("ksplashrc")
    , m_configGroup(m_config.group("KSplash"))
{
    KAboutData* about = new KAboutData("kcm_splashscreen", i18n("Configure Splash screen details"),
                                       "0.1", QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), "mart@kde.org");
    setAboutData(about);
    setButtons(Help | Apply | Default);

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSortingEnabled(true);
    connect(m_listWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this, SLOT(changed()));

    m_btnTest = new QPushButton( QIcon::fromTheme("document-preview"), i18n("Test Theme"), this );
    m_btnTest->setToolTip(i18n("Test the selected theme"));
    m_btnTest->setWhatsThis(i18n("This will test the selected theme."));
    //m_btnTest->setEnabled( false );
    connect(m_btnTest, SIGNAL(clicked()), SLOT(test()));

    layout->addWidget(m_listWidget);
    layout->addWidget(m_btnTest);
}


void KCMSplashScreen::load()
{
    QString currentPlugin = m_configGroup.readEntry("Theme", QString());
    if (currentPlugin.isEmpty()) {
        currentPlugin = m_access.metadata().pluginName();
    }

    m_listWidget->clear();
    QListWidgetItem* item = new QListWidgetItem(i18n("None"));
    m_listWidget->addItem(item);
    item->setData(Qt::UserRole + 1, "none");

    const QList<Plasma::Package> pkgs = LookAndFeelAccess::availablePackages("splashmainscript");
    for (const Plasma::Package &pkg : pkgs) {
        QListWidgetItem* item = new QListWidgetItem(pkg.metadata().name());
        item->setData(Qt::UserRole + 1, pkg.metadata().pluginName());
        m_listWidget->addItem(item);

        if (pkg.metadata().pluginName() == currentPlugin) {
            m_listWidget->setCurrentItem(item);
        }
    }
}


void KCMSplashScreen::save()
{
    QListWidgetItem* item = m_listWidget->currentItem();

    if (!item) {
        return;
    }

    const QString plugin = item->data(Qt::UserRole + 1).toString();

    if (plugin.isEmpty()) {
        return;
    } else if (plugin == "none") {
        m_configGroup.writeEntry("Theme", plugin);
        m_configGroup.writeEntry("Engine", "none");
    } else {
        m_configGroup.writeEntry("Theme", plugin);
        m_configGroup.writeEntry("Engine", "ksplashqml");
    }

    m_configGroup.sync();
}

void KCMSplashScreen::defaults()
{
    QListWidgetItem *item;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        item = m_listWidget->item(i);
        const QString plugin = item->data(Qt::UserRole + 1).toString();

        if (!plugin.isEmpty() && plugin == m_access.metadata().pluginName()) {
            m_listWidget->setCurrentItem(item);
            break;
        }
    }
}

void KCMSplashScreen::test()
{
    QListWidgetItem* item = m_listWidget->currentItem();

    if (!item) {
        return;
    }

    const QString plugin = item->data(Qt::UserRole + 1).toString();

    QProcess proc;
    QStringList arguments;
    arguments << plugin << "--test";
    if (proc.execute("ksplashqml", arguments)) {
        QMessageBox::critical(this, i18n("Error"), i18n("Failed to successfully test the splash screen."));
    }
}

#include "kcm.moc"
