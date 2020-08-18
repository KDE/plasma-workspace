/*
 *   Copyright (C) 2007 by Ivan Cukic <ivan.cukic+kde@gmail.com>
 *   Copyright (C) 2009 by Ana Cecília Martins <anaceciliamb@gmail.com>
 *   Copyright 2013 by Sebastian Kügler <sebas@kde.org>
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

#include "widgetexplorer.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQmlProperty>

#include <KAuthorized>
#include <klocalizedstring.h>
#include <KNewStuff3/KNS3/DownloadDialog>
#include <KWindowSystem>

#include <Plasma/Applet>
#include <Plasma/Corona>
#include <Plasma/Containment>
#include <Plasma/PluginLoader>
#include <qstandardpaths.h>

#include <KActivities/Consumer>

#include <KPackage/Package>
#include <KPackage/PackageStructure>
#include <KPackage/PackageLoader>

#include "kcategorizeditemsviewmodels_p.h"
#include "plasmaappletitemmodel_p.h"
#include "openwidgetassistant_p.h"
#include "config-workspace.h"

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
        : q(w),
          containment(nullptr),
          itemModel(w),
          filterModel(w),
          activitiesConsumer(new KActivities::Consumer())
    {
        QObject::connect(activitiesConsumer.data(), &Consumer::currentActivityChanged, q, [this] {
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
    //extra hash so we can look up the names of deleted applets
    QHash<Plasma::Applet *,QString> appletNames;
    QPointer<Plasma::OpenWidgetAssistant> openAssistant;
    KPackage::Package *package;

    PlasmaAppletItemModel itemModel;
    KCategorizedItemsViewModels::DefaultFilterModel filterModel;
    bool showSpecialFilters = true;
    DefaultItemFilterProxyModel filterItemModel;
    QPointer<KNS3::DownloadDialog> newStuffDialog;

    QScopedPointer<KActivities::Consumer> activitiesConsumer;
};

void WidgetExplorerPrivate::initFilters()
{
    filterModel.clear();

    filterModel.addFilter(i18n("All Widgets"),
                        KCategorizedItemsViewModels::Filter(), QIcon::fromTheme(QStringLiteral("plasma")));

    if (showSpecialFilters) {
        // Filters: Special
        filterModel.addFilter(i18n("Running"),
                            KCategorizedItemsViewModels::Filter(QStringLiteral("running"), true),
                            QIcon::fromTheme(QStringLiteral("dialog-ok")));

        filterModel.addFilter(i18n("Uninstallable"),
                            KCategorizedItemsViewModels::Filter(QStringLiteral("local"), true),
                            QIcon::fromTheme(QStringLiteral("edit-delete")));

        filterModel.addSeparator(i18n("Categories:"));
    }

    typedef QPair<QString, QString> catPair;
    QMap<QString, catPair > categories;
    QSet<QString> existingCategories = itemModel.categories();
    //foreach (const QString &category, Plasma::Applet::listCategories(application)) {
    QStringList cats;
    const QList<KPluginMetaData> list = PluginLoader::self()->listAppletMetaData(QString());

    for (auto& plugin : list) {
        if (!plugin.isValid()) {
            continue;
        }
        if (plugin.rawData().value("NoDisplay").toBool() || plugin.category() == QLatin1String("Containments") ||
            plugin.category().isEmpty()) {
            // we don't want to show the hidden category
            continue;
        }
        const QString c = plugin.category();
        if (-1 == cats.indexOf(c)) {
            cats << c;
        }
    }
    qWarning() << "TODO: port listCategories()";
    foreach (const QString &category, cats) {
        const QString lowerCaseCat = category.toLower();
        if (existingCategories.contains(lowerCaseCat)) {
            const QString trans = i18nd("libplasma5", category.toLocal8Bit());
            categories.insert(trans.toLower(), qMakePair(trans, lowerCaseCat));
        }
    }

    foreach (const catPair &category, categories) {
        filterModel.addFilter(category.first,
                              KCategorizedItemsViewModels::Filter(QStringLiteral("category"), category.second));
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
        emit showSpecialFiltersChanged();
    }
}

QList <QObject *>  WidgetExplorer::widgetsMenuActions()
{
    QList <QObject *> actionList;

    WidgetAction *action = nullptr;

    if (KAuthorized::authorize(QStringLiteral("ghns"))) {
        action = new WidgetAction(QIcon::fromTheme(QStringLiteral("internet-services")),
                                      i18n("Download New Plasma Widgets"), this);
        connect(action, &QAction::triggered, this, &WidgetExplorer::downloadWidgets);
        actionList << action;
    }

    action = new WidgetAction(this);
    action->setSeparator(true);
    actionList << action;

    action = new WidgetAction(QIcon::fromTheme(QStringLiteral("package-x-generic")),
                         i18n("Install Widget From Local File..."), this);
    QObject::connect(action, &QAction::triggered, this, &WidgetExplorer::openWidgetFile);
    actionList << action;

    return actionList;
}

void WidgetExplorerPrivate::initRunningApplets()
{
    //get applets from corona, count them, send results to model
    if (!containment) {
        return;
    }

    Plasma::Corona *c = containment->corona();

    //we've tried our best to get a corona
    //we don't want just one containment, we want them all
    if (!c) {
        qWarning() << "WidgetExplorer failed to find corona";
        return;
    }
    appletNames.clear();
    runningApplets.clear();

    QObject::connect(c, &Plasma::Corona::screenAdded, q, [this] (int screen) {screenAdded(screen);});
    QObject::connect(c, &Plasma::Corona::screenRemoved, q, [this] (int screen) {screenRemoved(screen);});

    const QList<Containment*> containments = c->containments();
    for (Containment *containment : containments) {
        if (containment->containmentType() == Plasma::Types::DesktopContainment
            && containment->activity() != activitiesConsumer->currentActivity()) {
            continue;
        }
        if (containment->screen() != -1) {
            addContainment(containment);
        }
    }

    //qDebug() << runningApplets;
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::screenAdded(int screen)
{
    const QList<Containment*> containments = containment->corona()->containments();
    for (auto c : containments) {
        if (c->screen() == screen) {
            addContainment(c);
        }
    }
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::screenRemoved(int screen)
{
    const QList<Containment*> containments = containment->corona()->containments();
        for (auto c : containments) {
        if (c->lastScreen() == screen) {
            removeContainment(c);
        }
    }
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::addContainment(Containment *containment)
{
    QObject::connect(containment, SIGNAL(appletAdded(Plasma::Applet*)), q, SLOT(appletAdded(Plasma::Applet*)));
    QObject::connect(containment, SIGNAL(appletRemoved(Plasma::Applet*)), q, SLOT(appletRemoved(Plasma::Applet*)));

    foreach (Applet *applet, containment->applets()) {
        if (applet->pluginMetaData().isValid()) {
            Containment *childContainment = applet->property("containment").value<Containment*>();
            if (childContainment) {
                addContainment(childContainment);
            }
            runningApplets[applet->pluginMetaData().pluginId()]++;
        } else {
            qDebug() << "Invalid plugin metadata. :(";
        }
    }
}

void WidgetExplorerPrivate::removeContainment(Plasma::Containment *containment)
{
    containment->disconnect(q);
    const QList<Applet*> applets = containment->applets();
    for (auto applet : applets) {
        if (applet->pluginMetaData().isValid()) {
            Containment *childContainment = applet->property("containment").value<Containment*>();
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

//WidgetExplorer

WidgetExplorer::WidgetExplorer(QObject *parent)
        : QObject(parent),
          d(new WidgetExplorerPrivate(this))
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
    emit applicationChanged();
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
    emit providesChanged();
}


void WidgetExplorer::setContainment(Plasma::Containment *containment)
{
    if (d->containment != containment) {
        if (d->containment) {
            d->containment->disconnect(this);
        }

        d->containment = containment;

        if (d->containment) {
            connect(d->containment, SIGNAL(destroyed(QObject*)), this, SLOT(containmentDestroyed()));
            connect(d->containment, &Applet::immutabilityChanged, this, &WidgetExplorer::immutabilityChanged);

        }

        d->initRunningApplets();
        emit containmentChanged();
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
    const QString p = PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/"+pluginName;
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
        emit shouldClose();
    }
}



void WidgetExplorer::downloadWidgets()
{
    if (!d->newStuffDialog) {
        d->newStuffDialog = new KNS3::DownloadDialog(QLatin1String("plasmoids.knsrc"));
        d->newStuffDialog.data()->setWindowTitle(i18n("Download New Plasma Widgets"));
        d->newStuffDialog.data()->setAttribute(Qt::WA_DeleteOnClose);
    }
    d->newStuffDialog.data()->show();

    emit shouldClose();
}

void WidgetExplorer::openWidgetFile()
{

    Plasma::OpenWidgetAssistant *assistant = d->openAssistant.data();
    if (!assistant) {
        assistant = new Plasma::OpenWidgetAssistant(nullptr);
        d->openAssistant = assistant;
    }

    KWindowSystem::setOnDesktop(assistant->winId(), KWindowSystem::currentDesktop());
    assistant->setAttribute(Qt::WA_DeleteOnClose, true);
    assistant->show();
    assistant->raise();
    assistant->setFocus();

    emit shouldClose();
}

void WidgetExplorer::uninstall(const QString &pluginName)
{
    static const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/";

    KPackage::PackageStructure *structure = KPackage::PackageLoader::self()->loadPackageStructure(QStringLiteral("Plasma/Applet"));

    KPackage::Package pkg(structure);
    pkg.uninstall(pluginName, packageRoot);

    //FIXME: moreefficient way rather a linear scan?
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

        foreach (Containment *c, containments) {
            const auto &applets = c->applets();

            foreach (Applet *applet, applets) {
                const auto &appletInfo = applet->pluginMetaData();

                if (appletInfo.isValid() && appletInfo.pluginId() == pluginName) {
                    applet->destroy();
                }
            }
        }
    }
}


#include "moc_widgetexplorer.cpp"
