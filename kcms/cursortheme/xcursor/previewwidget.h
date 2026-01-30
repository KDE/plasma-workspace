/*
    SPDX-FileCopyrightText: 2003-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "sortproxymodel.h"
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>

class CursorTheme;
class PreviewCursor;

class PreviewWidget : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(SortProxyModel *themeModel READ themeModel WRITE setThemeModel NOTIFY themeModelChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentSize READ currentSize WRITE setCurrentSize NOTIFY currentSizeChanged)
    Q_PROPERTY(int maximumCount READ maximumCount WRITE setMaximumCount NOTIFY maximumCountChanged)
    Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    explicit PreviewWidget(QQuickItem *parent = nullptr);
    ~PreviewWidget() override;

    void setTheme(const CursorTheme *theme, const int size);
    void setUseLables(bool);
    void updateImplicitSize();

    void setThemeModel(SortProxyModel *themeModel);
    SortProxyModel *themeModel();

    void setCurrentIndex(int idx);
    int currentIndex() const;

    void setCurrentSize(int size);
    int currentSize() const;

    int maximumCount() const;
    void setMaximumCount(int maximumCount);
    Q_SIGNAL void maximumCountChanged(int maximumCount);

    int padding() const;
    void setPadding(int padding);
    Q_SIGNAL void paddingChanged(int padding);

    int spacing() const;
    void setSpacing(int spacing);
    Q_SIGNAL void spacingChanged(int spacing);

    Q_INVOKABLE void refresh();

    void componentComplete() override;

Q_SIGNALS:
    void themeModelChanged();
    void currentIndexChanged();
    void currentSizeChanged();

protected:
    void paint(QPainter *) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *e) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void layoutItems();

    QList<PreviewCursor *> list;
    const PreviewCursor *current;
    bool needLayout : 1;
    QPointer<SortProxyModel> m_themeModel;
    int m_currentIndex;
    int m_currentSize;
    int m_maximumCount = 0;
    int m_padding = 0;
    int m_spacing = 0;
    QTimer m_animationTimer;
    size_t nextAnimationFrame;
};
