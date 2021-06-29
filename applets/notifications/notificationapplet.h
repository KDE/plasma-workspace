/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QQuickItem>
#include <QWindow>

#include <Plasma/Applet>

class QString;
class QRect;

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

Q_SIGNALS:
    void dragActiveChanged();
    void dragPixmapSizeChanged();
    void focussedPlasmaDialogChanged();

private slots:
    void doDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap);

private:
    bool m_dragActive = false;
    int m_dragPixmapSize = 48; // Bound to units.iconSizes.large in main.qml
};
