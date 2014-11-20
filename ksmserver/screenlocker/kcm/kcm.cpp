/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "kcm.h"
#include "kscreensaversettings.h"
#include "ui_kcm.h"
#include "screenlocker_interface.h"
#include <config-ksmserver.h>
#include <KCModule>
#include <KPluginFactory>
#include <QVBoxLayout>
#include <QQuickView>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMessageBox>
#include <QStandardItemModel>

#include <Plasma/Package>
#include <Plasma/PluginLoader>

class ScreenLockerKcmForm : public QWidget, public Ui::ScreenLockerKcmForm
{
    Q_OBJECT
public:
    explicit ScreenLockerKcmForm(QWidget *parent);
};

ScreenLockerKcmForm::ScreenLockerKcmForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}



ScreenLockerKcm::ScreenLockerKcm(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
{
    qmlRegisterType<QStandardItemModel>();
    ScreenLockerKcmForm *ui = new ScreenLockerKcmForm(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(ui);

    addConfig(KScreenSaverSettings::self(), ui);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[ScreenhotRole] = "screenshot";
    m_model->setItemRoleNames(roles);

    m_quickView = new QQuickView();
    QWidget *widget = QWidget::createWindowContainer(m_quickView, this);
    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
    package.setDefaultPackageRoot("plasma/kcms");
    package.setPath("screenlocker_kcm");
    m_quickView->rootContext()->setContextProperty("kcm", this);
    m_quickView->setSource(QUrl::fromLocalFile(package.filePath("mainscript")));
    setMinimumHeight(m_quickView->initialSize().height());

    layout->addWidget(widget);
}

QList<Plasma::Package> ScreenLockerKcm::availablePackages(const QString &component) const
{
    QList<Plasma::Package> packages;
    QStringList paths;
    const QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    for (const QString &path : dataPaths) {
        QDir dir(path + "/plasma/look-and-feel");
        paths << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    }

    for (const QString &path : paths) {
        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
        pkg.setPath(path);
        pkg.setFallbackPackage(Plasma::Package());
        if (component.isEmpty() || !pkg.filePath(component.toUtf8()).isEmpty()) {
            packages << pkg;
        }
    }

    return packages;
}

void ScreenLockerKcm::load()
{
    KCModule::load();

    m_package = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    KConfigGroup cg(KSharedConfig::openConfig("kdeglobals"), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        m_package.setPath(packageName);
    }

    QString currentPlugin = KScreenSaverSettings::theme();
    if (currentPlugin.isEmpty()) {
        currentPlugin = m_package.metadata().pluginName();
    }
    setSelectedPlugin(currentPlugin);

    m_model->clear();
    const QList<Plasma::Package> pkgs = availablePackages("lockscreenmainscript");
    for (const Plasma::Package &pkg : pkgs) {
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginName(), PluginNameRole);
        row->setData(pkg.filePath("previews", "lockscreen.png"), ScreenhotRole);
        m_model->appendRow(row);
    }
}

QStandardItemModel *ScreenLockerKcm::lockerModel()
{
    return m_model;
}

QString ScreenLockerKcm::selectedPlugin() const
{
    return m_selectedPlugin;
}

void ScreenLockerKcm::setSelectedPlugin(const QString &plugin)
{
    if (m_selectedPlugin == plugin) {
        return;
    }

    m_selectedPlugin = plugin;
    emit selectedPluginChanged();
    changed();
}

void ScreenLockerKcm::test(const QString &plugin)
{
    if (plugin.isEmpty() || plugin == "none") {
        return;
    }

    QProcess proc;
    QStringList arguments;
    arguments << plugin << "--testing";
    if (proc.execute(KSCREENLOCKER_GREET_BIN, arguments)) {
        QMessageBox::critical(this, i18n("Error"), i18n("Failed to successfully test the screen locker."));
    }
}

void ScreenLockerKcm::save()
{
    KCModule::save();

    KScreenSaverSettings::setTheme(m_selectedPlugin);

    KScreenSaverSettings::self()->save();
    // reconfigure through DBus
    OrgKdeScreensaverInterface interface(QStringLiteral("org.kde.screensaver"),
                                         QStringLiteral("/ScreenSaver"),
                                         QDBusConnection::sessionBus());
    if (interface.isValid()) {
        interface.configure();
    }
}

K_PLUGIN_FACTORY_WITH_JSON(ScreenLockerKcmFactory,
                           "screenlocker.json",
                           registerPlugin<ScreenLockerKcm>();)

#include "kcm.moc"
