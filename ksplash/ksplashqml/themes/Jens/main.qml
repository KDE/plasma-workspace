import QtQuick 2.1

Item {
    id: root
    width: screenSize.width
    height: screenSize.height

    Text {
        id: debug
        x: 100
        y: 100
        z: 1000
    }

    Item {
        id: topImage
        x:0
        y:0
        height: root.height/2
        width: root.width
        clip: true
        Image {
            x:0
            y:0
            width: root.width
            height: root.height
            source: "/home/david/pictures/Wallpapers/rect5399.png"
        }
    }
    
    Item {
        id: bottomImage
        x:0
        y:root.height/2
        height: root.height/2
        width: root.width
        clip: true
        
        Image {
            x:0
            y:-root.height/2
            width: root.width
            height: root.height
            source: "/home/david/pictures/Wallpapers/rect5399.png"
        }

        NumberAnimation on y { }
    }
    
    //slight hack. Property Animations aren't bindings, they're set on creation
    //so when the height changes after being loaded it still doesn't animate properly
    onHeightChanged :{
        animationLoader.sourceComponent = slideAnimation
    }

    Loader {
        id: animationLoader
    }

    Component {
        id: slideAnimation
        SequentialAnimation {
            id: splitAnimation
            running: true
            PauseAnimation {
                duration: 1000
            }

            //split the two images
            ParallelAnimation {
                PropertyAnimation {
                    property: "y"
                    target: topImage
                    to: -root.height/2
                    duration: 1000

                }

                PropertyAnimation {
                    property: "y"
                    target: bottomImage
                    to: root.height
                    duration: 1000
                }
            }
        }
    }
}
