import QtQuick 2.0

QtObject
{
    property int currentLayout : 0
    property var layouts : [
        { shortName: "us", longName: "English" },
        { shortName: "cz", longName: "Czech" }
    ]
}
