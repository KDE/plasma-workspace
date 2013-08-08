/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "view.h"
#include "containmentconfigview.h"
#include "panelconfigview.h"
#include "panelview.h"


#include <QDebug>
#include <QQuickItem>
#include <QQmlContext>
#include <QTimer>
#include <QScreen>
#include "plasma/pluginloader.h"


View::View(Plasma::Corona *corona, QWindow *parent)
    : QQuickView(parent),
      m_corona(corona)
{
    //FIXME: for some reason all windows must have alpha enable otherwise the ones that do won't paint.
    //Probably is an architectural problem
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);

    connect(screen(), &QScreen::virtualGeometryChanged,
            this, &View::screenGeometryChanged);

    if (!m_corona->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    setResizeMode(View::SizeRootObjectToView);
    setSource(QUrl::fromLocalFile(m_corona->package().filePath("views", "Desktop.qml")));

}

View::~View()
{
    
}

Plasma::Corona *View::corona() const
{
    return m_corona;
}

KConfigGroup View::config() const
{
    if (!containment()) {
        return KConfigGroup();
    }
    KConfigGroup views(KSharedConfig::openConfig(), "PlasmaViews");
    return KConfigGroup(&views, QString::number(containment()->screen()));
}

void View::setContainment(Plasma::Containment *cont)
{
    Plasma::Types::Location oldLoc = (Plasma::Types::Location)location();
    Plasma::Types::FormFactor oldForm = formFactor();

    if (m_containment) {
        disconnect(m_containment.data(), 0, this, 0);
        QObject *oldGraphicObject = m_containment.data()->property("graphicObject").value<QObject *>();
        if (oldGraphicObject) {
            //make sure the graphic object won't die with us
            oldGraphicObject->setParent(cont);
        }
    }

    m_containment = cont;

    if (oldLoc != location()) {
        emit locationChanged((Plasma::Types::Location)location());
    }
    if (oldForm != formFactor()) {
        emit formFactorChanged(formFactor());
    }

    emit containmentChanged();

    if (cont) {
        connect(cont, &Plasma::Containment::locationChanged,
                this, &View::locationChanged);
        connect(cont, &Plasma::Containment::formFactorChanged,
                this, &View::formFactorChanged);
        connect(cont, &Plasma::Containment::configureRequested,
                this, &View::showConfigurationInterface);
    } else {
        return;
    }

    QObject *graphicObject = m_containment.data()->property("graphicObject").value<QObject *>();

    if (graphicObject) {
        qDebug() << "using as graphic containment" << graphicObject << m_containment.data();

        //graphicObject->setProperty("visible", false);
        graphicObject->setProperty("drawWallpaper",
                                   (cont->containmentType() == Plasma::Types::DesktopContainment ||
                                    cont->containmentType() == Plasma::Types::CustomContainment));
        graphicObject->setProperty("parent", QVariant::fromValue(rootObject()));
        rootObject()->setProperty("containment", QVariant::fromValue(graphicObject));
    } else {
        qWarning() << "Containment graphic object not valid";
    }
}

Plasma::Containment *View::containment() const
{
    return m_containment.data();
}

//FIXME: wrong types
void View::setLocation(int location)
{
    m_containment.data()->setLocation((Plasma::Types::Location)location);
}

//FIXME: wrong types
int View::location() const
{
    if (!m_containment) {
        return Plasma::Types::Desktop;
    }
    return m_containment.data()->location();
}

Plasma::Types::FormFactor View::formFactor() const
{
    if (!m_containment) {
        return Plasma::Types::Planar;
    }
    return m_containment.data()->formFactor();
}

QRectF View::screenGeometry()
{
    return screen()->geometry();
}

void View::showConfigurationInterface(Plasma::Applet *applet)
{
    if (m_configView) {
        m_configView.data()->hide();
        m_configView.data()->deleteLater();
    }

    if (!applet || !applet->containment()) {
        return;
    }

    Plasma::Containment *cont = qobject_cast<Plasma::Containment *>(applet);
    PanelView *pv = qobject_cast< PanelView* >(this);

    if (cont && pv) {
        m_configView = new PanelConfigView(cont, pv);
    } else if (cont) {
        m_configView = new ContainmentConfigView(cont);
    } else {
        m_configView = new ConfigView(applet);
    }
    m_configView.data()->init();

    m_configView.data()->show();
}

#include "moc_view.cpp"
