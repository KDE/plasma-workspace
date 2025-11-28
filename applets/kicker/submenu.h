/*
    SPDX-FileCopyrightText: 2014 David Edmundson <kde@davidedmundson.co.uk>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <PlasmaQuick/Dialog>

class SubMenu : public PlasmaQuick::Dialog
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(bool dialogMirrored READ dialogMirrored WRITE setDialogMirrored NOTIFY dialogMirroredChanged)
    Q_PROPERTY(bool facingLeft READ facingLeft NOTIFY facingLeftChanged)

public:
    explicit SubMenu(QQuickItem *parent = nullptr);
    ~SubMenu() override;

    Q_INVOKABLE QRect availableScreenRectForItem(QQuickItem *item) const;

    QPoint popupPosition(QQuickItem *item, const QSize &size) override;

    int offset() const;
    void setOffset(int offset);
    bool dialogMirrored() const;
    void setDialogMirrored(bool mirrored);

    bool facingLeft() const
    {
        return m_facingLeft;
    }

Q_SIGNALS:
    void offsetChanged() const;
    void facingLeftChanged() const;
    void dialogMirroredChanged() const;

private:
    int m_offset;
    bool m_dialogMirrored;
    bool m_facingLeft;
};
