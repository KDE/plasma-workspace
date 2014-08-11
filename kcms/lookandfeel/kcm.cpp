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
    roles[HasSplashRole] = "hasSplash";
    roles[HasLockScreenRole] = "hasLockScreen";
    roles[HasRunCommandRole] = "hasRunCommand";
    roles[HasLogoutRole] = "hasLogout";

    roles[HasColorsRole] = "hasColors";
    roles[HasWidgetStyleRole] = "hasWidgetStyle";
    roles[HasIconsRole] = "hasIcons";
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
    for (const Plasma::Package &pkg : pkgs) {
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginName(), PluginNameRole);
        row->setData(pkg.filePath("screenshot"), ScreenhotRole);

        //What the package provides
        row->setData(!pkg.filePath("splashmainscript").isEmpty(), HasSplashRole);
        row->setData(!pkg.filePath("lockscreenmainscript").isEmpty(), HasLockScreenRole);
        row->setData(!pkg.filePath("runcommandmainscript").isEmpty(), HasRunCommandRole);
        row->setData(!pkg.filePath("logoutmainscript").isEmpty(), HasLogoutRole);

        if (!pkg.filePath("defaults").isEmpty()) {
            KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("defaults"));
            KConfigGroup cg(conf, "KDE");
            bool hasColors = !cg.readEntry("ColorScheme", QString()).isEmpty();
            row->setData(hasColors, HasColorsRole);
            if (!hasColors) {
                hasColors = !pkg.filePath("colors").isEmpty();
            }
            row->setData(!cg.readEntry("widgetStyle", QString()).isEmpty(), HasWidgetStyleRole);
            cg = KConfigGroup(conf, "Icons");
            row->setData(!cg.readEntry("Theme", QString()).isEmpty(), HasIconsRole);
        }

        m_model->appendRow(row);
    }
}


void KCMLookandFeel::save()
{
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    package.setPath(m_selectedPlugin);

    if (!package.isValid()) {
        return;
    }

    m_configGroup.writeEntry("LookAndFeelPackage", m_selectedPlugin);

    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "KDE");
        setWidgetStyle(cg.readEntry("widgetStyle", QString()));

        QString colorsFile = package.filePath("colors");
        QString colorScheme = cg.readEntry("ColorScheme", QString());
        if (!colorsFile.isEmpty()) {
            setColors(colorsFile);
        } else if (!colorScheme.isEmpty()) {
            colorScheme.remove('\''); // So Foo's does not become FooS
            QRegExp fixer("[\\W,.-]+(.?)");
            int offset;
            while ((offset = fixer.indexIn(colorScheme)) >= 0)
                colorScheme.replace(offset, fixer.matchedLength(), fixer.cap(1).toUpper());
            colorScheme.replace(0, 1, colorScheme.at(0).toUpper());
            QString src = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "color-schemes/" +  colorScheme + ".colors");
            setColors(src);
        }

        cg = KConfigGroup(conf, "Icons");
        setIcons(cg.readEntry("Theme", QString()));
    }

    m_configGroup.sync();
}

void KCMLookandFeel::defaults()
{
    setSelectedPlugin(m_access.metadata().pluginName());
}

void KCMLookandFeel::setWidgetStyle(const QString &style)
{
    //TODO
}

void KCMLookandFeel::setColors(const QString &colorFile)
{
    //TODO
}

void KCMLookandFeel::setIcons(const QString &theme)
{
    //TODO
}

#include "kcm.moc"
