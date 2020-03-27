import QtQuick 2.0

import org.kde.plasma.configuration 2.0

ConfigModel {
    ConfigCategory {
         name: i18n("Appearance")
         icon: "preferences-desktop-color"
         source: "config/ConfigAppearance.qml"
    }
    ConfigCategory {
         name: i18n("%1 Details", plasmoid.nativeInterface.faceName)
         icon: plasmoid.nativeInterface.faceIcon
         visible: plasmoid.nativeInterface.configPath.length > 0
         source: "config/FaceDetails.qml"
    }
    ConfigCategory {
         name: i18n("Sensors Details")
         icon: "ksysguardd"
         source: "config/ConfigSensors.qml"
    }
}

