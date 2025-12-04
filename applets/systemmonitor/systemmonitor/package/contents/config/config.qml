import QtQuick
import org.kde.ksysguard.sensors as Sensors

import org.kde.plasma.plasmoid
import org.kde.plasma.configuration

ConfigModel {
    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "config/ConfigAppearance.qml"
    }
    ConfigCategory {
         name: i18n("%1 Details", Plasmoid.faceController.name)
         icon: Plasmoid.faceController.icon
         visible: Plasmoid.faceController.faceConfigUi !== null
         source: "config/FaceDetails.qml"
    }
    ConfigCategory {
         name: i18n("Sensors Details")
         icon: "ksysguardd"
         source: "config/ConfigSensors.qml"
    }
}

