import QtQuick 2.0

QtObject
{
    property int currentLayout : 0
    property var layouts : [
        { shortName: "US", longName: "English" },
        { shortName: "CZ", longName: "Czech" }
    ]
}
