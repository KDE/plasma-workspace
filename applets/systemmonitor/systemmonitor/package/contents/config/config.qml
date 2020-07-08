import QtQuick 2.0
import org.kde.ksysguard.sensors 1.0 as Sensors
import org.kde.ksysguard.faces 1.0 as Faces

import org.kde.plasma.configuration 2.0

ConfigModel {
    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "config/ConfigAppearance.qml"
    }
    ConfigCategory {
         name: i18n("%1 Details", plasmoid.nativeInterface.faceController.name)
         icon: plasmoid.nativeInterface.faceController.icon
         visible: plasmoid.nativeInterface.faceController.faceConfigUi !== null
         source: "config/FaceDetails.qml"
    }
    ConfigCategory {
         name: i18n("Sensors Details")
         icon: "ksysguardd"
         source: "config/ConfigSensors.qml"
    }
}

