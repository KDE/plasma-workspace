/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
    SPDX-FileCopyrightText: 2009 Ana Cecília Martins <anaceciliamb@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "widgetexplorer.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlProperty>

#include <KAuthorized>
#include <KLocalizedString>
#include <KNewStuff3/KNS3/QtQuickDialogWrapper>
#include <KX11Extras>

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>
#include <QStandardPaths>

#include <KActivities/Consumer>

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KPackage/PackageStructure>

#include "config-workspace.h"
#include "kcategorizeditemsviewmodels_p.h"
#include "openwidgetassistant_p.h"

using namespace KActivities;
using namespace KCategorizedItemsViewModels;
using namespace Plasma;

WidgetAction::WidgetAction(QObject *parent)
    : QAction(parent)
{
}

WidgetAction::WidgetAction(const QIcon &icon, const QString &text, QObject *parent)
    : QAction(icon, text, parent)
{
}

class WidgetExplorerPrivate
{
public:
    WidgetExplorerPrivate(WidgetExplorer *w)
        : q(w)
        , containment(nullptr)
        , itemModel(w)
        , filterModel(w)
        , activitiesConsumer(new KActivities::Consumer())
    {
        QObject::connect(activitiesConsumer.get(), &Consumer::currentActivityChanged, q, [this] {
            initRunningApplets();
        });
    }

    void initFilters();
    void initRunningApplets();
    void screenAdded(int screen);
    void screenRemoved(int screen);
    void containmentDestroyed();

    void addContainment(Containment *containment);
    void removeContainment(Containment *containment);

    /**
     * Tracks a new running applet
     */
    void appletAdded(Plasma::Applet *applet);

    /**
     * A running applet is no more
     */
    void appletRemoved(Plasma::Applet *applet);

    WidgetExplorer *q;
    QString application;
    Plasma::Containment *containment;

    QHash<QString, int> runningApplets; // applet name => count
    // extra hash so we can look up the names of deleted applets
    QHash<Plasma::Applet *, QString> appletNames;
    QPointer<Plasma::OpenWidgetAssistant> openAssistant;
    KPackage::Package *package;

    PlasmaAppletItemModel itemModel;
    KCategorizedItemsViewModels::DefaultFilterModel filterModel;
    bool showSpecialFilters = true;
    DefaultItemFilterProxyModel filterItemModel;
    static QPointer<KNS3::QtQuickDialogWrapper> newStuffDialog;

    std::unique_ptr<KActivities::Consumer> activitiesConsumer;
};

QPointer<KNS3::QtQuickDialogWrapper> WidgetExplorerPrivate::newStuffDialog;

void WidgetExplorerPrivate::initFilters()
{
    filterModel.clear();

    filterModel.addFilter(i18n("All Widgets"), KCategorizedItemsViewModels::Filter(), QIcon::fromTheme(QStringLiteral("plasma")));

    if (showSpecialFilters) {
        // Filters: Special
        filterModel.addFilter(i18n("Running"),
                              KCategorizedItemsViewModels::Filter(QStringLiteral("running"), true),
                              QIcon::fromTheme(QStringLiteral("dialog-ok")));

        filterModel.addFilter(i18nc("@item:inmenu used in the widget filter. Filter widgets that can be un-installed from the system, which are usually "
                                    "installed by the user to a local place.",
                                    "Uninstallable"),
                              KCategorizedItemsViewModels::Filter(QStringLiteral("local"), true),
                              QIcon::fromTheme(QStringLiteral("edit-delete")));

        filterModel.addSeparator(i18n("Categories:"));
    }

    const QSet<QString> existingCategories = itemModel.categories();
    QSet<QString> cats;
    const QList<KPluginMetaData> list = PluginLoader::self()->listAppletMetaData(QString());

    for (const auto &plugin : list) {
        if (!plugin.isValid()) {
            continue;
        }
        if (plugin.rawData().value(QStringLiteral("NoDisplay")).toBool() || plugin.category() == QLatin1String("Containments") || plugin.category().isEmpty()) {
            // we don't want to show the hidden category
            continue;
        }
        const QString c = plugin.category();
        if (cats.contains(c)) {
            continue;
        }
        cats.insert(c);

        const QString lowerCaseCat = c.toLower();
        if (!existingCategories.contains(lowerCaseCat)) {
            continue;
        }

        filterModel.addFilter(i18nd("plasmashellprivateplugin", c.toLocal8Bit()),
                              KCategorizedItemsViewModels::Filter(QStringLiteral("category"), lowerCaseCat));
    }
}

void WidgetExplorer::classBegin()
{
}

void WidgetExplorer::componentComplete()
{
    d->itemModel.setStartupCompleted(true);
    setApplication();
    d->initRunningApplets();
}

QObject *WidgetExplorer::widgetsModel() const
{
    return &d->filterItemModel;
}

QObject *WidgetExplorer::filterModel() const
{
    return &d->filterModel;
}

bool WidgetExplorer::showSpecialFilters() const
{
    return d->showSpecialFilters;
}

void WidgetExplorer::setShowSpecialFilters(bool show)
{
    if (d->showSpecialFilters != show) {
        d->showSpecialFilters = show;
        d->initFilters();
        Q_EMIT showSpecialFiltersChanged();
    }
}

QList<QObject *> WidgetExplorer::widgetsMenuActions()
{
    QList<QObject *> actionList;

    WidgetAction *action = nullptr;

    if (KAuthorized::authorize(KAuthorized::GHNS)) {
        action = new WidgetAction(QIcon::fromTheme(QStringLiteral("internet-services")), i18n("Download New Plasma Widgets"), this);
        connect(action, &QAction::triggered, this, &WidgetExplorer::downloadWidgets);
        actionList << action;
    }

    action = new WidgetAction(this);
    action->setSeparator(true);
    actionList << action;

    action = new WidgetAction(QIcon::fromTheme(QStringLiteral("package-x-generic")), i18n("Install Widget From Local File…"), this);
    QObject::connect(action, &QAction::triggered, this, &WidgetExplorer::openWidgetFile);
    actionList << action;

    return actionList;
}

void WidgetExplorerPrivate::initRunningApplets()
{
    // get applets from corona, count them, send results to model
    if (!containment) {
        return;
    }

    Plasma::Corona *c = containment->corona();

    // we've tried our best to get a corona
    // we don't want just one containment, we want them all
    if (!c) {
        qWarning() << "WidgetExplorer failed to find corona";
        return;
    }
    appletNames.clear();
    runningApplets.clear();

    QObject::connect(c, &Plasma::Corona::screenAdded, q, [this](int screen) {
        screenAdded(screen);
    });
    QObject::connect(c, &Plasma::Corona::screenRemoved, q, [this](int screen) {
        screenRemoved(screen);
    });

    const QList<Containment *> containments = c->containments();
    for (Containment *containment : containments) {
        if (containment->containmentType() == Plasma::Types::DesktopContainment && containment->activity() != activitiesConsumer->currentActivity()) {
            continue;
        }
        if (containment->screen() != -1) {
            addContainment(containment);
        }
    }

    // qDebug() << runningApplets;
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::screenAdded(int screen)
{
    const QList<Containment *> containments = containment->corona()->containments();
    for (auto c : containments) {
        if (c->screen() == screen) {
            addContainment(c);
        }
    }
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::screenRemoved(int screen)
{
    const QList<Containment *> containments = containment->corona()->containments();
    for (auto c : containments) {
        if (c->lastScreen() == screen) {
            removeContainment(c);
        }
    }
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::addContainment(Containment *containment)
{
    QObject::connect(containment, SIGNAL(appletAdded(Plasma::Applet *)), q, SLOT(appletAdded(Plasma::Applet *)));
    QObject::connect(containment, SIGNAL(appletRemoved(Plasma::Applet *)), q, SLOT(appletRemoved(Plasma::Applet *)));

    const QList<Applet *> applets = containment->applets();
    for (auto applet : applets) {
        if (applet->pluginMetaData().isValid()) {
            runningApplets[applet->pluginMetaData().pluginId()]++;
        } else {
            qDebug() << "Invalid plugin metadata. :(";
        }
    }
}

void WidgetExplorerPrivate::removeContainment(Plasma::Containment *containment)
{
    containment->disconnect(q);
    const QList<Applet *> applets = containment->applets();
    for (auto applet : applets) {
        if (applet->pluginMetaData().isValid()) {
            Containment *childContainment = applet->property("containment").value<Containment *>();
            if (childContainment) {
                removeContainment(childContainment);
            }
            runningApplets[applet->pluginMetaData().pluginId()]--;
        }
    }
}

void WidgetExplorerPrivate::containmentDestroyed()
{
    containment = nullptr;
}

void WidgetExplorerPrivate::appletAdded(Plasma::Applet *applet)
{
    if (!applet->pluginMetaData().isValid()) {
        return;
    }
    QString name = applet->pluginMetaData().pluginId();

    runningApplets[name]++;
    appletNames.insert(applet, name);
    itemModel.setRunningApplets(name, runningApplets[name]);
}

void WidgetExplorerPrivate::appletRemoved(Plasma::Applet *applet)
{
    QString name = appletNames.take(applet);

    int count = 0;
    if (runningApplets.contains(name)) {
        count = runningApplets[name] - 1;

        if (count < 1) {
            runningApplets.remove(name);
        } else {
            runningApplets[name] = count;
        }
    }

    itemModel.setRunningApplets(name, count);
}

// WidgetExplorer

WidgetExplorer::WidgetExplorer(QObject *parent)
    : QObject(parent)
    , d(new WidgetExplorerPrivate(this))
{
    d->filterItemModel.setSortCaseSensitivity(Qt::CaseInsensitive);
    d->filterItemModel.setDynamicSortFilter(true);
    d->filterItemModel.setSourceModel(&d->itemModel);
    d->filterItemModel.sort(0);
}

WidgetExplorer::~WidgetExplorer()
{
    delete d;
}

void WidgetExplorer::setApplication(const QString &app)
{
    if (d->application == app && !app.isEmpty()) {
        return;
    }

    d->application = app;
    d->itemModel.setApplication(app);
    d->initFilters();

    d->itemModel.setRunningApplets(d->runningApplets);
    Q_EMIT applicationChanged();
}

QString WidgetExplorer::application()
{
    return d->application;
}

QStringList WidgetExplorer::provides() const
{
    return d->itemModel.provides();
}

void WidgetExplorer::setProvides(const QStringList &provides)
{
    if (d->itemModel.provides() == provides) {
        return;
    }

    d->itemModel.setProvides(provides);
    Q_EMIT providesChanged();
}

void WidgetExplorer::setContainment(Plasma::Containment *containment)
{
    if (d->containment != containment) {
        if (d->containment) {
            d->containment->disconnect(this);
        }

        d->containment = containment;

        if (d->containment) {
            connect(d->containment, SIGNAL(destroyed(QObject *)), this, SLOT(containmentDestroyed()));
            connect(d->containment, &Applet::immutabilityChanged, this, &WidgetExplorer::immutabilityChanged);
        }

        d->initRunningApplets();
        Q_EMIT containmentChanged();
    }
}

Containment *WidgetExplorer::containment() const
{
    return d->containment;
}

Plasma::Corona *WidgetExplorer::corona() const
{
    if (d->containment) {
        return d->containment->corona();
    }

    return nullptr;
}

void WidgetExplorer::addApplet(const QString &pluginName)
{
    const QString p = PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/" + pluginName;
    qWarning() << "-------->  load applet: " << pluginName << " relpath: " << p;

    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, p, QStandardPaths::LocateDirectory);

    qDebug() << " .. pathes: " << dirs;
    if (!dirs.count()) {
        qWarning() << "Failed to find plasmoid path for " << pluginName;
        return;
    }

    if (d->containment) {
        d->containment->createApplet(dirs.first());
    }
}

void WidgetExplorer::immutabilityChanged(Plasma::Types::ImmutabilityType type)
{
    if (type != Plasma::Types::Mutable) {
        Q_EMIT shouldClose();
    }
}

void WidgetExplorer::downloadWidgets()
{
    if (WidgetExplorerPrivate::newStuffDialog.isNull()) {
        WidgetExplorerPrivate::newStuffDialog = new KNS3::QtQuickDialogWrapper(QStringLiteral("plasmoids.knsrc"));
        connect(WidgetExplorerPrivate::newStuffDialog, &KNS3::QtQuickDialogWrapper::closed, WidgetExplorerPrivate::newStuffDialog, &QObject::deleteLater);

        WidgetExplorerPrivate::newStuffDialog->open();
    }

    Q_EMIT shouldClose();
}

void WidgetExplorer::openWidgetFile()
{
    Plasma::OpenWidgetAssistant *assistant = d->openAssistant.data();
    if (!assistant) {
        assistant = new Plasma::OpenWidgetAssistant(nullptr);
        d->openAssistant = assistant;
    }

    KX11Extras::setOnDesktop(assistant->winId(), KX11Extras::currentDesktop());
    assistant->setAttribute(Qt::WA_DeleteOnClose, true);
    assistant->show();
    assistant->raise();
    assistant->setFocus();

    Q_EMIT shouldClose();
}

void WidgetExplorer::uninstall(const QString &pluginName)
{
    static const QString packageRoot =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/";

    KPackage::PackageStructure *structure = KPackage::PackageLoader::self()->loadPackageStructure(QStringLiteral("Plasma/Applet"));

    KPackage::Package pkg(structure);
    pkg.uninstall(pluginName, packageRoot);

    // FIXME: moreefficient way rather a linear scan?
    for (int i = 0; i < d->itemModel.rowCount(); ++i) {
        QStandardItem *item = d->itemModel.item(i);
        if (item->data(PlasmaAppletItemModel::PluginNameRole).toString() == pluginName) {
            d->itemModel.takeRow(i);
            break;
        }
    }

    // now remove all instances of that applet
    if (corona()) {
        const auto &containments = corona()->containments();

        for (Containment *c : containments) {
            const auto &applets = c->applets();

            for (Applet *applet : applets) {
                const auto &appletInfo = applet->pluginMetaData();

                if (appletInfo.isValid() && appletInfo.pluginId() == pluginName) {
                    applet->destroy();
                }
            }
        }
    }
}

#include "moc_widgetexplorer.cpp"
