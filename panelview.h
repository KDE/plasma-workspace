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


#include <plasmaquickview.h>
#include "panelconfigview.h"
#include <QtCore/qpointer.h>

class ShellCorona;

class PanelView : public PlasmaQuickView
{
    Q_OBJECT
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY maximumLengthChanged)
    Q_PROPERTY(int minimumLength READ minimumLength WRITE setMinimumLength NOTIFY minimumLengthChanged)
    Q_PROPERTY(QScreen *screen READ screen NOTIFY screenChanged)
    Q_PROPERTY(VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode)

public:

    enum VisibilityMode {
        NormalPanel = 0,
        AutoHide,
        LetWindowsCover,
        WindowsGoBelow
    };
    Q_ENUMS(VisibilityMode)

    explicit PanelView(ShellCorona *corona, QWindow *parent = 0);
    virtual ~PanelView();

    virtual KConfigGroup config() const;

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    int offset() const;
    void setOffset(int offset);

    int thickness() const;
    void setThickness(int thickness);

    int length() const;
    void setLength(int value);

    int maximumLength() const;
    void setMaximumLength(int length);

    int minimumLength() const;
    void setMinimumLength(int length);

    VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

protected:
    void resizeEvent(QResizeEvent *ev);
    void showEvent(QShowEvent *event);

Q_SIGNALS:
    void alignmentChanged();
    void offsetChanged();
    void screenGeometryChanged();
    void thicknessChanged();
    void lengthChanged();
    void maximumLengthChanged();
    void minimumLengthChanged();
    void screenChanged(QScreen *screen);

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    virtual void showConfigurationInterface(Plasma::Applet *applet);
    void updateStruts();

private Q_SLOTS:
    void positionPanel();
    void restore();

private:
    int m_offset;
    int m_maxLength;
    int m_minLength;
    Qt::Alignment m_alignment;
    QPointer<ConfigView> m_panelConfigView;
    ShellCorona *m_corona;
    QTimer *m_strutsTimer;
    VisibilityMode m_visibilityMode;

    static const int STRUTSTIMERDELAY = 200;
};

#endif // PANELVIEW_H
