/*
    SPDX-FileCopyrightText: 2003-2007 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QPointer>
#include <QQuickPaintedItem>

#include "ui_stylepreview.h"

class QWidget;
class QStyle;

class PreviewItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QString styleName READ styleName WRITE setStyleName NOTIFY styleNameChanged)

    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)

public:
    explicit PreviewItem(QQuickItem *parent = nullptr);
    ~PreviewItem() override;

    QString styleName() const;
    void setStyleName(const QString &styleName);
    Q_SIGNAL void styleNameChanged();

    bool isValid() const;
    Q_SIGNAL void validChanged();

    Q_INVOKABLE void reload();

    void componentComplete() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    void paint(QPainter *painter) override;

    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

private:
    void sendHoverEvent(QHoverEvent *event);
    void dispatchEnterLeave(QWidget *enter, QWidget *leave, const QPointF &globalPosF);

    QString m_styleName;

    Ui::StylePreview m_ui;

    std::unique_ptr<QWidget> m_widget;
    QPointer<QWidget> m_lastWidgetUnderMouse;

    std::unique_ptr<QStyle> m_style;
};
