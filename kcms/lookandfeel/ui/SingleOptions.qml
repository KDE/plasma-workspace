import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcmutils as KCM
import org.kde.private.kcms.lookandfeel 1.0 as Private

KCM.GridView {
    view.currentIndex: lookAndFeelRow.index
    view.model: kcm.model

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display
        subtitle: model.hasDesktopLayout ? i18nc("@label", "Contains Desktop layout") : ""
        toolTip: model.description

        thumbnailAvailable: model.screenshot
        thumbnail: Image {
            anchors.fill: parent
            source: model.screenshot || ""
            sourceSize: Qt.size(delegate.GridView.view.cellWidth * Screen.devicePixelRatio,
                                delegate.GridView.view.cellHeight * Screen.devicePixelRatio)
        }
        actions: [
            Kirigami.Action {
                visible: model.fullScreenPreview !== ""
                icon.name: "view-preview"
                tooltip: i18nc("@info:tooltip", "Preview Theme")
                onTriggered: {
                    previewWindow.url = "file:/" + model.fullScreenPreview
                    previewWindow.showFullScreen()
                }
            },
            Kirigami.Action {
                icon.name: "edit-delete"
                tooltip: if (enabled) {
                    return i18nc("@info:tooltip", "Remove theme");
                } else if (delegate.GridView.isCurrentItem) {
                    return i18nc("@info:tooltip", "Cannot delete the active theme");
                } else {
                    return i18nc("@info:tooltip", "Cannot delete system-installed themes");
                }
                enabled: model.uninstallable && !delegate.GridView.isCurrentItem
                onTriggered: confirmDeletionDialog.incubateObject(selector, {
                    "pluginId": model.pluginName,
                });
            }
        ]
        onClicked: {
            kcm.settings.lookAndFeelPackage = model.pluginName;
        }
    }

    Private.ItemModelRow {
        id: lookAndFeelRow
        model: kcm.model
        role: "pluginName"
        value: kcm.settings.lookAndFeelPackage
    }
}
