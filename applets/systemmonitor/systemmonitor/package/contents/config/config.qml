import QtQuick 2.0
import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.configuration 2.0

ConfigModel {
    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "config/ConfigAppearance.qml"
    }
    ConfigCategory {
         name: i18n("%1 Details", Plasmoid.nativeInterface.faceController.name)
         icon: Plasmoid.nativeInterface.faceController.icon
         visible: Plasmoid.nativeInterface.faceController.faceConfigUi !== null
         source: "config/FaceDetails.qml"
    }
    ConfigCategory {
         name: i18n("Sensors Details")
         icon: "ksysguardd"
         source: "config/ConfigSensors.qml"
    }
}

