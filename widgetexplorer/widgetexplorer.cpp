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

#include <klocalizedstring.h>
#include <kservicetypetrader.h>
#include <kdeclarative/qmlobject.h>

#include <Plasma/Applet>
#include <Plasma/Corona>
#include <Plasma/Containment>
#include <Plasma/Package>
#include <Plasma/PackageStructure>
#include <qstandardpaths.h>

#include "kcategorizeditemsviewmodels_p.h"
#include "plasmaappletitemmodel_p.h"

using namespace KCategorizedItemsViewModels;
using namespace Plasma;

WidgetAction::WidgetAction(QObject *parent)
    : QAction(parent)
{
    qDebug() << "There we go.";
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
          containment(0),
          itemModel(w),
          filterModel(w),
          qmlObject(new QmlObject(w))
    {
    }

    void initFilters();
    void initRunningApplets();
    void containmentDestroyed();
    void setLocation(Plasma::Types::Location loc);

    /**
     * Tracks a new running applet
     */
    void appletAdded(Plasma::Applet *applet);

    /**
     * A running applet is no more
     */
    void appletRemoved(Plasma::Applet *applet);

    //this orientation is just for convenience, is the location that is important
    Qt::Orientation orientation;
    Plasma::Types::Location location;
    WidgetExplorer *q;
    QString application;
    Plasma::Containment *containment;

    QHash<QString, int> runningApplets; // applet name => count
    //extra hash so we can look up the names of deleted applets
    QHash<Plasma::Applet *,QString> appletNames;
    //QWeakPointer<Plasma::OpenWidgetAssistant> openAssistant;
    Plasma::Package *package;

    PlasmaAppletItemModel itemModel;
    KCategorizedItemsViewModels::DefaultFilterModel filterModel;
    DefaultItemFilterProxyModel filterItemModel;

    QmlObject *qmlObject;
};

void WidgetExplorerPrivate::initFilters()
{
    filterModel.addFilter(i18n("All Widgets"),
                          KCategorizedItemsViewModels::Filter(), QIcon::fromTheme("plasma"));

    // Filters: Special
    filterModel.addFilter(i18n("Running"),
                          KCategorizedItemsViewModels::Filter("running", true),
                          QIcon::fromTheme("dialog-ok"));

    filterModel.addSeparator(i18n("Categories:"));

    typedef QPair<QString, QString> catPair;
    QMap<QString, catPair > categories;
    QSet<QString> existingCategories = itemModel.categories();
    //foreach (const QString &category, Plasma::Applet::listCategories(application)) {
    QStringList cats;
    KService::List services = KServiceTypeTrader::self()->query("Plasma/Applet", QString());

    foreach (const QExplicitlySharedDataPointer<KService> service, services) {
        KPluginInfo info(service);
        if (info.property("NoDisplay").toBool() || info.category() == i18n("Containments") ||
            info.category().isEmpty()) {
            // we don't want to show the hidden category
            continue;
        }
        const QString c = info.category();
        if (-1 == cats.indexOf(c)) {
            cats << c;
        }
    }
    qWarning() << "TODO: port listCategories()";
    foreach (const QString &category, cats) {
        const QString lowerCaseCat = category.toLower();
        if (existingCategories.contains(lowerCaseCat)) {
            const QString trans = i18n(category.toLocal8Bit());
            categories.insert(trans.toLower(), qMakePair(trans, lowerCaseCat));
            qDebug() << "Categories: << " << lowerCaseCat;
        }
    }

    foreach (const catPair &category, categories) {
        filterModel.addFilter(category.first,
                              KCategorizedItemsViewModels::Filter("category", category.second));
    }

}


void WidgetExplorerPrivate::setLocation(const Plasma::Types::Location loc)
{
    Qt::Orientation orient;
    if (loc == Plasma::Types::LeftEdge || loc == Plasma::Types::RightEdge) {
        orient = Qt::Vertical;
    } else {
        orient = Qt::Horizontal;
    }

    if (location == loc) {
        return;
    }

    location = loc;

    if (orientation == orient) {
        return;
    }

    emit q->orientationChanged();
}

QObject *WidgetExplorer::widgetsModel() const
{
    return &d->filterItemModel;
}

QObject *WidgetExplorer::filterModel() const
{
    return &d->filterModel;
}

QList <QObject *>  WidgetExplorer::widgetsMenuActions()
{
    QList <QObject *> actionList;

    QSignalMapper *mapper = new QSignalMapper(this);
    QObject::connect(mapper, SIGNAL(mapped(QString)), this, SLOT(downloadWidgets(QString)));

    WidgetAction *action = new WidgetAction(QIcon::fromTheme("applications-internet"),
                                  i18n("Download New Plasma Widgets"), this);
    QObject::connect(action, SIGNAL(triggered(bool)), mapper, SLOT(map()));
    mapper->setMapping(action, QString());
    actionList << action;

    KService::List offers = KServiceTypeTrader::self()->query("Plasma/PackageStructure");
    foreach (const KService::Ptr &service, offers) {
        //qDebug() << service->property("X-Plasma-ProvidesWidgetBrowser");
        if (service->property("X-Plasma-ProvidesWidgetBrowser").toBool()) {
            WidgetAction *action = new WidgetAction(QIcon::fromTheme("applications-internet"),
                                          i18nc("%1 is a type of widgets, as defined by "
                                                "e.g. some plasma-packagestructure-*.desktop files",
                                                "Download New %1", service->name()), this);
            QObject::connect(action, SIGNAL(triggered(bool)), mapper, SLOT(map()));
            mapper->setMapping(action, service->property("X-KDE-PluginInfo-Name").toString());
            actionList << action;
        }
    }

    action = new WidgetAction(this);
    action->setSeparator(true);
    actionList << action;

    action = new WidgetAction(QIcon::fromTheme("package-x-generic"),
                         i18n("Install Widget From Local File..."), this);
    QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(openWidgetFile()));
    actionList << action;

    return actionList;
}

QList<QObject *> WidgetExplorer::extraActions() const
{
    QList<QObject *> actionList;
//     foreach (QAction *action, actions()) { // FIXME: where did actions() come from?
//         actionList << action;
//     }
    qWarning() << "extraactions needs reimplementation";
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
        qDebug() << "can't happen";
        return;
    }

    appletNames.clear();
    runningApplets.clear();
    QList<Containment*> containments = c->containments();
    foreach (Containment *containment, containments) {
        QObject::connect(containment, SIGNAL(appletAdded(Plasma::Applet*)), q, SLOT(appletAdded(Plasma::Applet*)));
        QObject::connect(containment, SIGNAL(appletRemoved(Plasma::Applet*)), q, SLOT(appletRemoved(Plasma::Applet*)));

        foreach (Applet *applet, containment->applets()) {
            if (applet->pluginInfo().isValid()) {
                runningApplets[applet->pluginInfo().pluginName()]++;
            } else {
                qDebug() << "Invalid plugininfo. :(";
            }
        }
    }

    //qDebug() << runningApplets;
    itemModel.setRunningApplets(runningApplets);
}

void WidgetExplorerPrivate::containmentDestroyed()
{
    containment = 0;
}

void WidgetExplorerPrivate::appletAdded(Plasma::Applet *applet)
{
    QString name = applet->pluginInfo().pluginName();

    runningApplets[name]++;
    appletNames.insert(applet, name);
    itemModel.setRunningApplets(name, runningApplets[name]);
}

void WidgetExplorerPrivate::appletRemoved(Plasma::Applet *applet)
{
    Plasma::Applet *a = (Plasma::Applet *)applet; //don't care if it's valid, just need the address

    QString name = appletNames.take(a);

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

WidgetExplorer::WidgetExplorer(QQuickItem *parent)
        :QQuickItem(parent),
        d(new WidgetExplorerPrivate(this))
{
    setLocation(Plasma::Types::LeftEdge);
    populateWidgetList();
    d->initRunningApplets();
    d->filterItemModel.setSortCaseSensitivity(Qt::CaseInsensitive);
    d->filterItemModel.setDynamicSortFilter(true);
    d->filterItemModel.setSourceModel(&d->itemModel);
    d->filterItemModel.sort(0);
}

WidgetExplorer::~WidgetExplorer()
{
     delete d;
}

void WidgetExplorer::setLocation(Plasma::Types::Location loc)
{
    d->setLocation(loc);
    emit(locationChanged(loc));
}

Plasma::Types::Location WidgetExplorer::location() const
{
    return d->location;
}

Qt::Orientation WidgetExplorer::orientation() const
{
    return d->orientation;
}

void WidgetExplorer::populateWidgetList(const QString &app)
{
    d->application = app;
    d->itemModel.setApplication(app);
    d->initFilters();

    d->itemModel.setRunningApplets(d->runningApplets);

}

QString WidgetExplorer::application()
{
    return d->application;
}

void WidgetExplorer::setSource(const QUrl &source)
{
    d->qmlObject->setInitializationDelayed(true);
    d->qmlObject->setSource(source);
    d->qmlObject->engine()->rootContext()->setContextProperty("widgetExplorer", this);
    d->qmlObject->completeInitialization();
    QQuickItem *i = qobject_cast<QQuickItem *>(d->qmlObject->rootObject());
    i->setParentItem(this);
}

QUrl WidgetExplorer::source() const
{
    return d->qmlObject->source();
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
            connect(d->containment, SIGNAL(immutabilityChanged(Plasma::Types::ImmutabilityType)), this, SLOT(immutabilityChanged(Plasma::Types::ImmutabilityType)));

            setLocation(containment->location());
        }

        d->initRunningApplets();
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

    return 0;
}

void WidgetExplorer::addApplet(const QString &pluginName)
{
    const QString p = "plasma/plasmoids/"+pluginName;
    qWarning() << "-------->  load applet: " << pluginName << " relpath: " << p;

    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, p, QStandardPaths::LocateDirectory);

    qDebug() << " .. pathes: " << dirs;
    if (!dirs.count()) {
        qWarning() << "Failed to find plasmoid path for " << pluginName;
        return;
    }
    Plasma::Applet *applet = Plasma::Applet::loadPlasmoid(dirs.first());

    if (applet) {
        if (d->containment) {
            d->containment->addApplet(applet);
        } else {
            qWarning() << "No containment set (but the applet loaded).";
        }
    } else {
        qWarning() << "Failed to load applet" << pluginName << dirs;
    }
}

void WidgetExplorer::immutabilityChanged(Plasma::Types::ImmutabilityType type)
{
    if (type != Plasma::Types::Mutable) {
        close();
    }
}



void WidgetExplorer::downloadWidgets(const QString &type)
{
    Plasma::PackageStructure *installer = 0;

    if (!type.isEmpty()) {
        QString constraint = QString("'%1' == [X-KDE-PluginInfo-Name]").arg(type);
        KService::List offers = KServiceTypeTrader::self()->query("Plasma/PackageStructure",
                                                                  constraint);
        if (offers.isEmpty()) {
            //qDebug() << "could not find requested PackageStructure plugin" << type;
        } else {
            KService::Ptr service = offers.first();
            QString error;
            // FIXME: port install to plasma2
//             installer = service->createInstance<Plasma::PackageStructure>(topLevelWidget(),
//                                                                           QVariantList(), &error);
            if (installer) {
//                 connect(installer, SIGNAL(newWidgetBrowserFinished()),
//                         installer, SLOT(deleteLater()));
            } else {
                //qDebug() << "found, but could not load requested PackageStructure plugin" << type
//                         << "; reported error was" << error;
            }
        }
    }

    if (installer) {
        //installer->createNewWidgetBrowser();
    } else {
        // we don't need to delete the default Applet::packageStructure as that
        // belongs to the applet
//       Plasma::Applet::packageStructure()->createNewWidgetBrowser();
        /**
          for reference in a libplasma2 world, the above line equates to this:

          KNS3::DownloadDialog *knsDialog = m_knsDialog.data();
          if (!knsDialog) {
          m_knsDialog = knsDialog = new KNS3::DownloadDialog("plasmoids.knsrc", parent);
          connect(knsDialog, SIGNAL(accepted()), this, SIGNAL(newWidgetBrowserFinished()));
          }

          knsDialog->show();
          knsDialog->raise();
         */
    }
    close();
}

void WidgetExplorer::openWidgetFile()
{
/*
    Plasma::OpenWidgetAssistant *assistant = d->openAssistant.data();
    if (!assistant) {
        assistant = new Plasma::OpenWidgetAssistant(0);
        d->openAssistant = assistant;
    }

    KWindowSystem::setOnDesktop(assistant->winId(), KWindowSystem::currentDesktop());
    assistant->setAttribute(Qt::WA_DeleteOnClose, true);
    assistant->show();
    assistant->raise();
    assistant->setFocus();
*/
    close();
}

void WidgetExplorer::uninstall(const QString &pluginName)
{
    const QString packageRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/plasma/plasmoids/";

    Plasma::Package pkg;
    pkg.setPath(packageRoot);
    pkg.uninstall(pluginName, packageRoot);

    //FIXME: moreefficient way rather a linear scan?
    for (int i = 0; i < d->itemModel.rowCount(); ++i) {
        QStandardItem *item = d->itemModel.item(i);
        if (item->data(PlasmaAppletItemModel::PluginNameRole).toString() == pluginName) {
            d->itemModel.takeRow(i);
            break;
        }
    }
}

void WidgetExplorer::close()
{
    //d->qmlObject->engine()->rootContext()->setContextProperty("widgetExplorer", 0);
    deleteLater();
}


#include "moc_widgetexplorer.cpp"
