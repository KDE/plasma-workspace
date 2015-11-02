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
#include <config-kscreenlocker.h>
#include <KActionCollection>
#include <KGlobalAccel>
#include <KCModule>
#include <KPluginFactory>
#include <KConfigDialogManager>
#include <QVBoxLayout>
#include <QQuickView>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMessageBox>
#include <QStandardItemModel>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

static const QString s_lockActionName = QStringLiteral("Lock Session");

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
    , m_actionCollection(new KActionCollection(this, QStringLiteral("ksmserver")))
    , m_ui(new ScreenLockerKcmForm(this))
{
    qmlRegisterType<QStandardItemModel>();
    KConfigDialogManager::changedMap()->insert(QStringLiteral("SelectImageButton"), SIGNAL(imagePathChanged(QString)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_ui);

    addConfig(KScreenSaverSettings::self(), m_ui);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[ScreenhotRole] = "screenshot";
    m_model->setItemRoleNames(roles);

    m_quickView = new QQuickView();
    QWidget *widget = QWidget::createWindowContainer(m_quickView, this);
    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Generic"));
    package.setDefaultPackageRoot(QStringLiteral("plasma/kcms"));
    package.setPath(QStringLiteral("screenlocker_kcm"));
    m_quickView->rootContext()->setContextProperty(QStringLiteral("kcm"), this);
    m_quickView->setSource(QUrl::fromLocalFile(package.filePath("mainscript")));
    setMinimumHeight(m_quickView->initialSize().height());

    layout->addWidget(widget);

    m_actionCollection->setConfigGlobal(true);
    QAction *a = m_actionCollection->addAction(s_lockActionName);
    a->setProperty("isConfigurationAction", true);
    m_ui->lockscreenShortcut->setCheckForConflictsAgainst(KKeySequenceWidget::None);
    a->setText(i18n("Lock Session"));
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>{Qt::ALT+Qt::CTRL+Qt::Key_L, Qt::Key_ScreenSaver});
    connect(m_ui->lockscreenShortcut, &KKeySequenceWidget::keySequenceChanged, this, &ScreenLockerKcm::shortcutChanged);
}

void ScreenLockerKcm::shortcutChanged(const QKeySequence &key)
{
    if (QAction *a = m_actionCollection->action(s_lockActionName)) {
        auto shortcuts = KGlobalAccel::self()->shortcut(a);
        m_ui->lockscreenShortcut->setProperty("changed", !shortcuts.contains(key));
    }
    changed();
}

QList<KPackage::Package> ScreenLockerKcm::availablePackages(const QString &component) const
{
    QList<KPackage::Package> packages;
    QStringList paths;
    const QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    for (const QString &path : dataPaths) {
        QDir dir(path + "/plasma/look-and-feel");
        paths << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    }

    for (const QString &path : paths) {
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        pkg.setPath(path);
        pkg.setFallbackPackage(KPackage::Package());
        if (component.isEmpty() || !pkg.filePath(component.toUtf8()).isEmpty()) {
            packages << pkg;
        }
    }

    return packages;
}

void ScreenLockerKcm::load()
{
    KCModule::load();

    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        m_package.setPath(packageName);
    }

    QString currentPlugin = KScreenSaverSettings::theme();
    if (currentPlugin.isEmpty()) {
        currentPlugin = m_package.metadata().pluginId();
    }
    setSelectedPlugin(currentPlugin);

    m_model->clear();
    const QList<KPackage::Package> pkgs = availablePackages(QStringLiteral("lockscreenmainscript"));
    for (const KPackage::Package &pkg : pkgs) {
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginId(), PluginNameRole);
        row->setData(pkg.filePath("previews", QStringLiteral("lockscreen.png")), ScreenhotRole);
        m_model->appendRow(row);
    }

    if (QAction *a = m_actionCollection->action(s_lockActionName)) {
        auto shortcuts = KGlobalAccel::self()->shortcut(a);
        if (!shortcuts.isEmpty()) {
            m_ui->lockscreenShortcut->setKeySequence(shortcuts.first());
        }
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
    if (plugin.isEmpty() || plugin == QLatin1String("none")) {
        return;
    }

    QProcess proc;
    QStringList arguments;
    arguments << plugin << QStringLiteral("--testing");
    if (proc.execute(KSCREENLOCKER_GREET_BIN, arguments)) {
        QMessageBox::critical(this, i18n("Error"), i18n("Failed to successfully test the screen locker."));
    }
}

void ScreenLockerKcm::save()
{
    if (!shouldSaveShortcut()) {
        QMetaObject::invokeMethod(this, "changed", Qt::QueuedConnection);
        return;
    }
    KCModule::save();

    KScreenSaverSettings::setTheme(m_selectedPlugin);

    KScreenSaverSettings::self()->save();
    if (m_ui->lockscreenShortcut->property("changed").toBool()) {
        if (QAction *a = m_actionCollection->action(s_lockActionName)) {
            KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>{m_ui->lockscreenShortcut->keySequence()}, KGlobalAccel::NoAutoloading);
            m_actionCollection->writeSettings();
        }
        m_ui->lockscreenShortcut->setProperty("changed", false);
    }
    // reconfigure through DBus
    OrgKdeScreensaverInterface interface(QStringLiteral("org.kde.screensaver"),
                                         QStringLiteral("/ScreenSaver"),
                                         QDBusConnection::sessionBus());
    if (interface.isValid()) {
        interface.configure();
    }
}

bool ScreenLockerKcm::shouldSaveShortcut()
{
    if (m_ui->lockscreenShortcut->property("changed").toBool()) {
        const QKeySequence &sequence = m_ui->lockscreenShortcut->keySequence();
        auto conflicting = KGlobalAccel::getGlobalShortcutsByKey(sequence);
        if (!conflicting.isEmpty()) {
            // Inform and ask the user about the conflict and reassigning
            // the keys sequence
            if (!KGlobalAccel::promptStealShortcutSystemwide(this, conflicting, sequence)) {
                return false;
            }
            KGlobalAccel::stealShortcutSystemwide(sequence);
        }
    }
    return true;
}

void ScreenLockerKcm::defaults()
{
    KCModule::defaults();
    m_ui->lockscreenShortcut->setKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_L);
}

K_PLUGIN_FACTORY_WITH_JSON(ScreenLockerKcmFactory,
                           "screenlocker.json",
                           registerPlugin<ScreenLockerKcm>();)

#include "kcm.moc"
