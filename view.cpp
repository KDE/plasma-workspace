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

#include <QDebug>
#include <QQuickItem>
#include <QQmlContext>
#include <QTimer>
#include "plasma/pluginloader.h"

#include <KGlobal>

View::View(Plasma::Corona *corona, QWindow *parent)
    : QQuickView(parent),
      m_corona(corona)
{
    //FIXME: for some reason all windows must have alpha enable otherwise the ones that do won't paint.
    //Probably is an architectural problem
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
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
    KConfigGroup views(KGlobal::config(), "PlasmaViews");
    return KConfigGroup(&views, QString::number(containment()->screen()));
}

void View::init()
{
    if (!m_corona->package().isValid()) {
        qWarning() << "Invalid home screen package";
    }

    setResizeMode(View::SizeRootObjectToView);
    setSource(QUrl::fromLocalFile(m_corona->package().filePath("views", "Desktop.qml")));
}

void View::setContainment(Plasma::Containment *cont)
{
    Plasma::Location oldLoc = location();
    Plasma::FormFactor oldForm = formFactor();

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
        emit locationChanged(location());
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
    } else {
        return;
    }

    QObject *graphicObject = m_containment.data()->property("graphicObject").value<QObject *>();
    if (graphicObject) {
        qDebug() << "using as graphic containment" << graphicObject << m_containment.data();

        //graphicObject->setProperty("visible", false);
        graphicObject->setProperty("drawWallpaper",
                                   (cont->containmentType() == Plasma::DesktopContainment ||
                                    cont->containmentType() == Plasma::CustomContainment));
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

Plasma::Location View::location()
{
    if (!m_containment) {
        return Plasma::Desktop;
    }
    return m_containment.data()->location();
}

Plasma::FormFactor View::formFactor()
{
    if (!m_containment) {
        return Plasma::Planar;
    }
    return m_containment.data()->formFactor();
}

#include "moc_view.cpp"
