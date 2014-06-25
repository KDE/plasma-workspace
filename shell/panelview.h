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

#include <view.h>
#include <QtCore/qpointer.h>
#include <Plasma/Theme>
#include <QTimer>


#include <configview.h>

class ShellCorona;

class PanelView : public PlasmaQuick::View
{
    Q_OBJECT
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY maximumLengthChanged)
    Q_PROPERTY(int minimumLength READ minimumLength WRITE setMinimumLength NOTIFY minimumLengthChanged)
    Q_PROPERTY(int distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(QScreen *screen READ screen WRITE setScreen NOTIFY screenChangedProxy)
    Q_PROPERTY(VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)

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

    Q_INVOKABLE void maximize();

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

    int distance() const;
    void setDistance(int dist);

    VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    /**
     * @returns the geometry of the panel given a distance
     */
    QRect geometryByDistance(int distance) const;

protected:
    void resizeEvent(QResizeEvent *ev);
    void showEvent(QShowEvent *event);
    void moveEvent(QMoveEvent *ev);
    bool event(QEvent *e);
    void updateMask();

Q_SIGNALS:
    void alignmentChanged();
    void offsetChanged();
    void screenGeometryChanged();
    void thicknessChanged();
    void lengthChanged();
    void maximumLengthChanged();
    void minimumLengthChanged();
    void distanceChanged();

    //QWindow does not have a property for screen. Adding this property requires re-implementing the signal
    void screenChangedProxy(QScreen *screen);
    void visibilityModeChanged();

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    virtual void showConfigurationInterface(Plasma::Applet *applet);
    void updateStruts();

private Q_SLOTS:
    void themeChanged();
    void positionPanel();
    void restore();
    void setAutoHideEnabled(bool autoHideEnabled);
    void showTemporarily();
    void containmentChanged();
    void statusChanged(Plasma::Types::ItemStatus);
    void restoreAutoHide();
    void screenDestroyed(QObject* screen);

private:
    void integrateScreen();

    int m_offset;
    int m_maxLength;
    int m_minLength;
    int m_distance;
    int m_thickness;
    Qt::Alignment m_alignment;
    QPointer<PlasmaQuick::ConfigView> m_panelConfigView;
    ShellCorona *m_corona;
    QTimer m_strutsTimer;
    VisibilityMode m_visibilityMode;
    Plasma::Theme m_theme;
    QTimer m_positionPaneltimer;
    QTimer m_unhideTimer;
    //only for the mask, not to actually paint
    Plasma::FrameSvg *m_background;

    static const int STRUTSTIMERDELAY = 200;
};

#endif // PANELVIEW_H
