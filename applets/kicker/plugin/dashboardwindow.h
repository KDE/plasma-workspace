/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Theme>

#include <QQuickItem>
#include <QQuickWindow>

class DashboardWindow : public QQuickWindow
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *mainItem READ mainItem WRITE setMainItem NOTIFY mainItemChanged)
    Q_PROPERTY(QQuickItem *visualParent READ visualParent WRITE setVisualParent NOTIFY visualParentChanged)
    Q_PROPERTY(QQuickItem *keyEventProxy READ keyEventProxy WRITE setKeyEventProxy NOTIFY keyEventProxyChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

    Q_CLASSINFO("DefaultProperty", "mainItem")

public:
    explicit DashboardWindow(QQuickItem *parent = nullptr);
    ~DashboardWindow() override;

    QQuickItem *mainItem() const;
    void setMainItem(QQuickItem *item);

    QQuickItem *visualParent() const;
    void setVisualParent(QQuickItem *item);

    QQuickItem *keyEventProxy() const;
    void setKeyEventProxy(QQuickItem *item);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);

    Q_INVOKABLE void toggle();

Q_SIGNALS:
    void mainItemChanged() const;
    void visualParentChanged() const;
    void keyEventProxyChanged() const;
    void backgroundColorChanged() const;
    void keyEscapePressed() const;

private Q_SLOTS:
    void updateTheme();
    void visualParentWindowChanged(QQuickWindow *window);
    void visualParentScreenChanged(QScreen *screen);

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    QQuickItem *m_mainItem;
    QPointer<QQuickItem> m_visualParentItem;
    QPointer<QQuickWindow> m_visualParentWindow;
    QPointer<QQuickItem> m_keyEventProxy;
    Plasma::Theme m_theme;
};
