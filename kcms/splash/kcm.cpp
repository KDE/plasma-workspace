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
#include "splashmodel.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>
#include <QQuickWidget>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>

#include <KLocalizedString>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

K_PLUGIN_FACTORY(KCMSplashScreenFactory, registerPlugin<KCMSplashScreen>();)

KCMSplashScreen::KCMSplashScreen(QWidget* parent, const QVariantList& args)
    : KCModule(parent, args)
    , m_config("ksplashrc")
    , m_configGroup(m_config.group("KSplash"))
{
    qmlRegisterType<SplashModel>();
    KAboutData* about = new KAboutData("kcm_splashscreen", i18n("Configure Splash screen details"),
                                       "0.1", QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), "mart@kde.org");
    setAboutData(about);
    setButtons(Help | Apply | Default);

    m_model = new SplashModel(this);
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
    package.setDefaultPackageRoot("plasma/kcms");
    package.setPath("kcm_splashscreen");
    m_quickWidget->rootContext()->setContextProperty("kcm", this);
    m_quickWidget->setSource(QUrl::fromLocalFile(package.filePath("mainscript")));

    layout->addWidget(m_quickWidget);
}

SplashModel *KCMSplashScreen::splashModel()
{
    return m_model;
}

QString KCMSplashScreen::selectedPlugin() const
{
    return m_selectedPlugin;
}

void KCMSplashScreen::setSelectedPlugin(const QString &plugin)
{
    if (m_selectedPlugin == plugin) {
        return;
    }

    m_selectedPlugin = plugin;
    emit selectedPluginChanged();
    changed();
}

void KCMSplashScreen::load()
{
    QString currentPlugin = m_configGroup.readEntry("Theme", QString());
    if (currentPlugin.isEmpty()) {
        currentPlugin = m_access.metadata().pluginName();
    }
    setSelectedPlugin(currentPlugin);

    m_model->clear();

    QStandardItem* row = new QStandardItem(i18n("None"));
    row->setData("none", SplashModel::PluginNameRole);
    m_model->appendRow(row);

    const QList<Plasma::Package> pkgs = LookAndFeelAccess::availablePackages("splashmainscript");
    for (const Plasma::Package &pkg : pkgs) {
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginName(), SplashModel::PluginNameRole);
        row->setData(pkg.filePath("splash", "screenshot.png"), SplashModel::ScreenhotRole);
        m_model->appendRow(row);
    }
}


void KCMSplashScreen::save()
{
    if (m_selectedPlugin.isEmpty()) {
        return;
    } else if (m_selectedPlugin == "none") {
        m_configGroup.writeEntry("Theme", m_selectedPlugin);
        m_configGroup.writeEntry("Engine", "none");
    } else {
        m_configGroup.writeEntry("Theme", m_selectedPlugin);
        m_configGroup.writeEntry("Engine", "ksplashqml");
    }

    m_configGroup.sync();
}

void KCMSplashScreen::defaults()
{
    setSelectedPlugin(m_access.metadata().pluginName());
}

void KCMSplashScreen::test(const QString &plugin)
{
    if (plugin.isEmpty() || plugin == "none") {
        return;
    }

    QProcess proc;
    QStringList arguments;
    arguments << plugin << "--test";
    if (proc.execute("ksplashqml", arguments)) {
        QMessageBox::critical(this, i18n("Error"), i18n("Failed to successfully test the splash screen."));
    }
}

#include "kcm.moc"
