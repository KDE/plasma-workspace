/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <Plasma/Applet>

class QQuickItem;
class QString;
class QRect;
class QWindow;

class NotificationApplet : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(bool dragActive READ dragActive NOTIFY dragActiveChanged)
    Q_PROPERTY(int dragPixmapSize READ dragPixmapSize WRITE setDragPixmapSize NOTIFY dragPixmapSizeChanged)

    Q_PROPERTY(QWindow *focussedPlasmaDialog READ focussedPlasmaDialog NOTIFY focussedPlasmaDialogChanged)
    Q_PROPERTY(QQuickItem *systemTrayRepresentation READ systemTrayRepresentation CONSTANT)

public:
    explicit NotificationApplet(QObject *parent, const QVariantList &data);
    ~NotificationApplet() override;

    void init() override;
    void configChanged() override;

    bool dragActive() const;

    int dragPixmapSize() const;
    void setDragPixmapSize(int dragPixmapSize);

    Q_INVOKABLE bool isDrag(int oldX, int oldY, int newX, int newY) const;
    Q_INVOKABLE void startDrag(QQuickItem *item, const QUrl &url, const QString &iconName);
    Q_INVOKABLE void startDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap);

    QWindow *focussedPlasmaDialog() const;
    QQuickItem *systemTrayRepresentation() const;

    Q_INVOKABLE void setSelectionClipboardText(const QString &text);

    Q_INVOKABLE bool isPrimaryScreen(const QRect &rect) const;

    Q_INVOKABLE void forceActivateWindow(QWindow *window);

signals:
    void dragActiveChanged();
    void dragPixmapSizeChanged();
    void focussedPlasmaDialogChanged();

private slots:
    void doDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap);

private:
    bool m_dragActive = false;
    int m_dragPixmapSize = 48; // Bound to units.iconSizes.large in main.qml
};
