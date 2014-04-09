import QtQuick 2.1

Item {
    id: root

    property int stage

    onStageChanged: {
        if (stage == 1) {
            introAnimation.running = true
        }
        if (stage == 2) {

        }
        if (stage == 3) {

        }
        if (stage == 4) {


        }
        if (stage == 5) {

        }
        if (stage == 6) {
            outroAnimation.running = true
        }
    }

    Item {
        id: topImage
        x: 0
        y: 0
        height: root.height / 2
        width: root.width
        clip: true
        Image {
            x: 0
            y: 0
            width: root.width
            height: root.height
            source: "images/background.png"
        }
    }
    
    Item {
        id: bottomImage
        x: 0
        y: topImage.height
        height: root.height / 2
        width: root.width
        clip: true
        
        Image {
            x: 0
            y: -topImage.height
            width: root.width
            height: root.height
            source: "images/background.png"
        }

        NumberAnimation on y { }
    }

    Rectangle {
        id: topRect
        width: parent.width
        height: (root.height / 3) - bottomRect.height - 2
        y: root.height
        color: "black"
        opacity: 0.3
    }

    Rectangle {
        id: bottomRect
        width: parent.width
        y: -height
        height: 50
        color: "black"
        opacity: 0.3
    }

    //slight hack. Property Animations aren't bindings, they're set on creation
    //so when the height changes after being loaded it still doesn't animate properly
//     onHeightChanged :{
//         animationLoader.sourceComponent = animationsComponent
//     }
//
//     Loader {
//         id: animationLoader
//     }
//
//     Component {
//         id: animationsComponent
//
//         Item {
            SequentialAnimation {
                id: introAnimation
                running: false

                ParallelAnimation {
                    PropertyAnimation {
                        property: "y"
                        target: topRect
                        to: root.height / 3
                        duration: 1000
                        easing.type: Easing.InOutBack
                        easing.overshoot: 1.0
                    }

                    PropertyAnimation {
                        property: "y"
                        target: bottomRect
                        to: 2 * (root.height / 3) - bottomRect.height
                        duration: 1000
                        easing.type: Easing.InOutBack
                        easing.overshoot: 1.0
                    }
                }
            }

            ParallelAnimation {
                id: outroAnimation
                running: false

                onStopped: {
                    topRect.y = Qt.binding(function() { return bottomImage.y + 2 });
                    bottomRect.y = Qt.binding(function() { return topImage.y + topImage.height - bottomRect.height });
                    splitAnimation.running = true
                }

                PropertyAnimation {
                    property: "y"
                    target: topRect
                    to: root.height / 2
                    duration: 500
                    //                 easing.type: Easing.InOutBack
                    //                 easing.overshoot: 1.0
                }

                PropertyAnimation {
                    property: "y"
                    target: bottomRect
                    to: (root.height / 2) - bottomRect.height - 2
                    duration: 500
                    //                 easing.type: Easing.InOutBack
                    //                 easing.overshoot: 1.0
                }
            }

            //split the two images and fade them out
            ParallelAnimation {
                id: splitAnimation
                running: false

                PropertyAnimation {
                    property: "y"
                    target: topImage
                    to: -topImage.height
                    duration: 1000
                }

                PropertyAnimation {
                    property: "y"
                    target: bottomImage
                    to: root.height
                    duration: 1000
                }

                PropertyAnimation {
                    property: "opacity"
                    target: topImage
                    to: 0.0
                    duration: 1000
                }
                PropertyAnimation {
                    property: "opacity"
                    target: bottomImage
                    to: 0.0
                    duration: 1000
                }
            }
//         }
//     }
}
