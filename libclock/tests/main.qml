import QtQuick 2.15
import QtQml 2.15
import org.kde.plasma.clock 1.0

Item {
    width: 500
    height: 500

    ListModel {
        id: tzModel
        ListElement {
            timeZone: null
        }
        ListElement {
            timeZone: "Europe/Berlin"
        }
        ListElement {
            timeZone: "US/Pacific"
        }
    }

    Column {
        Repeater {
            model: tzModel
            Text {
                id: delegate
                required property string timeZone
                Clock {
                    id: clock
                    timeZone: delegate.timeZone
                    trackSeconds: true
                }
                text: (timeZone || "System") + " " + Qt.formatDateTime(clock.dateTime, Qt.locale().timeFormat(Locale.LongFormat))
            }
        }
    }
}
