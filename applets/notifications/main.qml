import QtQuick
import QtQml
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    property int foo: 0

    onExpandedChanged: () => {
        foo = 1
    }

    fullRepresentation: Item {}

    onFooChanged: root.closePlasmoid()

    function closePlasmoid() {
        console.log("close")
    }
}
