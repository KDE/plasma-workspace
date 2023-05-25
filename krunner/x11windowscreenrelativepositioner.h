/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef X11WINDOWSCREENRELATIVEPOSITIONER_H
#define X11WINDOWSCREENRELATIVEPOSITIONER_H

#include <QMargins>
#include <QObject>
#include <QPointer>
#include <QWindow>

/**
 * This class positions a window relative to screen edges, keeping the position
 * in sync with screen and window size changes.
 *
 * Position changes will apply just before the next frame
 *
 */
class X11WindowScreenRelativePositioner : public QObject
{
    Q_OBJECT
public:
    /**
     * Construct a new X11WindowScreenRelativePositioner for the window
     * The created object is parented to the window
     */
    explicit X11WindowScreenRelativePositioner(QWindow *window);

    /**
     * @brief setAnchors
     * @param anchors
     */
    void setAnchors(Qt::Edges anchors);
    void setMargins(const QMargins &margins);

    bool eventFilter(QObject *watched, QEvent *event) override;

Q_SIGNALS:
    void anchorsChanged();
    void marginsChanged();

private:
    void polish();
    void updatePolish();
    void reposition();
    void handleScreenChanged();

    Qt::Edges m_anchors;
    QMargins m_margins;

    bool m_needsRepositioning;
    QWindow *m_window;
    QPointer<QScreen> m_screen;
};

#endif // X11WINDOWSCREENRELATIVEPOSITIONER_H
