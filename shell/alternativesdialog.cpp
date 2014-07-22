/*
 *   Copyright 2014 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "alternativesdialog.h"

#include <QQmlEngine>
#include <QQmlContext>

#include <kdeclarative/qmlobject.h>
#include <Plasma/Package>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

AlternativesDialog::AlternativesDialog(Plasma::Applet *applet, QQuickItem *parent)
    : PlasmaQuick::Dialog(parent),
      m_applet(applet)
{
    setVisualParent(applet->property("_plasma_graphicObject").value<QQuickItem *>());
    connect(applet, &QObject::destroyed, this, &AlternativesDialog::close);
    setLocation(applet->location());
    setFlags(flags()|Qt::WindowStaysOnTopHint);
    //We already have the proper shellpluginloader
    Plasma::Package pkg;
    if (applet && applet->containment() && applet->containment()->corona()) {
        pkg = applet->containment()->corona()->package();
    }
    //TODO: use the proper package: we must be in the corona
    pkg.setPath("org.kde.plasma.desktop");

    m_qmlObj = new KDeclarative::QmlObject(this);
    m_qmlObj->setInitializationDelayed(true);
    m_qmlObj->setSource(QUrl::fromLocalFile(pkg.filePath("explorer", "AppletAlternatives.qml")));
    m_qmlObj->engine()->rootContext()->setContextProperty("alternativesDialog", this);
    m_qmlObj->completeInitialization();
    setMainItem(qobject_cast<QQuickItem *>(m_qmlObj->rootObject()));
}

AlternativesDialog::~AlternativesDialog()
{
}

QStringList AlternativesDialog::appletProvides() const
{
    return m_applet->pluginInfo().property("X-Plasma-Provides").value<QStringList>();
}

QString AlternativesDialog::currentPlugin() const
{
    return m_applet->pluginInfo().pluginName();
}

void AlternativesDialog::loadAlternative(const QString &plugin)
{
    if (plugin == m_applet->pluginInfo().pluginName() || m_applet->isContainment()) {
        return;
    }

    Plasma::Containment *cont = m_applet->containment();
    if (!cont) {
        return;
    }

    QQuickItem *appletItem = m_applet->property("_plasma_graphicObject").value<QQuickItem *>();
    QQuickItem *contItem = cont->property("_plasma_graphicObject").value<QQuickItem *>();
    if (!appletItem || !contItem) {
        return;
    }

    //TODO: map the position to containment coordinates
    QMetaObject::invokeMethod(contItem, "createApplet", Q_ARG(QString, plugin), Q_ARG(QVariantList, QVariantList()), Q_ARG(QPoint, appletItem->mapToItem(contItem, QPointF(0,0)).toPoint()));

    m_applet->destroy();
}

//To emulate Qt::WA_DeleteOnClose that QWindow doesn't have
void AlternativesDialog::hideEvent(QHideEvent *ev)
{
    QQuickWindow::hideEvent(ev);
    deleteLater();
}

#include "moc_alternativesdialog.cpp"

