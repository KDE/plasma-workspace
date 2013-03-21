/*
 *   Copyright (C) 2007 by Ivan Cukic <ivan.cukic+kde@gmail.com>
 *   Copyright (C) 2009 by Ana Cecília Martins <anaceciliamb@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef WIDGETEXPLORER_H
#define WIDGETEXPLORER_H

#include <QtGui>

#include <KDE/KDialog>

#include <plasma/framesvg.h>

#include "plasmaappletitemmodel_p.h"

#include "plasmagenericshell_export.h"

namespace Plasma
{

class Corona;
class Containment;
class Applet;
class WidgetExplorerPrivate;
class WidgetExplorerPrivate;

//We need to access the separator property that is not exported by QAction
class WidgetAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator)

public:
    WidgetAction(QObject *parent = 0);
    WidgetAction(const QIcon &icon, const QString &text, QObject *parent);
};

class PLASMAGENERICSHELL_EXPORT WidgetExplorer : public QGraphicsWidget
{

    Q_OBJECT

    /**
     * Model that lists all applets
     */
    Q_PROPERTY(QObject * widgetsModel READ widgetsModel CONSTANT)

    /**
     * Model that lists all applets filters and categories
     */
    Q_PROPERTY(QObject * filterModel READ filterModel CONSTANT)

    /**
     * Actions for adding widgets, like download plasma widgets, download google gadgets, install from local file
     */
    Q_PROPERTY(QList<QObject *> widgetsMenuActions READ widgetsMenuActions NOTIFY widgetsMenuActionsChanged)

    /**
     * Extra actions assigned by the shell, like switch to activity manager
     */
    Q_PROPERTY(QList<QObject *> extraActions READ extraActions NOTIFY extraActionsChanged)

    /**
     * Plasma location of the panel containment the controller is associated to
     */
    Q_PROPERTY(Location location READ location NOTIFY locationChanged)
    Q_ENUMS(Location)

    /**
     * Orientation the controller will be disaplayed, depends from location
     */
    Q_PROPERTY(Qt::Orientation orientation READ orientation NOTIFY orientationChanged)

public:
    /**
    * The Location enumeration describes where on screen an element, such as an
    * Applet or its managing container, is positioned on the screen.
    **/
    enum Location {
        Floating = 0, /**< Free floating. Neither geometry or z-ordering
                        is described precisely by this value. */
        Desktop,      /**< On the planar desktop layer, extending across
                        the full screen from edge to edge */
        FullScreen,   /**< Full screen */
        TopEdge,      /**< Along the top of the screen*/
        BottomEdge,   /**< Along the bottom of the screen*/
        LeftEdge,     /**< Along the left side of the screen */
        RightEdge     /**< Along the right side of the screen */
    };

    explicit WidgetExplorer(Plasma::Location loc, QGraphicsItem *parent = 0);
    explicit WidgetExplorer(QGraphicsItem *parent = 0);
    ~WidgetExplorer();

    QString application();

    /**
     * Populates the widget list for the given application. This must be called
     * before the widget explorer will be usable as the widget list will remain
     * empty up to that point.
     *
     * @arg application the application which the widgets should be loaded for.
     */
    void populateWidgetList(const QString &application = QString());

    /**
     * Changes the current default containment to add applets to
     *
     * @arg containment the new default
     */
    void setContainment(Plasma::Containment *containment);

    /**
     * @return the current default containment to add applets to
     */
    Containment *containment() const;
    /**
     * @return the current corona this widget is added to
     */
    Plasma::Corona *corona() const;


    void setLocation(const Plasma::Location loc);
     //FIXME: it's asymmetric due to the problems of QML of exporting enums
    WidgetExplorer::Location location();

    Qt::Orientation orientation() const;


    QObject *widgetsModel() const;
    QObject *filterModel() const;

    QList <QObject *>  widgetsMenuActions();
    QList <QObject *>  extraActions() const;

    Q_INVOKABLE void uninstall(const QString &pluginName);

    Q_INVOKABLE QPoint tooltipPosition(QGraphicsObject *item, int tipWidth, int tipHeight);

Q_SIGNALS:
    void locationChanged(Plasma::Location loc);
    void orientationChanged();
    void closeClicked();
    void widgetsMenuActionsChanged();
    void extraActionsChanged();

public Q_SLOTS:
    /**
     * Adds currently selected applets
     */
    void addApplet(const QString &pluginName);
    void openWidgetFile();
    void downloadWidgets(const QString &type);

protected Q_SLOTS:
    void immutabilityChanged(Plasma::ImmutabilityType);

protected:
    void keyPressEvent(QKeyEvent *e);
    bool event(QEvent *e);
    void focusInEvent(QFocusEvent * event);

private:
    Q_PRIVATE_SLOT(d, void appletAdded(Plasma::Applet*))
    Q_PRIVATE_SLOT(d, void appletRemoved(Plasma::Applet*))
    Q_PRIVATE_SLOT(d, void containmentDestroyed())
    Q_PRIVATE_SLOT(d, void finished())

    WidgetExplorerPrivate * const d;
    friend class WidgetExplorerPrivate;
};

} // namespace Plasma

#endif // WIDGETEXPLORER_H
