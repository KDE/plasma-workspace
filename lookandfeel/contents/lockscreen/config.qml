import QtQuick 2.5
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.12 as Kirigami

Kirigami.FormLayout {
    property alias cfg_alwaysShowClock: alwaysClock.checked
    property alias cfg_showMediaControls: showMediaControls.checked

    twinFormLayouts: parentLayout

    QQC2.CheckBox {
        id: alwaysClock
        Kirigami.FormData.label: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Clock:")
        text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "verb, to show something", "Show always")
    }
    QQC2.CheckBox {
        id: showMediaControls
        Kirigami.FormData.label: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Media controls:")
        text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "verb, to show something", "Show")
    }
}
