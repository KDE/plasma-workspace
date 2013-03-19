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

#ifndef PANELVIEW_H
#define PANELVIEW_H


#include "view.h"
#include "panelconfigview.h"
#include <QtCore/qpointer.h>


class PanelView : public View
{
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)

public:
    explicit PanelView(Plasma::Corona *corona, QWindow *parent = 0);
    virtual ~PanelView();

    virtual KConfigGroup config() const;

    virtual void init();

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    int offset() const;
    void setOffset(int offset);

Q_SIGNALS:
    void offsetChanged();

private Q_SLOTS:
    void manageNewContainment();
    void showPanelController();
    void positionPanel();
    void restore();

private:
    int m_offset;
    int m_maxLength;
    int m_minLength;
    Qt::Alignment m_alignment;
    QPointer<PanelConfigView> m_panelConfigView;
};

#endif // PANELVIEW_H
