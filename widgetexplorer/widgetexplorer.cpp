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

#include <klocalizedstring.h>
#include <kservicetypetrader.h>

#include <Plasma/Applet>
#include <Plasma/Corona>
#include <Plasma/Containment>
#include <Plasma/Package>
#include <Plasma/PackageStructure>
#include <qstandardpaths.h>

#include "desktopview.h"
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
          desktopView(0)
    {
    }

    void initFilters();
    void initRunningApplets();
    void containmentDestroyed();

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
    //QWeakPointer<Plasma::OpenWidgetAssistant> openAssistant;
    Plasma::Package *package;

    PlasmaAppletItemModel itemModel;
    KCategorizedItemsViewModels::DefaultFilterModel filterModel;
    DefaultItemFilterProxyModel filterItemModel;
    DesktopView *desktopView;
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

WidgetExplorer::WidgetExplorer(QObject *parent)
        : QObject(parent),
          d(new WidgetExplorerPrivate(this))
{
    //FIXME: delay
    setApplication();
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


DesktopView *WidgetExplorer::desktopView()
{
    return d->desktopView;
}

void WidgetExplorer::setDesktopView(DesktopView *view)
{
    d->desktopView = view;
    if (view) {
        setContainment(view->containment());
    }
    emit desktopViewChanged();
}

void WidgetExplorer::setApplication(const QString &app)
{
    if (d->application == app) {
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
        emit shouldClose();
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
    emit shouldClose();
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
    emit shouldClose();
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


#include "moc_widgetexplorer.cpp"
