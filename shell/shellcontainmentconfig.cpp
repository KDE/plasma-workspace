/*
    SPDX-FileCopyrightText: 2021 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "shellcontainmentconfig.h"

#include <PlasmaActivities/Consumer>
#include <PlasmaActivities/Info>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KPackage/Package>

#include <QQmlContext>
#include <QQuickItem>
#include <QScreen>

#include "panelview.h"
#include "screenpool.h"
#include "shellcorona.h"
#include <chrono>

using namespace std::chrono_literals;

ScreenPoolModel::ScreenPoolModel(ShellCorona *corona, QObject *parent)
    : QAbstractListModel(parent)
    , m_corona(corona)
{
    m_reloadTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(200ms);

    connect(m_reloadTimer, &QTimer::timeout, this, &ScreenPoolModel::load);

    connect(m_corona, &Plasma::Corona::screenAdded, m_reloadTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_corona, &Plasma::Corona::screenRemoved, m_reloadTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
}

ScreenPoolModel::~ScreenPoolModel() = default;

QVariant ScreenPoolModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= int(m_screens.size())) {
        return QVariant();
    }
    const Data &d = m_screens.at(index.row());
    switch (role) {
    case ScreenIdRole:
        return d.id;
    case ScreenNameRole:
        return d.name;
    case ContainmentsRole: {
        auto *cont = m_containments.at(index.row());
        return QVariant::fromValue<QObject *>(cont);
    }
    case PrimaryRole:
        return d.primary;
    case EnabledRole:
        return d.enabled;
    }
    return QVariant();
}

int ScreenPoolModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_screens.size();
}

QHash<int, QByteArray> ScreenPoolModel::roleNames() const
{
    QHash<int, QByteArray> roles({
        {ScreenIdRole, QByteArrayLiteral("screenId")},
        {ScreenNameRole, QByteArrayLiteral("screenName")},
        {ContainmentsRole, QByteArrayLiteral("containments")},
        {EnabledRole, QByteArrayLiteral("isEnabled")},
        {PrimaryRole, QByteArrayLiteral("isPrimary")},
    });
    return roles;
}

void ScreenPoolModel::load()
{
    beginResetModel();
    m_screens.clear();
    qDeleteAll(m_containments);
    m_containments.clear();

    QSet<int> unknownScreenIds;
    for (auto *cont : m_corona->containments()) {
        connect(cont, &Plasma::Containment::destroyedChanged, this, &ScreenPoolModel::load, Qt::UniqueConnection);
        if (!cont->destroyed()) {
            unknownScreenIds.insert(cont->lastScreen());
        }
    }
    int knownId = 0;
    for (QScreen *screen : m_corona->screenPool()->screenOrder()) {
        Data d;
        unknownScreenIds.remove(knownId);
        d.id = knownId;
        if (screen->name().contains(QStringLiteral("eDP"))) {
            d.name = i18n("Internal Screen on %1", screen->name());
        } else if (screen->model().contains(screen->name())) {
            d.name = screen->model();
        } else {
            d.name = i18nc("Screen manufacturer and model on connector", "%1 %2 on %3", screen->manufacturer(), screen->model(), screen->name());
        }
        d.primary = knownId == 0;
        d.enabled = true;

        auto *conts = new ShellContainmentModel(m_corona, knownId, this);
        conts->load();

        // Exclude screens which don't have any containemnt assigned
        if (conts->rowCount() > 0) {
            m_containments.push_back(conts);
            m_screens.push_back(d);
        } else {
            delete conts;
        }
        ++knownId;
    }

    QList sortedIds = unknownScreenIds.values();
    std::sort(sortedIds.begin(), sortedIds.end());
    int i = 1;
    for (int id : sortedIds) {
        Data d;
        d.id = id;
        d.name = i18n("Disconnected Screen %1", id + 1);
        d.primary = id == 0;
        d.enabled = false;

        auto *conts = new ShellContainmentModel(m_corona, id, this);
        conts->load();
        m_containments.push_back(conts);
        m_screens.push_back(d);
        i++;
    }
    endResetModel();
}

void ScreenPoolModel::remove(int screenId)
{
    // Don't allow to remove currently used containemnts
    if (m_corona->screenPool()->screenForId(screenId)) {
        return;
    }

    // remove containments of *all* activities
    auto conts = m_corona->containmentsForScreen(screenId);
    for (auto *cont : std::as_const(conts)) {
        // Don't call destroy directly, so we can have the undo action notification
        auto *destroyAction = cont->internalAction("remove");
        if (destroyAction) {
            destroyAction->trigger();
        }
    }
}

// ---

ShellContainmentModel::ShellContainmentModel(ShellCorona *corona, int screenId, ScreenPoolModel *parent)
    : QAbstractListModel(parent)
    , m_screenId(screenId)
    , m_corona(corona)
    , m_screenPoolModel(parent)
    , m_activityConsumer(new KActivities::Consumer(this))
{
    m_reloadTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(200ms);

    connect(m_reloadTimer, &QTimer::timeout, this, &ShellContainmentModel::load);

    connect(m_corona, &ShellCorona::startupCompleted, this, &ShellContainmentModel::load);

    connect(m_corona, &Plasma::Corona::containmentAdded, m_reloadTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_corona, &Plasma::Corona::screenOwnerChanged, m_reloadTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    connect(m_corona, &ShellCorona::containmentPreviewReady, this, [this](Plasma::Containment *containment, const QString &path) {
        int i = 0;
        for (auto &d : m_containments) {
            if (d.containment == containment) {
                d.image = path;
                emit dataChanged(index(i, 0), index(i, 0));
                break;
            }
            ++i;
        }
    });
}

ShellContainmentModel::~ShellContainmentModel() = default;

QVariant ShellContainmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= int(m_containments.size())) {
        return QVariant();
    }
    const Data &d = m_containments.at(index.row());
    switch (role) {
    case ContainmentIdRole:
        return d.id;
    case ScreenRole:
        return d.screen;
    case EdgeRole:
        return ShellContainmentModel::plasmaLocationToString(d.edge);
    case EdgePositionRole:
        return qMax(0, m_edgeCount.value(d.screen).value(d.edge).indexOf(d.id));
    case PanelCountAtRightRole:
        return qMax(0, m_edgeCount.value(d.screen).value(Plasma::Types::RightEdge).count());
    case PanelCountAtTopRole:
        return qMax(0, m_edgeCount.value(d.screen).value(Plasma::Types::TopEdge).count());
    case PanelCountAtLeftRole:
        return qMax(0, m_edgeCount.value(d.screen).value(Plasma::Types::LeftEdge).count());
    case PanelCountAtBottomRole:
        return qMax(0, m_edgeCount.value(d.screen).value(Plasma::Types::BottomEdge).count());
    case ActivityRole: {
        const auto *activityInfo = m_activitiesInfos.value(d.activity);
        if (activityInfo) {
            return activityInfo->name();
        }
        break;
    }
    case IsActiveRole:
        return d.isActive;
    case ImageSourceRole:
        return d.image;
    case DestroyedRole:
        return d.containment->destroyed();
    }
    return QVariant();
}

int ShellContainmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_containments.size();
}

QHash<int, QByteArray> ShellContainmentModel::roleNames() const
{
    QHash<int, QByteArray> roles({
        {ContainmentIdRole, QByteArrayLiteral("containmentId")},
        {ScreenRole, QByteArrayLiteral("screen")},
        {EdgeRole, QByteArrayLiteral("edge")},
        {EdgePositionRole, QByteArrayLiteral("edgePosition")},
        {PanelCountAtRightRole, QByteArrayLiteral("panelCountAtRight")},
        {PanelCountAtTopRole, QByteArrayLiteral("panelCountAtTop")},
        {PanelCountAtLeftRole, QByteArrayLiteral("panelCountAtLeft")},
        {PanelCountAtBottomRole, QByteArrayLiteral("panelCountAtBottom")},
        {ActivityRole, QByteArrayLiteral("activity")},
        {IsActiveRole, QByteArrayLiteral("active")},
        {ImageSourceRole, QByteArrayLiteral("imageSource")},
        {DestroyedRole, QByteArrayLiteral("isDestroyed")},
    });
    return roles;
}

ScreenPoolModel *ShellContainmentModel::screenPoolModel() const
{
    return m_screenPoolModel;
}

void ShellContainmentModel::remove(int contId)
{
    if (contId < 0) {
        return;
    }

    auto *cont = containmentById(contId);
    if (cont) {
        disconnect(cont, nullptr, this, nullptr);
        // Don't call destroy directly, so we can have the undo action notification
        auto *destroyAction = cont->internalAction("remove");
        if (destroyAction) {
            destroyAction->trigger();
        }
    }
    load();
}

void ShellContainmentModel::moveContainementToScreen(unsigned int contId, int newScreen)
{
    if (contId == 0 || newScreen < 0) {
        return;
    }

    auto containmentIt = std::find_if(m_containments.begin(), m_containments.end(), [contId](Data &d) {
        return d.id == contId;
    });
    if (containmentIt == m_containments.end()) {
        return;
    }
    if (containmentIt->screen == newScreen) {
        return;
    }

    auto *cont = containmentById(contId);
    if (cont == nullptr) {
        return;
    }

    // If it's a panel, only move that one
    if (cont->containmentType() == Plasma::Containment::Panel || cont->containmentType() == Plasma::Containment::CustomPanel) {
        m_corona->setScreenForContainment(cont, newScreen);
    } else {
        // If it's a desktop, for now move all desktops for all activities
        const int oldScreen = cont->screen() >= 0 ? cont->screen() : cont->lastScreen();
        m_corona->swapDesktopScreens(oldScreen, newScreen);
    }
}

bool ShellContainmentModel::findContainment(unsigned int containmentId) const
{
    return m_containments.cend() != std::find_if(m_containments.cbegin(), m_containments.cend(), [containmentId](const Data &d) {
               return d.id == containmentId;
           });
}

void ShellContainmentModel::load()
{
    beginResetModel();

    for (auto &d : m_containments) {
        disconnect(d.containment, nullptr, this, nullptr);
    }
    m_containments.clear();
    m_edgeCount.clear();

    for (const auto *cont : m_corona->containments()) {
        // Skip the systray
        if (qobject_cast<Plasma::Applet *>(cont->parent())) {
            continue;
        }
        // Only allow current activity for now (panels always go in)
        if (cont->containmentType() != Plasma::Containment::Panel && cont->containmentType() != Plasma::Containment::CustomPanel
            && cont->activity() != m_activityConsumer->currentActivity()) {
            continue;
        }
        if (!m_edgeCount.contains(cont->lastScreen())) {
            m_edgeCount[cont->lastScreen()] = QHash<Plasma::Types::Location, QList<int>>();
            m_edgeCount[cont->lastScreen()][cont->location()] = QList<int>();
        }
        m_edgeCount[cont->lastScreen()][cont->location()].append(cont->id());
        m_corona->grabContainmentPreview(const_cast<Plasma::Containment *>(cont));
        Data d;
        d.id = cont->id();
        d.screen = cont->lastScreen();
        d.edge = cont->location();
        d.activity = cont->activity();
        d.isActive = cont->screen() != -1;
        d.containment = cont;
        d.image = containmentPreview(const_cast<Plasma::Containment *>(cont));

        if (cont->lastScreen() == m_screenId || (cont->lastScreen() == -1 && cont->screen() == m_screenId)) {
            m_containments.push_back(d);
            connect(cont, &QObject::destroyed, this, &ShellContainmentModel::load);
            connect(cont, &Plasma::Containment::destroyedChanged, this, &ShellContainmentModel::load);
            connect(cont, &Plasma::Containment::locationChanged, this, &ShellContainmentModel::load);
        }
    }
    endResetModel();
}

void ShellContainmentModel::loadActivitiesInfos()
{
    beginResetModel();
    for (const auto &cont : m_containments) {
        const auto activitId = cont.activity;
        if (activitId.isEmpty()) {
            continue;
        }
        auto *activityInfo = new KActivities::Info(cont.activity, this);
        if (activityInfo) {
            if (!m_activitiesInfos.value(cont.activity)) {
                m_activitiesInfos[cont.activity] = activityInfo;
            }
        }
    }
    endResetModel();
}

QString ShellContainmentModel::plasmaLocationToString(Plasma::Types::Location location)
{
    switch (location) {
    case Plasma::Types::Floating:
        return QStringLiteral("floating");
    case Plasma::Types::Desktop:
        return QStringLiteral("desktop");
    case Plasma::Types::FullScreen:
        return QStringLiteral("Full Screen");
    case Plasma::Types::TopEdge:
        return QStringLiteral("top");
    case Plasma::Types::BottomEdge:
        return QStringLiteral("bottom");
    case Plasma::Types::LeftEdge:
        return QStringLiteral("left");
    case Plasma::Types::RightEdge:
        return QStringLiteral("right");
    default:
        return QString("unknown");
    }
}

Plasma::Containment *ShellContainmentModel::containmentById(unsigned int id)
{
    for (auto *cont : m_corona->containments()) {
        if (cont->id() == id) {
            return cont;
        }
    }
    return nullptr;
}

QString ShellContainmentModel::containmentPreview(Plasma::Containment *containment)
{
    QString savedThumbnail = m_corona->containmentPreviewPath(containment);

    if (!savedThumbnail.isEmpty()) {
        return savedThumbnail;
    }

    m_corona->grabContainmentPreview(containment);

    // If not found, try to understand the configured wallpaper for the containment, assuming is using the Image plugin
    KSharedConfig::Ptr conf = KSharedConfig::openConfig(QLatin1String("plasma-") + m_corona->shell() + QLatin1String("-appletsrc"), KConfig::SimpleConfig);
    KConfigGroup containmentsGroup(conf, "Containments");
    KConfigGroup config = containmentsGroup.group(QString::number(containment->id()));
    auto wallpaperPlugin = config.readEntry("wallpaperplugin");
    auto wallpaperConfig = config.group("Wallpaper").group(wallpaperPlugin).group("General");

    if (wallpaperConfig.hasKey("Image")) {
        // Trying for the wallpaper
        auto wallpaper = wallpaperConfig.readEntry("Image", QString());
        if (!wallpaper.isEmpty()) {
            return wallpaper;
        }
    }
    if (wallpaperConfig.hasKey("Color")) {
        auto backgroundColor = wallpaperConfig.readEntry("Color", QColor(0, 0, 0));
        return backgroundColor.name();
    }

    return QString();
}

// ---

ShellContainmentConfig::ShellContainmentConfig(ShellCorona *corona, QWindow *parent)
    : QQmlApplicationEngine(parent)
    , m_corona(corona)
    , m_model(nullptr)
{
}

ShellContainmentConfig::~ShellContainmentConfig() = default;

void ShellContainmentConfig::init()
{
    m_model = new ScreenPoolModel(m_corona, this);
    m_model->load();

    auto *localizedContext = new KLocalizedContext(this);
    localizedContext->setTranslationDomain(QStringLiteral("plasma_shell_") + m_corona->shell());

    rootContext()->setContextObject(localizedContext);
    rootContext()->setContextProperty(QStringLiteral("ShellContainmentModel"), m_model);
    load(m_corona->kPackage().fileUrl("containmentmanagementui"));

    if (!rootObjects().isEmpty()) {
        auto *obj = qobject_cast<QWindow *>(rootObjects().first());
        connect(obj, &QWindow::visibleChanged, this, [this, obj]() {
            deleteLater();
        });
    }
}
