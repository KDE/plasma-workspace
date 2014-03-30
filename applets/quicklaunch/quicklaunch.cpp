/***************************************************************************
 *   Copyright (C) 2008 - 2009 by Lukas Appelhans <l.appelhans@gmx.de>     *
 *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>         *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "quicklaunch.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsWidget>

// KDE
#include <KConfig>
#include <KConfigGroup>
#include <KConfigDialog>
#include <KDesktopFile>
#include <KDialog>
#include <KEMailSettings>
#include <KGlobal>
#include <KGlobalSettings>
#include <KMenu>
#include <KMessageBox>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KOpenWithDialog>
#include <KPropertiesDialog>
#include <KSharedConfig>
#include <KShell>
#include <KStandardDirs>
#include <KWindowSystem>
#include <KUrl>

// Plasma
#include <Plasma/Applet>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/IconWidget>
#include <Plasma/ToolTipContent>
#include <Plasma/ToolTipManager>

// Own
#include "launcherdata.h"
#include "launchergrid.h"
#include "popup.h"
#include "popuplauncherlist.h"

using Plasma::Applet;
using Plasma::Constraints;
using Plasma::FormFactor;
using Plasma::IconWidget;
using Plasma::Location;
using Plasma::Svg;
using Plasma::ToolTipContent;
using Plasma::ToolTipManager;

namespace Quicklaunch {

K_EXPORT_PLASMA_APPLET(quicklaunch, Quicklaunch)

Quicklaunch::Quicklaunch(QObject *parent, const QVariantList &args)
  : Applet(parent, args),
    m_launcherGrid(0),
    m_layout(0),
    m_popupTrigger(0),
    m_popup(0),
    m_addLauncherAction(0),
    m_removeLauncherAction(0),
    m_contextMenuTriggeredOnPopup(false),
    m_contextMenuLauncherIndex(-1)
{
    setHasConfigurationInterface(true);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setBackgroundHints(TranslucentBackground);
}

Quicklaunch::~Quicklaunch()
{
    if (m_popup) {
        deletePopup();
    }
}

void Quicklaunch::init()
{
    // Initialize outer layout
    m_layout = new QGraphicsLinearLayout();
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(4);

    // Initialize icon area
    m_launcherGrid = new LauncherGrid();
    m_launcherGrid->setMaxSectionCountForced(true);
    m_launcherGrid->installEventFilter(this);

    m_layout->addItem(m_launcherGrid);
    m_layout->setStretchFactor(m_launcherGrid, 1);

    configChanged();
    iconSizeChanged();

    connect(
        m_launcherGrid, SIGNAL(launchersChanged()), SLOT(onLaunchersChanged()));

    connect(
        KGlobalSettings::self(),
        SIGNAL(iconChanged(int)), SLOT(iconSizeChanged()));

    setLayout(m_layout);
}

void Quicklaunch::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget(parent);
    uiConfig.setupUi(widget);
    connect(parent, SIGNAL(applyClicked()), SLOT(onConfigAccepted()));
    connect(parent, SIGNAL(okClicked()), SLOT(onConfigAccepted()));

    FormFactor appletFormFactor = formFactor();

    if (appletFormFactor == Plasma::Horizontal) {
        uiConfig.autoSectionCountEnabledLabel->setText(i18n(
            "Determine number of rows automatically:"));
        uiConfig.sectionCountLabel->setText(i18n(
            "Number of rows:"));
    } else if (appletFormFactor == Plasma::Planar) {
        // Hide wrapLimit / maxSectionCountForced when in planar
        // form factor.
        uiConfig.autoSectionCountEnabledLabel->hide();
        uiConfig.autoSectionCountEnabledCheckBox->hide();
        uiConfig.sectionCountLabel->hide();
        uiConfig.sectionCountSpinBox->hide();
    } else {
        uiConfig.autoSectionCountEnabledLabel->setText(i18n(
            "Determine number of columns automatically:"));
        uiConfig.sectionCountLabel->setText(i18n(
            "Number of columns:"));
    }

    uiConfig.autoSectionCountEnabledCheckBox->setChecked(
        m_launcherGrid->maxSectionCount() == 0);

    uiConfig.sectionCountSpinBox->setValue(
        m_launcherGrid->maxSectionCount() > 0
            ? m_launcherGrid->maxSectionCount()
            : 1);

    uiConfig.launcherNamesVisibleCheckBox->setChecked(
        m_launcherGrid->launcherNamesVisible());

    uiConfig.popupEnabledCheckBox->setChecked(m_popup != 0);

    parent->addPage(widget, i18n("General"), icon());

    connect(uiConfig.autoSectionCountEnabledCheckBox, SIGNAL(stateChanged(int)), parent, SLOT(settingsModified()));
    connect(uiConfig.sectionCountSpinBox, SIGNAL(valueChanged(int)), parent, SLOT(settingsModified()));
    connect(uiConfig.launcherNamesVisibleCheckBox, SIGNAL(stateChanged(int)), parent, SLOT(settingsModified()));
    connect(uiConfig.popupEnabledCheckBox, SIGNAL(stateChanged(int)), parent, SLOT(settingsModified()));
}

bool Quicklaunch::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type eventType = event->type();

    if (eventType == QEvent::GraphicsSceneContextMenu) {

        QGraphicsSceneContextMenuEvent *contextMenuEvent =
            static_cast<QGraphicsSceneContextMenuEvent*>(event);

        if (watched == m_launcherGrid) {
            int launcherIndex =
                m_launcherGrid->launcherIndexAtPosition(
                    m_launcherGrid->mapFromScene(contextMenuEvent->scenePos()));

            showContextMenu(contextMenuEvent->screenPos(), false, launcherIndex);
            return true;
        }

        if (m_popup != 0 && watched == m_popup->launcherList()) {

            PopupLauncherList *launcherList = m_popup->launcherList();
            int launcherIndex =
                launcherList->launcherIndexAtPosition(
                    launcherList->mapFromScene(contextMenuEvent->scenePos()));

            showContextMenu(contextMenuEvent->screenPos(), true, launcherIndex);
            return true;
        }
    }
    else if ((eventType == QEvent::Hide || eventType == QEvent::Show) &&
             m_popup != 0 &&
             watched == m_popup) {

            updatePopupTrigger();
    }
    else if (eventType == QEvent::GraphicsSceneDragEnter &&
             m_popupTrigger != 0 &&
             m_popup->isHidden() &&
             watched == m_popupTrigger) {

        m_popup->show();
        event->setAccepted(false);
        return true;
    }

    return false;
}

void Quicklaunch::constraintsEvent(Constraints constraints)
{
    if (constraints & Plasma::FormFactorConstraint) {
        FormFactor ff = formFactor();

        m_launcherGrid->setLayoutMode(
            ff == Plasma::Horizontal
                ? LauncherGrid::PreferRows
                : LauncherGrid::PreferColumns);

        if (ff == Plasma::Planar || ff == Plasma::MediaCenter) {
            // Ignore maxSectionCount in these form factors.
            m_launcherGrid->setMaxSectionCount(0);
        }

        // Apply icon size
        iconSizeChanged();

        m_layout->setOrientation(
            ff == Plasma::Vertical ? Qt::Vertical : Qt::Horizontal);
    }

    if (constraints & Plasma::LocationConstraint) {

        if (m_popupTrigger) {
            updatePopupTrigger();
        }
    }

    if (constraints & Plasma::ImmutableConstraint) {

        bool lock = immutability() != Plasma::Mutable;

        m_launcherGrid->setLocked(lock);
        if (m_popup) {
            m_popup->launcherList()->setLocked(lock);
        }
    }
}

void Quicklaunch::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    showContextMenu(event->screenPos(), 0, -1);
}

void Quicklaunch::configChanged()
{
    KConfigGroup config = this->config();

    // Migrate old configuration keys
    if (config.hasKey("dialogIconSize") ||
        config.hasKey("iconSize") ||
        config.hasKey("iconUrls") ||
        config.hasKey("showIconNames") ||
        config.hasKey("visibleIcons")) {

        // Migrate from Quicklaunch 0.1 config format
        QStringList iconUrls = config.readEntry("iconUrls", QStringList());

        int visibleIcons =
            qBound(-1, config.readEntry("visibleIcons", -1), iconUrls.size());

        bool showIconNames = config.readEntry("showIconNames", false);

        config.deleteEntry("dialogIconSize");
        config.deleteEntry("iconSize");
        config.deleteEntry("iconUrls");
        config.deleteEntry("showIconNames");
        config.deleteEntry("visibleIcons");

        QStringList launchers;
        QStringList launchersOnPopup;

        for (int i = 0; i < iconUrls.size(); i++) {
            if (visibleIcons == -1 || i < visibleIcons) {
                launchers.append(iconUrls.at(i));
            } else {
                launchersOnPopup.append(iconUrls.at(i));
            }
        }

        config.writeEntry("launchers", launchers);
        config.writeEntry("launchersOnPopup", launchersOnPopup);
        config.writeEntry("launcherNamesVisible", showIconNames);
    }

    if (config.hasKey("icons") ||
        config.hasKey("dialogIcons") ||
        config.hasKey("dialogEnabled") ||
        config.hasKey("iconNamesVisible") ||
        config.hasKey("maxRowsOrColumns") ||
        config.hasKey("maxRowsOrColumnsForced")) {

        // Migrate from quicklaunch 0.2 config format
        if (config.hasKey("icons")) {
            if (!config.hasKey("launchers")) {
                config.writeEntry(
                    "launchers",
                    config.readEntry("icons", QStringList()));
            }
            config.deleteEntry("icons");
        }

        if (config.hasKey("dialogIcons")) {
            if (!config.hasKey("launchersOnPopup")) {
                config.writeEntry(
                    "launchersOnPopup",
                    config.readEntry("dialogIcons", QStringList()));
            }
            config.deleteEntry("dialogIcons");
        }

        if (config.hasKey("dialogEnabled")) {
            if (!config.hasKey("popupEnabled")) {
                config.writeEntry(
                    "popupEnabled",
                    config.readEntry("dialogEnabled", false));
            }
            config.deleteEntry("dialogEnabled");
        }

        if (config.hasKey("iconNamesVisible")) {
            if (!config.hasKey("launcherNamesVisible")) {
                config.writeEntry(
                    "launcherNamesVisible",
                    config.readEntry("iconNamesVisible", false));
            }
            config.deleteEntry("iconNamesVisible");
        }

        if (config.hasKey("maxRowsOrColumns")) {
            if (config.hasKey("maxRowsOrColumnsForced")) {
                const bool maxRowsOrColumnsForced =
                    config.readEntry("maxRowsOrColumnsForced", false);

                if (maxRowsOrColumnsForced) {
                    config.writeEntry(
                        "sectionCount",
                        config.readEntry("maxRowsOrColumns", 0));
                }
                config.deleteEntry("maxRowsOrColumnsForced");
            }
            config.deleteEntry("maxRowsOrColumns");
        }
    }

    // Read new configuration
    const int sectionCount = config.readEntry("sectionCount", 0);
    const bool launcherNamesVisible = config.readEntry("launcherNamesVisible", false);
    const bool popupEnabled = config.readEntry("popupEnabled", false);

    QList<LauncherData> newLaunchers;
    QList<LauncherData> newLaunchersOnPopup;

    { // Read item lists
        QStringList newLauncherUrls =
            config.readEntry("launchers", QStringList());
        QStringList newLaunchersOnPopupUrls =
            config.readEntry("launchersOnPopup", QStringList());

        if (newLauncherUrls.isEmpty() && newLaunchersOnPopupUrls.isEmpty()) {
            newLauncherUrls = defaultLaunchers();
        }

        Q_FOREACH(const QString& launcherUrl, newLauncherUrls) {
            newLaunchers.append(LauncherData(launcherUrl));
        }

        Q_FOREACH(const QString& launcherUrl, newLaunchersOnPopupUrls) {
            newLaunchersOnPopup.append(LauncherData(launcherUrl));
        }
    }

    // Apply new configuration
    m_launcherGrid->setMaxSectionCount(sectionCount);
    m_launcherGrid->setLauncherNamesVisible(launcherNamesVisible);

    // Make sure the popup is in a proper state for the new configuration
    if (m_popup == 0 && (popupEnabled || !newLaunchersOnPopup.empty())) {
        initPopup();
    } else if (m_popup != 0 && (!popupEnabled && newLaunchersOnPopup.empty())) {
        deletePopup();
    }

    { // Check if any of the launchers in the main area have changed
        bool launchersChanged = false;

        if (newLaunchers.length() != m_launcherGrid->launcherCount()) {
            launchersChanged = true;
        } else {
            for (int i = 0; i < newLaunchers.length(); i++) {
                if (newLaunchers.at(i) != m_launcherGrid->launcherAt(i)) {
                    launchersChanged = true;
                }
            }
        }

        if (launchersChanged) {
            // Re-populate primary launchers
            m_launcherGrid->clear();
            m_launcherGrid->insert(-1, newLaunchers);
        }
    }

    { // Check if any of the launchers in the popup have changed
        bool popupLaunchersChanged = false;

        int currentPopupLauncherCount =
            m_popup == 0 ? 0 : m_popup->launcherList()->launcherCount();

        if (newLaunchersOnPopup.length() != currentPopupLauncherCount) {
            popupLaunchersChanged = true;
        }
        else if (m_popup != 0) {
            for (int i = 0; i < newLaunchersOnPopup.length(); i++) {
                if (newLaunchersOnPopup.at(i) != m_popup->launcherList()->launcherAt(i)) {
                    popupLaunchersChanged = true;
                }
            }
        }

        if (popupLaunchersChanged && !newLaunchersOnPopup.empty()) {
            // Re-populate popup launchers
            m_popup->launcherList()->clear();
            m_popup->launcherList()->insert(-1, newLaunchersOnPopup);
        }
    }
}

void Quicklaunch::iconSizeChanged()
{
    FormFactor ff = formFactor();

    if (ff == Plasma::Planar || ff == Plasma::MediaCenter) {
        m_launcherGrid->setPreferredIconSize(IconSize(KIconLoader::Desktop));
    } else {
        m_launcherGrid->setPreferredIconSize(IconSize(KIconLoader::Panel));
    }
}

void Quicklaunch::onConfigAccepted()
{
    const int sectionCount =
        uiConfig.autoSectionCountEnabledCheckBox->isChecked()
            ? 0
            : uiConfig.sectionCountSpinBox->value();
    const bool launcherNamesVisible = uiConfig.launcherNamesVisibleCheckBox->isChecked();
    const bool popupEnabled = uiConfig.popupEnabledCheckBox->isChecked();

    KConfigGroup config = this->config();
    bool changed = false;

    if (sectionCount != m_launcherGrid->maxSectionCount()) {
        config.writeEntry("sectionCount", sectionCount);
        changed = true;
    }

    if (launcherNamesVisible != m_launcherGrid->launcherNamesVisible()) {
        config.writeEntry("launcherNamesVisible", launcherNamesVisible);
        changed = true;
    }

    if (popupEnabled != (m_popup != 0)) {

        // Move all the launchers that are currently in the popup to
        // the main launcher list.
        if (m_popup) {
            PopupLauncherList *popupLauncherList = m_popup->launcherList();

            while (popupLauncherList->launcherCount() > 0) {
                m_launcherGrid->insert(
                    m_launcherGrid->launcherCount(),
                    popupLauncherList->launcherAt(0));

                popupLauncherList->removeAt(0);
            }
        }

        config.writeEntry("popupEnabled", popupEnabled);
        changed = true;
    }

    if (changed) {
        Q_EMIT configNeedsSaving();
    }
}

void Quicklaunch::onLaunchersChanged()
{
    // Save new launcher lists
    QStringList launchers;
    QStringList launchersOnPopup;

    for (int i = 0; i < m_launcherGrid->launcherCount(); i++) {
        launchers.append(m_launcherGrid->launcherAt(i).url().prettyUrl());
    }

    if (m_popup) {
        for (int i = 0; i < m_popup->launcherList()->launcherCount(); i++) {
            // XXX: Is prettyUrl() really needed?
            launchersOnPopup.append(m_popup->launcherList()->launcherAt(i).url().prettyUrl());
        }
    }

    KConfigGroup config = this->config();

    config.writeEntry("launchers", launchers);
    config.writeEntry("launchersOnPopup", launchersOnPopup);
    Q_EMIT configNeedsSaving();
}

void Quicklaunch::onPopupTriggerClicked()
{
    Q_ASSERT(m_popup);

    if (m_popup->isVisible()) {
        m_popup->hide();
    } else {
        m_popup->show();
    }
}

void Quicklaunch::onAddLauncherAction()
{
    QPointer<KOpenWithDialog> appChooseDialog = new KOpenWithDialog(0);
    appChooseDialog->hideRunInTerminal();
    appChooseDialog->setSaveNewApplications(true);

    const bool appChooseDialogAccepted = appChooseDialog->exec();

    if (!appChooseDialog || !appChooseDialogAccepted) {
        delete appChooseDialog;
        return;
    }

    QString programPath = appChooseDialog->service()->entryPath();
    QString programIcon = appChooseDialog->service()->icon();

    delete appChooseDialog;

    if (programIcon.isEmpty()) {
        // If the program chosen doesn't have an icon, then we give
        // it a default icon and open up its properties in a dialog
        // so the user can change it's icon and name etc
        KConfig kc(programPath, KConfig::SimpleConfig);
        KConfigGroup kcg = kc.group("Desktop Entry");
        kcg.writeEntry("Icon","system-run");
        kc.sync();

        QPointer<KPropertiesDialog> propertiesDialog =
            new KPropertiesDialog(KUrl(programPath), 0);

        const bool propertiesDialogAccepted = propertiesDialog->exec();

        if (!propertiesDialog || !propertiesDialogAccepted) {
            delete propertiesDialog;
            return;
        }

        // In case the name changed
        programPath = propertiesDialog->kurl().path();
        delete propertiesDialog;
    }

    if (m_contextMenuTriggeredOnPopup) {
        m_popup->launcherList()->insert(
            m_contextMenuLauncherIndex, KUrl::fromPath(programPath));
    }
    else {
        m_launcherGrid->insert(
            m_contextMenuLauncherIndex, KUrl::fromPath(programPath));
    }
}

void Quicklaunch::onEditLauncherAction()
{
    Q_ASSERT(m_contextMenuLauncherIndex != -1);

    LauncherData launcherData;

    if (m_contextMenuTriggeredOnPopup) {
        launcherData =
            m_popup->launcherList()->launcherAt(m_contextMenuLauncherIndex);
    } else {
        launcherData =
            m_launcherGrid->launcherAt(m_contextMenuLauncherIndex);
    }

    KUrl url(launcherData.url());

    // If the launcher does not point to a desktop file, create one,
    // so that user can change url, icon, text and description.
    bool desktopFileCreated = false;

    if (!url.isLocalFile() || !KDesktopFile::isDesktopFile(url.toLocalFile())) {

        QString desktopFilePath = determineNewDesktopFilePath("launcher");

        KConfig desktopFile(desktopFilePath);
        KConfigGroup desktopEntry(&desktopFile, "Desktop Entry");

        desktopEntry.writeEntry("Name", launcherData.name());
        desktopEntry.writeEntry("Comment", launcherData.description());
        desktopEntry.writeEntry("Icon", launcherData.icon());
        desktopEntry.writeEntry("Type", "Link");
        desktopEntry.writeEntry("URL", launcherData.url());

        desktopEntry.sync();

        url = KUrl::fromPath(desktopFilePath);
        desktopFileCreated = true;
    }

    QPointer<KPropertiesDialog> propertiesDialog = new KPropertiesDialog(url);

    if (propertiesDialog->exec() == QDialog::Accepted) {
        url = propertiesDialog->kurl();
        QString path = url.toLocalFile();

        // If the user has renamed the file, make sure that the new
        // file name has the extension ".desktop".
        if (!path.endsWith(QLatin1String(".desktop"))) {
            QFile::rename(path, path+".desktop");
            path += ".desktop";
            url = KUrl::fromLocalFile(path);
        }

        LauncherData newLauncherData(url);

        // TODO: This calls for a setLauncherDataAt method...
        if (m_contextMenuTriggeredOnPopup) {
            PopupLauncherList *popupLauncherList = m_popup->launcherList();
            popupLauncherList->insert(m_contextMenuLauncherIndex, newLauncherData);
            popupLauncherList->removeAt(m_contextMenuLauncherIndex+1);
        } else {
            m_launcherGrid->insert(m_contextMenuLauncherIndex, newLauncherData);
            m_launcherGrid->removeAt(m_contextMenuLauncherIndex+1);
        }

    } else {
        if (desktopFileCreated) {
            // User didn't save the data, delete the temporary desktop file.
            QFile::remove(url.toLocalFile());
        }
    }

    delete propertiesDialog;
}

void Quicklaunch::onRemoveLauncherAction()
{
    Q_ASSERT(m_contextMenuLauncherIndex != -1);

    if (m_contextMenuTriggeredOnPopup) {
        m_popup->launcherList()->removeAt(m_contextMenuLauncherIndex);
    } else {
        m_launcherGrid->removeAt(m_contextMenuLauncherIndex);
    }

}

void Quicklaunch::showContextMenu(
    const QPoint& screenPos,
    bool onPopup,
    int iconIndex)
{
    if (m_addLauncherAction == 0) {
        initActions();
    }

    m_contextMenuTriggeredOnPopup = onPopup;
    m_contextMenuLauncherIndex = iconIndex;

    KMenu m;
    m.addAction(m_addLauncherAction);
    if (iconIndex != -1) {
        m.addAction(m_editLauncherAction);
        m.addAction(m_removeLauncherAction);
    }

    m.addSeparator();
    m.addAction(action("configure"));

    if (containment() && containment()->corona()) {
        m.addAction(containment()->corona()->action("lock widgets"));
    }
    m.addAction(action("remove"));

    m.exec(screenPos);

    m_contextMenuTriggeredOnPopup = false;
    m_contextMenuLauncherIndex = -1;
}

void Quicklaunch::initActions()
{
    Q_ASSERT(!m_addLauncherAction);

    m_addLauncherAction = new QAction(KIcon("list-add"), i18n("Add Launcher..."), this);
    connect(m_addLauncherAction, SIGNAL(triggered(bool)), SLOT(onAddLauncherAction()));

    m_editLauncherAction = new QAction(KIcon("document-edit"), i18n("Edit Launcher..."), this);
    connect(m_editLauncherAction, SIGNAL(triggered(bool)), SLOT(onEditLauncherAction()));

    m_removeLauncherAction = new QAction(KIcon("list-remove"), i18n("Remove Launcher"), this);
    connect(m_removeLauncherAction, SIGNAL(triggered(bool)), SLOT(onRemoveLauncherAction()));
}

void Quicklaunch::initPopup()
{
    Q_ASSERT(!m_popupTrigger);
    Q_ASSERT(!m_popup);

    m_popup = new Popup(this);

    m_popup->installEventFilter(this);
    m_popup->launcherList()->installEventFilter(this);
    connect(m_popup->launcherList(), SIGNAL(launchersChanged()), SLOT(onLaunchersChanged()));

    // Initialize popup trigger
    m_popupTrigger = new IconWidget(this);
    m_popupTrigger->setContentsMargins(0, 0, 0, 0);
    m_popupTrigger->setPreferredWidth(KIconLoader::SizeSmall);
    m_popupTrigger->setPreferredHeight(KIconLoader::SizeSmall);
    m_popupTrigger->setAcceptDrops(true);
    m_popupTrigger->installEventFilter(this);
    ToolTipManager::self()->registerWidget(m_popupTrigger);
    updatePopupTrigger();

    m_layout->addItem(m_popupTrigger);
    m_layout->setStretchFactor(m_popupTrigger, 0);
    m_popupTrigger->show();

    connect(m_popupTrigger, SIGNAL(clicked()), SLOT(onPopupTriggerClicked()));
}

void Quicklaunch::updatePopupTrigger()
{
    Q_ASSERT(m_popupTrigger);
    Q_ASSERT(m_popup);

    bool popupHidden = m_popup->isHidden();

    // Set icon
    switch (location()) {
        case Plasma::TopEdge:
            m_popupTrigger->setSvg(
                "widgets/arrows", popupHidden ? "down-arrow" : "up-arrow");
            break;
        case Plasma::LeftEdge:
            m_popupTrigger->setSvg(
                "widgets/arrows", popupHidden ? "right-arrow" : "left-arrow");
            break;
        case Plasma::RightEdge:
            m_popupTrigger->setSvg(
                "widgets/arrows", popupHidden ? "left-arrow" : "right-arrow");
            break;
        default:
            m_popupTrigger->setSvg(
                "widgets/arrows", popupHidden ? "up-arrow" : "down-arrow");
    }

    // Set tooltip
    ToolTipContent toolTipContent;
    toolTipContent.setSubText(
        popupHidden ? i18n("Show hidden icons") : i18n("Hide icons"));
    ToolTipManager::self()->setContent(m_popupTrigger, toolTipContent);
}

void Quicklaunch::deletePopup()
{
    Q_ASSERT(m_popupTrigger);
    Q_ASSERT(m_popup);

    delete m_popup;
    delete m_popupTrigger;

    m_popup = 0;
    m_popupTrigger = 0;
}

QStringList Quicklaunch::defaultLaunchers()
{
    QStringList defaultLauncherPaths;

    defaultLauncherPaths << defaultBrowserPath();
    defaultLauncherPaths << defaultFileManagerPath();
    defaultLauncherPaths << defaultEmailClientPath();

    // Some people use the same program as browser and file manager.
    defaultLauncherPaths.removeDuplicates();

    QStringList defaultLauncherUrls;
    Q_FOREACH(const QString &path, defaultLauncherPaths) {
        if (!path.isEmpty() && QDir::isAbsolutePath(path)) {
            defaultLauncherUrls << KUrl::fromPath(path).url();
        }
    }
    return defaultLauncherUrls;
}

QString Quicklaunch::defaultBrowserPath()
{
    KConfigGroup globalConfigGeneral(KGlobal::config(), "General");

    if (globalConfigGeneral.hasKey("BrowserApplication")) {
        QString browser =
            globalConfigGeneral.readPathEntry("BrowserApplication", QString());

        if (!browser.isEmpty()) {
            if (browser.startsWith('!')) { // Literal command

                browser = browser.mid(1);

                // Strip away command line arguments, so we can treat this
                // as a file name.
                QStringList browserCmdArgs(
                    KShell::splitArgs(browser, KShell::AbortOnMeta));

                if (!browserCmdArgs.isEmpty()) {
                    browser = browserCmdArgs.at(0);
                } else {
                    browser.clear();
                }

                if (!browser.isEmpty()) {
                    QFileInfo browserFileInfo(browser);

                    if (browserFileInfo.isAbsolute()) {
                        if (browserFileInfo.isExecutable()) {
                            return browser;
                        }
                    } else { // !browserFileInfo.isAbsolute()
                        browser = KStandardDirs::findExe(browser);
                        if (!browser.isEmpty()) {
                            return browser;
                        }
                    }
                }
            } else {
                KService::Ptr service = KService::serviceByStorageId(browser);
                if (service && service->isValid()) {
                    return service->entryPath();
                }
            }
        }
    }

    // No global browser configured or configuration is invalid. Falling
    // back to MIME type association.
    KService::Ptr service;
    service = KMimeTypeTrader::self()->preferredService("text/html");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    service = KMimeTypeTrader::self()->preferredService("application/xml+xhtml");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    // Fallback to konqueror.
    service = KService::serviceByStorageId("konqueror");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    // Give up.
    return QString();
}

QString Quicklaunch::defaultFileManagerPath()
{
    KService::Ptr service;
    service = KMimeTypeTrader::self()->preferredService("inode/directory");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    // Fallback to dolphin.
    service = KService::serviceByStorageId("dolphin");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    // Give up.
    return QString();
}

QString Quicklaunch::defaultEmailClientPath()
{
    KEMailSettings emailSettings;
    QString mua = emailSettings.getSetting(KEMailSettings::ClientProgram);

    if (!mua.isEmpty()) {

        // Strip away command line arguments, so we can treat this
        // as a file name.
        QStringList muaCmdArgs(KShell::splitArgs(mua, KShell::AbortOnMeta));

        if (!muaCmdArgs.isEmpty()) {
            mua = muaCmdArgs.at(0);
        } else {
            mua.clear();
        }

        if (!mua.isEmpty()) {
            // Strictly speaking, this is incorrect, but it's much better to
            // find the service than just the plain command, so we'll search
            // for services that have the same name as the executable and
            // hope for the best.
            KService::Ptr service = KService::serviceByStorageId(mua);

            if (service && service->isValid()) {
                return service->entryPath();
            }

            // Fallback to the exectuable.
            QFileInfo muaFileInfo(mua);

            if (muaFileInfo.isAbsolute()) {
                if (muaFileInfo.isExecutable()) {
                    return mua;
                }
            } else { // !muaFileInfo.isAbsolute()
                mua = KStandardDirs::findExe(mua);

                if (!mua.isEmpty()) {
                    return mua;
                }
            }
        }
    }

    // Fallback to kmail (if it is installed).
    KService::Ptr service = KService::serviceByStorageId("kmail");
    if (service && service->isValid()) {
        return service->entryPath();
    }

    // Give up.
    return QString();
}

QString Quicklaunch::determineNewDesktopFilePath(const QString &baseName)
{
    QString desktopFilePath =
        KStandardDirs::locateLocal(
            "appdata", "quicklaunch/"+baseName+".desktop", true);

    QString appendix;

    while(QFile::exists(desktopFilePath)) {
        if (appendix.isEmpty()) {
            qsrand(QDateTime::currentDateTime().toTime_t());
            appendix += '-';
        }

        // Limit to [0-9] and [a-z] range.
        char newChar = qrand() % 36;
        newChar += newChar < 10 ? 48 : 97-10;
        appendix += newChar;

        desktopFilePath =
            KStandardDirs::locateLocal(
                "appdata", "quicklaunch/"+baseName+appendix+".desktop");
    }

    return desktopFilePath;
}
}

#include "quicklaunch.moc"
