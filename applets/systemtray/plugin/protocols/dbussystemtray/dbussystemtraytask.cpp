/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "dbussystemtraytask.h"
#include "debug.h"

#include "dbussystemtrayprotocol.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QMenu>
#include <QtGui/QIcon>
#include <QLoggingCategory>

#include <KJob>
#include <KIconLoader>

#include <Plasma/ServiceJob>
#include <Plasma/Applet>


namespace SystemTray
{

DBusSystemTrayTask::DBusSystemTrayTask(const QString &serviceName, Plasma::DataEngine *dataEngine, QObject *parent)
    : Task(parent),
      m_serviceName(serviceName),
      m_taskId(serviceName),
      m_customIconLoader(0),
      m_dataEngine(dataEngine),
      m_service(dataEngine->serviceForSource(serviceName)),
      m_isMenu(false),
      m_valid(false)
{
    //qCDebug(SYSTEMTRAY);
    m_service->setParent(this);

    m_dataEngine->connectSource(serviceName, this);
}

DBusSystemTrayTask::~DBusSystemTrayTask()
{
}

bool DBusSystemTrayTask::isValid() const
{
    return m_valid;
}

bool DBusSystemTrayTask::isEmbeddable() const
{
    return false; // this task cannot be embed because it only provides information to GUI part
}

bool DBusSystemTrayTask::isWidget() const
{
    return false; // isn't a widget
}

void DBusSystemTrayTask::setShortcut(QString text) {
    if (m_shortcut != text) {
        m_shortcut = text;
        emit changedShortcut();
    }
}


QString DBusSystemTrayTask::taskId() const
{
    return m_taskId;
}

QIcon DBusSystemTrayTask::icon() const
{
    return m_icon;
}

void DBusSystemTrayTask::activate1(int x, int y) const
{
    QVariantMap params;
    if (m_isMenu) {
        params = m_service->operationDescription("ContextMenu");
    } else {
        params = m_service->operationDescription("Activate");
    }
    params["x"] = x;
    params["y"] = y;
    KJob *job = m_service->startOperationCall(params);
    connect(job, SIGNAL(result(KJob*)), this, SLOT(_onContextMenu(KJob*)));
}

void DBusSystemTrayTask::activate2(int x, int y) const
{
    QVariantMap params = m_service->operationDescription("SecondaryActivate");
    params["x"] = x;
    params["y"] = y;
    m_service->startOperationCall(params);
}

void DBusSystemTrayTask::activateHorzScroll(int delta) const
{
    _activateScroll(delta, "Horizontal");
}

// Copied from kde-runtime/plasma/declarativeimports/core/iconitem.cpp
bool static hasm_svgIcon(QVariant source)
{
    // Set up compat envrionment, afterwards it is 100% copy'n'pastable.
    const QString element = source.toString();
    const QString filename = element.split("-").first();
    Plasma::Svg svgIcon;
    Plasma::Svg *m_svgIcon = &svgIcon;

    //try as a svg toolbar icon
    m_svgIcon->setImagePath("toolbar-icons/" + filename);

    //try as a svg normal icon (like systray)
    if (!m_svgIcon->isValid() || !m_svgIcon->hasElement(element)) {
        m_svgIcon->setImagePath("icons/" + filename);
    }
    m_svgIcon->setContainsMultipleImages(true);

    //success?
    if (m_svgIcon->isValid() && m_svgIcon->hasElement(element)) {
        return true;
    }
    return false;
}

QVariant DBusSystemTrayTask::customIcon(QVariant variant) const
{
    if (variant.canConvert<QString>()) {
        // If we have no icon loader there is nothing to be done with strings.
        if (!m_customIconLoader)
            return variant;

        // If an SVG icon can be found (via Plasma theme) the name needs to be
        // passed along for IconItem to be able to resolve to the theme name as
        // well.
        if (hasm_svgIcon(variant))
            return variant;

        // Otherwise return a QIcon from our custom icon loader.
        if (!QIcon::themeSearchPaths().contains(m_iconThemePath)) {
            QIcon::setThemeSearchPaths(QStringList(m_iconThemePath) << QIcon::themeSearchPaths());
        }
        return QVariant(QIcon::fromTheme(variant.toString()));
    } else {
        // Most importantly QIcons. Nothing to do for those.
        return variant;
    }
}

void DBusSystemTrayTask::activateVertScroll(int delta) const
{
    _activateScroll(delta, "Vertical");
}

void DBusSystemTrayTask::_activateScroll(int delta, QString direction) const
{
    QVariantMap params = m_service->operationDescription("Scroll");
    params["delta"] = delta;
    params["direction"] = direction;
    m_service->startOperationCall(params);
}

void DBusSystemTrayTask::activateContextMenu(int x, int y) const
{
    QVariantMap params = m_service->operationDescription("ContextMenu");
    params["x"] = x;
    params["y"] = y;
    KJob *job = m_service->startOperationCall(params);
    connect(job, SIGNAL(result(KJob*)), this, SLOT(_onContextMenu(KJob*)));
}

void DBusSystemTrayTask::_onContextMenu(KJob *job)
{
    if (QCoreApplication::closingDown()) {
        // apparently an edge case can be triggered due to the async nature of all this
        // see: https://bugs.kde.org/show_bug.cgi?id=251977
        return;
    }

    Plasma::ServiceJob *sjob = qobject_cast<Plasma::ServiceJob *>(job);
    if (!sjob) {
        return;
    }

    QMenu *menu = qobject_cast<QMenu *>(sjob->result().value<QObject *>());
    if (menu) {
        int x = sjob->parameters()["x"].toInt();
        int y = sjob->parameters()["y"].toInt();
        emit showContextMenu(x, y, QVariant::fromValue<QObject*>(menu));
    }
}

void DBusSystemTrayTask::dataUpdated(const QString &taskName, const Plasma::DataEngine::Data &properties)
{
    Q_UNUSED(taskName);

    const QString id = properties["Id"].toString();
    bool becomeValid = false;
    if (!id.isEmpty() && id != m_taskId) {
        m_taskId = id;
        m_valid = true;
        becomeValid = true;
        setObjectName(QString("SystemTray-%1").arg(m_taskId));
    }

    QString cat = properties["Category"].toString();
    if (!cat.isEmpty()) {
        int index = metaObject()->indexOfEnumerator("Category");
        int key = metaObject()->enumerator(index).keyToValue(cat.toLatin1());

        if (key != -1) {
            setCategory((Task::Category)key);
        }
    }

    if (properties["TitleChanged"].toBool() || becomeValid) {
        QString title = properties["Title"].toString();
        if (!title.isEmpty()) {
            bool is_title_changed = (name() != title);
            setName(title);
            if (is_title_changed) {
                emit changedTitle();
                emit changed(this);
            }
        }
    }

    if (properties["IconsChanged"].toBool() || becomeValid) {
        syncIcons(properties);
        emit changedIcons();
    }

    if (properties["StatusChanged"].toBool() || becomeValid) {
        syncStatus(properties["Status"].toString());
    }

    if (properties["ToolTipChanged"].toBool() || becomeValid) {
        syncToolTip(properties["ToolTipTitle"].toString(),
                    properties["ToolTipSubTitle"].toString(),
                    properties["ToolTipIcon"].value<QIcon>());
    }

    bool is_menu = properties["ItemIsMenu"].toBool();
    if (is_menu != m_isMenu) {
        m_isMenu = is_menu;
        emit changedIsMenu();
    }

    if (becomeValid) {
        DBusSystemTrayProtocol *protocol = qobject_cast<DBusSystemTrayProtocol*>(parent());
        if (protocol) {
            protocol->initedTask(this);
        }
    }
}

void DBusSystemTrayTask::syncIcons(const Plasma::DataEngine::Data &properties)
{
    m_icon = properties["Icon"].value<QIcon>();
    m_attentionIcon = properties["AttentionIcon"].value<QIcon>();

    QString icon_name            = properties["IconName"].toString();
    QString att_icon_name        = properties["AttentionIconName"].toString();
    QString movie_path           = properties["AttentionMovieName"].toString();
    QString overlay_icon_name    = properties["OverlayIconName"].value<QString>();
    QString icon_theme_path      = properties["IconThemePath"].value<QString>();
    bool is_icon_name_changed           = false;
    bool is_att_icon_name_changed       = false;
    bool is_movie_path_changed          = false;
    bool is_overlay_icon_name_changed   = false;

    if (icon_name != m_iconName) {
        m_iconName = icon_name;
        is_icon_name_changed = true;
    }

    if (att_icon_name != m_attentionIconName) {
        m_attentionIconName = att_icon_name;
        is_att_icon_name_changed = true;
    }

    if (!movie_path.isEmpty() && !QDir::isAbsolutePath(movie_path)) {
        movie_path = KIconLoader::global()->moviePath(movie_path, KIconLoader::Panel);
    }

    if (movie_path != m_moviePath) {
        m_moviePath = movie_path;
        is_movie_path_changed = true;
    }

    if (overlay_icon_name != m_overlayIconName) {
        m_overlayIconName = overlay_icon_name;
        is_overlay_icon_name_changed = true;
    }

    if (icon_theme_path != m_iconThemePath) {
        m_iconThemePath = icon_theme_path;

        if (m_customIconLoader) {
            delete m_customIconLoader;
            m_customIconLoader = 0;
        }
        const QString path = m_iconThemePath;
        if (!path.isEmpty()) {
            // FIXME: If last part of path is not "icons", this won't work!
            QStringList tokens = path.split('/', QString::SkipEmptyParts);
            if (tokens.length() >= 3 && tokens.takeLast() == QLatin1String("icons")) {
                QString appName = tokens.takeLast();
                QString prefix = QChar('/') + tokens.join("/");
                if (!QIcon::themeSearchPaths().contains(prefix)) {
                    QIcon::setThemeSearchPaths(QStringList(prefix) << QIcon::themeSearchPaths());
                }
                // We use a separate instance of KIconLoader to avoid
                // adding all application dirs to KIconLoader::global(), to
                // avoid potential icon name clashes between application
                // icons
                m_customIconLoader = new KIconLoader(appName, QStringList() /* dirs */, this);
            } else {
                qWarning() << "Wrong IconThemePath" << path << ": too short or does not end with 'icons'";
            }
        }

        // Trigger updates
        is_icon_name_changed = true;
        is_att_icon_name_changed = true;
        is_overlay_icon_name_changed = true;
        emit changedIcons();
    }

    // emit signals
    if (is_icon_name_changed) {
        emit changedIconName();
    }
    if (is_att_icon_name_changed) {
        emit changedAttIconName();
    }
    if (is_movie_path_changed) {
        emit changedMoviePath();
    }
    if (is_overlay_icon_name_changed) {
        emit changedOverlayIconName();
    }
}


//toolTip

void DBusSystemTrayTask::syncToolTip(const QString &title, const QString &subTitle, const QIcon &toolTipIcon)
{
    if (title != m_tooltipTitle) {
        m_tooltipTitle = title;
        emit changedTooltipTitle();
    }

    if (subTitle != m_tooltipText) {
        m_tooltipText = subTitle;
        emit changedTooltipText();
    }

    bool is_icon_name_changed = (m_tooltipIcon.name() != toolTipIcon.name());

    m_tooltipIcon = toolTipIcon;
    emit changedTooltip();

    if (is_icon_name_changed) {
        emit changedTooltipIconName();
    }
}


//Status

void DBusSystemTrayTask::syncStatus(QString newStatus)
{
    Task::Status status = (Task::Status)metaObject()->enumerator(metaObject()->indexOfEnumerator("Status")).keyToValue(newStatus.toLatin1());
    qCDebug(SYSTEMTRAY) << "ST2 status changed " << this->status() << status;

    if (this->status() == status) {
        return;
    }

    setStatus(status);
}

}

#include "dbussystemtraytask.moc"
