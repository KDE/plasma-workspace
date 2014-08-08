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
#include <QQuickWidget>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>
#include <QStandardItemModel>

#include <KLocalizedString>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

K_PLUGIN_FACTORY(KCMLookandFeelFactory, registerPlugin<KCMLookandFeel>();)

KCMLookandFeel::KCMLookandFeel(QWidget* parent, const QVariantList& args)
    : KCModule(parent, args)
    , m_config("kdeglobals")
    , m_configGroup(m_config.group("KDE"))
{
    qmlRegisterType<QStandardItemModel>();
    KAboutData* about = new KAboutData("kcm_lookandfeel", i18n("Configure Splash screen details"),
                                       "0.1", QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), "mart@kde.org");
    setAboutData(about);
    setButtons(Help | Apply | Default);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[ScreenhotRole] = "screenshot";
    m_model->setItemRoleNames(roles);
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
    package.setDefaultPackageRoot("plasma/kcms");
    package.setPath("kcm_lookandfeel");
    m_quickWidget->rootContext()->setContextProperty("kcm", this);
    m_quickWidget->setSource(QUrl::fromLocalFile(package.filePath("mainscript")));
    setMinimumHeight(m_quickWidget->initialSize().height());

    layout->addWidget(m_quickWidget);
}

QStandardItemModel *KCMLookandFeel::lookAndFeelModel()
{
    return m_model;
}

QString KCMLookandFeel::selectedPlugin() const
{
    return m_selectedPlugin;
}

void KCMLookandFeel::setSelectedPlugin(const QString &plugin)
{
    if (m_selectedPlugin == plugin) {
        return;
    }

    m_selectedPlugin = plugin;
    emit selectedPluginChanged();
    changed();
}

void KCMLookandFeel::load()
{
    setSelectedPlugin(m_access.metadata().pluginName());

    m_model->clear();

    const QList<Plasma::Package> pkgs = LookAndFeelAccess::availablePackages();
    for (const Plasma::Package &pkg : pkgs) {qWarning()<<"EEEE";
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginName(), PluginNameRole);
        row->setData(pkg.filePath("screenshot"), ScreenhotRole);
        m_model->appendRow(row);
    }
}


void KCMLookandFeel::save()
{
    m_configGroup.writeEntry("LookAndFeelPackage", m_selectedPlugin);
    m_configGroup.sync();
}

void KCMLookandFeel::defaults()
{
    setSelectedPlugin(m_access.metadata().pluginName());
}

#include "kcm.moc"
