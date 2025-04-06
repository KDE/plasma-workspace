import QtQuick 2.12
import QtTest 1.0
import org.kde.plasma.private.kicker 0.1

Item {
    id: root
    width: 400
    height: 400
    TriangleMouseFilter {
        edge: Qt.RightEdge
        // to simulate kicker's options at the bottom
        height: 300
        width: 300
        Column {
            anchors.fill: parent
            MouseArea {
                id: item1
                hoverEnabled: true
                height: 100
                width: parent.width
            }
            MouseArea {
                id: item2
                hoverEnabled: true
                height: 100
                width: parent.width
            }
            MouseArea {
                id: item3
                hoverEnabled: true
                height: 100
                width: parent.width
            }
        }
    }
    TestCase {
        when: windowShown

        name: "Triangle Mouse Filter tests"

        function test_triangle_filter() {
            mouseMove(root, 100, 350); // under the list
            compare(item3.containsMouse, false);

            mouseMove(root, 100, 290); // enter the last item
            // the first entrance is filtered
            compare(item3.containsMouse, false);

            // move up slightly
            mouseMove(root, 100, 250); // still in the last
            // but moved outside the filter triangle, accepted
            compare(item3.containsMouse, true);

            // move near the border
            mouseMove(root, 100, 205); // still in the last
            compare(item3.containsMouse, true);

            // move diagonally into item2, item3 should still get the event
            console.log("last");
            mouseMove(root, 290, 195);
            //item 3 might not still have containMouse true, as we don't filter leave events
            compare(item2.containsMouse, false);

            // but after a timeout it gets the mouse event
            wait(500);
            compare(item2.containsMouse, true);
        }

    }
}
