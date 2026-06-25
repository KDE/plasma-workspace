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

namespace KWayland::Client
{
class PlasmaShell;
}

class KlipperPopup : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT

public:
    explicit KlipperPopup();
    ~KlipperPopup() override;

    void show();

    void setPlasmaShell(KWayland::Client::PlasmaShell *plasmashell);

    void editCurrentClipboard();
    void showCurrentBarcode();

public Q_SLOTS:
    void hide();
    void resizePopup();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void positionOnScreen();
    void onFocusWindowChanged(QWindow *focusWindow);

    /**
     * The "document" (clipboard history)
     */
    std::shared_ptr<HistoryModel> m_model;

    std::shared_ptr<QQmlEngine> m_engine;
    KWayland::Client::PlasmaShell *m_plasmashell = nullptr;
};
