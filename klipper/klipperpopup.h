/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <PlasmaQuick/PlasmaWindow>

class QWindow;

class HistoryModel;

class KlipperPopup : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT

public:
    explicit KlipperPopup();
    ~KlipperPopup() override;

    void editCurrentClipboard();
    void showCurrentBarcode();
    void showActionMenu(int index, bool automaticallyInvoked = false);

public Q_SLOTS:
    void show();
    void hide();
    void resizePopup();
    void resizePopupHeight(qreal height);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void resizeMove(const QSize &popupSize);
    void positionOnScreen();
    void onFocusWindowChanged(QWindow *focusWindow);

    /**
     * The "document" (clipboard history)
     */
    std::shared_ptr<HistoryModel> m_model;

    std::shared_ptr<QQmlEngine> m_engine;
};
