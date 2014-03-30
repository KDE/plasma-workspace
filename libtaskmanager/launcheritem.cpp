/*****************************************************************

Copyright 2010 Anton Kreuzkamp <akreuzkamp@web.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/
// Own
#include "launcheritem.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KDesktopFile>
#include <KFileItem>
#include <QMimeData>
#include <QDir>
#include <KMimeTypeTrader>
#include <KRun>
#include <KService>
#include <KServiceTypeTrader>
#include <KToolInvocation>

// KIO
#include <kemailsettings.h> // no camelcase include

#include "groupmanager.h"
#include "taskitem.h"
#include "taskgroup.h"

namespace TaskManager
{

class LauncherItemPrivate
{
public:
    LauncherItemPrivate(LauncherItem *launcher)
        : q(launcher) {
    }

    void associateDestroyed(QObject *obj);

    LauncherItem *q;
    QUrl        url;
    QUrl        resolvedUrl;
    QIcon       icon;
    QString     name;
    QString     genericName;
    QString     wmClass;
    QSet<QObject *> associates;
};

LauncherItem::LauncherItem(QObject *parent, const QUrl &url)
    : AbstractGroupableItem(parent),
      d(new LauncherItemPrivate(this))
{
    if (url.isEmpty()) {
        d->icon = QIcon::fromTheme("unknown");
    } else {
        setLauncherUrl(url);
    }
}

LauncherItem::~LauncherItem()
{
    emit destroyed(this);
    delete d;
}

bool LauncherItem::associateItemIfMatches(AbstractGroupableItem *item)
{
    if (d->associates.contains(item)) {
        return false;
    }

    QUrl itemUrl = item->launcherUrl();

    if (!itemUrl.isEmpty()) {
        if (d->url == itemUrl || d->resolvedUrl == itemUrl) {
            d->associates.insert(item);
            connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(associateDestroyed(QObject*)));
            emit associationChanged();
            return true;
        } else if (!launcherUrl().isEmpty()) {
            return false;
        }
    }

    QString name;
    if (item->itemType() == TaskItemType && !item->isStartupItem()) {
        name = static_cast<TaskItem *>(item)->taskName().toLower();
    } else {
        name = item->name().toLower();
    }

    if (!name.isEmpty() && name.compare(d->name, Qt::CaseInsensitive) == 0) {
        d->associates.insert(item);
        connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(associateDestroyed(QObject*)));
        emit associationChanged();

        // Store this mapping!
        if (TaskItemType == item->itemType()) {
            static_cast<TaskItem *>(item)->setLauncherUrl(this);
        }

        return true;
    }

    return false;
}

bool LauncherItem::isAssociated(AbstractGroupableItem *item) const
{
    return d->associates.contains(item);
}

void LauncherItem::removeItemIfAssociated(AbstractGroupableItem *item)
{
    disconnect(item, SIGNAL(destroyed(QObject*)), this, SLOT(associateDestroyed(QObject*)));

    // now let's just pretend it was destroyed
    d->associateDestroyed(item);
}

bool LauncherItem::shouldShow(const GroupManager *manager) const
{
    if (!manager) {
        return d->associates.isEmpty();
    }

    const bool screen = manager->showOnlyCurrentScreen();
    const bool desk = manager->showOnlyCurrentDesktop();
    const bool activity = manager->showOnlyCurrentActivity();

    foreach (QObject *obj, d->associates) {
        TaskItem *item = qobject_cast<TaskItem *>(obj);
        if (!item || !item->task()) {
            continue;
        }

        if ((!screen || item->task()->isOnScreen(manager->screen())) &&
            (!desk || item->isOnCurrentDesktop()) &&
            (!activity || item->task()->isOnCurrentActivity())) {
            return false;
        }
    }

    return true;
}

void LauncherItemPrivate::associateDestroyed(QObject *obj)
{
    if (associates.remove(obj)) {
        emit q->associationChanged();
    }
}

QIcon LauncherItem::icon() const
{
    return d->icon;
}

QString LauncherItem::name() const
{
    return d->name;
}

QString LauncherItem::genericName() const
{
    return d->genericName;
}

QString LauncherItem::wmClass() const
{
    return d->wmClass;
}

void LauncherItem::setName(const QString& name)
{
    //NOTE: preferred is NOT a protocol, it's just a magic string
    if (d->url.scheme() != "preferred") {
        d->name = name;
    }
}

void LauncherItem::setGenericName(const QString& genericName)
{
    //NOTE: preferred is NOT a protocol, it's just a magic string
    if (d->url.scheme() != "preferred") {
        d->genericName = genericName;
    }
}

void LauncherItem::setWmClass(const QString &wmClass)
{
    d->wmClass = wmClass;
}

ItemType LauncherItem::itemType() const
{
    return LauncherItemType;
}

bool LauncherItem::isGroupItem() const
{
    return false;
}

void LauncherItem::launch()
{
    //NOTE: preferred is NOT a protocol, it's just a magic string
    if (d->url.scheme() == "preferred") {
        KService::Ptr service = KService::serviceByStorageId(defaultApplication());

        if (!service) {
            return;
        }

        QString desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, service->entryPath());
        if (desktopFile.isNull()) {
            desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, service->entryPath());
        }
        new KRun(QUrl::fromLocalFile(desktopFile), 0);
    } else {
        new KRun(d->url, 0);
    }
}

void LauncherItem::addMimeData(QMimeData* mimeData) const
{
    mimeData->setData("text/uri-list", d->url.url().toAscii());
}

QUrl LauncherItem::launcherUrl() const
{
    return d->url;
}

//Ugly hack written by Aaron Seigo from plasmagenericshell/scripting/scriptengine.cpp
QString LauncherItem::defaultApplication() const
{
    const QString application = d->url.host();
    if (application.isEmpty()) {
        return QString();
    }

    // FIXME: there are some pretty horrible hacks below, in the sense that they assume a very
    // specific implementation system. there is much room for improvement here. see
    // kdebase-runtime/kcontrol/componentchooser/ for all the gory details ;)
    if (application.compare("mailer", Qt::CaseInsensitive) == 0) {
        KEMailSettings settings;

        // in KToolInvocation, the default is kmail; but let's be friendlier :)
        QString command = settings.getSetting(KEMailSettings::ClientProgram);
        if (command.isEmpty()) {
            if (KService::Ptr kontact = KService::serviceByStorageId("kontact")) {
                return kontact->storageId();
            } else if (KService::Ptr kmail = KService::serviceByStorageId("kmail")) {
                return kmail->storageId();
            }
        }

        if (!command.isEmpty()) {
            if (settings.getSetting(KEMailSettings::ClientTerminal) == "true") {
                KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
                const QString preferredTerminal = confGroup.readPathEntry("TerminalApplication",
                                                  QString::fromLatin1("konsole"));
                command = preferredTerminal + QString::fromLatin1(" -e ") + command;
            }

            return command;
        }
    } else if (application.compare("browser", Qt::CaseInsensitive) == 0) {
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        QString browserApp = config.readPathEntry("BrowserApplication", QString());
        if (browserApp.isEmpty()) {
            const KService::Ptr htmlApp = KMimeTypeTrader::self()->preferredService(QLatin1String("text/html"));
            if (htmlApp) {
                browserApp = htmlApp->storageId();
            }
        } else if (browserApp.startsWith('!')) {
            browserApp = browserApp.mid(1);
        }

        return browserApp;
    } else if (application.compare("terminal", Qt::CaseInsensitive) == 0) {
        KConfigGroup confGroup(KSharedConfig::openConfig(), "General");
        return confGroup.readPathEntry("TerminalApplication", QString::fromLatin1("konsole"));
    } else if (application.compare("filemanager", Qt::CaseInsensitive) == 0) {
        KService::Ptr service = KMimeTypeTrader::self()->preferredService("inode/directory");
        if (service) {
            return service->storageId();
        }
    } else if (application.compare("windowmanager", Qt::CaseInsensitive) == 0) {
        KConfig cfg("ksmserverrc", KConfig::NoGlobals);
        KConfigGroup confGroup(&cfg, "General");
        return confGroup.readEntry("windowManager", QString::fromLatin1("kwin"));
    } else if (KService::Ptr service = KMimeTypeTrader::self()->preferredService(application)) {
        return service->storageId();
    } else {
        // try the files in share/apps/kcm_componentchooser/*.desktop
        QStringList directories = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "kcm_componentchooser", QStandardPaths::LocateDirectory);
        QStringList services;
        foreach(const QString& directory, directories) {
            QDir d(directory);
            foreach(const QString& f, d.entryList(QStringList("*.desktop")))
                services += d.absoluteFilePath(f);
        }

        //kDebug() << "ok, trying in" << services.count();
        foreach (const QString & service, services) {
            KConfig config(service, KConfig::SimpleConfig);
            KConfigGroup cg = config.group(QByteArray());
            const QString type = cg.readEntry("valueName", QString());
            //kDebug() << "    checking" << service << type << application;
            if (type.compare(application, Qt::CaseInsensitive) == 0) {
                KConfig store(cg.readPathEntry("storeInFile", "null"));
                KConfigGroup storeCg(&store, cg.readEntry("valueSection", QString()));
                const QString exec = storeCg.readPathEntry(cg.readEntry("valueName", "kcm_componenchooser_null"),
                                     cg.readEntry("defaultImplementation", QString()));
                if (!exec.isEmpty()) {
                    return exec;
                }

                break;
            }
        }
    }

    return "";
}

void LauncherItem::setLauncherUrl(const QUrl& url)
{
    // Takes care of improperly escaped characters and resolves paths
    // into file:/// URLs
    QUrl newUrl(url.url());

    if (d->url.scheme() != "preferred" && newUrl == d->url) {
        return;
    }

    d->url = newUrl;

    if (d->url.isLocalFile() && KDesktopFile::isDesktopFile(d->url.toLocalFile())) {
        KDesktopFile f(d->url.toLocalFile());

        if (f.tryExec()) {
            d->icon = QIcon::fromTheme(f.readIcon());
            d->name = f.readName();
            d->genericName = f.readGenericName();
        } else {
            d->url = QUrl();
            return;
        }
    } else if (d->url.scheme() == "preferred") {
        //NOTE: preferred is NOT a protocol, it's just a magic string
        const KService::Ptr service = KService::serviceByStorageId(defaultApplication());

        if (service) {
            QString desktopFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, service->entryPath());

            KDesktopFile f(desktopFile);
            KConfigGroup cg(&f, "Desktop Entry");

            d->icon = QIcon::fromTheme(f.readIcon());
            const QString exec = cg.readEntry("Exec", QString());
            d->name = cg.readEntry("Name", QString());
            if (d->name.isEmpty() && !exec.isEmpty()) {
                d->name = exec.split(' ').at(0);
            }
            d->genericName = f.readGenericName();
            d->resolvedUrl = QUrl::fromLocalFile(desktopFile);
        } else {
            d->url = QUrl();
        }
    } else {
        const KFileItem fileItem(d->url);
        d->icon = QIcon::fromTheme(fileItem.iconName());
    }

    if (d->name.isEmpty()) {
        d->name = d->url.fileName();
    }

    if (d->icon.isNull()) {
        d->icon = QIcon::fromTheme("unknown");
    }
}

bool LauncherItem::isValid() const
{
    return d->url.isValid();
}

void LauncherItem::setIcon(const QIcon& icon)
{
    //NOTE: preferred is NOT a protocol, it's just a magic string
    if (d->url.scheme() != "preferred") {
        d->icon = icon;
    }
}

bool LauncherItem::demandsAttention() const
{
    return false;
}
bool LauncherItem::isActionSupported(NET::Action) const
{
    return false;
}
bool LauncherItem::isActive() const
{
    return false;
}
bool LauncherItem::isAlwaysOnTop() const
{
    return false;
}
bool LauncherItem::isFullScreen() const
{
    return false;
}
bool LauncherItem::isKeptBelowOthers() const
{
    return false;
}
bool LauncherItem::isMaximized() const
{
    return false;
}
bool LauncherItem::isMinimized() const
{
    return false;
}
bool LauncherItem::isOnAllDesktops() const
{
    return false;
}
bool LauncherItem::isOnCurrentDesktop() const
{
    return false;
}
bool LauncherItem::isShaded() const
{
    return false;
}
int LauncherItem::desktop() const
{
    return 0;
}

void LauncherItem::setAlwaysOnTop(bool)
{
}
void LauncherItem::setFullScreen(bool)
{
}
void LauncherItem::setKeptBelowOthers(bool)
{
}
void LauncherItem::setMaximized(bool)
{
}
void LauncherItem::setMinimized(bool)
{
}
void LauncherItem::setShaded(bool)
{
}
void LauncherItem::toDesktop(int)
{
}
void LauncherItem::toggleAlwaysOnTop()
{
}
void LauncherItem::toggleFullScreen()
{
}
void LauncherItem::toggleKeptBelowOthers()
{
}
void LauncherItem::toggleMaximized()
{
}
void LauncherItem::toggleMinimized()
{
}
void LauncherItem::toggleShaded()
{
}
void LauncherItem::close()
{
}
//END


} // TaskManager namespace

#include "moc_launcheritem.cpp"
