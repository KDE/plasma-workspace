import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

ColumnLayout {
   BreezeLabel { //should be a heading but we want it _loads_ bigger
        text: Qt.formatTime(timeSource.data["Local"]["Time"], Locale.ShortFormat)
        //we fill the width then align the text so that we can make the text shrink to fit
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight

        font.weight: Font.DemiBold
        fontSizeMode: Text.HorizontalFit
        font.pointSize: 36
    }

    BreezeLabel {
        text: Qt.formatDate(timeSource.data["Local"]["Date"], Locale.LongFormat);
        Layout.alignment: Qt.AlignRight
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight
        visible: pmSource.connectedSources != ""
       
        BatteryIcon {
            hasBattery: true
            percent: pmSource.data["Battery0"]["Percent"]
            pluggedIn: pmSource.data["AC Adapter"]["Plugged in"]

            height: 20 //FIXME
            width: 20
        }

        BreezeLabel {
            text: i18n("%1\% battery remaining", pmSource.data["Battery0"]["Percent"])
            Layout.alignment: Qt.AlignRight
            wrapMode: Text.Wrap
        }
    }

    
    PlasmaCore.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: sources
        onSourceAdded: {
            if (source == "Battery0") {
                disconnectSource(source);
                connectSource(source);
            }
        }
        onSourceRemoved: {
            if (source == "Battery0") {
                disconnectSource(source);
            }
        }
    }

    PlasmaCore.DataSource {
        id: timeSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 1000
    }

    
} 
