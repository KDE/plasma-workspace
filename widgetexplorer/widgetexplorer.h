/*
 *   Copyright (C) 2007 by Ivan Cukic <ivan.cukic+kde@gmail.com>
 *   Copyright (C) 2009 by Ana Cecília Martins <anaceciliamb@gmail.com>
 *   Copyright 2013 by Sebastian Kügler <sebas@kde.org>
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

#include <QAction>
#include <QQuickItem>

#include "plasmaappletitemmodel_p.h"

namespace Plasma {
    class Corona;
    class Containment;
    class Applet;
}
class WidgetExplorerPrivate;

//We need to access the separator property that is not exported by QAction
class WidgetAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator NOTIFY separatorChanged)

public:
    WidgetAction(QObject *parent = 0);
    WidgetAction(const QIcon &icon, const QString &text, QObject *parent);

Q_SIGNALS:
    void separatorChanged();
};

class WidgetExplorer : public QQuickItem
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
    Q_PROPERTY(Plasma::Types::Location location READ location NOTIFY locationChanged)
    Q_ENUMS(Location)

    /**
     * Orientation the controller will be disaplayed, depends from location
     */
    Q_PROPERTY(Qt::Orientation orientation READ orientation NOTIFY orientationChanged)

public:
    explicit WidgetExplorer(QQuickItem *parent = 0);
    ~WidgetExplorer();

    QString application();

    /**
     * Sets the path of the QML file to parse and execute
     *
     * @param path the absolute path of a QML file
     */
    void setSource(const QUrl &source);

    /**
     * @return the absolute path of the current QML file
     */
    QUrl source() const;

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
    Plasma::Containment *containment() const;
    /**
     * @return the current corona this widget is added to
     */
    Plasma::Corona *corona() const;


    void setLocation(const Plasma::Types::Location loc);
    Plasma::Types::Location location() const;

    Qt::Orientation orientation() const;


    QObject *widgetsModel() const;
    QObject *filterModel() const;

    QList <QObject *>  widgetsMenuActions();
    QList <QObject *>  extraActions() const;

    /**
     * Uninstall a plasmoid with a given plugin name. only user-installed ones are uninstallable
     */
    Q_INVOKABLE void uninstall(const QString &pluginName);

    Q_INVOKABLE void close();
    //Q_INVOKABLE QPoint tooltipPosition(QGraphicsObject *item, int tipWidth, int tipHeight);

Q_SIGNALS:
    void locationChanged(Plasma::Types::Location loc);
    void orientationChanged();
    void widgetsMenuActionsChanged();
    void extraActionsChanged();
    void closed();

public Q_SLOTS:
    /**
     * Adds currently selected applets
     */
    void addApplet(const QString &pluginName);
    void openWidgetFile();
    void downloadWidgets(const QString &type);

protected Q_SLOTS:
    void immutabilityChanged(Plasma::Types::ImmutabilityType);

private:
    Q_PRIVATE_SLOT(d, void appletAdded(Plasma::Applet*))
    Q_PRIVATE_SLOT(d, void appletRemoved(Plasma::Applet*))
    Q_PRIVATE_SLOT(d, void containmentDestroyed())

    WidgetExplorerPrivate * const d;
    friend class WidgetExplorerPrivate;
};


#endif // WIDGETEXPLORER_H
