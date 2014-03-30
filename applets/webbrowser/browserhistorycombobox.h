/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef BROWSERHISTORYCOMBOBOX_H
#define BROWSERHISTORYCOMBOBOX_H

#include <QtGui/QGraphicsProxyWidget>

class KComboBox;

namespace Plasma
{

class ComboBoxPrivate;

/**
 * @class ComboBox plasma/widgets/combobox.h <Plasma/Widgets/ComboBox>
 *
 * @short Provides a Plasma-themed combo box.
 */
class BrowserHistoryComboBox : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(KComboBox *nativeWidget READ nativeWidget WRITE setNativeWidget)

    Q_PROPERTY(qreal animationUpdate READ animationUpdate WRITE setAnimationUpdate)

public:
    explicit BrowserHistoryComboBox(QGraphicsWidget *parent = 0);
    ~BrowserHistoryComboBox();

    /**
     * @return the display text
     */
    QString text() const;

    /**
     * Sets the stylesheet used to control the visual display of this ComboBox
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * Sets the combo box wrapped by this ComboBox (widget must inherit KComboBox), ownership is transferred to the ComboBox
     *
     * @arg combo box that will be wrapped by this ComboBox
     * @since KDE4.4
     */
    void setNativeWidget(KComboBox *nativeWidget);

    /**
     * @return the native widget wrapped by this ComboBox
     */
    KComboBox *nativeWidget() const;

    /**
     * Adds an item to the combo box with the given text. The
     * item is appended to the list of existing items.
     */
    void addItem(const QString &text);

    void setProgressValue(int value);
    
    void setDisplayProgress(bool enable);

public Q_SLOTS:
    void clear();

Q_SIGNALS:
    /**
     * This signal is sent when the user chooses an item in the combobox.
     * The item's text is passed.
     */
    void activated(const QString &text);

    /**
     * This signal is sent whenever the currentIndex in the combobox changes 
     * either through user interaction or programmatically. 
     * The item's text is passed.
     */
    void textChanged(const QString &text);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void changeEvent(QEvent *event);

private Q_SLOTS:
    void setAnimationUpdate(qreal progress);
    qreal animationUpdate() const;

private:
    ComboBoxPrivate * const d;

    friend class ComboBoxPrivate;
    Q_PRIVATE_SLOT(d, void syncBorders())
};

} // namespace Plasma

#endif // multiple inclusion guard
