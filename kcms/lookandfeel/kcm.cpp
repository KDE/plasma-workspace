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
    , m_applyColors(true)
    , m_applyWidgetStyle(true)
    , m_applyIcons(true)
    , m_applyPlasmaTheme(true)
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
        row->setData(pkg.filePath("previews", "preview.png"), ScreenhotRole);

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

            cg = KConfigGroup(conf, "PlasmaTheme");
            row->setData(!cg.readEntry("name", QString()).isEmpty(), HasPlasmaThemeRole);
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
        if (m_applyWidgetStyle) {
            setWidgetStyle(cg.readEntry("widgetStyle", QString()));
        }

        if (m_applyColors) {
            QString colorsFile = package.filePath("colors");
            QString colorScheme = cg.readEntry("ColorScheme", QString());
            if (!colorsFile.isEmpty()) {
                if (!colorScheme.isEmpty()) {
                    setColors(colorScheme, colorsFile);
                } else {
                    setColors(package.metadata().name(), colorsFile);
                }
            } else if (!colorScheme.isEmpty()) {
                colorScheme.remove('\''); // So Foo's does not become FooS
                QRegExp fixer("[\\W,.-]+(.?)");
                int offset;
                while ((offset = fixer.indexIn(colorScheme)) >= 0)
                    colorScheme.replace(offset, fixer.matchedLength(), fixer.cap(1).toUpper());
                colorScheme.replace(0, 1, colorScheme.at(0).toUpper());
                QString src = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "color-schemes/" +  colorScheme + ".colors");
                setColors(colorScheme, src);
            }
        }

        if (m_applyIcons) {
            cg = KConfigGroup(conf, "Icons");
            setIcons(cg.readEntry("Theme", QString()));
        }

        if (m_applyPlasmaTheme) {
            cg = KConfigGroup(conf, "Theme");
            setPlasmaTheme(cg.readEntry("name", QString()));
        }
    }

    m_configGroup.sync();
}

void KCMLookandFeel::defaults()
{
    setSelectedPlugin(m_access.metadata().pluginName());
}

void KCMLookandFeel::setWidgetStyle(const QString &style)
{
    if (style.isEmpty()) {
        return;
    }

    m_configGroup.writeEntry("widgetStyle", style);
    m_configGroup.sync();
}

void KCMLookandFeel::setColors(const QString &scheme, const QString &colorFile)
{
    if (scheme.isEmpty() || colorFile.isEmpty()) {
        return;
    }

    m_configGroup.writeEntry("ColorScheme", scheme);

    KSharedConfigPtr conf = KSharedConfig::openConfig(colorFile);
    foreach (const QString &grp, conf->groupList()) {
      KConfigGroup cg(conf, grp);
      KConfigGroup cg2(&m_config, grp);
      cg.copyTo(&cg2);
  }
}

void KCMLookandFeel::setIcons(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KConfigGroup cg(&m_config, "Icons");
    cg.writeEntry("Theme", theme);
}

void KCMLookandFeel::setPlasmaTheme(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KConfig config("plasmarc");
    KConfigGroup cg(&config, "Theme");
    cg.writeEntry("name", theme);
    cg.sync();
}

void KCMLookandFeel::setApplyColors(bool apply)
{
    if (m_applyColors == apply) {
        return;
    }

    m_applyColors = apply;
    emit applyColorsChanged();
}

bool KCMLookandFeel::applyColors() const
{
    return m_applyColors;
}

void KCMLookandFeel::setApplyWidgetStyle(bool apply)
{
    if (m_applyWidgetStyle == apply) {
        return;
    }

    m_applyWidgetStyle = apply;
    emit applyWidgetStyleChanged();
}

bool KCMLookandFeel::applyWidgetStyle() const
{
    return m_applyWidgetStyle;
}

void KCMLookandFeel::setApplyIcons(bool apply)
{
    if (m_applyIcons == apply) {
        return;
    }

    m_applyIcons = apply;
    emit applyIconsChanged();
}

bool KCMLookandFeel::applyIcons() const
{
    return m_applyIcons;
}

void KCMLookandFeel::setApplyPlasmaTheme(bool apply)
{
    if (m_applyPlasmaTheme == apply) {
        return;
    }

    m_applyPlasmaTheme = apply;
    emit applyPlasmaThemeChanged();
}

bool KCMLookandFeel::applyPlasmaTheme() const
{
    return m_applyPlasmaTheme;
}

#include "kcm.moc"
