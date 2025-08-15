/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Plasma/Applet>
#include <Plasma/Containment>

#include <QPointer>

class DBusMenuModel;
class DBusMenuView;
class QQuickItem;

class AppMenuApplet : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(QObject *containment READ containment CONSTANT)
    Q_PROPERTY(DBusMenuModel *model READ model WRITE setModel NOTIFY modelChanged)

    Q_PROPERTY(int view READ view WRITE setView NOTIFY viewChanged)

    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)

    Q_PROPERTY(QQuickItem *buttonGrid READ buttonGrid WRITE setButtonGrid NOTIFY buttonGridChanged)

public:
    enum ViewType {
        FullView,
        CompactView,
    };

    explicit AppMenuApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~AppMenuApplet() override;

    void init() override;

    int currentIndex() const;

    QQuickItem *buttonGrid() const;
    void setButtonGrid(QQuickItem *buttonGrid);

    DBusMenuModel *model() const;
    void setModel(DBusMenuModel *model);

    int view() const;
    void setView(int type);

Q_SIGNALS:
    void modelChanged();
    void viewChanged();
    void currentIndexChanged();
    void buttonGridChanged();
    void requestActivateIndex(int index);

public Q_SLOTS:
    void trigger(QQuickItem *ctx, int idx);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setCurrentIndex(int currentIndex);
    void onMenuAboutToHide();

    int m_currentIndex = -1;
    int m_viewType = FullView;
    QPointer<QQuickItem> m_buttonGrid;
    QPointer<DBusMenuModel> m_model;
    QPointer<DBusMenuView> m_view;
    static int s_refs;
};
