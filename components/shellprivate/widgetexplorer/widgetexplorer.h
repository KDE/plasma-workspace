/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
    SPDX-FileCopyrightText: 2009 Ana Cecília Martins <anaceciliamb@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAction>
#include <QObject>
#include <QQmlParserStatus>

#include <Plasma>

#include "plasmaappletitemmodel_p.h"

namespace Plasma
{
class Corona;
class Containment;
class Applet;
}
class WidgetExplorerPrivate;

// We need to access the separator property that is not exported by QAction
class WidgetAction : public QAction
{
    Q_OBJECT
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator NOTIFY separatorChanged)

public:
    explicit WidgetAction(QObject *parent = nullptr);
    WidgetAction(const QIcon &icon, const QString &text, QObject *parent);

Q_SIGNALS:
    void separatorChanged();
};

class WidgetExplorer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    /**
     * Model that lists all applets
     */
    Q_PROPERTY(QObject *widgetsModel READ widgetsModel CONSTANT)

    /**
     * Model that lists all applets filters and categories
     */
    Q_PROPERTY(QObject *filterModel READ filterModel CONSTANT)

    /**
     * Whether to show special filters such as "Running" and "Uninstallable" in the filterModel.
     */
    Q_PROPERTY(bool showSpecialFilters READ showSpecialFilters WRITE setShowSpecialFilters NOTIFY showSpecialFiltersChanged)

    /**
     * Actions for adding widgets, like download plasma widgets, download google gadgets, install from local file
     */
    Q_PROPERTY(QList<QObject *> widgetsMenuActions READ widgetsMenuActions NOTIFY widgetsMenuActionsChanged)

    /**
     * The application that owns the widget list. different application may show different lists
     */
    Q_PROPERTY(QString application READ application WRITE setApplication NOTIFY applicationChanged)

    /**
     * Set the features the listed applets must provide: needed for listing alternatives
     * to a particular applet
     */
    Q_PROPERTY(QStringList provides READ provides WRITE setProvides NOTIFY providesChanged)

    Q_PROPERTY(Plasma::Containment *containment READ containment WRITE setContainment NOTIFY containmentChanged)

public:
    explicit WidgetExplorer(QObject *parent = nullptr);
    ~WidgetExplorer() override;

    QString application();

    /**
     * Populates the widget list for the given application. This must be called
     * before the widget explorer will be usable as the widget list will remain
     * empty up to that point.
     *
     * @arg application the application which the widgets should be loaded for.
     */
    void setApplication(const QString &application = QString());

    QStringList provides() const;
    void setProvides(const QStringList &provides);

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

    QObject *widgetsModel() const;
    QObject *filterModel() const;

    bool showSpecialFilters() const;
    void setShowSpecialFilters(bool show);

    QList<QObject *> widgetsMenuActions();

    /**
     * Uninstall a plasmoid with a given plugin name. only user-installed ones are uninstallable
     */
    Q_INVOKABLE void uninstall(const QString &pluginName);

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void widgetsMenuActionsChanged();
    void extraActionsChanged();
    void shouldClose();
    void viewChanged();
    void applicationChanged();
    void containmentChanged();
    void providesChanged();

public Q_SLOTS:
    /**
     * Adds currently selected applets
     */
    void addApplet(const QString &pluginName);
    void openWidgetFile();
    void downloadWidgets();

Q_SIGNALS:
    void showSpecialFiltersChanged() const;

protected Q_SLOTS:
    void immutabilityChanged(Plasma::Types::ImmutabilityType);

private:
    Q_PRIVATE_SLOT(d, void appletAdded(Plasma::Applet *))
    Q_PRIVATE_SLOT(d, void appletRemoved(Plasma::Applet *))
    Q_PRIVATE_SLOT(d, void containmentDestroyed())

    WidgetExplorerPrivate *const d;
    friend class WidgetExplorerPrivate;
};
