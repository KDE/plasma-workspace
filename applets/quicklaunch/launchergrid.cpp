/***************************************************************************
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************/
#include "launchergrid.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QEvent>
#include <QtCore/QMimeData>
#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QBrush>
#include <QtGui/QDrag>
#include <QtGui/QGraphicsGridLayout>
#include <QtGui/QGraphicsItem>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsSceneResizeEvent>
#include <QtGui/QGraphicsWidget>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QSizePolicy>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QWidget>

// KDE
#include <KIcon>
#include <KIconLoader>
#include <KUrl>

// Plasma
#include <Plasma/Plasma>
#include <Plasma/IconWidget>
#include <Plasma/Theme>
#include <Plasma/ToolTipContent>
#include <Plasma/ToolTipManager>

// stdlib
#include <math.h>

// Own
#include "icongridlayout.h"
#include "launcherdata.h"
#include "launcher.h"

using Plasma::Theme;

namespace Quicklaunch {

class DropMarker : public Launcher {

public:
    DropMarker(LauncherGrid *parent)
        : Launcher(LauncherData(), parent)
    {
        hide();
    }

protected:
    void paint(
        QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        QWidget *widget)
    {
        // This mirrors the behavior of the panel spacer committed by mart
        // workspace/plasma/desktop/containments/panel/panel.cpp R875513)
        QColor brushColor(Theme::defaultTheme()->color(Theme::TextColor));
        brushColor.setAlphaF(0.3);

        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(brushColor));

        painter->drawRoundedRect(contentsRect(), 4, 4);
        Launcher::paint(painter, option, widget);
    }
};

LauncherGrid::LauncherGrid(QGraphicsItem *parent)
:
    QGraphicsWidget(parent),
    m_launchers(),
    m_launcherNamesVisible(false),
    m_preferredIconSize(),
    m_locked(false),
    m_layout(new IconGridLayout()),
    m_mousePressedPos(),
    m_dropMarker(new DropMarker(this)),
    m_dropMarkerIndex(-1),
    m_placeHolder(0)
{
    m_dropMarker->setOrientation(Qt::Vertical);

    setLayout(m_layout);
    initPlaceHolder();

    setLocked(false);
}

bool LauncherGrid::launcherNamesVisible() const
{
    return m_launcherNamesVisible;
}

void LauncherGrid::setLauncherNamesVisible(bool enable)
{
    if (enable == m_launcherNamesVisible) {
        return;
    }

    Q_FOREACH (Launcher *launcher, m_launchers) {
        launcher->setNameVisible(enable);
    }
    m_dropMarker->setNameVisible(enable);
    m_launcherNamesVisible = enable;
}

void LauncherGrid::setPreferredIconSize(int size)
{
    QSizeF newSize(size, size);

    if (newSize == m_preferredIconSize) {
        return;
    }

    m_preferredIconSize = newSize;
    m_dropMarker->setPreferredIconSize(newSize);

    Q_FOREACH (Launcher *launcher, m_launchers) {
        launcher->setPreferredIconSize(newSize);
    }

    if (m_placeHolder) {
        m_placeHolder->setPreferredIconSize(newSize);
    }
}

LauncherGrid::LayoutMode LauncherGrid::layoutMode() const
{
    IconGridLayout::Mode mode = m_layout->mode();

    if (mode == IconGridLayout::PreferColumns) {
        return PreferColumns;
    } else {
        return PreferRows;
    }
}

void LauncherGrid::setLayoutMode(LauncherGrid::LayoutMode mode)
{
    if (mode == PreferColumns) {
        m_layout->setMode(IconGridLayout::PreferColumns);
    } else {
        m_layout->setMode(IconGridLayout::PreferRows);
    }
}

int LauncherGrid::maxSectionCount() const
{
    return m_layout->maxSectionCount();
}

void LauncherGrid::setMaxSectionCount(int maxSectionCount)
{
    m_layout->setMaxSectionCount(maxSectionCount);
}

bool LauncherGrid::maxSectionCountForced() const
{
    return m_layout->maxSectionCountForced();
}

void LauncherGrid::setMaxSectionCountForced(bool enable)
{
    m_layout->setMaxSectionCountForced(enable);
}

bool LauncherGrid::locked() const
{
    return m_locked;
}

void LauncherGrid::setLocked(bool enable)
{
    m_locked = enable;
    setAcceptDrops(!enable);
}

IconGridLayout *LauncherGrid::layout() const
{
    return m_layout;
}

int LauncherGrid::launcherCount() const
{
    return m_launchers.size();
}

void LauncherGrid::clear()
{
    while(launcherCount() > 0) {
        removeAt(0);
    }
}

void LauncherGrid::insert(int index, const LauncherData &launcherData)
{
    QList<LauncherData> launcherDataList;
    launcherDataList.append(launcherData);
    insert(index, launcherDataList);
}

void LauncherGrid::insert(int index, const QList<LauncherData> &launcherDataList)
{
    if (launcherDataList.size() == 0) {
        return;
    }

    if (m_launchers.size() == 0 && m_placeHolder) {
        deletePlaceHolder();
        index = 0;
    }
    else if (index < 0 || index > m_launchers.size()) {
        index = m_launchers.size();
    }

    Q_FOREACH(const LauncherData &launcherData, launcherDataList) {

        Launcher *launcher = new Launcher(launcherData);

        launcher->setNameVisible(m_launcherNamesVisible);
        launcher->setOrientation(Qt::Vertical);

        if (m_preferredIconSize.isValid()) {
            launcher->setPreferredIconSize(m_preferredIconSize);
        }

        launcher->installEventFilter(this);
        connect(launcher, SIGNAL(clicked()), this, SIGNAL(launcherClicked()));

        m_launchers.insert(index, launcher);

        int layoutIndex = index;

        if (m_dropMarkerIndex != -1) {
            if (m_dropMarkerIndex <= index) {
                layoutIndex++;
            } else {
                m_dropMarkerIndex++;
            }
        }

        m_layout->insertItem(layoutIndex, launcher);
        index++;
    }

    Q_EMIT launchersChanged();
}

void LauncherGrid::removeAt(int index)
{
    int layoutIndex = index;

    if (m_dropMarkerIndex != -1) {
        if (m_dropMarkerIndex <= index) {
            layoutIndex++;
        } else {
            m_dropMarkerIndex--;
        }
    }

    m_layout->removeAt(layoutIndex);
    delete m_launchers.takeAt(index);

    if (m_launchers.size() == 0 && m_dropMarkerIndex == -1) {
        initPlaceHolder();
    }

    Q_EMIT launchersChanged();
}

LauncherData LauncherGrid::launcherAt(int index) const
{
    return m_launchers.at(index)->launcherData();
}

int LauncherGrid::launcherIndexAtPosition(const QPointF& pos) const
{
    for (int i = 0; i < m_launchers.size(); i++) {
        if (m_launchers.at(i)->geometry().contains(pos)) {
            return i;
        }
    }
    return -1;
}

bool LauncherGrid::eventFilter(QObject *watched, QEvent *event)
{
    Launcher *sourceLauncher =
        qobject_cast<Launcher*>(watched);

    // Start of Drag & Drop operations
    if (sourceLauncher && !m_locked) {

        if (event->type() == QEvent::GraphicsSceneMousePress) {
            m_mousePressedPos =
                static_cast<QGraphicsSceneMouseEvent*>(event)->pos();

            return false;
        }
        else if (event->type() == QEvent::GraphicsSceneMouseMove) {

            QGraphicsSceneMouseEvent *mouseEvent =
                static_cast<QGraphicsSceneMouseEvent*>(event);

            if ((m_mousePressedPos - mouseEvent->pos()).manhattanLength() >=
                    QApplication::startDragDistance()) {

                LauncherData sourceData = sourceLauncher->launcherData();

                QMimeData *mimeData = new QMimeData();
                sourceData.populateMimeData(mimeData);

                QPointer<QDrag> drag = new QDrag(mouseEvent->widget());
                drag->setMimeData(mimeData);
                drag->setPixmap(sourceLauncher->icon().pixmap(16, 16));

                int launcherIndex = m_launchers.indexOf(sourceLauncher);

                removeAt(launcherIndex);

                Qt::DropAction dropAction = drag->exec();

                if (dropAction != Qt::MoveAction) {
                    // Restore the icon.
                    insert(launcherIndex, sourceData);
                }
                return true;
            }
        }
    }
    return false;
}

void LauncherGrid::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_ASSERT(!m_locked);

    Qt::DropAction proposedAction = event->proposedAction();

    if (proposedAction != Qt::CopyAction && proposedAction != Qt::MoveAction) {

        Qt::DropActions possibleActions = event->possibleActions();

        if (possibleActions & Qt::CopyAction) {
            event->setProposedAction(Qt::CopyAction);
        }
        else if (possibleActions & Qt::MoveAction) {
            event->setProposedAction(Qt::MoveAction);
        } else {
            event->setAccepted(false);
            return;
        }
    }

    // Initialize drop marker
    const QMimeData *mimeData = event->mimeData();

    if (LauncherData::canDecode(mimeData)) {
        QList<LauncherData> data = LauncherData::fromMimeData(mimeData);

        if (data.size() > 0) {

            if (data.size() == 1) {
                m_dropMarker->setLauncherData(data.at(0));
            } else {
                m_dropMarker->setLauncherData(LauncherData());
                m_dropMarker->setIcon("document-multiple");

                if (m_launcherNamesVisible) {
                    m_dropMarker->setText(i18n("Multiple items"));
                } else {
                    m_dropMarker->setText(QString());
                }
            }

            if (m_launchers.size() != 0) {
                m_dropMarkerIndex = determineDropMarkerIndex(
                    mapFromScene(event->scenePos()));

            } else {
                deletePlaceHolder();
                m_dropMarkerIndex = 0;
            }

            m_layout->insertItem(m_dropMarkerIndex, m_dropMarker);
            m_dropMarker->show();

            event->accept();
        } else {
            event->setAccepted(false);
            return;
        }
    } else {
        event->setAccepted(false);
        return;
    }
}

void LauncherGrid::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    // DragMoveEvents are always preceded by DragEnterEvents
    Q_ASSERT(m_dropMarkerIndex != -1);

    int newDropMarkerIndex =
        determineDropMarkerIndex(mapFromScene(event->scenePos()));

    if (newDropMarkerIndex != m_dropMarkerIndex) {
        m_layout->moveItem(m_dropMarkerIndex, newDropMarkerIndex);
        m_dropMarkerIndex = newDropMarkerIndex;
    }
}

void LauncherGrid::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event);

    if (m_dropMarkerIndex != -1) {

        m_dropMarker->hide();
        m_layout->removeAt(m_dropMarkerIndex);
        m_dropMarker->setLauncherData(LauncherData());
        m_dropMarkerIndex = -1;

        if (m_launchers.size() == 0) {
            initPlaceHolder();
        }
    }
}

void LauncherGrid::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    int dropIndex = m_dropMarkerIndex;

    if (dropIndex != -1) {
        m_dropMarker->hide();
        m_layout->removeAt(m_dropMarkerIndex);
        m_dropMarker->setLauncherData(LauncherData());
        m_dropMarkerIndex = -1;
    }

    const QMimeData *mimeData = event->mimeData();

    if (LauncherData::canDecode(mimeData)) {

        QList<LauncherData> data = LauncherData::fromMimeData(mimeData);
        insert(dropIndex, data);
    }

    event->accept();
}

void LauncherGrid::onPlaceHolderActivated()
{
    Q_ASSERT(m_placeHolder);
    Plasma::ToolTipManager::self()->show(m_placeHolder);
}

void LauncherGrid::initPlaceHolder()
{
    Q_ASSERT(!m_placeHolder);

    m_placeHolder = new Plasma::IconWidget(KIcon("fork"), QString(), this);
    m_placeHolder->setPreferredIconSize(m_dropMarker->preferredIconSize());

    Plasma::ToolTipContent tcp(
        i18n("Quicklaunch"),
        i18n("Add launchers by Drag and Drop or by using the context menu."),
        m_placeHolder->icon());
    Plasma::ToolTipManager::self()->setContent(m_placeHolder, tcp);

    connect(m_placeHolder, SIGNAL(activated()), SLOT(onPlaceHolderActivated()));

    m_layout->addItem(m_placeHolder);
}

void LauncherGrid::deletePlaceHolder()
{
    Q_ASSERT(m_placeHolder);

    m_layout->removeAt(0);

    delete m_placeHolder;
    m_placeHolder = 0;
}

int LauncherGrid::determineDropMarkerIndex(const QPointF &localPos) const
{
    if (m_placeHolder) {
        return 0;
    }

    // Determine the new index of the drop marker.
    const int rowCount = m_layout->rowCount();
    const int columnCount = m_layout->columnCount();

    int row = 0;
    while (row + 1 < rowCount && localPos.y() > m_layout->itemAt(row + 1, 0)->geometry().top()) {
        row++;
    }

    int col = 0;
    while (col + 1 < columnCount && localPos.x() > m_layout->itemAt(0, col + 1)->geometry().left()) {
        col++;
    }

    int newDropMarkerIndex = row * columnCount + col;

    if (newDropMarkerIndex > m_launchers.size())
        newDropMarkerIndex = m_launchers.size();

    return newDropMarkerIndex;
}
}

#include "launchergrid.moc"
