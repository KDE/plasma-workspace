/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
    SPDX-FileCopyrightText: 2009 Ana Cecília Martins <anaceciliamb@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "widgetexplorer.h"

#include <QDebug>
#include <QFileDialog>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlProperty>

#include <KAuthorized>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNSWidgets/Dialog>

#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>
#include <QStandardPaths>

#include <PlasmaActivities/Consumer>

#include <KPackage/Package>
#include <KPackage/PackageJob>
#include <KPackage/PackageLoader>
#include <KPackage/PackageStructure>

#include "config-workspace.h"
#include "kcategorizeditemsviewmodels_p.h"

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
    KPackage::Package *package;

    PlasmaAppletItemModel itemModel;
    KCategorizedItemsViewModels::DefaultFilterModel filterModel;
    bool showSpecialFilters = true;
    DefaultItemFilterProxyModel filterItemModel;
    static QPointer<KNSWidgets::Dialog> newStuffDialog;

    std::unique_ptr<KActivities::Consumer> activitiesConsumer;
};

QPointer<KNSWidgets::Dialog> WidgetExplorerPrivate::newStuffDialog;

QString readTranslatedCategory(const QString &category, const QString &plugin)
{
    static const QList<KLazyLocalizedString> possibleTranslatslations{
        kli18nc("applet category", "Accessibility"),
        kli18nc("applet category", "Application Launchers"),
        kli18nc("applet category", "Astronomy"),
        kli18nc("applet category", "Date and Time"),
        kli18nc("applet category", "Development Tools"),
        kli18nc("applet category", "Education"),
        kli18nc("applet category", "Environment and Weather"),
        kli18nc("applet category", "Examples"),
        kli18nc("applet category", "File System"),
        kli18nc("applet category", "Fun and Games"),
        kli18nc("applet category", "Graphics"),
        kli18nc("applet category", "Language"),
        kli18nc("applet category", "Mapping"),
        kli18nc("applet category", "Miscellaneous"),
        kli18nc("applet category", "Multimedia"),
        kli18nc("applet category", "Online Services"),
        kli18nc("applet category", "Productivity"),
        kli18nc("applet category", "System Information"),
        kli18nc("applet category", "Utilities"),
        kli18nc("applet category", "Windows and Tasks"),
        kli18nc("applet category", "Clipboard"),
        kli18nc("applet category", "Tasks"),
    };
    const auto it = std::find_if(possibleTranslatslations.begin(), possibleTranslatslations.end(), [&category](const KLazyLocalizedString &str) {
        return category == str.untranslatedText();
    });
    if (it == possibleTranslatslations.cend()) {
        qDebug() << category << "from" << plugin << "is not a known category that can be translated ";
        return category;
    }
    return it->toString();
}

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

    struct CategoryInfo {
        QString untranslated;
        QString translated;
    };
    std::vector<CategoryInfo> categories;
    categories.reserve(itemModel.rowCount());
    for (int i = 0; i < itemModel.rowCount(); ++i) {
        if (PlasmaAppletItem *p = dynamic_cast<PlasmaAppletItem *>(itemModel.item(i))) {
            const QString translated = readTranslatedCategory(p->category(), p->pluginName());
            if (!translated.isEmpty()) {
                categories.push_back({p->category(), translated});
            }
        }
    }
    std::sort(categories.begin(), categories.end(), [](const CategoryInfo &left, const CategoryInfo &right) {
        return left.translated < right.translated;
    });
    auto end = std::unique(categories.begin(), categories.end(), [](const CategoryInfo left, const CategoryInfo right) {
        return left.translated == right.translated;
    });
    std::for_each(categories.begin(), end, [this](const CategoryInfo &category) {
        filterModel.addFilter(category.translated, KCategorizedItemsViewModels::Filter(QStringLiteral("category"), category.untranslated.toLower()));
    });
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
        if (containment->containmentType() == Plasma::Containment::Desktop && containment->activity() != activitiesConsumer->currentActivity()) {
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
    QObject::connect(containment, &Plasma::Containment::appletAdded, q, [this](Plasma::Applet *a, const QRectF &) {
        appletAdded(a);
    });
    QObject::connect(containment, &Plasma::Containment::appletRemoved, q, [this](Plasma::Applet *a) {
        appletRemoved(a);
    });

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
        WidgetExplorerPrivate::newStuffDialog = new KNSWidgets::Dialog(QStringLiteral("plasmoids.knsrc"));
        connect(WidgetExplorerPrivate::newStuffDialog, &KNSWidgets::Dialog::finished, WidgetExplorerPrivate::newStuffDialog, &QObject::deleteLater);

        WidgetExplorerPrivate::newStuffDialog->open();
    }

    Q_EMIT shouldClose();
}

void WidgetExplorer::openWidgetFile()
{
    QFileDialog *dialog = new QFileDialog;
    dialog->setMimeTypeFilters({"application/x-plasma"});
    dialog->setWindowTitle(i18n("Select Plasmoid File"));
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(dialog, &QFileDialog::fileSelected, [](const QString &packageFilePath) {
        if (packageFilePath.isEmpty()) {
            // TODO: user visible error handling
            qDebug() << "hm. no file path?";
            return;
        }

        auto job = KPackage::PackageJob::install(QStringLiteral("Plasma/Applet"), packageFilePath);
        connect(job, &KJob::result, [packageFilePath](KJob *job) {
            if (job->error()) {
                KMessageBox::error(nullptr, i18n("Installing the package %1 failed.", packageFilePath), i18n("Installation Failure"));
            }
        });
    });

    dialog->show();

    Q_EMIT shouldClose();
}

void WidgetExplorer::uninstall(const QString &pluginName)
{
    static const QString packageRoot =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/";
    KPackage::PackageJob::uninstall(QStringLiteral("Plasma/Applet"), pluginName, packageRoot);

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
