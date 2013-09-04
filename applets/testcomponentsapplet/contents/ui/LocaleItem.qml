import QtQuick 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.locale 2.0


Item {
    width: childrenRect.width
    height: childrenRect.height

    Locale {
        id: locale
    }
    Column {
        PlasmaComponents.Label {
            text: "Locale Time Bindings"
        }
        PlasmaComponents.Label {
            text: locale.formatLocaleTime( "11:12:13", Locale.TimeWithoutAmPm|Locale.TimeWithoutSeconds )
        }
        PlasmaComponents.Label {
            text: locale.formatDateTime("2013-04-12", Locale.ShortDate ,Locale.Seconds )
        }
    }
}