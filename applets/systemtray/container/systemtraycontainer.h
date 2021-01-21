/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *
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

#ifndef SYSTEMTRAYCONTAINER_H
#define SYSTEMTRAYCONTAINER_H

#include <Plasma/Applet>

class QQuickItem;

/**
 * @brief Thin wrapping 'Plasma::Applet' for SystemTray.
 *
 * SystemTray is of 'Plasma::Containment' type. To have it presented as a widget in Plasma we need a wrapping applet.
 */
class SystemTrayContainer : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *internalSystray READ internalSystray NOTIFY internalSystrayChanged)

public:
    SystemTrayContainer(QObject *parent, const QVariantList &args);
    ~SystemTrayContainer() override;

    void init() override;

    QQuickItem *internalSystray();

protected:
    void constraintsEvent(Plasma::Types::Constraints constraints) override;
    void ensureSystrayExists();

Q_SIGNALS:
    void internalSystrayChanged();

private:
    QPointer<Plasma::Containment> m_innerContainment;
    QPointer<QQuickItem> m_internalSystray;
};

#endif
